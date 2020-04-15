/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "WifiHal"

#include <utils/Log.h>
#include "WifiHalManager.h"
#include <mozilla/ClearOnShutdown.h>

static const char WIFI_INTERFACE_NAME[] = "android.hardware.wifi@1.0::IWifi";

WifiHal* WifiHal::s_Instance = nullptr;
mozilla::Mutex WifiHal::s_Lock("wifi-hidl");

WifiHal::WifiHal()
    : mWifi(nullptr),
      mWifiChip(nullptr),
      mStaIface(nullptr),
      mP2pIface(nullptr),
      mApIface(nullptr),
      mDeathRecipient(nullptr),
      mServiceManager(nullptr),
      mServiceManagerDeathRecipient(nullptr) {
  InitServiceManager();
}

WifiHal* WifiHal::Get() {
  if (s_Instance == nullptr) {
    s_Instance = new WifiHal();
  }
  mozilla::ClearOnShutdown(&s_Instance);
  return s_Instance;
}

void WifiHal::CleanUp() {
  if (s_Instance != nullptr) {
    delete s_Instance;
    s_Instance = nullptr;
  }
}

void WifiHal::ServiceManagerDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "IServiceManager HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);
  if (mOuter != nullptr) {
    mOuter->mServiceManager = nullptr;
    mOuter->InitServiceManager();
  }
}

void WifiHal::WifiServiceDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGD(LOG_TAG, "IWifi HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);
  if (mOuter != nullptr) {
    mOuter->mWifi = nullptr;
    mOuter->InitWifiInterface();
  }
}

