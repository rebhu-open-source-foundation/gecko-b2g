/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
