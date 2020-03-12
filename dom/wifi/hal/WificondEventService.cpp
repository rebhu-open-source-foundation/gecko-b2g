/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

#define EVENT_SCAN_RESULT_READY "SCAN_RESULT_READY"

mozilla::Mutex WificondEventService::s_Lock("wificond-event");
WificondEventService* WificondEventService::s_Instance = nullptr;

WificondEventService* WificondEventService::CreateService(
    const std::string& aInterfaceName) {
  if (s_Instance) {
    return s_Instance;
  }

  // Create new instance
  s_Instance = new WificondEventService(aInterfaceName);
  ClearOnShutdown(&s_Instance);

  android::sp<::android::IServiceManager> sm = defaultServiceManager();
  android::sp<::android::ProcessState> ps(::android::ProcessState::self());
  if (android::defaultServiceManager()->addService(
          android::String16(WificondEventService::GetServiceName()),
          s_Instance) != android::OK) {
    WIFI_LOGE(LOG_TAG, "Failed to add service: %s",
              WificondEventService::GetServiceName());
    s_Instance = nullptr;
    return nullptr;
  }
  ps->startThreadPool();

  return s_Instance;
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
  MutexAutoLock lock(s_Lock);

  nsCString iface(mStaInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SCAN_RESULT_READY));

  if (mEventCallback) {
    mEventCallback(event, iface);
  }
  return android::binder::Status::ok();
}

android::binder::Status WificondEventService::OnScanFailed() {
  WIFI_LOGE(LOG_TAG, "scan failed...");
  return android::binder::Status::ok();
}