Result_t WifiHal::StartWifiModule() {
  // initialize wifi hal interface at first.
  InitWifiInterface();

  if (mWifi == nullptr) {
    WIFI_LOGE(LOG_TAG, "startWifi: mWifi is null");
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }

  int32_t triedCount = 0;
  while (triedCount <= START_HAL_RETRY_TIMES) {
    WifiStatus status;
    mWifi->start([&status](WifiStatus s) {
      status = s;
      WIFI_LOGD(LOG_TAG, "start wifi: %d", status.code);
    });

    if (status.code == WifiStatusCode::SUCCESS) {
      if (triedCount != 0) {
        WIFI_LOGD(LOG_TAG, "start IWifi succeeded after trying %d times",
                  triedCount);
      }
      return nsIWifiResult::SUCCESS;
    } else if (status.code == WifiStatusCode::ERROR_NOT_AVAILABLE) {
      WIFI_LOGD(LOG_TAG, "Cannot start IWifi: Retrying...");
      usleep(300);
      triedCount++;
    } else {
      WIFI_LOGE(LOG_TAG, "Cannot start IWifi");
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }
  WIFI_LOGE(LOG_TAG, "Cannot start IWifi after trying %d times", triedCount);
  return nsIWifiResult::ERROR_COMMAND_FAILED;
}

Result_t WifiHal::StopWifiModule() {
  if (mWifi == nullptr) {
    WIFI_LOGE(LOG_TAG, "stopWifi: mWifi is null");
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }

  WifiStatus status;
  mWifi->stop([&status](WifiStatus s) {
    status = s;
    WIFI_LOGD(LOG_TAG, "stop wifi: %d", status.code);
  });

  if (status.code != WifiStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Cannot stop IWifi: %d", status.code);
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiHal::TearDownInterface(const wifiNameSpace::IfaceType& aType) {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;

  result = RemoveInterfaceInternal(aType);
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }
  result = StopWifiModule();
  if (result != nsIWifiResult::SUCCESS) {
    return result;
  }
  mWifi = nullptr;
  mServiceManager = nullptr;
  return nsIWifiResult::SUCCESS;
}

Result_t WifiHal::InitHalInterface() {
  if (mWifi != nullptr) {
    return nsIWifiResult::SUCCESS;
  }
  return InitServiceManager();
}

Result_t WifiHal::InitServiceManager() {
  MutexAutoLock lock(s_Lock);
  if (mServiceManager != nullptr) {
    // service already existed.
    return nsIWifiResult::SUCCESS;
  }

  mServiceManager =
      ::android::hidl::manager::V1_0::IServiceManager::getService();
  if (mServiceManager == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to get HIDL service manager");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  if (mServiceManagerDeathRecipient == nullptr) {
    mServiceManagerDeathRecipient =
        new ServiceManagerDeathRecipient(s_Instance);
  }
  Return<bool> linked =
      mServiceManager->linkToDeath(mServiceManagerDeathRecipient, 0);
  if (!linked || !linked.isOk()) {
    WIFI_LOGE(LOG_TAG, "Error on linkToDeath to IServiceManager");
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  if (!mServiceManager->registerForNotifications(WIFI_INTERFACE_NAME, "",
                                                 this)) {
    WIFI_LOGE(LOG_TAG, "Failed to register for notifications to IWifi");
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiHal::InitWifiInterface() {
  MutexAutoLock lock(s_Lock);
  if (mWifi != nullptr) {
    return nsIWifiResult::SUCCESS;
  }

  mWifi = IWifi::getService();
  if (mWifi != nullptr) {
    if (mDeathRecipient == nullptr) {
      mDeathRecipient = new WifiServiceDeathRecipient(s_Instance);
    }

    if (mDeathRecipient != nullptr) {
      Return<bool> linked = mWifi->linkToDeath(mDeathRecipient, 0 /*cookie*/);
      if (!linked || !linked.isOk()) {
        WIFI_LOGE(LOG_TAG, "Failed to link to wifi hal death notifications");
        return nsIWifiResult::ERROR_COMMAND_FAILED;
      }
    }

    WifiStatus status;
    mWifi->registerEventCallback(this, [&status](WifiStatus s) { status = s; });

    if (status.code != WifiStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "registerEventCallback failed: %d, reason: %s",
                status.code, status.description.c_str());
      mWifi = nullptr;
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }

    // wifi hal just initialized, stop wifi in case driver is loaded.
    StopWifiModule();
  } else {
    WIFI_LOGE(LOG_TAG, "get wifi hal failed");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t WifiHal::GetCapabilities(uint32_t& aCapabilities) {
  if (!mWifiChip.get()) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }
  WifiStatus response;
  mWifiChip->getCapabilities(
      [&](const WifiStatus& status,
          hidl_bitfield<IWifiChip::ChipCapabilityMask> capabilities) {
        response = status;
        aCapabilities = capabilities;
      });
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

Result_t WifiHal::GetDriverModuleInfo(nsAString& aDriverVersion,
                                      nsAString& aFirmwareVersion) {
  if (!mWifiChip.get()) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }
  WifiStatus response;
  IWifiChip::ChipDebugInfo chipInfo;
  mWifiChip->requestChipDebugInfo(
      [&](const WifiStatus& status,
          const IWifiChip::ChipDebugInfo& chipDebugInfo) {
        response = status;
        chipInfo = chipDebugInfo;
      });

  if (response.code == WifiStatusCode::SUCCESS) {
    nsString driverInfo(
        NS_ConvertUTF8toUTF16(chipInfo.driverDescription.c_str()));
    nsString firmwareInfo(
        NS_ConvertUTF8toUTF16(chipInfo.firmwareDescription.c_str()));

    aDriverVersion.Assign(driverInfo);
    aFirmwareVersion.Assign(firmwareInfo);
  }
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

Result_t WifiHal::SetLowLatencyMode(bool aEnable) {
#if 0
  IWifiChip::LatencyMode mode;
  if (aEnable) {
      mode = IWifiChip::LatencyMode::LOW;
  } else {
      mode = IWifiChip::LatencyMode::NORMAL;
  }

  WifiStatus response;
  HIDL_SET(mWifiChip, setLatencyMode, WifiStatus,
            response, mode);
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
#else
  return nsIWifiResult::ERROR_NOT_SUPPORTED;
#endif
}

Result_t WifiHal::ConfigChipAndCreateIface(
    const wifiNameSpace::IfaceType& aType, std::string& aIfaceName /* out */) {
  if (mWifi == nullptr) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }

  WifiStatus response;
  hidl_vec<ChipId> chipIds;
  mWifi->getChipIds([&](const WifiStatus& status, const hidl_vec<ChipId>& Ids) {
    WIFI_LOGD(LOG_TAG, "getChipIds status code: %d", status.code);
    chipIds = Ids;
  });

  mWifi->getChip(chipIds[0],
                 [&, this](const WifiStatus& status,
                           const android::sp<IWifiChip>& chip) mutable {
                   response = status;
                   mWifiChip = chip;
                 });

  if (mWifiChip == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to get wifi chip with error %d", response.code);
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  ConfigChipByType(mWifiChip, aType);
  if (aType == wifiNameSpace::IfaceType::STA) {
    mWifiChip->createStaIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiStaIface>& iface) mutable {
          response = status;
          mStaIface = iface;
          aIfaceName = QueryInterfaceName(mStaIface);
        });
  } else if (aType == wifiNameSpace::IfaceType::P2P) {
    mWifiChip->createP2pIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiP2pIface>& iface) mutable {
          response = status;
          mP2pIface = iface;
          aIfaceName = QueryInterfaceName(mP2pIface);
        });
  } else if (aType == wifiNameSpace::IfaceType::AP) {
    mWifiChip->createApIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiApIface>& iface) mutable {
          response = status;
          mApIface = iface;
          aIfaceName = QueryInterfaceName(mApIface);
        });
  } else {
    WIFI_LOGE(LOG_TAG, "invalid interface type %d", aType);
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }

  if (response.code != WifiStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to create interface, type:%d, status code:%d",
              aType, response.code);
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  mIfaceNameMap[aType] = aIfaceName;
  WIFI_LOGD(LOG_TAG, "chip configure completed");
  return nsIWifiResult::SUCCESS;
}

