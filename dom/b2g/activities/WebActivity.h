/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
