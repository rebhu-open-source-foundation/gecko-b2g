/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "WifiNative"

#include "WifiNative.h"
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
WifiEventCallback* WifiNative::s_Callback = nullptr;

WifiNative::WifiNative()
    : mScanEventService(nullptr),
      mPnoScanEventService(nullptr),
      mSoftapEventService(nullptr) {
  s_WifiHal = WifiHal::Get();
  s_WificondControl = WificondControl::Get();
  s_SoftapManager = SoftapManager::Get();
  s_SupplicantStaManager = SupplicantStaManager::Get();
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
    aResult->mStatus = StartWifi(aResult->mStaInterface);
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
  } else if (aOptions.mCmd == nsIWifiCommand::GET_LINK_LAYER_STATS) {
    wifiNameSpace::StaLinkLayerStats stats;
    aResult->mStatus = GetLinkLayerStats(stats);

    RefPtr<nsLinkLayerStats> linkLayerStats = new nsLinkLayerStats(
        stats.iface.beaconRx, stats.iface.avgRssiMgmt, stats.timeStampInMs);

    RefPtr<nsLinkLayerPacketStats> wmeBePktStats = new nsLinkLayerPacketStats(
        stats.iface.wmeBePktStats.rxMpdu, stats.iface.wmeBePktStats.txMpdu,
        stats.iface.wmeBePktStats.lostMpdu, stats.iface.wmeBePktStats.retries);

    RefPtr<nsLinkLayerPacketStats> wmeBkPktStats = new nsLinkLayerPacketStats(
        stats.iface.wmeBkPktStats.rxMpdu, stats.iface.wmeBkPktStats.txMpdu,
        stats.iface.wmeBkPktStats.lostMpdu, stats.iface.wmeBkPktStats.retries);

    RefPtr<nsLinkLayerPacketStats> wmeViPktStats = new nsLinkLayerPacketStats(
        stats.iface.wmeViPktStats.rxMpdu, stats.iface.wmeViPktStats.txMpdu,
        stats.iface.wmeViPktStats.lostMpdu, stats.iface.wmeViPktStats.retries);

    RefPtr<nsLinkLayerPacketStats> wmeVoPktStats = new nsLinkLayerPacketStats(
        stats.iface.wmeVoPktStats.rxMpdu, stats.iface.wmeVoPktStats.txMpdu,
        stats.iface.wmeVoPktStats.lostMpdu, stats.iface.wmeVoPktStats.retries);

    size_t numRadio = stats.radios.size();
    nsTArray<RefPtr<nsLinkLayerRadioStats>> radios(numRadio);

    for (auto& radio : stats.radios) {
      size_t numTxTime = radio.txTimeInMsPerLevel.size();
      nsTArray<uint32_t> txTimeInMsPerLevel(numTxTime);

      for (auto& txTime : radio.txTimeInMsPerLevel) {
        txTimeInMsPerLevel.AppendElement(txTime);
      }
      RefPtr<nsLinkLayerRadioStats> radioStats = new nsLinkLayerRadioStats(
          radio.onTimeInMs, radio.txTimeInMs, radio.rxTimeInMs,
          radio.onTimeInMsForScan, txTimeInMsPerLevel);
      radios.AppendElement(radioStats);
    }

    linkLayerStats->updatePacketStats(wmeBePktStats, wmeBkPktStats,
                                      wmeViPktStats, wmeVoPktStats);
    linkLayerStats->updateRadioStats(radios);
    aResult->updateLinkLayerStats(linkLayerStats);
  } else if (aOptions.mCmd == nsIWifiCommand::SIGNAL_POLL) {
    std::vector<int32_t> pollResult;
    aResult->mStatus = SignalPoll(pollResult);

    size_t num = pollResult.size();
    if (num > 0) {
      nsTArray<int32_t> pollArray(num);
      for (int32_t& element : pollResult) {
        pollArray.AppendElement(element);
      }
      aResult->updateSignalPoll(pollArray);
    }
  } else if (aOptions.mCmd == nsIWifiCommand::START_SINGLE_SCAN) {
    aResult->mStatus = StartSingleScan(&aOptions.mScanSettings);
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_SINGLE_SCAN) {
    aResult->mStatus = StopSingleScan();
  } else if (aOptions.mCmd == nsIWifiCommand::START_PNO_SCAN) {
    aResult->mStatus = StartPnoScan(&aOptions.mPnoScanSettings);
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_PNO_SCAN) {
    aResult->mStatus = StopPnoScan();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_SCAN_RESULTS) {
    std::vector<Wificond::NativeScanResult> nativeScanResults;

    if (aOptions.mScanType == nsIScanSettings::USE_SINGLE_SCAN) {
      aResult->mStatus = GetScanResults(nativeScanResults);
    } else if (aOptions.mScanType == nsIScanSettings::USE_PNO_SCAN) {
      aResult->mStatus = GetPnoScanResults(nativeScanResults);
    } else {
      WIFI_LOGE(LOG_TAG, "Invalid scan type: %d", aOptions.mScanType);
    }

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
  } else if (aOptions.mCmd == nsIWifiCommand::GET_SOFTAP_STATION_NUMBER) {
    aResult->mStatus = GetSoftapStations(aResult->mNumStations);
  } else {
    WIFI_LOGE(LOG_TAG, "ExecuteCommand: Unknown command %d", aOptions.mCmd);
    return false;
  }
  WIFI_LOGD(LOG_TAG, "command result: id=%d, status=%d", aResult->mId,
            aResult->mStatus);

  return true;
}

