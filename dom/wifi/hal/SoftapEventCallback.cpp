/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
