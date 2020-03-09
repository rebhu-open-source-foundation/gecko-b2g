/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_B2G_h
#define mozilla_dom_B2G_h

#include "mozilla/dom/BindingDeclarations.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/TetheringManagerBinding.h"

#ifdef MOZ_B2G_RIL
#  include "mozilla/dom/IccManager.h"
#  include "mozilla/dom/Voicemail.h"
#  include "mozilla/dom/Telephony.h"
#  include "mozilla/dom/MobileConnectionArray.h"
#  include "mozilla/dom/DataCallManagerBinding.h"
#endif

#ifdef HAS_KOOST_MODULES
#  include "mozilla/dom/ExternalAPI.h"
#endif
#ifdef MOZ_B2G_BT
#  include "mozilla/dom/bluetooth/BluetoothManager.h"
#endif
#ifndef DISABLE_WIFI
#  include "mozilla/dom/WifiManagerBinding.h"
#endif

namespace mozilla {
namespace dom {

class B2G final : public nsISupports, public nsWrapperCache {
  nsCOMPtr<nsIGlobalObject> mOwner;

 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(B2G)

  explicit B2G(nsIGlobalObject* aGlobal);

  nsIGlobalObject* GetParentObject() const { return mOwner; }
  virtual JSObject* WrapObject(JSContext* aCtx,
                               JS::Handle<JSObject*> aGivenProto) override;

  TetheringManager* GetTetheringManager(ErrorResult& aRv);

#ifdef MOZ_B2G_RIL
  IccManager* GetIccManager(ErrorResult& aRv);
  Voicemail* GetVoicemail(ErrorResult& aRv);
  MobileConnectionArray* GetMobileConnections(ErrorResult& aRv);
  Telephony* GetTelephony(ErrorResult& aRv);
  DataCallManager* GetDataCallManager(ErrorResult& aRv);
#endif  // MOZ_B2G_RIL

#ifdef HAS_KOOST_MODULES
  ExternalAPI* GetExternalapi(ErrorResult& aRv);
#endif
#ifdef MOZ_B2G_BT
  bluetooth::BluetoothManager* GetBluetooth(ErrorResult& aRv);
#endif
#ifndef DISABLE_WIFI
  WifiManager* GetWifiManager(ErrorResult& aRv);
#endif
  static bool HasWifiManagerSupport(JSContext* /* unused */, JSObject* aGlobal);

  // Shutting down.
  void Shutdown();

 private:
  ~B2G();
  RefPtr<TetheringManager> mTetheringManager;
#ifdef MOZ_B2G_RIL
  RefPtr<IccManager> mIccManager;
  RefPtr<Voicemail> mVoicemail;
  RefPtr<MobileConnectionArray> mMobileConnections;
  RefPtr<Telephony> mTelephony;
  RefPtr<DataCallManager> mDataCallManager;
#endif

#ifdef HAS_KOOST_MODULES
  RefPtr<ExternalAPI> mExternalAPI;
#endif
#ifdef MOZ_B2G_BT
  RefPtr<bluetooth::BluetoothManager> mBluetooth;
#endif
#ifndef DISABLE_WIFI
  RefPtr<WifiManager> mWifiManager;
#endif
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_B2G_h
