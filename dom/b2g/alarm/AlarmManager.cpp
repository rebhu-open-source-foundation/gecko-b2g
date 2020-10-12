/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/AlarmManager.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"

static mozilla::LazyLogModule sAlarmManagerLog("AlarmManager");
#define LOG(...) \
  MOZ_LOG(sAlarmManagerLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {
namespace dom {

class OnGetAllWorkerRunnable final : public WorkerRunnable,
                                     public StructuredCloneHolder {
 public:
  OnGetAllWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                         already_AddRefed<PromiseWorkerProxy>&& aProxy,
                         nsresult aStatus)
      : WorkerRunnable(aWorkerPrivate),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mProxy(std::move(aProxy)),
        mStatus(aStatus) {
    LOG("OnGetAllWorkerRunnable constructor.");
  }

  bool WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override {
    LOG("OnGetAllWorkerRunnable::WorkerRun");
    RefPtr<Promise> promise = mProxy->WorkerPromise();

    JS::RootedObject globalObject(aCx, JS::CurrentGlobalOrNull(aCx));
    if (NS_WARN_IF(!globalObject)) {
      LOG("!globalObject");
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    nsCOMPtr<nsIGlobalObject> global = xpc::NativeGlobal(globalObject);
    if (NS_WARN_IF(!global)) {
      LOG("!global");
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    JS::RootedValue result(aCx);
    ErrorResult rv;
    Read(global, aCx, &result, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Read failed. rv:[%u]", rv.ErrorCodeAsInt());
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
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
  ~OnGetAllWorkerRunnable() {
    MOZ_ASSERT(mProxy);
    mProxy->CleanUp();
  };

  RefPtr<PromiseWorkerProxy> mProxy;
  nsresult mStatus;
};

class AlarmGetAllCallback final : public nsIAlarmGetAllCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMGETALLCALLBACK
  explicit AlarmGetAllCallback(Promise* aPromise);
  explicit AlarmGetAllCallback(PromiseWorkerProxy* aPromiseWorkerProxy);

 protected:
  ~AlarmGetAllCallback();

 private:
  RefPtr<Promise> mPromise;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

NS_IMPL_ISUPPORTS(AlarmGetAllCallback, nsIAlarmGetAllCallback)

AlarmGetAllCallback::AlarmGetAllCallback(Promise* aPromise)
    : mPromise(aPromise) {
  LOG("AlarmGetAllCallback constructor. Promise*");
}

AlarmGetAllCallback::AlarmGetAllCallback(
    PromiseWorkerProxy* aPromiseWorkerProxy)
    : mPromiseWorkerProxy(aPromiseWorkerProxy) {
  LOG("AlarmGetAllCallback constructor. PromiseWorkerProxy*");
}

AlarmGetAllCallback::~AlarmGetAllCallback() {}

NS_IMETHODIMP
AlarmGetAllCallback::OnGetAll(nsresult aStatus, JS::HandleValue aResult,
                              JSContext* aCx) {
  if (mPromise) {
    LOG("OnGetAll mPromise. aCx:[%p]", aCx);

    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolveWithClone(aCx, aResult);
    } else {
      mPromise->MaybeRejectWithClone(aCx, aResult);
    }
  } else if (mPromiseWorkerProxy) {
    LOG("OnGetAll mPromiseWorkerProxy. aCx:[%p]", aCx);
    MutexAutoLock lock(mPromiseWorkerProxy->Lock());
    if (mPromiseWorkerProxy->CleanedUp()) {
      LOG("mPromiseWorkerProxy->CleanedUp(), return NS_OK.");
      return NS_OK;
    }
    WorkerPrivate* worker = mPromiseWorkerProxy->GetWorkerPrivate();
    RefPtr<OnGetAllWorkerRunnable> r = new OnGetAllWorkerRunnable(
        worker, mPromiseWorkerProxy.forget(), aStatus);
    ErrorResult rv;
    r->Write(aCx, aResult, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Write failed. rv:[%u]", rv.ErrorCodeAsInt());
    }

    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

class AlarmGetAllRunnable : public Runnable {
 public:
  explicit AlarmGetAllRunnable(PromiseWorkerProxy* aPromiseWorkerProxy,
                               const nsACString& aUrl)
      : Runnable("dom::AlarmGetAllRunnable"),
        mPromiseWorkerProxy(aPromiseWorkerProxy),
        mUrl(aUrl) {
    LOG("AlarmGetAllRunnable constructor. mUrl:[%s]", mUrl.get());
  }

  NS_IMETHOD
  Run() override {
    LOG("AlarmGetAllRunnable::Run");
    RefPtr<nsIAlarmGetAllCallback> callback =
        new AlarmGetAllCallback(mPromiseWorkerProxy);
    nsCOMPtr<nsIAlarmProxy> alarmProxy =
        do_CreateInstance("@mozilla.org/dom/alarm/proxy;1");
    MOZ_ASSERT(alarmProxy);
    alarmProxy->Init();
    alarmProxy->GetAll(mUrl, callback);
    return NS_OK;
  }

 private:
  ~AlarmGetAllRunnable() = default;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  nsAutoCString mUrl;
};

class OnAddWorkerRunnable final : public WorkerRunnable,
                                  public StructuredCloneHolder {
 public:
  OnAddWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                      already_AddRefed<PromiseWorkerProxy>&& aProxy,
                      nsresult aStatus)
      : WorkerRunnable(aWorkerPrivate),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mProxy(std::move(aProxy)),
        mStatus(aStatus) {
    LOG("OnAddWorkerRunnable constructor.");
  }

  bool WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override {
    LOG("OnAddWorkerRunnable::WorkerRun");
    RefPtr<Promise> promise = mProxy->WorkerPromise();

    JS::RootedObject globalObject(aCx, JS::CurrentGlobalOrNull(aCx));
    if (NS_WARN_IF(!globalObject)) {
      LOG("!globalObject");
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    nsCOMPtr<nsIGlobalObject> global = xpc::NativeGlobal(globalObject);
    if (NS_WARN_IF(!global)) {
      LOG("!global");
      promise->MaybeReject(NS_ERROR_UNEXPECTED);
      return false;
    }
    JS::RootedValue result(aCx);
    ErrorResult rv;
    Read(global, aCx, &result, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Read failed. rv:[%u]", rv.ErrorCodeAsInt());
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
  ~OnAddWorkerRunnable() {
    MOZ_ASSERT(mProxy);
    mProxy->CleanUp();
  };

  RefPtr<PromiseWorkerProxy> mProxy;
  nsresult mStatus;
};

class AlarmAddCallback final : public nsIAlarmAddCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMADDCALLBACK
  explicit AlarmAddCallback(Promise* aPromise);
  explicit AlarmAddCallback(PromiseWorkerProxy* aPromiseWorkerProxy);

 protected:
  ~AlarmAddCallback();

 private:
  RefPtr<Promise> mPromise;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

NS_IMPL_ISUPPORTS(AlarmAddCallback, nsIAlarmAddCallback)

AlarmAddCallback::AlarmAddCallback(Promise* aPromise) : mPromise(aPromise) {
  LOG("AlarmAddCallback constructor. Promise*");
}

AlarmAddCallback::AlarmAddCallback(PromiseWorkerProxy* aPromiseWorkerProxy)
    : mPromiseWorkerProxy(aPromiseWorkerProxy) {
  LOG("AlarmAddCallback constructor. PromiseWorkerProxy*");
}

AlarmAddCallback::~AlarmAddCallback() {}

NS_IMETHODIMP
AlarmAddCallback::OnAdd(nsresult aStatus, JS::HandleValue aResult,
                        JSContext* aCx) {
  if (mPromise) {
    LOG("OnAdd mPromise aCx:[%p]", aCx);
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(aResult);
    } else {
      mPromise->MaybeReject(aResult);
    }
  } else if (mPromiseWorkerProxy) {
    LOG("OnAdd mPromiseWorkerProxy aCx:[%p]", aCx);
    MutexAutoLock lock(mPromiseWorkerProxy->Lock());
    if (mPromiseWorkerProxy->CleanedUp()) {
      LOG("mPromiseWorkerProxy->CleanedUp(), return NS_OK.");
      return NS_OK;
    }
    WorkerPrivate* worker = mPromiseWorkerProxy->GetWorkerPrivate();
    RefPtr<OnAddWorkerRunnable> r =
        new OnAddWorkerRunnable(worker, mPromiseWorkerProxy.forget(), aStatus);
    ErrorResult rv;
    r->Write(aCx, aResult, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Write failed. rv:[%u]", rv.ErrorCodeAsInt());
    }
    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

class AlarmAddRunnable : public Runnable, public StructuredCloneHolder {
 public:
  AlarmAddRunnable(AlarmManager* aAlarmManager,
                   PromiseWorkerProxy* aPromiseWorkerProxy,
                   const nsACString& aUrl)
      : Runnable("dom::AlarmAddRunnable"),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mAlarmManager(aAlarmManager),
        mPromiseWorkerProxy(aPromiseWorkerProxy),
        mUrl(aUrl) {
    LOG("AlarmAddRunnable constructor. mUrl:[%s]", mUrl.get());
  }
  NS_IMETHOD
  Run() override {
    LOG("AlarmAddRunnable::Run");
    MOZ_ASSERT(mAlarmManager);
    AutoSafeJSContext cx;
    ErrorResult rv;
    JS::RootedValue options(cx);
    Read(mAlarmManager->GetParentObject(), cx, &options, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Read failed. rv:[%u]", rv.ErrorCodeAsInt());
      return rv.StealNSResult();
    }
    JS_WrapValue(cx, &options);

    RefPtr<nsIAlarmAddCallback> callback =
        new AlarmAddCallback(mPromiseWorkerProxy);
    nsCOMPtr<nsIAlarmProxy> alarmProxy =
        do_CreateInstance("@mozilla.org/dom/alarm/proxy;1");
    MOZ_ASSERT(alarmProxy);
    alarmProxy->Init();
    alarmProxy->Add(mUrl, options, callback);
    return NS_OK;
  }

 private:
  ~AlarmAddRunnable() = default;
  RefPtr<AlarmManager> mAlarmManager;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  nsAutoCString mUrl;
};

class AlarmRemoveRunnable : public Runnable {
 public:
  explicit AlarmRemoveRunnable(long aId, const nsACString& aUrl)
      : Runnable("dom::AlarmRemoveRunnable"), mId(aId), mUrl(aUrl) {
    LOG("AlarmRemoveRunnable constructor. mUrl:[%s] mId:[%d]", mUrl.get(), mId);
  }

  NS_IMETHOD
  Run() override {
    LOG("AlarmRemoveRunnable::Run");
    nsCOMPtr<nsIAlarmProxy> alarmProxy =
        do_CreateInstance("@mozilla.org/dom/alarm/proxy;1");
    MOZ_ASSERT(alarmProxy);
    alarmProxy->Init();
    alarmProxy->Remove(mUrl, mId);
    return NS_OK;
  }

 private:
  ~AlarmRemoveRunnable() = default;
  long mId;
  nsAutoCString mUrl;
};

AlarmManager::AlarmManager(nsIGlobalObject* aGlobal) : mGlobal(aGlobal) {
  LOG("AlarmManager constructor. aGlobal:[%p]", aGlobal);
  MOZ_ASSERT(aGlobal);

  Maybe<ClientInfo> clientInfo = mGlobal->GetClientInfo();
  if (clientInfo.isSome()) {
    // System app might call AlarmManager from the system startup chrome url.
    // To ensure proper system message receiver, replace the url with system
    // app's origin.
    // Other modules or services in Gecko of chrome origins should call
    // AlarmService directly, not through AlarmManager or AlarmProxy.
    LOG("clientInfo->URL() [%s]", clientInfo->URL().get());
    if (nsContentUtils::IsCallerChrome()) {
      nsAutoCString systemStartupUrl;
      nsresult rv =
          Preferences::GetCString("b2g.system_startup_url", systemStartupUrl);
      if (NS_SUCCEEDED(rv) && clientInfo->URL().Equals(systemStartupUrl)) {
        mUrl = "https://system.local"_ns;
      }
    } else {
      nsCOMPtr<nsIIOService> ios(do_GetIOService());
      nsCOMPtr<nsIURI> uri;
      nsresult rv =
          ios->NewURI(clientInfo->URL(), nullptr, nullptr, getter_AddRefs(uri));
      NS_ENSURE_SUCCESS_VOID(rv);
      uri->GetPrePath(mUrl);
    }
  } else {
    LOG("Error: !clientInfo.isSome()");
  }

  LOG("mUrl:[%s]", mUrl.get());
  // Alarms added by apps should be well-managed according to app urls.
  // Empty urls may cause ambiguity.
  if (NS_IsMainThread() && !mUrl.IsEmpty()) {
    mAlarmProxy = do_CreateInstance("@mozilla.org/dom/alarm/proxy;1");
    MOZ_ASSERT(mAlarmProxy);
    mAlarmProxy->Init();
  }
}

AlarmManager::~AlarmManager() {}

already_AddRefed<Promise> AlarmManager::GetAll() {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  if (NS_WARN_IF(mUrl.IsEmpty())) {
    LOG("Empty url on GetAll");
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  if (NS_IsMainThread()) {
    LOG("AlarmManager::GetAll NS_IsMainThread() %s", mUrl.get());
    if (NS_WARN_IF(!mAlarmProxy)) {
      LOG("!mAlarmProxy on GetAll");
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }
    RefPtr<nsIAlarmGetAllCallback> callback = new AlarmGetAllCallback(promise);
    mAlarmProxy->GetAll(mUrl, callback);
  } else {
    LOG("AlarmManager::GetAll !NS_IsMainThread() mUrl:[%s]", mUrl.get());
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    RefPtr<PromiseWorkerProxy> proxy =
        PromiseWorkerProxy::Create(worker, promise);
    if (!proxy) {
      LOG("!proxy");
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }

    RefPtr<AlarmGetAllRunnable> r = new AlarmGetAllRunnable(proxy, mUrl);
    NS_DispatchToMainThread(r);
  }

  return promise.forget();
}

already_AddRefed<Promise> AlarmManager::Add(JSContext* aCx,
                                            const AlarmOptions& aOptions) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  if (NS_WARN_IF(mUrl.IsEmpty())) {
    LOG("Empty url on Add");
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  if (NS_IsMainThread()) {
    LOG("AlarmManager::Add NS_IsMainThread() mUrl[%s]", mUrl.get());
    if (NS_WARN_IF(!mAlarmProxy)) {
      LOG("!mAlarmProxy on Add");
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }
    // We're about to pass the dictionary to a JS-implemented component, so
    // rehydrate it in a system scode so that security wrappers don't get in the
    // way. See bug 1161748 comment 16.
    bool ok;
    JS::RootedValue optionsValue(aCx);
    {
      JSAutoRealm ar(aCx, xpc::PrivilegedJunkScope());
      ok = ToJSValue(aCx, aOptions, &optionsValue);
      if (!ok) {
        promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
        return promise.forget();
      }
    }
    ok = JS_WrapValue(aCx, &optionsValue);
    if (!ok) {
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
      return promise.forget();
    }

    RefPtr<nsIAlarmAddCallback> callback = new AlarmAddCallback(promise);
    mAlarmProxy->Add(mUrl, optionsValue, callback);
  } else {
    LOG("AlarmManager::Add !NS_IsMainThread() mUrl:[%s]", mUrl.get());
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    RefPtr<PromiseWorkerProxy> proxy =
        PromiseWorkerProxy::Create(worker, promise);
    if (!proxy) {
      LOG("!proxy");
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }

    JS::RootedValue optionsValue(aCx);
    if (NS_WARN_IF(!ToJSValue(aCx, aOptions, &optionsValue))) {
      LOG("!ToJSValue");
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
      return promise.forget();
    }

    ErrorResult rv;
    RefPtr<AlarmAddRunnable> r = new AlarmAddRunnable(this, proxy, mUrl);
    r->Write(aCx, optionsValue, rv);
    if (NS_WARN_IF(rv.Failed())) {
      LOG("Write failed. rv:[%u]", rv.ErrorCodeAsInt());
    }
    NS_DispatchToMainThread(r);
    if (NS_WARN_IF(rv.Failed())) {
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
      return promise.forget();
    }
  }

  return promise.forget();
}

void AlarmManager::Remove(long aId) {
  if (NS_WARN_IF(mUrl.IsEmpty())) {
    LOG("Empty url on Remove. aId:[%d]", aId);
    return;
  }
  if (NS_IsMainThread()) {
    LOG("AlarmManager::Remove NS_IsMainThread() mUrl:[%s] aId:[%d]", mUrl.get(), aId);
    if (NS_WARN_IF(!mAlarmProxy)) {
      LOG("!mAlarmProxy on Remove");
      return;
    }
    mAlarmProxy->Remove(mUrl, aId);
  } else {
    LOG("AlarmManager::Remove !NS_IsMainThread() mUrl:[%s] aId:[%d]", mUrl.get(), aId);
    RefPtr<AlarmRemoveRunnable> r = new AlarmRemoveRunnable(aId, mUrl);
    NS_DispatchToMainThread(r);
  }
}

nsresult AlarmManager::PermissionCheck() { return NS_OK; }

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(AlarmManager, mGlobal)
NS_IMPL_CYCLE_COLLECTING_ADDREF(AlarmManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(AlarmManager)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(AlarmManager)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject* AlarmManager::WrapObject(JSContext* aCx,
                                   JS::Handle<JSObject*> aGivenProto) {
  return AlarmManager_Binding::Wrap(aCx, this, aGivenProto);
}

}  // namespace dom
}  // namespace mozilla
