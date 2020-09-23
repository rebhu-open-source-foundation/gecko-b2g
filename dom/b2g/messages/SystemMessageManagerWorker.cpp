/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/SystemMessageManagerWorker.h"

#include "mozilla/dom/SystemMessageSubscription.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerScope.h"
#include "nsIGlobalObject.h"

namespace mozilla {
namespace dom {

namespace {

class CreateSubscriptionRunnable final : public WorkerMainThreadRunnable {
 public:
  explicit CreateSubscriptionRunnable(WorkerPrivate* aWorkerPrivate,
                                      PromiseWorkerProxy* aProxy,
                                      const nsAString& aMessageName,
                                      const nsCString& aScope)
      : WorkerMainThreadRunnable(aWorkerPrivate,
                                 "SystemMessage :: CreateSubscription"_ns),
        mProxy(aProxy),
        mMessageName(aMessageName),
        mScope(aScope) {}

  bool MainThreadRun() override {
    AssertIsOnMainThread();

    MutexAutoLock lock(mProxy->Lock());
    if (mProxy->CleanedUp()) {
      mError = NS_ERROR_DOM_ABORT_ERR;
      return true;
    }

    nsCOMPtr<nsIPrincipal> principal =
        mProxy->GetWorkerPrivate()->GetPrincipal();
    if (!principal) {
      mError = NS_ERROR_DOM_ABORT_ERR;
      return true;
    }

    RefPtr<SystemMessageSubscription> subscription =
        SystemMessageSubscription::Create(principal, nullptr, mProxy, mScope,
                                          mMessageName);
    mError = subscription->Subscribe();
    return true;
  }

  nsresult mError;

 private:
  ~CreateSubscriptionRunnable() {}

  RefPtr<PromiseWorkerProxy> mProxy;
  nsString mMessageName;
  nsCString mScope;
};

}  // anonymous namespace

SystemMessageManagerWorker::SystemMessageManagerWorker() : mOuter(nullptr) {}

SystemMessageManagerWorker::~SystemMessageManagerWorker() {
  MOZ_DIAGNOSTIC_ASSERT(!mOuter);
}

already_AddRefed<Promise> SystemMessageManagerWorker::Subscribe(
    const nsAString& aMessageName, ErrorResult& aRv) {
  MOZ_DIAGNOSTIC_ASSERT(mOuter);
  WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(worker);
  worker->AssertIsOnWorkerThread();

  nsCOMPtr<nsIGlobalObject> global = worker->GlobalScope();
  RefPtr<Promise> promise = Promise::Create(global, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  RefPtr<PromiseWorkerProxy> proxy =
      PromiseWorkerProxy::Create(worker, promise);
  if (!proxy) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  RefPtr<CreateSubscriptionRunnable> r = new CreateSubscriptionRunnable(
      worker, proxy, aMessageName, mOuter->mScope);
  ErrorResult rv;
  r->Dispatch(Canceling, rv);
  if (rv.Failed()) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  if (NS_FAILED(r->mError)) {
    promise->MaybeReject(r->mError);
    return promise.forget();
  }

  return promise.forget();

  return nullptr;
}

void SystemMessageManagerWorker::AddOuter(SystemMessageManager* aOuter) {
  MOZ_DIAGNOSTIC_ASSERT(aOuter);
  MOZ_DIAGNOSTIC_ASSERT(!mOuter);
  mOuter = aOuter;
}

void SystemMessageManagerWorker::RemoveOuter(SystemMessageManager* aOuter) {
  MOZ_DIAGNOSTIC_ASSERT(aOuter);
  MOZ_DIAGNOSTIC_ASSERT(mOuter == aOuter);
  mOuter = nullptr;
}

}  // namespace dom
}  // namespace mozilla
