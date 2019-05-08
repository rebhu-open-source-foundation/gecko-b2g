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

static const int32_t CONNECT_TO_SUPPLICANT_RETRY_INTERVAL_US = 100000;
static const int32_t CONNECT_TO_SUPPLICANT_RETRY_TIMES = 50;

/* static */
WifiHal* WifiNative::sWifiHal = nullptr;
WificondControl* WifiNative::sWificondControl = nullptr;
SupplicantStaManager* WifiNative::sSupplicantStaManager = nullptr;

WifiNative::WifiNative(EventCallback aCallback) {
  sWifiHal = WifiHal::Get();
  sWificondControl = WificondControl::Get();
  sSupplicantStaManager = SupplicantStaManager::Get();

  mEventCallback = aCallback;
  sSupplicantStaManager->RegisterEventCallback(mEventCallback);
}

bool WifiNative::ExecuteCommand(CommandOptions& aOptions, nsWifiResult* aResult,
                                const nsCString& aInterface) {
  WIFI_LOGD(LOG_TAG, "WifiNative::ExecuteCommand +++");

  // Always correlate the opaque ids.
  aResult->mId = aOptions.mId;

  if (aOptions.mCmd == nsIWifiCommand::INITIALIZE) {
    aResult->mStatus = InitHal();
  } else if (aOptions.mCmd == nsIWifiCommand::START_WIFI) {
    aResult->mStatus = StartWifi();
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_WIFI) {
    aResult->mStatus = StopWifi();
  } else if (aOptions.mCmd == nsIWifiCommand::GET_STA_CAPABILITIES) {
    aResult->mStatus = GetStaCapabilities(aResult->mCapabilities);
  } else if (aOptions.mCmd == nsIWifiCommand::GET_DEBUG_LEVEL) {
    aResult->mStatus = GetDebugLevel(aResult->mDebugLevel);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_DEBUG_LEVEL) {
    aResult->mStatus = SetDebugLevel(&aOptions.mDebugLevel);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_POWER_SAVE) {
    aResult->mStatus = SetPowerSave(aOptions.mPowerSave);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_SUSPEND_MODE) {
    aResult->mStatus = SetSuspendMode(aOptions.mSuspendMode);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_EXTERNAL_SIM) {
    aResult->mStatus = SetExternalSim(aOptions.mExternalSim);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_AUTO_RECONNECT) {
    aResult->mStatus = SetAutoReconnect(aOptions.mAutoReconnect);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_COUNTRY_CODE) {
    aResult->mStatus = SetCountryCode(aOptions.mCountryCode);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_BT_COEXIST_MODE) {
    aResult->mStatus = SetBtCoexistenceMode(aOptions.mBtCoexistenceMode);
  } else if (aOptions.mCmd == nsIWifiCommand::SET_BT_COEXIST_SCAN_MODE) {
    aResult->mStatus =
        SetBtCoexistenceScanMode(aOptions.mBtCoexistenceScanMode);
  } else if (aOptions.mCmd == nsIWifiCommand::START_SINGLE_SCAN) {
    aResult->mStatus = StartSingleScan();
  } else if (aOptions.mCmd == nsIWifiCommand::STOP_SINGLE_SCAN) {
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
      std::string info_element_str(result.info_element.begin(),
                                   result.info_element.end());
      nsString ssid(NS_ConvertUTF8toUTF16(ssid_str.c_str()));
      nsString bssid(NS_ConvertUTF8toUTF16(bssid_str.c_str()));
      nsString info_element(NS_ConvertUTF8toUTF16(info_element_str.c_str()));
      uint32_t frequency = result.frequency;
      uint32_t tsf = result.tsf;
      uint32_t capability = result.capability;
      int32_t signal = result.signal_mbm;
      bool associated = result.associated;

      RefPtr<nsScanResult> scanResult =
          new nsScanResult(ssid, bssid, info_element, frequency, tsf,
                           capability, signal, associated);
      scanResults.AppendElement(scanResult);
    }
    aResult->updateScanResults(scanResults);
  } else if (aOptions.mCmd == nsIWifiCommand::CONNECT) {
    aResult->mStatus = Connect(&aOptions.mConfig);
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
  if (!sWifiHal->InitWifiInterface()) {
    return false;
  }

  if (!sWificondControl->InitWificondInterface()) {
    return false;
  }

  // init supplicant hal
  if (!sSupplicantStaManager->IsSupplicantReady() &&
      !sSupplicantStaManager->InitSupplicantInterface()) {
    return false;
  }

  return true;
}

bool WifiNative::DeinitHal() { return true; }

/**
 * StartWifi() - to enable wifi
 *
 * 1. load wifi driver module, configure chip.
 * 2. setup client mode interface.
 * 3. start supplicant.
 *
 */
