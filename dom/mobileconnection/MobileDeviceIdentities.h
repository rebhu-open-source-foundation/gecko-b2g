/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_MobileDeviceIdentities_h
#define mozilla_dom_MobileDeviceIdentities_h

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/MobileConnectionDeviceIdsBinding.h"
#include "nsIMobileDeviceIdentities.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

struct JSContext;

namespace mozilla {
namespace dom {

class MobileDeviceIdentities final : public nsIMobileDeviceIdentities {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEDEVICEIDENTITIES

  MobileDeviceIdentities(const nsAString& aImei, const nsAString& aImeisv,
                         const nsAString& aEsn, const nsAString& aMeid);

  void Update(nsIMobileDeviceIdentities* aIdentities);

 private:
  ~MobileDeviceIdentities() {}

 private:
  nsString mImei;
  nsString mImeisv;
  nsString mEsn;
  nsString mMeid;
};

class DOMMobileDeviceIdentities final : public nsWrapperCache, nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMobileDeviceIdentities)

  explicit DOMMobileDeviceIdentities(nsPIDOMWindowInner* aWindow);

  DOMMobileDeviceIdentities(const nsAString& aImei, const nsAString& aImeisv,
                         const nsAString& aEsn, const nsAString& aMeid);

  void Update(nsIMobileDeviceIdentities* aIdentities);

  nsPIDOMWindowInner*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // DOMMobileConnectionDeviceIds WebIDL interface
  void GetImei(nsAString& aImei) const {
    mIdentities->GetImei(aImei);
  }

  void GetImeisv(nsAString& aImeisv) const {
    mIdentities->GetImeisv(aImeisv);
  }

  void GetEsn(nsAString& aEsn) const {
    mIdentities->GetEsn(aEsn);
  }

  void GetMeid(nsAString& aMeid) const {
    mIdentities->GetMeid(aMeid);
  }

  MobileDeviceIdentities* GetIdentities() const { return mIdentities; }

private:
  ~DOMMobileDeviceIdentities() { mIdentities = nullptr; }

private:
  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  RefPtr<MobileDeviceIdentities> mIdentities;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_MobileDeviceIdentities_h
