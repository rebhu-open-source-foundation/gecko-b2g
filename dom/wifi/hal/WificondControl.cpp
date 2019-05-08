/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "WificondControl"
#include "WificondControl.h"

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
using ::android::binder::Status;
using ::android::net::wifi::IScanEvent;

static const char* CTL_START_PROPERTY = "ctl.start";
static const char* CTL_STOP_PROPERTY = "ctl.stop";
static const char* SUPPLICANT_SERVICE_NAME = "wpa_supplicant";
static const char* WIFICOND_SERVICE_NAME = "wificond";
static const int32_t WIFICOND_POLL_DELAY = 500000;
static const int32_t WIFICOND_RETRY_COUNT = 20;

WificondControl* WificondControl::sInstance = nullptr;

WificondControl::WificondControl()
    : mWificond(nullptr), mClientInterface(nullptr), mScanner(nullptr) {}

WificondControl* WificondControl::Get() {
  MOZ_ASSERT(NS_IsMainThread());

  if (sInstance) {
    return sInstance;
  }

  // Create new instance
  sInstance = new WificondControl();

  ClearOnShutdown(&sInstance);
  return sInstance;
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
    WIFI_LOGE(LOG_TAG, "Failed to create wificond instance.");
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
    WIFI_LOGE(LOG_TAG, "start wificond failed.");
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
  InitWificondInterface();

  if (mWificond == nullptr) {
    return false;
  }

  if (!mWificond->createClientInterface(aIfaceName, &mClientInterface).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to create client interface.");
    return false;
  }

  if (!mClientInterface->getWifiScannerImpl(&mScanner).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to get WificondScannerImpl.");
    return false;
  }

  // TODO: should create scan event handler here?
  if (mScanner->subscribeScanEvents(aScanCallback).isOk()) {
    WIFI_LOGE(LOG_TAG, "subscribe scan event success");
  } else {
    WIFI_LOGE(LOG_TAG, "subscribe scan event failed");
  }

  return true;
}

bool WificondControl::StartSingleScan() {
  if (mScanner == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi scanner interface.");
    return false;
  }

  SingleScanSettings settings;

  // TODO: add settings from framework
  settings.scan_type_ = IWifiScannerImpl::SCAN_TYPE_LOW_SPAN;

  ChannelSettings channel;
  HiddenNetwork hidden;

  channel.frequency_ = 2412;
  const uint8_t tempSsid[] = {'K', 'a', 'i', 'O', 'S'};
  hidden.ssid_ = std::vector<uint8_t>(tempSsid, tempSsid + sizeof(tempSsid));

  settings.channel_settings_ = {channel};
  settings.hidden_networks_ = {hidden};

  bool success = false;
  mScanner->scan(settings, &success);
  if (!success) {
    WIFI_LOGE(LOG_TAG, "Failed to start single scan.");
    return false;
  }
  return true;
}

bool WificondControl::StopSingleScan() { return true; }

bool WificondControl::StartPnoScan() { return true; }

bool WificondControl::StopPnoScan() { return true; }

bool WificondControl::GetScanResults(
    std::vector<NativeScanResult>& aScanResults) {
  if (mScanner == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi scanner interface.");
    return false;
  }

  if (!mScanner->getScanResults(&aScanResults).isOk()) {
    WIFI_LOGE(LOG_TAG, "Get scan results failed.");
    return false;
  }

  // TODO: scan results parsing
  return true;
}

bool WificondControl::SignalPoll() {
  if (mClientInterface == nullptr) {
    WIFI_LOGE(LOG_TAG, "Invalid wifi client interface.");
    return false;
  }

  std::vector<int32_t> signal;
  if (!mClientInterface->signalPoll(&signal).isOk()) {
    WIFI_LOGE(LOG_TAG, "Failed to get signal strength.");
    return false;
  }

  // TODO: update result
  return true;
}
