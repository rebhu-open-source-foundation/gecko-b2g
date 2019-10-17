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
#include "nsISystemMessageListener.h"

namespace mozilla {
namespace dom {

static StaticRefPtr<SystemMessageService> sSystemMessageService;

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
                                nsIURI* aWorkerURI) {
  // Provide for subscribing from app's manifest during install.
  return NS_OK;
}

void SystemMessageService::DoSubscribe(const nsAString& aMessageName,
                                       const nsACString& aWorkerSpec,
                                       const nsACString& aScope,
                                       const nsACString& aOriginSuffix,
                                       nsISystemMessageListener* aListener) {
  SubscriberTable* table = mSubscribers.LookupOrAdd(aMessageName);
  nsAutoPtr<SubscriberInfo> info(new SubscriberInfo(aScope, aOriginSuffix));
  table->Put(aWorkerSpec, info.forget());
  aListener->OnSubscribe(NS_OK);

  return;
}

void SystemMessageService::Unsubscribe(nsIURI* aWorkerURI) {
  // This will be call when an app is uninstalled.
  if (!aWorkerURI) {
    return;
  }

  nsCString workerSpec;
  aWorkerURI->GetSpec(workerSpec);
  for (auto iter = mSubscribers.Iter(); !iter.Done(); iter.Next()) {
    SubscriberTable* table = iter.Data();
    table->Remove(workerSpec);
  }
}

}  // namespace dom
}  // namespace mozilla
