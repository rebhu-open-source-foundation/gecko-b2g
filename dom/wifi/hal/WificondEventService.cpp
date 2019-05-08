/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "WificondEventService"
#include "WifiCommon.h"
#include "WificondEventService.h"

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::interface_cast;
using ::android::String16;
using ::android::binder::Status;

mozilla::Mutex WificondEventService::sLock("wificond-event");
WificondEventService* WificondEventService::sInstance = nullptr;

WificondEventService* WificondEventService::CreateService(
    const std::string& aInterfaceName) {
  if (sInstance) {
    return sInstance;
  }

  // Create new instance
  sInstance = new WificondEventService(aInterfaceName);
  ClearOnShutdown(&sInstance);

  android::sp<::android::IServiceManager> sm = defaultServiceManager();
  android::sp<::android::ProcessState> ps(::android::ProcessState::self());
  if (android::defaultServiceManager()->addService(
          android::String16(WificondEventService::GetServiceName()),
          sInstance) != android::OK) {
    WIFI_LOGE(LOG_TAG, "Failed to add service: %s",
              WificondEventService::GetServiceName());
    sInstance = nullptr;
    return nullptr;
  }
  ps->startThreadPool();

  return sInstance;
}

void WificondEventService::RegisterEventCallback(EventCallback aCallback) {
  mEventCallback = aCallback;
}

void WificondEventService::UnregisterEventCallback() {
  mEventCallback = nullptr;
}

/**
 * Implement IScanEvent
 */
android::binder::Status WificondEventService::OnScanResultReady() {
  MutexAutoLock lock(sLock);

  nsCString iface(mStaInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING("SCAN_RESULT_READY"));

  if (mEventCallback) {
    mEventCallback(event, iface);
  }
  return android::binder::Status::ok();
}

android::binder::Status WificondEventService::OnScanFailed() {
  WIFI_LOGE(LOG_TAG, "scan failed...");
  return android::binder::Status::ok();
}
