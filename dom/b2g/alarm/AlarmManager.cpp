/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "mozilla/dom/AlarmManager.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "nsIAlarmProxy.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"

static nsCOMPtr<nsIAlarmProxy> sAlarmProxy;

namespace mozilla {
namespace dom {

already_AddRefed<nsIAlarmProxy> GetOrCreateAlarmProxy(nsISupports* aSupports) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sAlarmProxy) {
    sAlarmProxy = do_CreateInstance("@mozilla.org/dom/alarm/proxy;1");
    MOZ_ASSERT(sAlarmProxy);
    sAlarmProxy->Init(aSupports);
  }

  nsCOMPtr<nsIAlarmProxy> proxy = sAlarmProxy;
  return proxy.forget();
}

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
    : mPromise(aPromise) {}

AlarmGetAllCallback::AlarmGetAllCallback(
    PromiseWorkerProxy* aPromiseWorkerProxy)
    : mPromiseWorkerProxy(aPromiseWorkerProxy) {}

AlarmGetAllCallback::~AlarmGetAllCallback() {}

NS_IMETHODIMP
AlarmGetAllCallback::OnGetAll(nsresult aStatus, JS::HandleValue aResult,
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
    RefPtr<OnGetAllWorkerRunnable> r = new OnGetAllWorkerRunnable(
        worker, mPromiseWorkerProxy.forget(), aStatus);
    ErrorResult rv;
    r->Write(aCx, aResult, rv);
    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

class AlarmGetAllRunnable : public Runnable {
 public:
  AlarmGetAllRunnable(PromiseWorkerProxy* aPromiseWorkerProxy)
      : Runnable("dom::AlarmGetAllRunnable"),
        mPromiseWorkerProxy(aPromiseWorkerProxy) {}

  NS_IMETHOD
  Run() override {
    RefPtr<nsIAlarmGetAllCallback> callback =
        new AlarmGetAllCallback(mPromiseWorkerProxy);
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(nullptr);
    proxy->GetAll(nullptr, callback);
    return NS_OK;
  }

 private:
  ~AlarmGetAllRunnable() = default;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
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

AlarmAddCallback::AlarmAddCallback(Promise* aPromise) : mPromise(aPromise) {}

AlarmAddCallback::AlarmAddCallback(PromiseWorkerProxy* aPromiseWorkerProxy)
    : mPromiseWorkerProxy(aPromiseWorkerProxy) {}

AlarmAddCallback::~AlarmAddCallback() {}

NS_IMETHODIMP
AlarmAddCallback::OnAdd(nsresult aStatus, JS::HandleValue aResult,
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
    RefPtr<OnAddWorkerRunnable> r =
        new OnAddWorkerRunnable(worker, mPromiseWorkerProxy.forget(), aStatus);
    ErrorResult rv;
    r->Write(aCx, aResult, rv);
    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

class AlarmAddRunnable : public Runnable, public StructuredCloneHolder {
 public:
  AlarmAddRunnable(AlarmManager* aAlarmManager,
                   PromiseWorkerProxy* aPromiseWorkerProxy)
      : Runnable("dom::AlarmAddRunnable"),
        StructuredCloneHolder(CloningSupported, TransferringSupported,
                              StructuredCloneScope::SameProcess),
        mAlarmManager(aAlarmManager),
        mPromiseWorkerProxy(aPromiseWorkerProxy) {}
  NS_IMETHOD
  Run() override {
    MOZ_ASSERT(mAlarmManager);
    AutoSafeJSContext cx;
    ErrorResult rv;
    JS::RootedValue options(cx);
    Read(mAlarmManager->GetParentObject(), cx, &options, rv);
    if (NS_WARN_IF(rv.Failed())) {
      return rv.StealNSResult();
    }
    JS_WrapValue(cx, &options);

    RefPtr<nsIAlarmAddCallback> callback =
        new AlarmAddCallback(mPromiseWorkerProxy);
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(nullptr);
    proxy->Add(nullptr, options, callback);
    return NS_OK;
  }

 private:
  ~AlarmAddRunnable() = default;
  RefPtr<AlarmManager> mAlarmManager;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

class AlarmRemoveRunnable : public Runnable {
 public:
  AlarmRemoveRunnable(long aId)
      : Runnable("dom::AlarmRemoveRunnable"), mId(aId) {}

  NS_IMETHOD
  Run() override {
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(nullptr);
    proxy->Remove(mId);
    return NS_OK;
  }

 private:
  ~AlarmRemoveRunnable() = default;
  long mId;
};

AlarmManager::AlarmManager(nsIGlobalObject* aGlobal) : mGlobal(aGlobal) {
  MOZ_ASSERT(aGlobal);
}

AlarmManager::~AlarmManager() {}

already_AddRefed<Promise> AlarmManager::GetAll() {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  if (NS_IsMainThread()) {
    RefPtr<nsIAlarmGetAllCallback> callback = new AlarmGetAllCallback(promise);

    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(mGlobal);
    proxy->GetAll(mGlobal, callback);
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

    RefPtr<AlarmGetAllRunnable> r = new AlarmGetAllRunnable(proxy);
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

  if (NS_IsMainThread()) {
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
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(mGlobal);
    proxy->Add(mGlobal, optionsValue, callback);
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

    JS::RootedValue optionsValue(aCx);
    if (NS_WARN_IF(!ToJSValue(aCx, aOptions, &optionsValue))) {
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
      return promise.forget();
    }

    ErrorResult rv;
    RefPtr<AlarmAddRunnable> r = new AlarmAddRunnable(this, proxy);
    r->Write(aCx, optionsValue, rv);
    NS_DispatchToMainThread(r);
    if (NS_WARN_IF(rv.Failed())) {
      promise->MaybeReject(NS_ERROR_DOM_OPERATION_ERR);
      return promise.forget();
    }
  }

  return promise.forget();
}

void AlarmManager::Remove(long aId) {
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(mGlobal);
    proxy->Remove(aId);
  } else {
    RefPtr<AlarmRemoveRunnable> r = new AlarmRemoveRunnable(aId);
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
