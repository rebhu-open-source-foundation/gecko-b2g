/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef mozilla_dom_WebActivity_h
#define mozilla_dom_WebActivity_h

#include "mozilla/dom/WebActivityBinding.h"
#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "nsIActivityProxy.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {
namespace {
class ActivityInitRunnable;
}  // namespace

class Promise;
class PromiseWorkerProxy;

class ActivityStartCallback final : public nsIActivityStartCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACTIVITYSTARTCALLBACK
  explicit ActivityStartCallback(Promise* aPromise);
  explicit ActivityStartCallback(PromiseWorkerProxy* aPromiseWorkerProxy);

 protected:
  ~ActivityStartCallback();

 private:
  RefPtr<Promise> mPromise;
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

class WebActivity final : public nsISupports,
                          public nsWrapperCache

{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebActivity)

  nsIGlobalObject* GetParentObject() const { return mGlobal; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  explicit WebActivity(nsIGlobalObject* aGlobal);

  // WebIDL methods
  static already_AddRefed<WebActivity> Constructor(
      const GlobalObject& aGlobal, const WebActivityOptions& aOptions,
      ErrorResult& aRv);

  already_AddRefed<Promise> Start(ErrorResult& aRv);

  void Cancel();

  friend class ActivityInitRunnable;

 protected:
  ~WebActivity();

  nsresult PermissionCheck();

  nsresult Initialize(const GlobalObject& aOwner,
                      const WebActivityOptions& aOptions);

  void ActivityProxyInit(nsISupports* aSupports, JS::HandleValue aOptions,
                         const nsACString& aURL);

  nsCOMPtr<nsIGlobalObject> mGlobal;
  bool mIsStarted;
  nsString mId;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_WebActivity_h
