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

namespace Wificond = ::com::android::server::wifi::wificond;

class WificondControl : virtual public android::RefBase {
 public:
  WificondControl();

  static WificondControl* Get();

  Result_t InitWificondInterface();
  Result_t StartWificond();
  Result_t StopWificond();
  Result_t TearDownClientInterface(const std::string& aIfaceName);
  Result_t TearDownSoftapInterface(const std::string& aIfaceName);

  Result_t StartSupplicant();
  Result_t StopSupplicant();

  Result_t SetupClientIface(
      const std::string& aIfaceName,
      const android::sp<android::net::wifi::IScanEvent>& aScanCallback,
      const android::sp<android::net::wifi::IPnoScanEvent>& aPnoScanCallback);
  Result_t SetupApIface(
      const std::string& aIfaceName,
      const android::sp<android::net::wifi::IApInterfaceEventCallback>& aApCallback);
  Result_t StartSoftap(ConfigurationOptions* aConfig);
  Result_t StartSingleScan(ScanSettingsOptions* aScanSettings);
  Result_t StopSingleScan();
  Result_t StartPnoScan(PnoScanSettingsOptions* aPnoScanSettings);
  Result_t StopPnoScan();
  Result_t GetScanResults(
      std::vector<Wificond::NativeScanResult>& aScanResults);
  Result_t GetPnoScanResults(
      std::vector<Wificond::NativeScanResult>& aPnoScanResults);
  Result_t GetChannelsForBand(uint32_t aBandMask,
                              std::vector<int32_t>& aChannels);

  Result_t SignalPoll();
  Result_t GetSoftapStations(uint32_t& aNumStations);

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
