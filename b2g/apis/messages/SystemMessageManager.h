/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
