/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "SoftapManager"

#include "SoftapManager.h"
#include <string.h>
#include <utils/Log.h>
#include <mozilla/ClearOnShutdown.h>

mozilla::Mutex SoftapManager::s_Lock("softap");
SoftapManager* SoftapManager::s_Instance = nullptr;

void SoftapManager::ServiceManagerDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "IServiceManager HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);

  if (mOuter != nullptr) {
    mOuter->mServiceManager = nullptr;
  }
}

void SoftapManager::HostapdDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "IHostapd HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);

  if (mOuter != nullptr) {
    mOuter->mHostapd = nullptr;
  }
}

SoftapManager::SoftapManager()
    : mServiceManager(nullptr),
      mHostapd(nullptr),
      mServiceManagerDeathRecipient(nullptr),
      mHostapdDeathRecipient(nullptr) {}

SoftapManager::~SoftapManager() {}

SoftapManager* SoftapManager::Get() {
  if (s_Instance == nullptr) {
    s_Instance = new SoftapManager();
    mozilla::ClearOnShutdown(&s_Instance);
  }
  return s_Instance;
}

void SoftapManager::CleanUp() {
  if (s_Instance != nullptr) {
    SoftapManager::Get()->DeinitInterface();

    delete s_Instance;
    s_Instance = nullptr;
  }
}

bool SoftapManager::InitInterface() {
  if (mHostapd != nullptr) {
    return true;
  }
  if (!InitServiceManager()) {
    return false;
  }
  // start hostapd daemon through lazy HAL.
  IHostapd_1_1::getService();
  return true;
}

bool SoftapManager::DeinitInterface() { return TearDownInterface(); }

bool SoftapManager::IsInterfaceInitializing() {
  MutexAutoLock lock(s_Lock);
  return mServiceManager != nullptr;
}

bool SoftapManager::IsInterfaceReady() {
  MutexAutoLock lock(s_Lock);
  return mHostapd != nullptr;
}

bool SoftapManager::InitServiceManager() {
  MutexAutoLock lock(s_Lock);
  if (mServiceManager != nullptr) {
    // service already existed.
    return true;
  }
  mHostapd = nullptr;
  mServiceManager =
      ::android::hidl::manager::V1_0::IServiceManager::getService();
  if (mServiceManager == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to get HIDL service manager");
    return false;
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
    return false;
  }

  // interface name android.hardware.wifi.hostapd@1.1::IHostapd
  if (!mServiceManager->registerForNotifications(
      IHostapd_1_1::descriptor, "", this)) {
    WIFI_LOGE(LOG_TAG, "Failed to register for notifications to IHostapd");
    mServiceManager = nullptr;
    return false;
  }
  return true;
}

bool SoftapManager::InitHostapdInterface() {
  MutexAutoLock lock(s_Lock);
  if (mHostapd != nullptr) {
    return true;
  }

  mHostapd = IHostapd_1_1::getService();
  if (mHostapd == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to get hostapd interface");
    return false;
  }

  if (mHostapdDeathRecipient == nullptr) {
    mHostapdDeathRecipient = new HostapdDeathRecipient(s_Instance);
  }
  if (mHostapdDeathRecipient != nullptr) {
    Return<bool> linked =
        mHostapd->linkToDeath(mHostapdDeathRecipient, 0 /*cookie*/);
    if (!linked || !linked.isOk()) {
      WIFI_LOGE(LOG_TAG, "Failed to link to hostapd hal death notifications");
      mHostapd = nullptr;
      return false;
    }
  }

  HostapdStatus response;
  mHostapd->registerCallback(
      this, [&](const HostapdStatus& status) { response = status; });
  if (response.code != HostapdStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to register hostapd callback: %d, reason: %s",
              response.code, response.debugMessage.c_str());
    mHostapd = nullptr;
    return false;
  }
  return true;
}

bool SoftapManager::TearDownInterface() {
  MutexAutoLock lock(s_Lock);
  if (mHostapd.get()) {
    mHostapd->terminate();
  }
  mHostapd = nullptr;
  mServiceManager = nullptr;
  return true;
}

