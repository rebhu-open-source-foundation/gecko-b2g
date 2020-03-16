/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/SystemMessageService.h"
#include "mozilla/dom/SystemMessageServiceChild.h"
#include "mozilla/dom/ServiceWorkerManager.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/StaticPtr.h"
#include "js/JSON.h"
#include "nsISystemMessageListener.h"

namespace mozilla {
namespace dom {

static StaticRefPtr<SystemMessageService> sSystemMessageService;

namespace {

nsresult SerializeFromJSVal(JSContext* aCx, JS::HandleValue aValue,
                            nsAString& aResult) {
  aResult.Truncate();
  nsAutoString serializedValue;

  if (aValue.isObject()) {
    JS::RootedValue value(aCx, aValue.get());
    NS_ENSURE_TRUE(nsContentUtils::StringifyJSON(aCx, &value, serializedValue),
                   NS_ERROR_XPC_BAD_CONVERT_JS);
    NS_ENSURE_TRUE(!serializedValue.IsEmpty(), NS_ERROR_FAILURE);
    aResult = serializedValue;
  } else {
    MOZ_ASSERT(false, "SystemMessageData should be an JS object.");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

}  // anonymous namespace

NS_IMPL_ISUPPORTS(SystemMessageService, nsISystemMessageService)

SystemMessageService::SystemMessageService() {}

SystemMessageService::~SystemMessageService() {}

/*static*/
already_AddRefed<SystemMessageService> SystemMessageService::GetInstance() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  if (!sSystemMessageService) {
    sSystemMessageService = new SystemMessageService();
    ClearOnShutdown(&sSystemMessageService);
  }

  RefPtr<SystemMessageService> service = sSystemMessageService;
  return service.forget();
}

// nsISystemMessageService methods
NS_IMETHODIMP
SystemMessageService::Subscribe(const nsAString& aMessageName,
                                const nsACString& aOrigin) {
  // Provide for subscribing from app's manifest during install.
  return NS_OK;
}

NS_IMETHODIMP
SystemMessageService::SendMessage(const nsAString& aMessageName,
                                  JS::HandleValue aMessage,
                                  const nsACString& aOrigin, JSContext* aCx) {
  SubscriberTable* table = mSubscribers.Get(aMessageName);
  if (!table) {
    return NS_OK;
  }
  SubscriberInfo* info = table->Get(aOrigin);
  if (!info) {
    return NS_OK;
  }

  nsAutoString messageData;
  if (NS_FAILED(SerializeFromJSVal(aCx, aMessage, messageData))) {
    return NS_OK;
  }

  nsresult rv;
  SystemMessageDispatcher dispatcher(info->mScope, info->mOriginSuffix,
                                     aMessageName, messageData);
  // If info->mID is 0, the subscriber is called from parent process.
  if (!ServiceWorkerParentInterceptEnabled() && info->mID) {
    nsTArray<ContentParent*> contentActors;
    ContentParent::GetAll(contentActors);
    for (uint32_t i = 0; i < contentActors.Length(); ++i) {
      if (!contentActors[i]->GetRemoteType().EqualsLiteral(
              DEFAULT_REMOTE_TYPE)) {
        continue;
      }
      // TODO: PushNotifier transmit the permission to content scope here, not
      // sure whether we should do that too, since our permission model is TBD,
      // leave it as TODO.

      // TODO: Ideally, we should dispatch events to the content process where
      // this subscriber belongs, however, childID will be re-generated once the
      // content process is killed and relaunched, and there is no way to find
      // the mapping from ContentParent currently. We follow the fix from
      // upstream (bug 1300112), where we always dispatch events to the first
      // available content process. File Bug 80246 to follow up.

      if (dispatcher.SendToChild(contentActors[i])) {
        break;
      }
    }
  } else {
    rv = dispatcher.NotifyWorkers();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

void SystemMessageService::DoSubscribe(uint64_t aID,
                                       const nsAString& aMessageName,
                                       const nsACString& aOrigin,
                                       const nsACString& aScope,
                                       const nsACString& aOriginSuffix,
                                       nsISystemMessageListener* aListener) {
  SubscriberTable* table = mSubscribers.LookupOrAdd(aMessageName);
  nsAutoPtr<SubscriberInfo> info(
      new SubscriberInfo(aID, aScope, aOriginSuffix));
  table->Put(aOrigin, info.forget());
  aListener->OnSubscribe(NS_OK);

  return;
}

void SystemMessageService::Unsubscribe(const nsACString& aOrigin) {
  // This will be call when an app is uninstalled.
  for (auto iter = mSubscribers.Iter(); !iter.Done(); iter.Next()) {
    auto &table = iter.Data();
    table->Remove(aOrigin);
  }
}

SystemMessageDispatcher::SystemMessageDispatcher(
    const nsACString& aScope, const nsACString& aOriginSuffix,
    const nsAString& aMessageName, const nsAString& aMessageData)
    : mScope(aScope),
      mOriginSuffix(aOriginSuffix),
      mMessageName(aMessageName),
      mMessageData(aMessageData) {}

SystemMessageDispatcher::~SystemMessageDispatcher() {}

nsresult SystemMessageDispatcher::NotifyWorkers() {
  RefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (!swm) {
    return NS_ERROR_FAILURE;
  }

  return swm->SendSystemMessageEvent(mOriginSuffix, mScope, mMessageName,
                                     mMessageData);
}

bool SystemMessageDispatcher::SendToChild(ContentParent* aActor) {
  return aActor->SendSystemMessage(mScope, mOriginSuffix, mMessageName,
                                   mMessageData);
}

}  // namespace dom
}  // namespace mozilla
