/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_AlarmManager_h
#define mozilla_dom_AlarmManager_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AlarmManagerBinding.h"
#include "nsIAlarmProxy.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;

class AlarmManager final : public nsISupports,
                           public nsWrapperCache

{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AlarmManager)

  nsIGlobalObject* GetParentObject() const { return mGlobal; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  explicit AlarmManager(nsIGlobalObject* aGlobal);

  already_AddRefed<Promise> GetAll();
  already_AddRefed<Promise> Add(JSContext* aCx, const AlarmOptions& options);

  void Remove(long aId);

 protected:
  ~AlarmManager();

  nsresult PermissionCheck();

  nsCOMPtr<nsIGlobalObject> mGlobal;

  nsAutoCString mUrl;

  nsCOMPtr<nsIAlarmProxy> mAlarmProxy;
};

}  // namespace dom
}  // namespace mozilla

#endif
