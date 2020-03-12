/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "WificondControl"
#include "WificondControl.h"
#include "nsIWifiElement.h"
#include "nsIMutableArray.h"

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>
#include <cutils/properties.h>

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::interface_cast;
using ::android::String16;
using ::android::net::wifi::IApInterfaceEventCallback;
using ::android::net::wifi::IScanEvent;

static const char* CTL_START_PROPERTY = "ctl.start";
static const char* CTL_STOP_PROPERTY = "ctl.stop";
static const char* SUPPLICANT_SERVICE_NAME = "wpa_supplicant";
static const char* WIFICOND_SERVICE_NAME = "wificond";
static const int32_t WIFICOND_POLL_DELAY = 500000;
static const int32_t WIFICOND_RETRY_COUNT = 20;

WificondControl* WificondControl::s_Instance = nullptr;

WificondControl::WificondControl()
    : mWificond(nullptr),
      mClientInterface(nullptr),
      mApInterface(nullptr),
      mScanner(nullptr) {}

WificondControl* WificondControl::Get() {
  MOZ_ASSERT(NS_IsMainThread());

  if (s_Instance) {
    return s_Instance;
  }

  // Create new instance
  s_Instance = new WificondControl();

  ClearOnShutdown(&s_Instance);
  return s_Instance;
}

/**
 * android::IBinder::DeathRecipient
 */
void WificondControl::WificondDeathRecipient::binderDied(
    const android::wp<IBinder>& who __unused) {
  WIFI_LOGE(LOG_TAG, "wificond died...");
}

bool WificondControl::InitWificondInterface() {
  if (mWificond != nullptr) {
    return true;
  }

  android::sp<::android::IServiceManager> sm = defaultServiceManager();
  android::sp<IBinder> binder;
  int32_t retry = 0;
  do {
    binder = sm->getService(String16(WIFICOND_SERVICE_NAME));
    if (binder != nullptr) {
      break;
    }
    usleep(WIFICOND_POLL_DELAY);
  } while (retry++ < WIFICOND_RETRY_COUNT);

  if (binder != nullptr) {
    mWificond = interface_cast<IWificond>(binder);

    mWificondDeathRecipient = new WificondDeathRecipient();
    binder->linkToDeath(mWificondDeathRecipient);
  } else {
    WIFI_LOGE(LOG_TAG, "Failed to create wificond instance");
    return false;
  }

  return true;
}

bool WificondControl::StartWificond() {
  // start wificond
  char value[PROPERTY_VALUE_MAX];
  property_get("init.svc.wificond", value, "");
  if (!strncmp(value, "running", 7)) {
    WIFI_LOGD(LOG_TAG, "wificond is already running...");
    return true;
  }

  if (property_set(CTL_START_PROPERTY, WIFICOND_SERVICE_NAME) != 0) {
    WIFI_LOGE(LOG_TAG, "start wificond failed");
    return false;
  }
  return true;
}

bool WificondControl::StopWificond() { return true; }

bool WificondControl::TearDownClientInterface(const std::string& aIfaceName) {
  if (!InitWificondInterface()) {
    return false;
  }

  if (mScanner != nullptr) {
    mScanner->unsubscribeScanEvents();
    mScanner->unsubscribePnoScanEvents();
  }

  bool success = false;
  if (!mWificond->tearDownClientInterface(aIfaceName, &success).isOk()) {
    WIFI_LOGE(LOG_TAG,
              "Failed to teardown client interface due to remote exception");
    return false;
  }
  if (!success) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown client interface");
    return false;
  }

  mWificond = nullptr;
  mClientInterface = nullptr;
  mScanner = nullptr;
  return true;
}

bool WificondControl::TearDownSoftapInterface(const std::string& aIfaceName) {
  if (mWificond == nullptr || mApInterface == nullptr) {
    WIFI_LOGE(LOG_TAG, "No valid interface handler");
    return false;
  }
  bool success = false;
  if (!mWificond->tearDownApInterface(aIfaceName, &success).isOk()) {
    WIFI_LOGE(LOG_TAG,
              "Failed to teardown ap interface due to remote exception");
    return false;
  }
  if (!success) {
    WIFI_LOGE(LOG_TAG, "Failed to teardown ap interface");
    return false;
  }
  mApInterface = nullptr;
  return true;
}

/**
 * StartSupplicant() - To start supplicant daemon.
 *
 * android 8.1: enable supplicant by IClientInterface -> libwifi-system.so -> ctl.start
 * android 9.0: enable supplicant by IWificon -> libwifi-system.so -> ctl.start
 * android 10:  enable supplicant by ctl.start directly
 */
bool WificondControl::StartSupplicant() {
  bool supplicantStarted = false;
  supplicantStarted =
      (property_set(CTL_START_PROPERTY, SUPPLICANT_SERVICE_NAME) == 0);
  return supplicantStarted;
}

