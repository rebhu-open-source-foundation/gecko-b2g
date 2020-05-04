/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_MobileNetworkInfo_h
#define mozilla_dom_MobileNetworkInfo_h

#include "mozilla/dom/MobileNetworkInfoBinding.h"
#include "nsIMobileNetworkInfo.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class MobileNetworkInfo final : public nsIMobileNetworkInfo {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILENETWORKINFO

  MobileNetworkInfo(const nsAString& aShortName, const nsAString& aLongName,
                    const nsAString& aMcc, const nsAString& aMnc,
                    const nsAString& aState);

  void Update(nsIMobileNetworkInfo* aInfo);

 private:
  ~MobileNetworkInfo() {}

 private:
  nsString mShortName;
  nsString mLongName;
  nsString mMcc;
  nsString mMnc;
  nsString mState;
};

class DOMMobileNetworkInfo final : public nsWrapperCache, nsISupports {
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMobileNetworkInfo)

  explicit DOMMobileNetworkInfo(nsPIDOMWindowInner* aWindow);

  DOMMobileNetworkInfo(const nsAString& aShortName, const nsAString& aLongName,
                       const nsAString& aMcc, const nsAString& aMnc,
                       const nsAString& aState);

  void
  Update(nsIMobileNetworkInfo* aInfo);

  nsPIDOMWindowInner*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // DOMMobileNetworkInfo WebIDL interface
  Nullable<MobileNetworkState> GetState() const {
    uint32_t i = 0;
    for (const EnumEntry* entry = MobileNetworkStateValues::strings;
         entry->value; ++entry, ++i) {
      nsString state;
      mInfo->GetState(state);
      if (state.EqualsASCII(entry->value)) {
        return Nullable<MobileNetworkState>(static_cast<MobileNetworkState>(i));
      }
    }

    return Nullable<MobileNetworkState>();
  }

  void GetShortName(nsAString& aShortName) const {
    mInfo->GetShortName(aShortName);
  }

  void GetLongName(nsAString& aLongName) const {
    mInfo->GetShortName(aLongName);
  }

  void GetMcc(nsAString& aMcc) const { mInfo->GetMcc(aMcc); }

  void GetMnc(nsAString& aMnc) const { mInfo->GetMnc(aMnc); }

  MobileNetworkInfo* GetNetwork() const { return mInfo; }

 private:
  ~DOMMobileNetworkInfo() { mInfo = nullptr; }

 private:
  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  RefPtr<MobileNetworkInfo> mInfo;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_MobileNetworkInfo_h
