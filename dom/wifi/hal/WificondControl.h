/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
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
