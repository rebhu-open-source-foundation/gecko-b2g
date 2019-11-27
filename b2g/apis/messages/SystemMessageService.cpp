/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "mozilla/dom/SystemMessageService.h"
#include "mozilla/dom/SystemMessageServiceChild.h"
#include "mozilla/dom/ServiceWorkerManager.h"
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

  RefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (!swm) {
    return NS_ERROR_FAILURE;
  }

  return swm->SendSystemMessageEvent(info->mOriginSuffix, info->mScope,
                                     aMessageName, messageData);
}

void SystemMessageService::DoSubscribe(const nsAString& aMessageName,
                                       const nsACString& aOrigin,
                                       const nsACString& aScope,
                                       const nsACString& aOriginSuffix,
                                       nsISystemMessageListener* aListener) {
  SubscriberTable* table = mSubscribers.LookupOrAdd(aMessageName);
  nsAutoPtr<SubscriberInfo> info(new SubscriberInfo(aScope, aOriginSuffix));
  table->Put(aOrigin, info.forget());
  aListener->OnSubscribe(NS_OK);

  return;
}

void SystemMessageService::Unsubscribe(const nsACString& aOrigin) {
  // This will be call when an app is uninstalled.
  for (auto iter = mSubscribers.Iter(); !iter.Done(); iter.Next()) {
    SubscriberTable* table = iter.Data();
    table->Remove(aOrigin);
  }
}

}  // namespace dom
}  // namespace mozilla
