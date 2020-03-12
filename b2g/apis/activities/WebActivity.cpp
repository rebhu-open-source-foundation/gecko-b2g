/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/WebActivity.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/StructuredCloneHolder.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "nsContentUtils.h"
#include "nsIConsoleService.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"

namespace mozilla {
namespace dom {

static nsCOMPtr<nsIActivityProxy> sActivityProxy;

namespace {

already_AddRefed<nsIActivityProxy> GetOrCreateActivityProxy() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sActivityProxy) {
    sActivityProxy = do_CreateInstance("@mozilla.org/dom/activities/proxy;1");
    MOZ_ASSERT(sActivityProxy);
  }

  nsCOMPtr<nsIActivityProxy> proxy = sActivityProxy;
  return proxy.forget();
}

class ActivityInitRunnable : public WorkerMainThreadRunnable,
                             public StructuredCloneHolder {
 public:
  ActivityInitRunnable(WorkerPrivate* aWorkerPrivate, WebActivity* aWebActivity)
      : WorkerMainThreadRunnable(
            aWorkerPrivate,
            NS_LITERAL_CSTRING("WebActivity :: ActivityInitialize")),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mWebActivity(aWebActivity) {}

  bool MainThreadRun() override {
    AutoSafeJSContext cx;
    ErrorResult rv;
    JS::RootedValue options(cx);
    Read(mWebActivity->GetParentObject(), cx, &options, rv);
    if (NS_WARN_IF(rv.Failed())) {
      return false;
    }
    JS_WrapValue(cx, &options);

    nsCOMPtr<nsIPrincipal> principal = mWorkerPrivate->GetPrincipal();
    if (NS_WARN_IF(!principal)) {
      return false;
    }
    nsAutoCString url;
    principal->GetAsciiSpec(url);

    mWebActivity->ActivityProxyInit(nullptr, options, url);

    return true;
  }

 private:
  ~ActivityInitRunnable() = default;
  RefPtr<WebActivity> mWebActivity;
};

class ActivityStartRunnable : public Runnable {
 public:
  ActivityStartRunnable(nsString aId, PromiseWorkerProxy* aPromiseWorkerProxy)
      : Runnable("dom::ActivityStartRunnable"),
        mId(aId),
        mPromiseWorkerProxy(aPromiseWorkerProxy) {}

  NS_IMETHOD
  Run() override {
    RefPtr<nsIActivityStartCallback> callback =
        new ActivityStartCallback(mPromiseWorkerProxy);
    nsCOMPtr<nsIActivityProxy> proxy = GetOrCreateActivityProxy();
    proxy->Start(mId, callback);
    return NS_OK;
  }

 private:
  ~ActivityStartRunnable() = default;
  nsString mId;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

class OnStartWorkerRunnable final : public WorkerRunnable,
                                    public StructuredCloneHolder {
 public:
  OnStartWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                        already_AddRefed<PromiseWorkerProxy>&& aProxy,
                        nsresult aStatus)
      : WorkerRunnable(aWorkerPrivate),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mProxy(std::move(aProxy)),
        mStatus(aStatus) {}

  bool WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override {
    RefPtr<Promise> promise = mProxy->WorkerPromise();

    JS::RootedObject globalObject(aCx, JS::CurrentGlobalOrNull(aCx));
    if (NS_WARN_IF(!globalObject)) {
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    nsCOMPtr<nsIGlobalObject> global = xpc::NativeGlobal(globalObject);
    if (NS_WARN_IF(!global)) {
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    JS::RootedValue result(aCx);
    ErrorResult rv;
    Read(global, aCx, &result, rv);
    if (NS_WARN_IF(rv.Failed())) {
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    JS_WrapValue(aCx, &result);

    if (NS_SUCCEEDED(mStatus)) {
      promise->MaybeResolve(result);
    } else {
      promise->MaybeReject(result);
    }

    return true;
  }

 private:
  ~OnStartWorkerRunnable() {
    MOZ_ASSERT(mProxy);
    mProxy->CleanUp();
  };

  RefPtr<PromiseWorkerProxy> mProxy;
  nsresult mStatus;
};

class ActivityCancelRunnable : public Runnable {
 public:
  ActivityCancelRunnable(nsString aId)
      : Runnable("dom::ActivityCancelRunnable"), mId(aId) {}

  NS_IMETHOD
  Run() override {
    nsCOMPtr<nsIActivityProxy> proxy = GetOrCreateActivityProxy();
    proxy->Cancel(mId);
    return NS_OK;
  }

 private:
  ~ActivityCancelRunnable() = default;
  nsString mId;
};

}  // namespace

NS_IMPL_ISUPPORTS(ActivityStartCallback, nsIActivityStartCallback)

ActivityStartCallback::ActivityStartCallback(Promise* aPromise)
    : mPromise(aPromise) {}

ActivityStartCallback::ActivityStartCallback(
    PromiseWorkerProxy* aPromiseWorkerProxy)
    : mPromiseWorkerProxy(aPromiseWorkerProxy) {}

ActivityStartCallback::~ActivityStartCallback() {}

NS_IMETHODIMP
ActivityStartCallback::OnStart(nsresult aStatus, JS::HandleValue aResult,
                               JSContext* aCx) {
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(aResult);
    } else {
      mPromise->MaybeReject(aResult);
    }
  } else if (mPromiseWorkerProxy) {
    MutexAutoLock lock(mPromiseWorkerProxy->Lock());
    if (mPromiseWorkerProxy->CleanedUp()) {
      return NS_OK;
    }
    WorkerPrivate* worker = mPromiseWorkerProxy->GetWorkerPrivate();
    RefPtr<OnStartWorkerRunnable> r = new OnStartWorkerRunnable(
        worker, mPromiseWorkerProxy.forget(), aStatus);
    ErrorResult rv;
    r->Write(aCx, aResult, rv);
    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

WebActivity::WebActivity(nsIGlobalObject* aGlobal)
    : mGlobal(aGlobal), mIsStarted(false) {}

WebActivity::~WebActivity() {}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebActivity, mGlobal)
NS_IMPL_CYCLE_COLLECTING_ADDREF(WebActivity)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebActivity)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebActivity)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject* WebActivity::WrapObject(JSContext* aCx,
                                  JS::Handle<JSObject*> aGivenProto) {
  return WebActivity_Binding::Wrap(aCx, this, aGivenProto);
}

