/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "WifiNative"

#include "WifiNative.h"
#include "WificondEventService.h"
#include <cutils/properties.h>
#include <string.h>
#include "js/CharacterEncoding.h"

using namespace mozilla::dom;

static const int32_t CONNECTION_RETRY_INTERVAL_US = 100000;
static const int32_t CONNECTION_RETRY_TIMES = 50;

/* static */
WifiHal* WifiNative::s_WifiHal = nullptr;
WificondControl* WifiNative::s_WificondControl = nullptr;
SoftapManager* WifiNative::s_SoftapManager = nullptr;
SupplicantStaManager* WifiNative::s_SupplicantStaManager = nullptr;

WifiNative::WifiNative(EventCallback aCallback) {
  s_WifiHal = WifiHal::Get();
  s_WificondControl = WificondControl::Get();
  s_SoftapManager = SoftapManager::Get();
  s_SupplicantStaManager = SupplicantStaManager::Get();

  mEventCallback = aCallback;
  s_SupplicantStaManager->RegisterEventCallback(mEventCallback);
}

bool WifiNative::ExecuteCommand(CommandOptions& aOptions, nsWifiResult* aResult,
                                const nsCString& aInterface) {
  // Always correlate the opaque ids.
  aResult->mId = aOptions.mId;

  if (aOptions.mCmd == nsIWifiCommand::INITIALIZE) {
    aResult->mStatus = InitHal();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_MODULE_VERSION) {
    aResult->mStatus =
        GetDriverModuleInfo(aResult->mDriverVersion, aResult->mFirmwareVersion);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_CAPABILITIES) {
    aResult->mStatus = GetCapabilities(aResult->mCapabilities);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_LOW_LATENCY_MODE) {
    aResult->mStatus = SetLowLatencyMode(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_CONCURRENCY_PRIORITY) {
    aResult->mStatus = SetConcurrencyPriority(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::START_WIFI) {
    aResult->mStatus = StartWifi();
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_WIFI) {
    aResult->mStatus = StopWifi();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_MAC_ADDRESS) {
    aResult->mStatus = GetMacAddress(aResult->mMacAddress);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_STA_IFACE) {
    aResult->mStatus = GetClientInterfaceName(aResult->mStaInterface);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_STA_CAPABILITIES) {
    aResult->mStatus = GetStaCapabilities(aResult->mStaCapabilities);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_DEBUG_LEVEL) {
    aResult->mStatus = GetDebugLevel(aResult->mDebugLevel);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_DEBUG_LEVEL) {
    aResult->mStatus = SetDebugLevel(&aOptions.mDebugLevel);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_POWER_SAVE) {
    aResult->mStatus = SetPowerSave(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_SUSPEND_MODE) {
    aResult->mStatus = SetSuspendMode(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_EXTERNAL_SIM) {
    aResult->mStatus = SetExternalSim(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_AUTO_RECONNECT) {
    aResult->mStatus = SetAutoReconnect(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_COUNTRY_CODE) {
    aResult->mStatus = SetCountryCode(aOptions.mCountryCode);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_BT_COEXIST_MODE) {
    aResult->mStatus = SetBtCoexistenceMode(aOptions.mBtCoexistenceMode);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_BT_COEXIST_SCAN_MODE) {
    aResult->mStatus = SetBtCoexistenceScanMode(aOptions.mEnabled);
  } else if (aOptions.mCmd == nsIWifiCommand::START_SINGLE_SCAN) {
    aResult->mStatus = StartSingleScan(&aOptions.mScanSettings);
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_SINGLE_SCAN) {
    aResult->mStatus = StopSingleScan();
  } else if (aOptions.mCmd == nsIWifiCommand::START_PNO_SCAN) {
    aResult->mStatus = StartPnoScan();
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_PNO_SCAN) {
    aResult->mStatus = StopPnoScan();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_SCAN_RESULTS) {
    std::vector<NativeScanResult> nativeScanResults;
    aResult->mStatus = GetScanResults(nativeScanResults);

    if (nativeScanResults.empty()) {
      WIFI_LOGD(LOG_TAG, "No scan result available");
      return false;
    }
    size_t num = nativeScanResults.size();
    nsTArray<RefPtr<nsScanResult>> scanResults(num);

    for (auto result : nativeScanResults) {
      std::string ssid_str(result.ssid.begin(), result.ssid.end());
      std::string bssid_str = ConvertMacToString(result.bssid);
      nsString ssid(NS_ConvertUTF8toUTF16(ssid_str.c_str()));
      nsString bssid(NS_ConvertUTF8toUTF16(bssid_str.c_str()));
      uint32_t frequency = result.frequency;
      uint32_t tsf = result.tsf;
      uint32_t capability = result.capability;

      int32_t signal = result.signal_mbm;
      bool associated = result.associated;

      size_t ie_size = result.info_element.size();
      nsTArray<uint8_t> info_element(ie_size);
      for (auto& element : result.info_element) {
        info_element.AppendElement(element);
      }
      RefPtr<nsScanResult> scanResult =
          new nsScanResult(ssid, bssid, info_element, frequency, tsf,
                           capability, signal, associated);
      scanResults.AppendElement(scanResult);
    }
    aResult->updateScanResults(scanResults);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_PNO_SCAN_RESULTS) {
    std::vector<NativeScanResult> nativeScanResults;
    aResult->mStatus = GetPnoScanResults(nativeScanResults);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_CHANNELS_FOR_BAND) {
    std::vector<int32_t> channels;
    aResult->mStatus = GetChannelsForBand(aOptions.mBandMask, channels);
    size_t num = channels.size();
    if (num > 0) {
      nsTArray<int32_t> channel_array(num);
      for (int32_t& ch : channels) {
        channel_array.AppendElement(ch);
      }
      aResult->updateChannels(channel_array);
    }
  } else if (aOptions.mCmd == nsIWifiCommand::CONNECT) {
    aResult->mStatus = Connect(&aOptions.mConfig);
  } else if (aOptions.mCmd == nsIWifiCommand::RECONNECT) {
    aResult->mStatus = Reconnect();
  } else if (aOptions.mCmd == nsIWifiCommand::REASSOCIATE) {
    aResult->mStatus = Reassociate();
  } else if (aOptions.mCmd == nsIWifiCommand::DISCONNECT) {
    aResult->mStatus = Disconnect();
  } else if (aOptions.mCmd == nsIWifiCommand::REMOVE_NETWORKS) {
    aResult->mStatus = RemoveNetworks();
  } else if (aOptions.mCmd == nsIWifiCommand::START_SOFTAP) {
    aResult->mStatus =
        StartSoftAp(&aOptions.mSoftapConfig, aResult->mApInterface);
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_SOFTAP) {
    aResult->mStatus = StopSoftAp();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_AP_IFACE) {
    aResult->mStatus = GetSoftApInterfaceName(aResult->mApInterface);
  } else {
    WIFI_LOGE(LOG_TAG, "ExecuteCommand: Unknown command %d", aOptions.mCmd);
    return false;
  }
  WIFI_LOGD(LOG_TAG, "command result: id=%d, status=%d", aResult->mId,
            aResult->mStatus);

  return true;
}

bool WifiNative::InitHal() {
  // make sure wifi hal is ready
  if (!s_WifiHal->InitHalInterface()) {
    return false;
  }

  if (!s_WificondControl->InitWificondInterface()) {
    return false;
  }

  // init supplicant hal
  if (!s_SupplicantStaManager->IsInterfaceInitializing() &&
      !s_SupplicantStaManager->InitInterface()) {
    return false;
  }
  return true;
}

bool WifiNative::DeinitHal() { return true; }

bool WifiNative::GetCapabilities(uint32_t& aCapabilities) {
  return s_WifiHal->GetCapabilities(aCapabilities);
}

bool WifiNative::GetDriverModuleInfo(nsAString& aDriverVersion,
                                     nsAString& aFirmwareVersion) {
  return s_WifiHal->GetDriverModuleInfo(aDriverVersion, aFirmwareVersion);
}

bool WifiNative::SetLowLatencyMode(bool aEnable) {
  return true;  // s_WifiHal->SetLowLatencyMode(aEnable);
}

bool WifiNative::SetConcurrencyPriority(bool aEnable) {
  return s_SupplicantStaManager->SetConcurrencyPriority(aEnable);
}

/**
 * StartWifi() - to enable wifi
 *
 * 1. load wifi driver module, configure chip.
 * 2. setup client mode interface.
 * 3. start supplicant.
 *
 */
bool WifiNative::StartWifi() {
  if (!s_WifiHal->StartWifiModule()) {
    WIFI_LOGE(LOG_TAG, "Failed to start wifi");
    return false;
  }

  WIFI_LOGD(LOG_TAG, "module loaded, try to configure...");
  s_WifiHal->ConfigChipAndCreateIface(wifiNameSpace::IfaceType::STA,
                                      mStaInterfaceName);

  WificondEventService* eventService =
      WificondEventService::CreateService(mStaInterfaceName);
  if (eventService == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to create scan event service");
    return false;
  }
  eventService->RegisterEventCallback(mEventCallback);

  if (!StartSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant");
    return false;
  }
  // supplicant initialized, register death handler
  SupplicantDeathHandler* deathHandler = new SupplicantDeathHandler();
  s_SupplicantStaManager->RegisterDeathHandler(deathHandler);

  if (!s_WificondControl->SetupClientIface(
          mStaInterfaceName,
          android::interface_cast<android::net::wifi::IScanEvent>(
              eventService))) {
    WIFI_LOGE(LOG_TAG, "Failed to setup iface in wificond");
    s_WificondControl->TearDownClientInterface(mStaInterfaceName);
    return false;
  }

  if (!s_SupplicantStaManager->SetupStaInterface(mStaInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to setup iface in supplicant");
    return false;
  }
  return true;
}

bool WifiNative::StopWifi() {
  if (!StopSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to stop supplicant");
    return false;
  }

  // teardown wificond interfaces.
  if (!s_WificondControl->TearDownClientInterface(mStaInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown wificond interfaces");
    return false;
  }

  // unregister supplicant death Handler
  s_SupplicantStaManager->UnregisterDeathHandler();

  if (!s_WifiHal->TearDownInterface(wifiNameSpace::IfaceType::STA)) {
    WIFI_LOGE(LOG_TAG, "Failed to stop wifi");
    return false;
  }
  return true;
}

/**
 * Steps to setup supplicant.
 *
 * 1. initialize supplicant hidl client.
 * 2. start supplicant daemon through wificond or ctl.stat.
 * 3. wait for hidl client registration ready.
 *
 */
bool WifiNative::StartSupplicant() {
  // start supplicant hal
  if (!s_SupplicantStaManager->IsInterfaceReady() &&
      !s_SupplicantStaManager->InitInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant hal");
    return false;
  }

  // start supplicant from wificond.
  if (!s_WificondControl->StartSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to start supplicant daemon");
    return false;
  }

  bool connected = false;
  int32_t connectTries = 0;
  while (!connected && connectTries++ < CONNECTION_RETRY_TIMES) {
    // Check if the initialization is complete.
    if (s_SupplicantStaManager->IsInterfaceReady()) {
      connected = true;
      break;
    }
    usleep(CONNECTION_RETRY_INTERVAL_US);
  }
  return connected;
}

bool WifiNative::StopSupplicant() {
  // teardown supplicant hal interfaces
  if (!s_SupplicantStaManager->DeinitInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown iface in supplicant");
    return false;
  }

  // TODO: stop supplicant daemon for android 8.1
  if (!s_WificondControl->StopSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to stop supplicant");
    return false;
  }

  return true;
}

bool WifiNative::GetMacAddress(nsAString& aMacAddress) {
  return s_SupplicantStaManager->GetMacAddress(aMacAddress);
}

bool WifiNative::GetClientInterfaceName(nsAString& aIfaceName) {
  nsString iface(NS_ConvertUTF8toUTF16(mStaInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return (aIfaceName.Length() > 0 ? true : false);
}

bool WifiNative::GetSoftApInterfaceName(nsAString& aIfaceName) {
  nsString iface(NS_ConvertUTF8toUTF16(mApInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return (aIfaceName.Length() > 0 ? true : false);
}

bool WifiNative::GetStaCapabilities(uint32_t& aStaCapabilities) {
  return s_WifiHal->GetStaCapabilities(aStaCapabilities);
}

bool WifiNative::GetDebugLevel(uint32_t& aLevel) {
  return s_SupplicantStaManager->GetSupplicantDebugLevel(aLevel);
}

bool WifiNative::SetDebugLevel(SupplicantDebugLevelOptions* aLevel) {
  return s_SupplicantStaManager->SetSupplicantDebugLevel(aLevel);
}

bool WifiNative::SetPowerSave(bool aEnable) {
  return s_SupplicantStaManager->SetPowerSave(aEnable);
}

bool WifiNative::SetSuspendMode(bool aEnable) {
  return s_SupplicantStaManager->SetSuspendMode(aEnable);
}

bool WifiNative::SetExternalSim(bool aEnable) {
  return s_SupplicantStaManager->SetExternalSim(aEnable);
}

bool WifiNative::SetAutoReconnect(bool aEnable) {
  return s_SupplicantStaManager->SetAutoReconnect(aEnable);
}

bool WifiNative::SetBtCoexistenceMode(uint8_t aMode) {
  return s_SupplicantStaManager->SetBtCoexistenceMode(aMode);
}

bool WifiNative::SetBtCoexistenceScanMode(bool aEnable) {
  return s_SupplicantStaManager->SetBtCoexistenceScanMode(aEnable);
}

bool WifiNative::SetCountryCode(const nsAString& aCountryCode) {
  std::string countryCode = NS_ConvertUTF16toUTF8(aCountryCode).get();
  return s_SupplicantStaManager->SetCountryCode(countryCode);
}

bool WifiNative::StartSingleScan(ScanSettingsOptions* aScanSettings) {
  return s_WificondControl->StartSingleScan(aScanSettings);
}

bool WifiNative::StopSingleScan() {
  return s_WificondControl->StopSingleScan();
}

bool WifiNative::StartPnoScan() { return true; }

bool WifiNative::StopPnoScan() { return true; }

bool WifiNative::GetScanResults(std::vector<NativeScanResult>& aScanResults) {
  return s_WificondControl->GetScanResults(aScanResults);
}

bool WifiNative::GetPnoScanResults(
    std::vector<NativeScanResult>& aScanResults) {
  return true;
}

bool WifiNative::GetChannelsForBand(uint32_t aBandMask,
                                    std::vector<int32_t>& aChannels) {
  return s_WificondControl->GetChannelsForBand(aBandMask, aChannels);
}

bool WifiNative::Connect(ConfigurationOptions* aConfig) {
  // abort ongoing scan before connect
  s_WificondControl->StopSingleScan();

  if (!s_SupplicantStaManager->ConnectToNetwork(aConfig)) {
    WIFI_LOGE(LOG_TAG, "Failed to connect %s",
              NS_ConvertUTF16toUTF8(aConfig->mSsid).get());
    return false;
  }
  return true;
}

bool WifiNative::Reconnect() { return s_SupplicantStaManager->Reconnect(); }

bool WifiNative::Reassociate() { return s_SupplicantStaManager->Reassociate(); }

bool WifiNative::Disconnect() { return s_SupplicantStaManager->Disconnect(); }

bool WifiNative::RemoveNetworks() {
  return s_SupplicantStaManager->RemoveNetworks();
}

bool WifiNative::StartSoftAp(SoftapConfigurationOptions* aSoftapConfig,
                             nsAString& aIfaceName) {
  // Load wifi driver module and configure as ap mode.
  if (!s_WifiHal->StartWifiModule()) {
    return false;
  }
  if (!StartAndConnectHostapd()) {
    return false;
  }
  if (!s_WifiHal->ConfigChipAndCreateIface(wifiNameSpace::IfaceType::AP,
                                           mApInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to create AP interface");
    return false;
  }
  SoftapEventService* eventService =
      SoftapEventService::CreateService(mApInterfaceName);
  if (eventService == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to create softap event service");
    return false;
  }
  eventService->RegisterEventCallback(mEventCallback);
  if (!s_WificondControl->SetupApIface(
          mApInterfaceName,
          android::interface_cast<
              android::net::wifi::IApInterfaceEventCallback>(eventService))) {
    WIFI_LOGE(LOG_TAG, "Failed to setup softap iface in wificond");
    s_WificondControl->TearDownSoftapInterface(mApInterfaceName);
    return false;
  }
  // Up to now, ap interface should be ready to setup country code.
  std::string countryCode =
      NS_ConvertUTF16toUTF8(aSoftapConfig->mCountryCode).get();
  if (!s_WifiHal->SetSoftapCountryCode(countryCode)) {
    WIFI_LOGE(LOG_TAG, "Failed to set country code");
    return false;
  }
  s_SoftapManager->SetSoftapCountryCode(countryCode);

  // start softap from hostapd.
  if (!s_SoftapManager->StartSoftap(mApInterfaceName, aSoftapConfig)) {
    WIFI_LOGE(LOG_TAG, "Failed to start softap");
    return false;
  }

  nsString iface(NS_ConvertUTF8toUTF16(mApInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return (aIfaceName.Length() > 0 ? true : false);
}

bool WifiNative::StopSoftAp() {
  if (!s_SoftapManager->StopSoftap(mApInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to stop softap");
    return false;
  }
  if (!s_WificondControl->TearDownSoftapInterface(mApInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown ap interface in wificond");
    return false;
  }
  if (!StopHostapd()) {
    WIFI_LOGE(LOG_TAG, "Failed to stop hostapd");
    return false;
  }
  if (!s_WifiHal->TearDownInterface(wifiNameSpace::IfaceType::AP)) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown softap interface");
    return false;
  }
  return true;
}

bool WifiNative::StartAndConnectHostapd() {
  if (!s_SoftapManager->InitInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize hostapd interface");
    return false;
  }
  bool connected = false;
  int32_t connectTries = 0;
  while (!connected && connectTries++ < CONNECTION_RETRY_TIMES) {
    // Check if the initialization is complete.
    if (s_SoftapManager->IsInterfaceReady()) {
      connected = true;
      break;
    }
    usleep(CONNECTION_RETRY_INTERVAL_US);
  }
  return connected;
}

bool WifiNative::StopHostapd() {
  if (!s_SoftapManager->DeinitInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to tear down hostapd interface");
    return false;
  }
  return true;
}

void WifiNative::SupplicantDeathHandler::OnDeath() {
  // supplicant died, start to clean up.
  WIFI_LOGE(LOG_TAG, "Supplicant DIED: ##############################");
}
