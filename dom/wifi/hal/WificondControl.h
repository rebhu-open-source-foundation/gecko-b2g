/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WificondControl_H
#define WificondControl_H

#include "mozilla/Mutex.h"

#include <android-base/macros.h>
#include <android/net/wifi/IWificond.h>
#include <android/net/wifi/IWifiScannerImpl.h>

using ::android::net::wifi::IApInterface;
using ::android::net::wifi::IClientInterface;
using ::android::net::wifi::IWificond;
using ::android::net::wifi::IWifiScannerImpl;
using ::com::android::server::wifi::wificond::ChannelSettings;
using ::com::android::server::wifi::wificond::HiddenNetwork;
using ::com::android::server::wifi::wificond::NativeScanResult;
using ::com::android::server::wifi::wificond::PnoSettings;
using ::com::android::server::wifi::wificond::SingleScanSettings;

class WificondControl : virtual public android::RefBase {
 public:
  WificondControl();

  static WificondControl* Get();

  bool InitWificondInterface();
  bool StartWificond();
  bool StopWificond();
  bool TearDownClientInterface(const std::string& aIfaceName);
  bool TearDownSoftapInterface(const std::string& aIfaceName);

  bool StartSupplicant();
  bool StopSupplicant();

  bool SetupClientIface(
      const std::string& aIfaceName,
      const android::sp<android::net::wifi::IScanEvent>& aScanCallback);
  bool SetupApIface(
      const std::string& aIfaceName,
      const android::sp<android::net::wifi::IApInterfaceEventCallback>& aApCallback);
  bool StartSoftap(ConfigurationOptions* aConfig);
  bool StartSingleScan(ScanSettingsOptions* aScanSettings);
  bool StopSingleScan();
  bool StartPnoScan();
  bool StopPnoScan();
  bool GetScanResults(std::vector<NativeScanResult>& aScanResults);
  bool GetChannelsForBand(uint32_t aBandMask, std::vector<int32_t>& aChannels);

  bool SignalPoll();

 private:
  class WificondDeathRecipient : public android::IBinder::DeathRecipient {
   public:
    explicit WificondDeathRecipient() {}
    virtual ~WificondDeathRecipient() {}

    virtual void binderDied(const android::wp<android::IBinder>& who);
  };

  virtual ~WificondControl() {}

  static WificondControl* s_Instance;

  android::sp<IWificond> mWificond;
  android::sp<IClientInterface> mClientInterface;
  android::sp<IApInterface> mApInterface;
  android::sp<IWifiScannerImpl> mScanner;
  android::sp<WificondDeathRecipient> mWificondDeathRecipient;

  DISALLOW_COPY_AND_ASSIGN(WificondControl);
};

#endif  // WificondControl_H
