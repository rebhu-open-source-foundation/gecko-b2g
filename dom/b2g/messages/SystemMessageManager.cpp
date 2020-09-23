/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/SystemMessageManager.h"

#include "mozilla/dom/SystemMessageManagerBinding.h"
#include "mozilla/dom/SystemMessageManagerWorker.h"
#include "mozilla/dom/SystemMessageSubscription.h"
#include "mozilla/dom/Promise.h"
#include "nsIGlobalObject.h"

namespace mozilla {
namespace dom {

SystemMessageManager::SystemMessageManager(
    nsIGlobalObject* aGlobal, const nsACString& aScope,
    already_AddRefed<SystemMessageManagerImpl> aImpl)
    : mGlobal(aGlobal), mScope(aScope), mImpl(aImpl) {
  mImpl->AddOuter(this);
}

SystemMessageManager::~SystemMessageManager() { mImpl->RemoveOuter(this); }

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(SystemMessageManager, mGlobal)
NS_IMPL_CYCLE_COLLECTING_ADDREF(SystemMessageManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SystemMessageManager)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SystemMessageManager)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject* SystemMessageManager::WrapObject(JSContext* aCx,
                                           JS::Handle<JSObject*> aGivenProto) {
  return SystemMessageManager_Binding::Wrap(aCx, this, aGivenProto);
}

// static
already_AddRefed<SystemMessageManager> SystemMessageManager::Create(
    nsIGlobalObject* aGlobal, const nsACString& aScope, ErrorResult& aRv) {
  RefPtr<SystemMessageManagerImpl> impl;
  if (NS_IsMainThread()) {
    impl = new SystemMessageManagerMain();
  } else {
    impl = new SystemMessageManagerWorker();
  }

  RefPtr<SystemMessageManager> ret =
      new SystemMessageManager(aGlobal, aScope, impl.forget());
  return ret.forget();
}

already_AddRefed<Promise> SystemMessageManager::Subscribe(
    const nsAString& aMessageName, ErrorResult& aRv) {
  return mImpl->Subscribe(aMessageName, aRv);
}

SystemMessageManagerMain::SystemMessageManagerMain() : mOuter(nullptr) {}

SystemMessageManagerMain::~SystemMessageManagerMain() {
  MOZ_DIAGNOSTIC_ASSERT(!mOuter);
}

already_AddRefed<Promise> SystemMessageManagerMain::Subscribe(
    const nsAString& aMessageName, ErrorResult& aRv) {
  MOZ_DIAGNOSTIC_ASSERT(mOuter);
  RefPtr<Promise> promise = Promise::Create(mOuter->GetParentObject(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsCOMPtr<nsIPrincipal> principal =
      mOuter->GetParentObject()->PrincipalOrNull();
  if (!principal) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  RefPtr<SystemMessageSubscription> subscription =
      SystemMessageSubscription::Create(principal, promise, nullptr,
                                        mOuter->mScope, aMessageName);
  nsresult rv = subscription->Subscribe();
  if (NS_FAILED(rv)) {
    promise->MaybeReject(rv);
    return promise.forget();
  }

  return promise.forget();
}

void SystemMessageManagerMain::AddOuter(SystemMessageManager* aOuter) {
  MOZ_DIAGNOSTIC_ASSERT(aOuter);
  MOZ_DIAGNOSTIC_ASSERT(!mOuter);
  mOuter = aOuter;
}

void SystemMessageManagerMain::RemoveOuter(SystemMessageManager* aOuter) {
  MOZ_DIAGNOSTIC_ASSERT(aOuter);
  MOZ_DIAGNOSTIC_ASSERT(mOuter == aOuter);
  mOuter = nullptr;
}

}  // namespace dom
}  // namespace mozilla