/*static*/
already_AddRefed<WebActivity> WebActivity::Constructor(
    const GlobalObject& aOwner, const WebActivityOptions& aOptions,
    ErrorResult& aRv) {
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aOwner.GetAsSupports());
  RefPtr<WebActivity> activity = new WebActivity(global);

  nsresult rv;
  rv = activity->PermissionCheck();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  rv = activity->Initialize(aOwner, aOptions);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return nullptr;
  }

  return activity.forget();
}

nsresult WebActivity::PermissionCheck() {
  if (NS_IsMainThread()) {
    auto* window = mGlobal->AsInnerWindow();
    if (!window) {
      return NS_ERROR_UNEXPECTED;
    }

    Document* document = window->GetExtantDoc();

    bool isActive;
    window->GetDocShell()->GetIsActive(&isActive);

    if (!isActive && !nsContentUtils::IsChromeDoc(document)) {
      nsCOMPtr<nsIConsoleService> console(
          do_GetService("@mozilla.org/consoleservice;1"));
      NS_ENSURE_TRUE(console, NS_ERROR_FAILURE);

      nsString message = NS_LITERAL_STRING(
          "Can only start activity from user input or chrome code");
      console->LogStringMessage(message.get());

      return NS_ERROR_FAILURE;
    }
  } else {
    // TODO: Same security policy with Cliends.openWindow
    // WebActivity is allowed only when called as the result of a notification
    // click event. (Bug 80958)
  }

  return NS_OK;
}

nsresult WebActivity::Initialize(const GlobalObject& aOwner,
                                 const WebActivityOptions& aOptions) {
  if (NS_IsMainThread()) {
    // We're about to pass the dictionary to a JS-implemented component, so
    // rehydrate it in a system scode so that security wrappers don't get in the
    // way. See bug 1161748 comment 16.
    bool ok;
    JS::RootedValue optionsValue(aOwner.Context());
    {
      JSAutoRealm ar(aOwner.Context(), xpc::PrivilegedJunkScope());
      ok = ToJSValue(aOwner.Context(), aOptions, &optionsValue);
      NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
    }
    ok = JS_WrapValue(aOwner.Context(), &optionsValue);
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

    nsCOMPtr<nsIPrincipal> principal = mGlobal->PrincipalOrNull();
    NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);
    nsAutoCString url;
    principal->GetAsciiSpec(url);

    ActivityProxyInit(aOwner.GetAsSupports(), optionsValue, url);
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    JS::RootedValue optionsValue(aOwner.Context());
    if (NS_WARN_IF(!ToJSValue(aOwner.Context(), aOptions, &optionsValue))) {
      return NS_ERROR_FAILURE;
    }

    ErrorResult rv;
    RefPtr<ActivityInitRunnable> initRunnable =
        new ActivityInitRunnable(worker, this);
    initRunnable->Write(aOwner.Context(), optionsValue, rv);
    initRunnable->Dispatch(Canceling, rv);
    if (NS_WARN_IF(rv.Failed())) {
      return NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}

already_AddRefed<Promise> WebActivity::Start(ErrorResult& aRv) {
  RefPtr<Promise> promise = Promise::Create(mGlobal, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  if (mIsStarted) {
    promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
    return promise.forget();
  }
  mIsStarted = true;

  if (NS_IsMainThread()) {
    RefPtr<nsIActivityStartCallback> callback =
        new ActivityStartCallback(promise);
    nsCOMPtr<nsIActivityProxy> proxy = GetOrCreateActivityProxy();
    proxy->Start(mId, callback);
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    RefPtr<PromiseWorkerProxy> proxy =
        PromiseWorkerProxy::Create(worker, promise);
    if (!proxy) {
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }

    RefPtr<ActivityStartRunnable> r = new ActivityStartRunnable(mId, proxy);
    NS_DispatchToMainThread(r);
  }

  return promise.forget();
}

void WebActivity::Cancel() {
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIActivityProxy> proxy = GetOrCreateActivityProxy();
    proxy->Cancel(mId);
  } else {
    RefPtr<ActivityCancelRunnable> r = new ActivityCancelRunnable(mId);
    NS_DispatchToMainThread(r);
  }
}

void WebActivity::ActivityProxyInit(nsISupports* aSupports,
                                    JS::HandleValue aOptions,
                                    const nsACString& aURL) {
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIActivityProxy> proxy = GetOrCreateActivityProxy();
  proxy->Init(aSupports, aOptions, aURL, mId);
}

}  // namespace dom
}  // namespace mozilla
