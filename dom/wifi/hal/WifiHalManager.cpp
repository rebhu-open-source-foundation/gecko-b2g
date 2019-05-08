/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "WifiHal"

#include <utils/Log.h>
#include "WifiHalManager.h"
#include <mozilla/ClearOnShutdown.h>

WifiHal* WifiHal::sInstance = nullptr;
mozilla::Mutex WifiHal::sLock("wifi-hidl");

WifiHal::WifiHal()
    : mWifi(nullptr),
      mStaIface(nullptr),
      mP2pIface(nullptr),
      mApIface(nullptr),
      mDeathRecipient(nullptr) {
  InitWifiInterface();
}

WifiHal* WifiHal::Get() {
  if (sInstance == nullptr) {
    sInstance = new WifiHal();
  }
  mozilla::ClearOnShutdown(&sInstance);
  return sInstance;
}

void WifiHal::CleanUp() {
  if (sInstance != nullptr) {
    delete sInstance;
    sInstance = nullptr;
  }
}

void WifiHal::WifiServiceDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGD(LOG_TAG, "IWifi HAL died, cleanup instance.");
  MutexAutoLock lock(sLock);
  if (mOuter != nullptr) {
    mOuter->mWifi = nullptr;
    mOuter->InitWifiInterface();
  }
}

bool WifiHal::StartWifi() {
  // initialize wifi hal interface at first.
  InitWifiInterface();

  if (mWifi == nullptr) {
    WIFI_LOGE(LOG_TAG, "startWifi: mWifi is null");
    return false;
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
      return true;
    } else if (status.code == WifiStatusCode::ERROR_NOT_AVAILABLE) {
      WIFI_LOGD(LOG_TAG, "Cannot start IWifi: Retrying...");
      usleep(300);
      triedCount++;
    } else {
      WIFI_LOGE(LOG_TAG, "Cannot start IWifi");
      return false;
    }
  }
  WIFI_LOGE(LOG_TAG, "Cannot start IWifi after trying %d times", triedCount);
  return false;
}

bool WifiHal::StopWifi() {
  if (mWifi == nullptr) {
    WIFI_LOGE(LOG_TAG, "stopWifi: mWifi is null");
    return false;
  }

  WifiStatus status;
  mWifi->stop([&status](WifiStatus s) {
    status = s;
    WIFI_LOGD(LOG_TAG, "stop wifi: %d", status.code);
  });

  if (status.code != WifiStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Cannot stop IWifi: %d", status.code);
    return false;
  }

  mStaIface = nullptr;
  mP2pIface = nullptr;
  mApIface = nullptr;
  return true;
}

bool WifiHal::TearDownInterface() {
  if (!StopWifi()) {
    return false;
  }
  mWifi = nullptr;
  return true;
}

bool WifiHal::InitWifiInterface() {
  MutexAutoLock lock(sLock);
  if (mWifi != nullptr) {
    return true;
  }

  mWifi = IWifi::getService();
  if (mWifi != nullptr) {
    if (mDeathRecipient == nullptr) {
      mDeathRecipient = new WifiServiceDeathRecipient(sInstance);
    }

    if (mDeathRecipient != nullptr) {
      Return<bool> linked = mWifi->linkToDeath(mDeathRecipient, 0 /*cookie*/);
      if (!linked || !linked.isOk()) {
        WIFI_LOGE(LOG_TAG, "Failed to link to wifi hal death notifications");
        return false;
      }
    }

    WifiStatus status;
    mWifi->registerEventCallback(this, [&status](WifiStatus s) { status = s; });

    if (status.code != WifiStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "registerEventCallback failed: %d, reason: %s",
                status.code, status.description.c_str());
      mWifi = nullptr;
      return false;
    }

    // wifi hal just initialized, stop wifi in case driver is loaded.
    StopWifi();
  } else {
    WIFI_LOGE(LOG_TAG, "get wifi hal failed");
    return false;
  }
  return true;
}

