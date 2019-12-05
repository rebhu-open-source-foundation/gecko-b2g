/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_SystemMessageService_h
#define mozilla_dom_SystemMessageService_h

#include "mozilla/ClearOnShutdown.h"
#include "nsISystemMessageService.h"

class nsISystemMessageListener;

namespace mozilla {
namespace dom {

class ContentParent;

class SystemMessageService final : public nsISystemMessageService {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMMESSAGESERVICE

  static already_AddRefed<SystemMessageService> GetInstance();

  void DoSubscribe(uint64_t aID, const nsAString& aMessageName,
                   const nsACString& aOrigin, const nsACString& aScope,
                   const nsACString& aOriginSuffix,
                   nsISystemMessageListener* aListener);

  void Unsubscribe(const nsACString& aOrigin);

 private:
  SystemMessageService();
  ~SystemMessageService();

  struct SubscriberInfo {
    SubscriberInfo(uint64_t aID, const nsACString& aScope,
                   const nsACString& aOriginSuffix)
        : mID(aID), mScope(aScope), mOriginSuffix(aOriginSuffix) {}
    uint64_t mID;
    nsCString mScope;
    nsCString mOriginSuffix;
  };
  typedef nsClassHashtable<nsCStringHashKey, SubscriberInfo> SubscriberTable;
  nsClassHashtable<nsStringHashKey, SubscriberTable> mSubscribers;
};

class SystemMessageDispatcher final {
 public:
  SystemMessageDispatcher(const nsACString& aScope,
                          const nsACString& aOriginSuffix,
                          const nsAString& aMessageName,
                          const nsAString& aMessageData);
  ~SystemMessageDispatcher();
  nsresult NotifyWorkers();
  bool SendToChild(ContentParent* aActor);

 private:
  nsCString mScope;
  nsCString mOriginSuffix;
  nsString mMessageName;
  nsString mMessageData;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_SystemMessageService_h
