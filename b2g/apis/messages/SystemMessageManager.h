/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_SystemMessageManager_h
#define mozilla_dom_SystemMessageManager_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsWrapperCache.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;
class PromiseWorkerProxy;

class SystemMessageManager final : public nsISupports,
                                   public nsWrapperCache

{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SystemMessageManager)

  nsIGlobalObject* GetParentObject() const { return mGlobal; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<SystemMessageManager> Create(nsIGlobalObject* aGlobal,
                                                       const nsACString& aScope,
                                                       ErrorResult& aRv);
  // WebIDL methods
  already_AddRefed<Promise> Subscribe(const nsAString& aMessageName,
                                      ErrorResult& aRv);

  nsresult CreateSubscription(Promise* aPromise, PromiseWorkerProxy* aProxy,
                              const nsAString& aMessageName);

 private:
  explicit SystemMessageManager(nsIGlobalObject* aGlobal,
                                const nsACString& aScope);
  explicit SystemMessageManager(const nsACString& aScope);
  ~SystemMessageManager();

  nsCOMPtr<nsIGlobalObject> mGlobal;
  nsCString mScope;
};
}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_SystemMessageManager_h