void WifiNative::RegisterEventCallback(WifiEventCallback* aCallback) {
  s_Callback = aCallback;
  if (s_SupplicantStaManager) {
    s_SupplicantStaManager->RegisterEventCallback(s_Callback);
  }
}

void WifiNative::UnregisterEventCallback() {
  if (s_SupplicantStaManager) {
    s_SupplicantStaManager->UnregisterEventCallback();
  }
  s_Callback = nullptr;
}

Result_t WifiNative::InitHal() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  // make sure wifi hal is ready
  result = s_WifiHal->InitHalInterface();
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }

  result = s_WificondControl->InitWificondInterface();
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }

  // init supplicant hal
  if (!s_SupplicantStaManager->IsInterfaceInitializing()) {
    result = s_SupplicantStaManager->InitInterface();
    if (result != nsIWifiResult::SUCCESS) {
      return result;
    }
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiNative::DeinitHal() { return nsIWifiResult::SUCCESS; }

Result_t WifiNative::GetCapabilities(uint32_t& aCapabilities) {
  return s_WifiHal->GetCapabilities(aCapabilities);
}

Result_t WifiNative::GetDriverModuleInfo(nsAString& aDriverVersion,
                                         nsAString& aFirmwareVersion) {
  return s_WifiHal->GetDriverModuleInfo(aDriverVersion, aFirmwareVersion);
}

Result_t WifiNative::SetLowLatencyMode(bool aEnable) {
  return s_WifiHal->SetLowLatencyMode(aEnable);
}

Result_t WifiNative::SetConcurrencyPriority(bool aEnable) {
  return s_SupplicantStaManager->SetConcurrencyPriority(aEnable);
}

/**
 * To enable wifi and start supplicant
 *
 * @param aIfaceName - output wlan module interface name
 *
 * 1. load wifi driver module, configure chip.
 * 2. setup client mode interface.
 * 3. start supplicant.
 */
Result_t WifiNative::StartWifi(nsAString& aIfaceName) {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  result = s_WifiHal->StartWifiModule();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to start wifi");
    return result;
  }

  WIFI_LOGD(LOG_TAG, "module loaded, try to configure...");
  result = s_WifiHal->ConfigChipAndCreateIface(wifiNameSpace::IfaceType::STA,
                                               mStaInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to create sta interface");
    return result;
  } else {
    s_WifiHal->EnableLinkLayerStats();
  }

  // here create scan and pno scan event service,
  // which implement scan callback from wificond
  mScanEventService = ScanEventService::CreateService(mStaInterfaceName);
  if (mScanEventService == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to create scan event service");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  mScanEventService->RegisterEventCallback(s_Callback);

  mPnoScanEventService = PnoScanEventService::CreateService(mStaInterfaceName);
  if (mPnoScanEventService == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to create pno scan event service");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  mPnoScanEventService->RegisterEventCallback(s_Callback);

  result = StartSupplicant();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant");
    return result;
  }

  // supplicant initialized, register death handler
  SupplicantDeathHandler* deathHandler = new SupplicantDeathHandler();
  s_SupplicantStaManager->RegisterDeathHandler(deathHandler);

  result = s_WificondControl->SetupClientIface(
      mStaInterfaceName,
      android::interface_cast<android::net::wifi::IScanEvent>(
          mScanEventService),
      android::interface_cast<android::net::wifi::IPnoScanEvent>(
          mPnoScanEventService));
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to setup iface in wificond");
    s_WificondControl->TearDownClientInterface(mStaInterfaceName);
    return result;
  }

  result = s_SupplicantStaManager->SetupStaInterface(mStaInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to setup iface in supplicant");
    return result;
  }

  nsString iface(NS_ConvertUTF8toUTF16(mStaInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return CHECK_SUCCESS(aIfaceName.Length() > 0);
}

/**
 * To disable wifi
 *
 * 1. clean supplicant hidl client and stop supplicant
 * 2. clean client interfaces in wificond
 * 3. clean wifi hidl client and unload wlan module
 */
Result_t WifiNative::StopWifi() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  result = StopSupplicant();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to stop supplicant");
    return result;
  }

  if (mScanEventService) {
    mScanEventService->UnregisterEventCallback();
  }
  if (mPnoScanEventService) {
    mPnoScanEventService->UnregisterEventCallback();
  }

  // teardown wificond interfaces.
  result = s_WificondControl->TearDownClientInterface(mStaInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown wificond interfaces");
    return result;
  }

  // unregister supplicant death Handler
  s_SupplicantStaManager->UnregisterDeathHandler();

  result = s_WifiHal->TearDownInterface(wifiNameSpace::IfaceType::STA);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to stop wifi");
    return result;
  }
  return nsIWifiResult::SUCCESS;
}

