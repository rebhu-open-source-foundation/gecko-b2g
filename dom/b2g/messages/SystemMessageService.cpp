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

#undef LOG
mozilla::LazyLogModule gSystemMessageServiceLog("SystemMessageService");
#define LOG(...) \
  MOZ_LOG(gSystemMessageServiceLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

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
SystemMessageService::Subscribe(nsIPrincipal* aPrincipal,
                                const nsAString& aMessageName,
                                const nsACString& aScope) {
  if (!aPrincipal) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsAutoCString origin;
  rv = aPrincipal->GetOrigin(origin);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoCString originSuffix;
  rv = aPrincipal->GetOriginSuffix(originSuffix);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DoSubscribe(aMessageName, origin, aScope, originSuffix, nullptr);

  return NS_OK;
}

NS_IMETHODIMP
SystemMessageService::Unsubscribe(nsIPrincipal* aPrincipal) {
  if (!aPrincipal) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  nsAutoCString origin;
  rv = aPrincipal->GetOrigin(origin);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (auto iter = mSubscribers.Iter(); !iter.Done(); iter.Next()) {
    auto& table = iter.Data();
    table->Remove(origin);
  }

  LOG("Unsubscribe: subscriber origin=%s", origin.get());
  DebugPrintSubscribersTable();

  return NS_OK;
}

NS_IMETHODIMP
SystemMessageService::SendMessage(const nsAString& aMessageName,
                                  JS::HandleValue aMessage,
                                  const nsACString& aOrigin, JSContext* aCx) {
  LOG("SendMessage: name=%s", NS_LossyConvertUTF16toASCII(aMessageName).get());
  LOG("             to=%s", nsCString(aOrigin).get());

  SubscriberTable* table = mSubscribers.Get(aMessageName);
  if (!table) {
    LOG(" %s has no subscribers.",
        NS_LossyConvertUTF16toASCII(aMessageName).get());
    DebugPrintSubscribersTable();
    return NS_OK;
  }
  SubscriberInfo* info = table->Get(aOrigin);
  if (!info) {
    LOG(" %s did not subscribe %s.", (nsCString(aOrigin)).get(),
        NS_LossyConvertUTF16toASCII(aMessageName).get());
    DebugPrintSubscribersTable();
    return NS_OK;
  }

  nsAutoString messageData;
  if (NS_FAILED(SerializeFromJSVal(aCx, aMessage, messageData))) {
    return NS_OK;
  }

  // ServiceWorkerParentInterceptEnabled() is default to true in 73.0b2., which
  // whether the ServiceWorker is registered on parent processes or content
  // processes, its ServiceWorker will be spawned and executed on a content
  // process (This behavior is more secured as well).
  // As a result, we don't need to consider whether the SystemMessage.subscribe
  // is called on a content process or a parent process, and since this
  // SystemMessageService::SendMessage() is for used on chrome process only,
  // we can use ServiceWorkerManager to send SystemMessageEvent directly.
  RefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  if (!swm) {
    return NS_ERROR_FAILURE;
  }

  LOG("Sending message %s to %s",
      NS_LossyConvertUTF16toASCII(aMessageName).get(), (info->mScope).get());
  return swm->SendSystemMessageEvent(info->mOriginSuffix, info->mScope,
                                     aMessageName, messageData);
}

void SystemMessageService::DoSubscribe(const nsAString& aMessageName,
                                       const nsACString& aOrigin,
                                       const nsACString& aScope,
                                       const nsACString& aOriginSuffix,
                                       nsISystemMessageListener* aListener) {
  SubscriberTable* table = mSubscribers.LookupOrAdd(aMessageName);
  UniquePtr<SubscriberInfo> info(new SubscriberInfo(aScope, aOriginSuffix));
  table->Put(aOrigin, std::move(info));

  if (aListener) {
    aListener->OnSubscribe(NS_OK);
  }

  LOG("DoSubscribe: message name=%s",
      NS_LossyConvertUTF16toASCII(aMessageName).get());
  LOG("             subscriber origin=%s", nsCString(aOrigin).get());

  return;
}

void SystemMessageService::DebugPrintSubscribersTable() {
  LOG("Subscribed system messages count: %d\n", mSubscribers.Count());
  for (auto iter = mSubscribers.Iter(); !iter.Done(); iter.Next()) {
    nsString key = nsString(iter.Key());
    LOG("key(message name): %s\n", NS_LossyConvertUTF16toASCII(key).get());
    auto& table = iter.Data();
    for (auto iter2 = table->Iter(); !iter2.Done(); iter2.Next()) {
      nsCString key = nsCString(iter2.Key());
      LOG(" subscriber table key(origin): %s\n", key.get());
      auto& entry = iter2.Data();
      LOG("                  scope: %s\n", (entry->mScope).get());
      LOG("                  originSuffix: %s\n", (entry->mOriginSuffix).get());
    }
  }
}

}  // namespace dom
}  // namespace mozilla