bool WifiNative::StartWifi() {
  if (!sWifiHal->StartWifi()) {
    WIFI_LOGE(LOG_TAG, "Failed to start wifi");
    return false;
  }

  WIFI_LOGD(LOG_TAG, "module loaded, try to configure...");
  sWifiHal->ConfigChipAndCreateIface(wifiNameSpace::IfaceType::STA,
                                     mStaInterfaceName);

  WificondEventService* eventService =
      WificondEventService::CreateService(mStaInterfaceName);
  eventService->RegisterEventCallback(mEventCallback);

  if (!StartSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant");
    return false;
  }
  // supplicant initialized, register death handler
  SupplicantDeathHandler* deathHandler = new SupplicantDeathHandler();
  sSupplicantStaManager->RegisterDeathHandler(deathHandler);

  if (!sWificondControl->SetupClientIface(
          mStaInterfaceName,
          android::interface_cast<android::net::wifi::IScanEvent>(eventService))) {
    WIFI_LOGE(LOG_TAG, "Failed to setup iface in wificond");
    return false;
  }

  if (!sSupplicantStaManager->SetupStaInterface(mStaInterfaceName)) {
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
  if (!sWificondControl->TearDownClientInterface(mStaInterfaceName)) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown wificond interfaces");
    return false;
  }

  // unregister supplicant death Handler
  sSupplicantStaManager->UnregisterDeathHandler();

  if (!sWifiHal->TearDownInterface()) {
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
  if (!sSupplicantStaManager->InitSupplicantInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize supplicant hal");
    return false;
  }

  // start supplicant from wificond.
  if (!sWificondControl->StartSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to start supplicant daemon");
    return false;
  }

  bool connected = false;
  int32_t connectTries = 0;
  while (!connected && connectTries++ < CONNECT_TO_SUPPLICANT_RETRY_TIMES) {
    // Check if the initialization is complete.
    if (sSupplicantStaManager->IsSupplicantReady()) {
      connected = true;
      break;
    }
    usleep(CONNECT_TO_SUPPLICANT_RETRY_INTERVAL_US);
  }
  return connected;
}

bool WifiNative::StopSupplicant() {
  // teardown supplicant hal interfaces
  if (!sSupplicantStaManager->DeinitSupplicantInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown iface in supplicant");
    return false;
  }

  // TODO: stop supplicant daemon for android 8.1
  if (!sWificondControl->StopSupplicant()) {
    WIFI_LOGE(LOG_TAG, "Failed to stop supplicant");
    return false;
  }

  return true;
}

bool WifiNative::GetStaCapabilities(uint32_t& aCapabilities) {
  return sWifiHal->GetCapabilities(aCapabilities);
}

bool WifiNative::GetDebugLevel(uint32_t& aLevel) {
  aLevel = (uint32_t)sSupplicantStaManager->GetSupplicantDebugLevel();
  return true;
}

bool WifiNative::SetDebugLevel(SupplicantDebugLevelOptions* aLevel) {
  return sSupplicantStaManager->SetSupplicantDebugLevel(aLevel);
}

bool WifiNative::SetPowerSave(bool aEnable) {
  return sSupplicantStaManager->SetPowerSave(aEnable);
}

bool WifiNative::SetSuspendMode(bool aEnable) {
  return sSupplicantStaManager->SetSuspendMode(aEnable);
}

bool WifiNative::SetExternalSim(bool aEnable) {
  return sSupplicantStaManager->SetExternalSim(aEnable);
}

bool WifiNative::SetAutoReconnect(bool aEnable) {
  return sSupplicantStaManager->SetAutoReconnect(aEnable);
}

bool WifiNative::SetBtCoexistenceMode(uint8_t aMode) {
  return sSupplicantStaManager->SetBtCoexistenceMode(aMode);
}

bool WifiNative::SetBtCoexistenceScanMode(bool aEnable) {
  return sSupplicantStaManager->SetBtCoexistenceScanMode(aEnable);
}

bool WifiNative::SetCountryCode(const nsAString& aCountryCode) {
  std::string countryCode = NS_ConvertUTF16toUTF8(aCountryCode).get();
  return sSupplicantStaManager->SetCountryCode(countryCode);
}

bool WifiNative::StartSingleScan() {
  return sWificondControl->StartSingleScan();
}

bool WifiNative::StopSingleScan() { return true; }

bool WifiNative::StartPnoScan() { return true; }

bool WifiNative::StopPnoScan() { return true; }

bool WifiNative::GetScanResults(std::vector<NativeScanResult>& aScanResults) {
  return sWificondControl->GetScanResults(aScanResults);
}

bool WifiNative::Connect(ConfigurationOptions* aConfig) {
  // abort ongoing scan before connect
  sWificondControl->StopSingleScan();

  if (!sSupplicantStaManager->ConnectToNetwork(aConfig)) {
    WIFI_LOGE(LOG_TAG, "Failed to connect %s",
              NS_ConvertUTF16toUTF8(aConfig->mSsid).get());
    return false;
  }
  return true;
}

void WifiNative::SupplicantDeathHandler::OnDeath() {
  // supplicant died, start to clean up.
  WIFI_LOGE(LOG_TAG, "Supplicant DIED: ##############################");
}
