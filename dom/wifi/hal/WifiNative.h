/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef WifiNative_h
#define WifiNative_h

#include "WifiHalManager.h"
#include "WificondControl.h"
#include "SupplicantStaManager.h"
#include "SupplicantEventCallback.h"
#include "nsString.h"
#include "nsWifiElement.h"
#include "nsIWifiCommand.h"
#include "mozilla/UniquePtr.h"

class WifiNative {
 public:
  WifiNative(EventCallback aCallback);

  bool ExecuteCommand(CommandOptions& aOptions, nsWifiResult* aResult,
                      const nsCString& aInterface);

 private:
  bool InitHal();
  bool DeinitHal();

  bool StartWifi();
  bool StopWifi();

  bool StartSupplicant();
  bool StopSupplicant();
  bool GetStaCapabilities(uint32_t& aCapabilities);
  bool GetDebugLevel(uint32_t& aDebugLevel);
  bool SetDebugLevel(SupplicantDebugLevelOptions* aLevel);
  bool SetPowerSave(bool aEnable);
  bool SetSuspendMode(bool aEnable);
  bool SetExternalSim(bool aEnable);
  bool SetAutoReconnect(bool aEnable);
  bool SetCountryCode(const nsAString& aCountryCode);
  bool SetBtCoexistenceMode(uint8_t aMode);
  bool SetBtCoexistenceScanMode(bool aEnable);

  bool StartSingleScan();
  bool StopSingleScan();
  bool StartPnoScan();
  bool StopPnoScan();
  bool GetScanResults(std::vector<NativeScanResult>& aScanResults);

  bool Connect(ConfigurationOptions* aConfig);

  class SupplicantDeathHandler : virtual public SupplicantDeathEventHandler {
    virtual void OnDeath() override;
  };

  static WifiHal* sWifiHal;
  static WificondControl* sWificondControl;
  static SupplicantStaManager* sSupplicantStaManager;

  std::string mStaInterfaceName;

  EventCallback mEventCallback;
};

#endif  // WifiNative_h
