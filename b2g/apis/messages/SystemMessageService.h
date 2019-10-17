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

class SystemMessageService final : public nsISystemMessageService {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMMESSAGESERVICE

  static already_AddRefed<SystemMessageService> GetInstance();

  void DoSubscribe(const nsAString& aMessageName, const nsACString& aWorkerSpec,
                   const nsACString& aScope, const nsACString& aOriginSuffix,
                   nsISystemMessageListener* aListener);

  void Unsubscribe(nsIURI* aWorkerURI);

 private:
  SystemMessageService();
  ~SystemMessageService();

  struct SubscriberInfo {
    SubscriberInfo(const nsACString& aScope, const nsACString& aOriginSuffix)
        : mScope(aScope), mOriginSuffix(aOriginSuffix) {}
    nsCString mScope;
    nsCString mOriginSuffix;
  };
  typedef nsClassHashtable<nsCStringHashKey, SubscriberInfo> SubscriberTable;
  nsClassHashtable<nsStringHashKey, SubscriberTable> mSubscribers;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_SystemMessageService_h
