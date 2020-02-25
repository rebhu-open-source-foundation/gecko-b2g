/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "SoftapEventCallback"

#include "WifiCommon.h"
#include "SoftapEventCallback.h"

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::String16;

SoftapEventService* SoftapEventService::s_Instance = nullptr;

SoftapEventService* SoftapEventService::CreateService(
    const std::string& aInterfaceName) {
  if (s_Instance) {
    return s_Instance;
  }
  // Create new instance
  s_Instance = new SoftapEventService(aInterfaceName);
  ClearOnShutdown(&s_Instance);

  android::sp<::android::IServiceManager> sm = defaultServiceManager();
  android::sp<::android::ProcessState> ps(::android::ProcessState::self());
  if (android::defaultServiceManager()->addService(
          android::String16(SoftapEventService::GetServiceName()),
          s_Instance) != android::OK) {
    WIFI_LOGE(LOG_TAG, "Failed to add service: %s",
              SoftapEventService::GetServiceName());
    s_Instance = nullptr;
    return nullptr;
  }
  ps->startThreadPool();
  return s_Instance;
}

void SoftapEventService::RegisterEventCallback(EventCallback aCallback) {
  mEventCallback = aCallback;
}

void SoftapEventService::UnregisterEventCallback() { mEventCallback = nullptr; }

/**
 * Implement IApInterfaceEventCallback
 */
android::binder::Status SoftapEventService::onNumAssociatedStationsChanged(
    int32_t numStations) {
  return android::binder::Status::ok();
}

android::binder::Status SoftapEventService::onSoftApChannelSwitched(
    int32_t frequency, int32_t bandwidth) {
  return android::binder::Status::ok();
}