bool SoftapManager::StartSoftap(const std::string& aInterfaceName,
                                SoftapConfigurationOptions* aSoftapConfig) {
  if (aSoftapConfig == nullptr || aSoftapConfig->mSsid.IsEmpty()) {
    WIFI_LOGE(LOG_TAG, "Invalid configuration");
    return false;
  }
  if (aSoftapConfig->mBand == nsISoftapConfiguration::AP_BAND_5GHZ) {
    if (mCountryCode.empty()) {
      WIFI_LOGE(LOG_TAG, "Must have country code for 5G band");
      return false;
    }
  }
  if (mHostapd == nullptr) {
    return InitHostapdInterface();
  }
  IHostapd::IfaceParams ifaceParams;
  IHostapd::NetworkParams networkParams;

  ifaceParams.ifaceName = aInterfaceName;
  ifaceParams.hwModeParams.enable80211N = aSoftapConfig->mEnable11N;
  ifaceParams.hwModeParams.enable80211AC = aSoftapConfig->mEnable11AC;
  ifaceParams.channelParams.enableAcs = aSoftapConfig->mEnableACS;
  ifaceParams.channelParams.acsShouldExcludeDfs = aSoftapConfig->mAcsExcludeDfs;
  ifaceParams.channelParams.channel = aSoftapConfig->mChannel;
  ifaceParams.channelParams.band =
      (android::hardware::wifi::hostapd::V1_0::IHostapd::Band)aSoftapConfig->mBand;
  // Use 1.1 version for addAccessPoint().
  IHostapd_1_1::IfaceParams ifaceParams_1_1;
  ifaceParams_1_1.V1_0 = ifaceParams;

  std::string ssid(NS_ConvertUTF16toUTF8(aSoftapConfig->mSsid).get());
  networkParams.ssid = std::vector<uint8_t>(ssid.begin(), ssid.end());
  networkParams.isHidden = aSoftapConfig->mHidden;

  if (aSoftapConfig->mKeyManagement == nsISoftapConfiguration::SECURITY_NONE) {
    networkParams.encryptionType = IHostapd::EncryptionType::NONE;
  } else if (aSoftapConfig->mKeyManagement ==
             nsISoftapConfiguration::SECURITY_WPA) {
    networkParams.encryptionType = IHostapd::EncryptionType::WPA;
    std::string psk(NS_ConvertUTF16toUTF8(aSoftapConfig->mKey).get());
    networkParams.pskPassphrase = psk;
  } else if (aSoftapConfig->mKeyManagement ==
             nsISoftapConfiguration::SECURITY_WPA2) {
    networkParams.encryptionType = IHostapd::EncryptionType::WPA2;
    std::string psk(NS_ConvertUTF16toUTF8(aSoftapConfig->mKey).get());
    networkParams.pskPassphrase = psk;
  } else {
    WIFI_LOGD(LOG_TAG, "Invalid softap security type");
    networkParams.encryptionType = IHostapd::EncryptionType::NONE;
  }
  HostapdStatus response;
  mHostapd->addAccessPoint_1_1(
      ifaceParams_1_1, networkParams,
      [&](const HostapdStatus& status) { response = status; });
  if (response.code != HostapdStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to start softap: %s", ssid.c_str());
    return false;
  }
  return true;
}

bool SoftapManager::StopSoftap(const std::string& aInterfaceName) {
  HostapdStatus response;
  HIDL_SET(mHostapd, removeAccessPoint, HostapdStatus, response,
           aInterfaceName);
  return (response.code == HostapdStatusCode::SUCCESS);
}

void SoftapManager::SetSoftapCountryCode(const std::string& aCountryCode) {
  mCountryCode = aCountryCode;
}

/**
 * IServiceNotification
 */
Return<void> SoftapManager::onRegistration(const hidl_string& fqName,
                                           const hidl_string& name,
                                           bool preexisting) {
  // start to initialize hostapd hidl interface.
  if (!InitHostapdInterface()) {
    WIFI_LOGE(LOG_TAG, "Failed to initialize hostapd interface");
    mServiceManager = nullptr;
    mHostapd = nullptr;
  }
  return Return<void>();
}

/**
 * IHostapdCallback implementation
 */
Return<void> SoftapManager::onFailure(const hidl_string& ifaceName) {
  WIFI_LOGD(LOG_TAG, "IHostapdCallback::onFailure()");
  return android::hardware::Void();
}
