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
#include "nsCharSeparatedTokenizer.h"

#undef LOG
mozilla::LazyLogModule gSystemMessageServiceLog("SystemMessageService");
#define LOG(...) \
  MOZ_LOG(gSystemMessageServiceLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {
namespace dom {

static StaticRefPtr<SystemMessageService> sSystemMessageService;
static nsDataHashtable<nsStringHashKey, nsCString>
    sSystemMessagePermissionsTable;

namespace {

/**
 * About sSystemMessagePermissionsTable.
 * Key: Name of system message.
 * Data: Name of permission. (Please lookup from the PermissionsTable.jsm)
 *       For example, "alarm" messages require "alarms" permission, then do
 *       sSystemMessagePermissionsTable.Put(u"alarm"_ns, "alarms"_ns);
 *
 *       If your system message do not need to specify any permission, please
 *       set EmptyCString().
 *
 *       If your system message requires multiple permissions, please use ","
 *       to separate them, i.e. "camera,cellbroadcast".
 *
 *       If your system message requires access permission, such as
 *       "settings": [read, write], please expand them manually, i.e.
 *       "settings:read,settings:write".
 */
void BuildPermissionsTable() {
  /**
   * For efficient lookup and systematic indexing, please help to arrange the
   * key names (system message names) in alphabetical order.
   **/
  sSystemMessagePermissionsTable.Put(u"activity"_ns, EmptyCString());
  /**
   * Note: Please do NOT directly add new entries at the bottom of this table,
   * try to insert them alphabetically.
   **/
}

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
    BuildPermissionsTable();
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
  if (!HasPermission(aMessageName, aOrigin)) {
    if (aListener) {
      aListener->OnSubscribe(NS_ERROR_DOM_SECURITY_ERR);
    }
    LOG("Permission denied.");
    return;
  }

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

bool SystemMessageService::HasPermission(const nsAString& aMessageName,
                                         const nsACString& aOrigin) {
  LOG("Checking permission: message name=%s",
      NS_LossyConvertUTF16toASCII(aMessageName).get());
  LOG("             subscriber origin=%s", nsCString(aOrigin).get());

  nsAutoCString permNames;
  if (!sSystemMessagePermissionsTable.Get(aMessageName, &permNames)) {
    LOG("Did not define in system message permissions table.");
    return false;
  }

  nsCOMPtr<nsIPermissionManager> permMgr =
      mozilla::services::GetPermissionManager();
  nsCOMPtr<nsIPrincipal> principal =
      BasePrincipal::CreateContentPrincipal(aOrigin);

  if (principal && permMgr && !permNames.IsEmpty()) {
    nsCCharSeparatedTokenizer tokenizer(permNames, ',');
    while (tokenizer.hasMoreTokens()) {
      nsAutoCString permName(tokenizer.nextToken());
      if (permName.IsEmpty()) {
        continue;
      }
      uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;
      permMgr->TestExactPermissionFromPrincipal(principal, permName, &perm);
      LOG("Permission of %s: %d", permName.get(), perm);
      if (perm != nsIPermissionManager::ALLOW_ACTION) {
        return false;
      }
    }
  }

  return true;
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
