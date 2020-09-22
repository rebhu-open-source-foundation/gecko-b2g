/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/SystemMessageSubscription.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/SystemMessageService.h"
#include "mozilla/dom/SystemMessageServiceChild.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "mozilla/dom/WorkerPrivate.h"

namespace mozilla {
namespace dom {

namespace {

class OnSubscribeWorkerRunnable final : public WorkerRunnable {
 public:
  OnSubscribeWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                            already_AddRefed<PromiseWorkerProxy>&& aProxy,
                            nsresult aStatus)
      : WorkerRunnable(aWorkerPrivate),
        mProxy(std::move(aProxy)),
        mStatus(aStatus) {}

  bool WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override {
    RefPtr<Promise> promise = mProxy->WorkerPromise();

    if (NS_SUCCEEDED(mStatus)) {
      promise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      promise->MaybeReject(mStatus);
    }

    mProxy->CleanUp();

    return true;
  }

 private:
  ~OnSubscribeWorkerRunnable() {}

  RefPtr<PromiseWorkerProxy> mProxy;
  nsresult mStatus;
};

SystemMessageService* GetSystemMessageService() {
  nsCOMPtr<nsISystemMessageService> service =
      do_GetService("@mozilla.org/systemmessage-service;1");
  MOZ_ASSERT(service);
  return static_cast<SystemMessageService*>(service.get());
}

}  // anonymous namespace

NS_IMPL_ISUPPORTS(SystemMessageSubscription, nsISystemMessageListener)

/*static*/
already_AddRefed<SystemMessageSubscription> SystemMessageSubscription::Create(
    nsIPrincipal* aPrincipal, Promise* aPromise, PromiseWorkerProxy* aProxy,
    const nsACString& aScope, const nsAString& aMessageName) {
  // Create from main only, if subscribe is called from main thread, mPromise
  // is valid, else if subscribe is called from worker thread, mProxy is valid.
  // Base on mPromise or mProxy to resolve promise on main thread or worker
  // thread.
  // All methods should be called on main thread.

  RefPtr<SystemMessageSubscription> subscription =
      new SystemMessageSubscription(aPrincipal, aPromise, aProxy, aScope,
                                    aMessageName);

  return subscription.forget();
}

SystemMessageSubscription::SystemMessageSubscription(
    nsIPrincipal* aPrincipal, Promise* aPromise, PromiseWorkerProxy* aProxy,
    const nsACString& aScope, const nsAString& aMessageName)
    : mPrincipal(aPrincipal),
      mPromise(aPromise),
      mProxy(aProxy),
      mScope(aScope),
      mMessageName(aMessageName) {}

SystemMessageSubscription::~SystemMessageSubscription() {}

// nsISystemMessageListener methods.
NS_IMETHODIMP
SystemMessageSubscription::OnSubscribe(nsresult aStatus) {
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  } else if (mProxy) {
    MutexAutoLock lock(mProxy->Lock());
    if (mProxy->CleanedUp()) {
      return NS_OK;
    }
    WorkerPrivate* worker = mProxy->GetWorkerPrivate();
    RefPtr<OnSubscribeWorkerRunnable> r =
        new OnSubscribeWorkerRunnable(worker, mProxy.forget(), aStatus);
    MOZ_ALWAYS_TRUE(r->Dispatch());
  }

  return NS_OK;
}

nsresult SystemMessageSubscription::Subscribe() {
  nsresult rv;
  nsAutoCString origin;
  rv = mPrincipal->GetOrigin(origin);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString originSuffix;
  rv = mPrincipal->GetOriginSuffix(originSuffix);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    SystemMessageServiceChild* child = new SystemMessageServiceChild(this);
    contentChild->SendPSystemMessageServiceConstructor(child);
    SubscribeRequest request(mMessageName, origin, mScope, originSuffix);
    child->SendRequest(request);
    return NS_OK;
  }

  // Call from parent process (or in-proces app).
  SystemMessageService* service = GetSystemMessageService();
  MOZ_ASSERT(service);
  service->DoSubscribe(mMessageName, origin, mScope, originSuffix, this);

  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla
