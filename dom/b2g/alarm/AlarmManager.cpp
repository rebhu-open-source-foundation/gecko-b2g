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

class AlarmGetAllCallback final : public nsIAlarmGetAllCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMGETALLCALLBACK
  explicit AlarmGetAllCallback(Promise* aPromise);

 protected:
  ~AlarmGetAllCallback();

 private:
  RefPtr<Promise> mPromise;
};

NS_IMPL_ISUPPORTS(AlarmGetAllCallback, nsIAlarmGetAllCallback)

AlarmGetAllCallback::AlarmGetAllCallback(Promise* aPromise)
    : mPromise(aPromise) {}

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
  }

  return NS_OK;
}

class AlarmAddCallback final : public nsIAlarmAddCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIALARMADDCALLBACK
  explicit AlarmAddCallback(Promise* aPromise);

 protected:
  ~AlarmAddCallback();

 private:
  RefPtr<Promise> mPromise;
};

NS_IMPL_ISUPPORTS(AlarmAddCallback, nsIAlarmAddCallback)

AlarmAddCallback::AlarmAddCallback(Promise* aPromise) : mPromise(aPromise) {}

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
  }

  return NS_OK;
}

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
  }

  return promise.forget();
}

void AlarmManager::Remove(long aId) {
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIAlarmProxy> proxy = GetOrCreateAlarmProxy(mGlobal);
    proxy->Remove(aId);
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
