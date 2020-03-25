/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_B2G_h
#define mozilla_dom_B2G_h

#include "mozilla/dom/BindingDeclarations.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/TetheringManagerBinding.h"

#ifdef MOZ_B2G_RIL
#  include "mozilla/dom/CellBroadcast.h"
#  include "mozilla/dom/IccManager.h"
#  include "mozilla/dom/Voicemail.h"
#  include "mozilla/dom/Telephony.h"
#  include "mozilla/dom/MobileConnectionArray.h"
#  include "mozilla/dom/DataCallManagerBinding.h"
#  include "mozilla/dom/SubsidyLockManager.h"
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

#include "mozilla/dom/DownloadManagerBinding.h"

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
  CellBroadcast* GetCellBroadcast(ErrorResult& aRv);
  SubsidyLockManager* GetSubsidyLockManager(ErrorResult& aRv);
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

  DownloadManager* GetDownloadManager(ErrorResult& aRv);

  // Shutting down.
  void Shutdown();

 private:
  ~B2G();
  RefPtr<TetheringManager> mTetheringManager;
#ifdef MOZ_B2G_RIL
  RefPtr<CellBroadcast> mCellBroadcast;
  RefPtr<IccManager> mIccManager;
  RefPtr<Voicemail> mVoicemail;
  RefPtr<MobileConnectionArray> mMobileConnections;
  RefPtr<Telephony> mTelephony;
  RefPtr<DataCallManager> mDataCallManager;
  RefPtr<SubsidyLockManager> mSubsidyLocks;
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

  RefPtr<DownloadManager> mDownloadManager;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_B2G_h