Result_t WifiHal::GetStaCapabilities(uint32_t& aStaCapabilities) {
  if (!mStaIface.get()) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }
  WifiStatus response;
  mStaIface->getCapabilities(
      [&](const WifiStatus& status,
          hidl_bitfield<IWifiStaIface::StaIfaceCapabilityMask> capabilities) {
        response = status;
        aStaCapabilities = capabilities;
      });
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

Result_t WifiHal::EnableLinkLayerStats() {
  WifiStatus response;
  bool debugEnabled = false;
  HIDL_SET(mStaIface, enableLinkLayerStatsCollection, WifiStatus, response,
           debugEnabled);
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

Result_t WifiHal::GetLinkLayerStats(wifiNameSpace::StaLinkLayerStats& aStats) {
  if (!mStaIface.get()) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }
  WifiStatus response;
  mStaIface->getLinkLayerStats(
      [&](const WifiStatus& status,
          const wifiNameSpace::StaLinkLayerStats& stats) {
        response = status;
        aStats = stats;
      });
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

Result_t WifiHal::SetSoftapCountryCode(std::string aCountryCode) {
  if (aCountryCode.length() != 2) {
    WIFI_LOGE(LOG_TAG, "Invalid country code: %s", aCountryCode.c_str());
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }
  std::array<int8_t, 2> countryCode;
  countryCode[0] = aCountryCode.at(0);
  countryCode[1] = aCountryCode.at(1);

  WifiStatus response;
  HIDL_SET(mApIface, setCountryCode, WifiStatus, response, countryCode);
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

std::string WifiHal::GetInterfaceName(const wifiNameSpace::IfaceType& aType) {
  return mIfaceNameMap.at(aType);
}

Result_t WifiHal::ConfigChipByType(const android::sp<IWifiChip>& aChip,
                                   const wifiNameSpace::IfaceType& aType) {
  uint32_t modeId = UINT32_MAX;
  aChip->getAvailableModes(
      [&modeId, &aType](const WifiStatus& status,
                        const hidl_vec<IWifiChip::ChipMode>& modes) {
        WIFI_LOGD(LOG_TAG, "getAvailableModes status code: %d", status.code);
        for (const auto& mode : modes) {
          for (const auto& combination : mode.availableCombinations) {
            for (const auto& limit : combination.limits) {
              if (limit.types.end() !=
                  std::find(limit.types.begin(), limit.types.end(), aType)) {
                modeId = mode.id;
              }
            }
          }
        }
      });

  WifiStatus response;
  aChip->configureChip(
      modeId, [&response](const WifiStatus& status) { response = status; });

  if (response.code != WifiStatusCode::SUCCESS) {
    WIFI_LOGD(LOG_TAG, "configureChip for %d failed, status code: %d", aType,
              response.code);
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

std::string WifiHal::QueryInterfaceName(const android::sp<IWifiIface>& aIface) {
  std::string ifaceName;
  aIface->getName([&ifaceName](const WifiStatus& status,
                               const hidl_string& name) { ifaceName = name; });
  return ifaceName;
}

Result_t WifiHal::RemoveInterfaceInternal(
    const wifiNameSpace::IfaceType& aType) {
  WifiStatus response;
  std::string ifname = mIfaceNameMap.at(aType);
  switch (aType) {
    case wifiNameSpace::IfaceType::STA:
      HIDL_SET(mWifiChip, removeStaIface, WifiStatus, response, ifname);
      mStaIface = nullptr;
      break;
    case wifiNameSpace::IfaceType::P2P:
      HIDL_SET(mWifiChip, removeP2pIface, WifiStatus, response, ifname);
      mP2pIface = nullptr;
      break;
    case wifiNameSpace::IfaceType::AP:
      HIDL_SET(mWifiChip, removeApIface, WifiStatus, response, ifname);
      mApIface = nullptr;
      break;
    default:
      WIFI_LOGE(LOG_TAG, "Invalid interface type");
      return nsIWifiResult::ERROR_INVALID_ARGS;
  }
  return CHECK_SUCCESS(response.code == WifiStatusCode::SUCCESS);
}

/**
 * IServiceNotification
 */
Return<void> WifiHal::onRegistration(const hidl_string& fqName,
                                     const hidl_string& name,
                                     bool preexisting) {
  if (InitWifiInterface() != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "initialize IWifi failed.");
    mWifi = nullptr;
  }
  return Return<void>();
}

/**
 * IWifiEventCallback implementation
 */
Return<void> WifiHal::onStart() {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onStart()");
  return android::hardware::Void();
}

Return<void> WifiHal::onStop() {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onStop()");
  return android::hardware::Void();
}

Return<void> WifiHal::onFailure(const WifiStatus& status) {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onFailure(): %d", status.code);
  return android::hardware::Void();
}

/**
 * IWifiChipEventCallback implementation
 */
Return<void> WifiHal::onChipReconfigured(uint32_t modeId) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onChipReconfigured()");
  return android::hardware::Void();
}

Return<void> WifiHal::onChipReconfigureFailure(const WifiStatus& status) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onChipReconfigureFailure()");
  return android::hardware::Void();
}

Return<void> WifiHal::onIfaceAdded(wifiNameSpace::IfaceType type,
                                   const hidl_string& name) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onIfaceAdded()");
  return android::hardware::Void();
}

