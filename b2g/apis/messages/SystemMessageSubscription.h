/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_SystemMessageSubscription_h
#define mozilla_dom_SystemMessageSubscription_h

#include "mozilla/AlreadyAddRefed.h"
#include "nsISystemMessageListener.h"

class nsIPrincipal;

namespace mozilla {
namespace dom {

class Promise;
class PromiseWorkerProxy;

class SystemMessageSubscription final : public nsISystemMessageListener {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMMESSAGELISTENER

  static already_AddRefed<SystemMessageSubscription> Create(
      nsIPrincipal* aPrincipal, Promise* aPromise, PromiseWorkerProxy* aProxy,
      const nsACString& aScope, const nsAString& aMessageName);

  nsresult Subscribe();

 private:
  explicit SystemMessageSubscription(nsIPrincipal* aPrincipal,
                                     Promise* aPromise,
                                     PromiseWorkerProxy* aProxy,
                                     const nsACString& aScope,
                                     const nsAString& aMessageName);
  ~SystemMessageSubscription();

  void Initialize();

  nsCOMPtr<nsIPrincipal> mPrincipal;
  // Promise is valid iff SystemMessageManager.subscribe is called from main
  // thread, otherwise it is null.
  RefPtr<Promise> mPromise;
  // PromiseWorkerProxy is valid iff SystemMessageManager.subscribe is called
  // from worker thread, otherwise it is null.
  RefPtr<PromiseWorkerProxy> mProxy;
  nsCString mScope;
  nsString mMessageName;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_SystemMessageSubscription_h