bool WifiHal::ConfigChipAndCreateIface(const wifiNameSpace::IfaceType& aType,
                                       std::string& aIfaceName /* out */) {
  if (mWifi == nullptr) {
    return false;
  }

  hidl_vec<ChipId> chipIds;
  mWifi->getChipIds([&](const WifiStatus& status, const hidl_vec<ChipId>& Ids) {
    WIFI_LOGD(LOG_TAG, "getChipIds status code: %d", status.code);
    chipIds = Ids;
  });

  android::sp<IWifiChip> wifiChip;
  mWifi->getChip(chipIds[0],
                 [&wifiChip](const WifiStatus& status,
                             const android::sp<IWifiChip>& chip) mutable {
                   WIFI_LOGD(LOG_TAG, "getChip status code: %d", status.code);
                   wifiChip = chip;
                 });

  WifiStatus response;
  ConfigChipByType(wifiChip, aType);
  if (aType == wifiNameSpace::IfaceType::STA) {
    wifiChip->createStaIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiStaIface>& iface) mutable {
          response = status;
          mStaIface = iface;
          aIfaceName = QueryInterfaceName(mStaIface);
        });
  } else if (aType == wifiNameSpace::IfaceType::P2P) {
    wifiChip->createP2pIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiP2pIface>& iface) mutable {
          response = status;
          mP2pIface = iface;
          aIfaceName = QueryInterfaceName(mP2pIface);
        });
  } else if (aType == wifiNameSpace::IfaceType::AP) {
    wifiChip->createApIface(
        [&response, &aIfaceName, this](
            const WifiStatus& status,
            const android::sp<IWifiApIface>& iface) mutable {
          response = status;
          mApIface = iface;
          aIfaceName = QueryInterfaceName(mApIface);
        });
  } else {
    WIFI_LOGE(LOG_TAG, "invalid interface type %d", aType);
    return false;
  }

  if (response.code != WifiStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to create interface, type:%d, status code:%d",
              aType, response.code);
    return false;
  }

  mIfaceNameMap[aType] = aIfaceName;
  WIFI_LOGD(LOG_TAG, "chip configure completed");
  return true;
}

bool WifiHal::GetCapabilities(uint32_t& aCapabilities) {
  if (!mStaIface.get()) {
    return false;
  }
  WifiStatus response;
  mStaIface->getCapabilities(
      [&](const WifiStatus& status,
          hidl_bitfield<IWifiStaIface::StaIfaceCapabilityMask> capabilities) {
        response = status;
        if (response.code == WifiStatusCode::SUCCESS) {
          aCapabilities = capabilities;
        }
      });
  return (response.code == WifiStatusCode::SUCCESS);
}

std::string WifiHal::GetInterfaceName(const wifiNameSpace::IfaceType& aType) {
  return mIfaceNameMap.at(aType);
}

bool WifiHal::ConfigChipByType(const android::sp<IWifiChip>& aChip,
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
    return false;
  }
  return true;
}

std::string WifiHal::QueryInterfaceName(const android::sp<IWifiIface>& aIface) {
  std::string ifaceName;
  aIface->getName([&ifaceName](const WifiStatus& status,
                               const hidl_string& name) { ifaceName = name; });
  return ifaceName;
}

/**
 * IWifiEventCallback implementation
 */
Return<void> WifiHal::onStart() {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onStart()");
  return Void();
}

Return<void> WifiHal::onStop() {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onStop()");
  return Void();
}

Return<void> WifiHal::onFailure(const WifiStatus& status) {
  WIFI_LOGD(LOG_TAG, "WifiEventCallback.onFailure(): %d", status.code);
  return Void();
}

/**
 * IWifiChipEventCallback implementation
 */
Return<void> WifiHal::onChipReconfigured(uint32_t modeId) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onChipReconfigured()");
  return Void();
}

Return<void> WifiHal::onChipReconfigureFailure(const WifiStatus& status) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onChipReconfigureFailure()");
  return Void();
}

Return<void> WifiHal::onIfaceAdded(wifiNameSpace::IfaceType type,
                                   const hidl_string& name) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onIfaceAdded()");
  return Void();
}

Return<void> WifiHal::onIfaceRemoved(wifiNameSpace::IfaceType type,
                                     const hidl_string& name) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onIfaceRemoved()");
  return Void();
}

Return<void> WifiHal::onDebugRingBufferDataAvailable(
    const WifiDebugRingBufferStatus& status, const hidl_vec<uint8_t>& data) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onDebugRingBufferDataAvailable()");
  return Void();
}

Return<void> WifiHal::onDebugErrorAlert(int32_t errorCode,
                                        const hidl_vec<uint8_t>& debugData) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onDebugErrorAlert()");
  return Void();
}

Return<void> WifiHal::onRadioModeChange(
    const hidl_vec<IWifiChipEventCallback::RadioModeInfo>& radioModeInfos) {
  WIFI_LOGD(LOG_TAG, "WifiChipEventCallback.onRadioModeChange()");
  return Void();
}

/**
 * IWifiStaIfaceEventCallback implementation
 */
Return<void> WifiHal::onBackgroundScanFailure(uint32_t cmdId) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundScanFailure()");
  return Void();
}

Return<void> WifiHal::onBackgroundFullScanResult(uint32_t cmdId,
                                                 uint32_t bucketsScanned,
                                                 const StaScanResult& result) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundFullScanResult()");
  return Void();
}

Return<void> WifiHal::onBackgroundScanResults(
    uint32_t cmdId,
    const ::android::hardware::hidl_vec<StaScanData>& scanDatas) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onBackgroundScanResults()");
  return Void();
}

Return<void> WifiHal::onRssiThresholdBreached(
    uint32_t cmdId,
    const ::android::hardware::hidl_array<uint8_t, 6>& currBssid,
    int32_t currRssi) {
  WIFI_LOGD(LOG_TAG, "WifiStaIfaceEventCallback.onRssiThresholdBreached()");
  return Void();
}