Return<void> WifiHal::onIfaceRemoved(wifiNameSpace::IfaceType type,
                                     const hidl_string& name) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onIfaceRemoved()");
  return android::hardware::Void();
}

Return<void> WifiHal::onDebugRingBufferDataAvailable(
    const WifiDebugRingBufferStatus& status, const hidl_vec<uint8_t>& data) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onDebugRingBufferDataAvailable()");
  return android::hardware::Void();
}

Return<void> WifiHal::onDebugErrorAlert(int32_t errorCode,
                                        const hidl_vec<uint8_t>& debugData) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onDebugErrorAlert()");
  return android::hardware::Void();
}

Return<void> WifiHal::onRadioModeChange(
    const hidl_vec<IWifiChipEventCallback::RadioModeInfo>& radioModeInfos) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onRadioModeChange()");
  return android::hardware::Void();
}

/**
 * IWifiStaIfaceEventCallback implementation
 */
Return<void> WifiHal::onBackgroundScanFailure(uint32_t cmdId) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundScanFailure()");
  return android::hardware::Void();
}

Return<void> WifiHal::onBackgroundFullScanResult(uint32_t cmdId,
                                                 uint32_t bucketsScanned,
                                                 const StaScanResult& result) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundFullScanResult()");
  return android::hardware::Void();
}

Return<void> WifiHal::onBackgroundScanResults(
    uint32_t cmdId,
    const ::android::hardware::hidl_vec<StaScanData>& scanDatas) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundScanResults()");
  return android::hardware::Void();
}

Return<void> WifiHal::onRssiThresholdBreached(
    uint32_t cmdId,
    const ::android::hardware::hidl_array<uint8_t, 6>& currBssid,
    int32_t currRssi) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onRssiThresholdBreached()");
  return android::hardware::Void();
}