/**
 * Steps to setup supplicant.
 *
 * 1. initialize supplicant hidl client.
 * 2. start supplicant daemon through wificond or ctl.stat.
 * 3. wait for hidl client registration ready.
 */
Result_t WifiNative::StartSupplicant() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  // start supplicant hal
  if (!s_SupplicantStaManager->IsInterfaceReady()) {
    result = s_SupplicantStaManager->InitInterface();
    if (result != nsIWifiResult::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant hal");
      return result;
    }
  }

  // start supplicant from wificond.
  result = s_WificondControl->StartSupplicant();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to start supplicant daemon");
    return result;
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
  return CHECK_SUCCESS(connected);
}

Result_t WifiNative::StopSupplicant() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  // teardown supplicant hal interfaces
  result = s_SupplicantStaManager->DeinitInterface();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown iface in supplicant");
    return result;
  }

  // TODO: stop supplicant daemon for android 8.1
  result = s_WificondControl->StopSupplicant();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to stop supplicant");
    return result;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiNative::GetMacAddress(nsAString& aMacAddress) {
  return s_SupplicantStaManager->GetMacAddress(aMacAddress);
}

Result_t WifiNative::GetClientInterfaceName(nsAString& aIfaceName) {
  nsString iface(NS_ConvertUTF8toUTF16(mStaInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return CHECK_SUCCESS(aIfaceName.Length() > 0);
}

Result_t WifiNative::GetSoftApInterfaceName(nsAString& aIfaceName) {
  nsString iface(NS_ConvertUTF8toUTF16(mApInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return CHECK_SUCCESS(aIfaceName.Length() > 0);
}

Result_t WifiNative::GetStaCapabilities(uint32_t& aStaCapabilities) {
  return s_WifiHal->GetStaCapabilities(aStaCapabilities);
}

Result_t WifiNative::GetDebugLevel(uint32_t& aLevel) {
  return s_SupplicantStaManager->GetSupplicantDebugLevel(aLevel);
}

Result_t WifiNative::SetDebugLevel(SupplicantDebugLevelOptions* aLevel) {
  return s_SupplicantStaManager->SetSupplicantDebugLevel(aLevel);
}

Result_t WifiNative::SetPowerSave(bool aEnable) {
  return s_SupplicantStaManager->SetPowerSave(aEnable);
}

Result_t WifiNative::SetSuspendMode(bool aEnable) {
  return s_SupplicantStaManager->SetSuspendMode(aEnable);
}

Result_t WifiNative::SetExternalSim(bool aEnable) {
  return s_SupplicantStaManager->SetExternalSim(aEnable);
}

Result_t WifiNative::SetAutoReconnect(bool aEnable) {
  return s_SupplicantStaManager->SetAutoReconnect(aEnable);
}

Result_t WifiNative::SetBtCoexistenceMode(uint8_t aMode) {
  return s_SupplicantStaManager->SetBtCoexistenceMode(aMode);
}

Result_t WifiNative::SetBtCoexistenceScanMode(bool aEnable) {
  return s_SupplicantStaManager->SetBtCoexistenceScanMode(aEnable);
}

Result_t WifiNative::SignalPoll(std::vector<int32_t>& aPollResult) {
  return s_WificondControl->SignalPoll(aPollResult);
}

Result_t WifiNative::GetLinkLayerStats(
    wifiNameSpace::StaLinkLayerStats& aStats) {
  return s_WifiHal->GetLinkLayerStats(aStats);
}

Result_t WifiNative::SetCountryCode(const nsAString& aCountryCode) {
  std::string countryCode = NS_ConvertUTF16toUTF8(aCountryCode).get();
  return s_SupplicantStaManager->SetCountryCode(countryCode);
}

Result_t WifiNative::StartSingleScan(ScanSettingsOptions* aScanSettings) {
  return s_WificondControl->StartSingleScan(aScanSettings);
}

Result_t WifiNative::StopSingleScan() {
  return s_WificondControl->StopSingleScan();
}

Result_t WifiNative::StartPnoScan(PnoScanSettingsOptions* aPnoScanSettings) {
  return s_WificondControl->StartPnoScan(aPnoScanSettings);
}

Result_t WifiNative::StopPnoScan() {
  return s_WificondControl->StopPnoScan();
}

Result_t WifiNative::GetScanResults(
    std::vector<Wificond::NativeScanResult>& aScanResults) {
  return s_WificondControl->GetScanResults(aScanResults);
}

Result_t WifiNative::GetPnoScanResults(
    std::vector<Wificond::NativeScanResult>& aPnoScanResults) {
  return s_WificondControl->GetPnoScanResults(aPnoScanResults);
}

Result_t WifiNative::GetChannelsForBand(uint32_t aBandMask,
                                        std::vector<int32_t>& aChannels) {
  return s_WificondControl->GetChannelsForBand(aBandMask, aChannels);
}

/**
 * To make wifi connection with assigned configuration
 *
 * @param aConfig - the network configuration to be set
 */
Result_t WifiNative::Connect(ConfigurationOptions* aConfig) {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  // abort ongoing scan before connect
  s_WificondControl->StopSingleScan();

  result = s_SupplicantStaManager->ConnectToNetwork(aConfig);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to connect %s",
              NS_ConvertUTF16toUTF8(aConfig->mSsid).get());
    return result;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiNative::Reconnect() {
  return s_SupplicantStaManager->Reconnect();
}

Result_t WifiNative::Reassociate() {
  return s_SupplicantStaManager->Reassociate();
}

Result_t WifiNative::Disconnect() {
  return s_SupplicantStaManager->Disconnect();
}

/**
 * To remove all configured networks in supplicant
 */
Result_t WifiNative::RemoveNetworks() {
  return s_SupplicantStaManager->RemoveNetworks();
}

/**
 * To enable wifi hotspot
 *
 * @param aSoftapConfig - the softap configuration to be set
 * @param aIfaceName - out the interface name for AP mode
 *
 * 1. load driver module and configure chip as AP mode
 * 2. start hostapd hidl service an register callback
 * 3. with lazy hal designed, hostapd daemon should be
 *    started while getService() of IHostapd
 * 4. setup ap in wificond, which will listen to event from driver
 */
Result_t WifiNative::StartSoftAp(SoftapConfigurationOptions* aSoftapConfig,
                                 nsAString& aIfaceName) {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  // Load wifi driver module and configure as ap mode.
  result = s_WifiHal->StartWifiModule();
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }
  result = StartAndConnectHostapd();
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }
  result = s_WifiHal->ConfigChipAndCreateIface(wifiNameSpace::IfaceType::AP,
                                               mApInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to create AP interface");
    return result;
  }
  mSoftapEventService = SoftapEventService::CreateService(mApInterfaceName);
  if (mSoftapEventService == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to create softap event service");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  mSoftapEventService->RegisterEventCallback(s_Callback);

  result = s_WificondControl->SetupApIface(
      mApInterfaceName,
      android::interface_cast<android::net::wifi::IApInterfaceEventCallback>(
          mSoftapEventService));
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to setup softap iface in wificond");
    s_WificondControl->TearDownSoftapInterface(mApInterfaceName);
    return result;
  }
  // Up to now, ap interface should be ready to setup country code.
  std::string countryCode =
      NS_ConvertUTF16toUTF8(aSoftapConfig->mCountryCode).get();
  result = s_WifiHal->SetSoftapCountryCode(countryCode);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to set country code");
    return result;
  }

  // start softap from hostapd.
  result = s_SoftapManager->StartSoftap(mApInterfaceName,
                                        countryCode,
                                        aSoftapConfig);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to start softap");
    return result;
  }

  nsString iface(NS_ConvertUTF8toUTF16(mApInterfaceName.c_str()));
  aIfaceName.Assign(iface);
  return CHECK_SUCCESS(aIfaceName.Length() > 0);
}