bool WificondControl::StopSupplicant() {
  bool supplicantStopped = false;
  supplicantStopped =
      (property_set(CTL_STOP_PROPERTY, SUPPLICANT_SERVICE_NAME) == 0);
  return supplicantStopped;
}

bool WificondControl::SetupClientIface(
    const std::string& aIfaceName,
    const android::sp<IScanEvent>& aScanCallback) {
  // retrieve wificond handle
  if (!InitWificondInterface()) {
    return false;
  }
  if (!mWificond->createClientInterface(aIfaceName, &mClientInterface).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to create client interface");
    return false;
  }
  if (!mClientInterface->getWifiScannerImpl(&mScanner).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to get WificondScannerImpl");
    return false;
  }
  if (mScanner->subscribeScanEvents(aScanCallback).isOk()) {
    WIFI_LOGD(LOG_TAG, "subscribe scan event success");
  } else {
    WIFI_LOGE(LOG_TAG, "subscribe scan event failed");
  }
  return true;
}

bool WificondControl::SetupApIface(
    const std::string& aIfaceName,
    const android::sp<IApInterfaceEventCallback>& aApCallback) {
  if (!InitWificondInterface()) {
    return false;
  }
  if (!mWificond->createApInterface(aIfaceName, &mApInterface).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to create ap interface");
    return false;
  }
  bool success = false;
  mApInterface->registerCallback(aApCallback, &success);
  if (!success) {
    WIFI_LOGE(LOG_TAG, "Failed to register softap callback");
    return false;
  }
  return true;
}

bool WificondControl::StartSingleScan(ScanSettingsOptions* aScanSettings) {
  if (mScanner == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi scanner interface");
    return false;
  }
  SingleScanSettings settings;
  settings.scan_type_ = aScanSettings->mScanType;

  std::vector<ChannelSettings> channels;
  for (auto& freq : aScanSettings->mChannels) {
    ChannelSettings channel;
    channel.frequency_ = freq;
    channels.push_back(channel);
  }

  std::vector<HiddenNetwork> hiddenNetworks;
  if (!aScanSettings->mHiddenNetworks.IsEmpty()) {
    for (auto& net : aScanSettings->mHiddenNetworks) {
      HiddenNetwork hidden;
      std::string ssid_str = NS_ConvertUTF16toUTF8(net).get();
      std::vector<uint8_t> ssid(ssid_str.begin(), ssid_str.end());
      hidden.ssid_ = ssid;
      hiddenNetworks.push_back(hidden);
    }
  }
  settings.channel_settings_ = channels;
  settings.hidden_networks_ = hiddenNetworks;

  bool success = false;
  mScanner->scan(settings, &success);
  if (!success) {
    WIFI_LOGE(LOG_TAG, "Failed to start single scan");
    return false;
  }
  return true;
}

bool WificondControl::StopSingleScan() {
  if (mScanner == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi scanner interface");
    return false;
  }
  return mScanner->abortScan().isOk();
}

bool WificondControl::StartPnoScan() { return true; }

bool WificondControl::StopPnoScan() { return true; }

bool WificondControl::GetScanResults(
    std::vector<NativeScanResult>& aScanResults) {
  if (mScanner == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi scanner interface");
    return false;
  }

  if (!mScanner->getScanResults(&aScanResults).isOk()) {
    WIFI_LOGE(LOG_TAG, "Get scan results failed");
    return false;
  }
  return true;
}

bool WificondControl::GetChannelsForBand(uint32_t aBandMask,
                                         std::vector<int32_t>& aChannels) {
  if (mWificond == nullptr) {
    return false;
  }
  std::unique_ptr<std::vector<int32_t>> channels;
  if (aBandMask & nsIScanSettings::BAND_2_4_GHZ) {
    mWificond->getAvailable2gChannels(&channels);
    aChannels.insert(aChannels.end(), (*channels).begin(), (*channels).end());
  }
  if (aBandMask & nsIScanSettings::BAND_5_GHZ) {
    mWificond->getAvailable5gNonDFSChannels(&channels);
    aChannels.insert(aChannels.end(), (*channels).begin(), (*channels).end());
  }
  if (aBandMask & nsIScanSettings::BAND_5_GHZ_DFS) {
    mWificond->getAvailable5gNonDFSChannels(&channels);
    aChannels.insert(aChannels.end(), (*channels).begin(), (*channels).end());
  }
  return true;
}

bool WificondControl::SignalPoll() {
  if (mClientInterface == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi client interface");
    return false;
  }

  std::vector<int32_t> signal;
  if (!mClientInterface->signalPoll(&signal).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to get signal strength");
    return false;
  }
  return true;
}
