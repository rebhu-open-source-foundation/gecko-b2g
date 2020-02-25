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
#include "SoftapManager.h"
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
  bool GetCapabilities(uint32_t& aCapabilities);
  bool GetDriverModuleInfo(nsAString& aDriverVersion,
                           nsAString& aFirmwareVersion);
  bool SetLowLatencyMode(bool aEnable);
  bool SetConcurrencyPriority(bool aEnable);
  bool GetHostWakeReason(uint32_t& aCapabilities);

  bool StartWifi();
  bool StopWifi();
  bool GetMacAddress(nsAString& aMacAddress);
  bool GetClientInterfaceName(nsAString& aIfaceName);
  bool GetSoftApInterfaceName(nsAString& aIfaceName);

  bool StartSupplicant();
  bool StopSupplicant();
  bool GetStaCapabilities(uint32_t& aStaCapabilities);
  bool GetDebugLevel(uint32_t& aDebugLevel);
  bool SetDebugLevel(SupplicantDebugLevelOptions* aLevel);
  bool SetPowerSave(bool aEnable);
  bool SetSuspendMode(bool aEnable);
  bool SetExternalSim(bool aEnable);
  bool SetAutoReconnect(bool aEnable);
  bool SetCountryCode(const nsAString& aCountryCode);
  bool SetBtCoexistenceMode(uint8_t aMode);
  bool SetBtCoexistenceScanMode(bool aEnable);

  bool StartSingleScan(ScanSettingsOptions* aScanSettings);
  bool StopSingleScan();
  bool StartPnoScan();
  bool StopPnoScan();
  bool GetScanResults(std::vector<NativeScanResult>& aScanResults);
  bool GetPnoScanResults(std::vector<NativeScanResult>& aScanResults);
  bool GetChannelsForBand(uint32_t aBandMask, std::vector<int32_t>& aChannels);

  bool Connect(ConfigurationOptions* aConfig);
  bool Reconnect();
  bool Reassociate();
  bool Disconnect();
  bool RemoveNetworks();

  bool StartSoftAp(SoftapConfigurationOptions* aSoftapConfig,
                   nsAString& aIfaceName);
  bool StopSoftAp();
  bool StartAndConnectHostapd();
  bool StopHostapd();

  class SupplicantDeathHandler : virtual public SupplicantDeathEventHandler {
    virtual void OnDeath() override;
  };

  static WifiHal* s_WifiHal;
  static WificondControl* s_WificondControl;
  static SupplicantStaManager* s_SupplicantStaManager;
  static SoftapManager* s_SoftapManager;

  std::string mStaInterfaceName;
  std::string mApInterfaceName;

  EventCallback mEventCallback;
};

#endif  // WifiNative_h