/**
 * To disable wifi hotspot
 *
 * 1. clean hostapd hidl client and stop daemon
 * 2. clean ap interfaces in wificond
 * 3. clean wifi hidl client and unload wlan module
 */
Result_t WifiNative::StopSoftAp() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  result = s_SoftapManager->StopSoftap(mApInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to stop softap");
    return result;
  }
  if (mSoftapEventService) {
    mSoftapEventService->UnregisterEventCallback();
  }
  result = s_WificondControl->TearDownSoftapInterface(mApInterfaceName);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown ap interface in wificond");
    return result;
  }
  result = StopHostapd();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to stop hostapd");
    return result;
  }
  result = s_WifiHal->TearDownInterface(wifiNameSpace::IfaceType::AP);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown softap interface");
    return result;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiNative::StartAndConnectHostapd() {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  result = s_SoftapManager->InitInterface();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize hostapd interface");
    return result;
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
  return CHECK_SUCCESS(connected);
}

Result_t WifiNative::StopHostapd() {
  Result_t result = s_SoftapManager->DeinitInterface();
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to tear down hostapd interface");
    return result;
  }
  return nsIWifiResult::SUCCESS;
}

void WifiNative::SupplicantDeathHandler::OnDeath() {
  // supplicant died, start to clean up.
  WIFI_LOGE(LOG_TAG, "Supplicant DIED: ##############################");
}

Result_t WifiNative::GetSoftapStations(uint32_t& aNumStations) {
  return s_WificondControl->GetSoftapStations(aNumStations);
}
