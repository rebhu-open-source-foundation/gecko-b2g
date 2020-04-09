/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "ScanEventService"

#include "WifiCommon.h"
#include "ScanEventService.h"

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <utils/String16.h>

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::interface_cast;
using ::android::String16;

static std::string g_InterfaceName;

/* event name */
#define EVENT_SCAN_RESULT_READY "SCAN_RESULT_READY"
#define EVENT_SCAN_RESULT_FAILED "SCAN_RESULT_FAILED"
#define EVENT_PNO_SCAN_FOUND "PNO_SCAN_FOUND"
#define EVENT_PNO_SCAN_FAILED "PNO_SCAN_FAILED"

/**
 * EventCallbackHandler
 */
void EventCallbackHandler::RegisterEventCallback(WifiEventCallback* aCallback) {
  mCallback = aCallback;
}

void EventCallbackHandler::UnregisterEventCallback() { mCallback = nullptr; }

/**
 * ScanEventService
 */
mozilla::Mutex ScanEventService::s_Lock("scan_lock");
android::sp<ScanEventService> ScanEventService::s_ScanEvent = nullptr;

android::sp<ScanEventService> ScanEventService::CreateService(
    const std::string& aInterfaceName) {
  if (s_ScanEvent) {
    return s_ScanEvent;
  }
  g_InterfaceName = aInterfaceName;

  // Create new instance
  s_ScanEvent = new ScanEventService();
  ClearOnShutdown(&s_ScanEvent);

  if (BinderService<ScanEventService>::publish() != android::OK) {
    WIFI_LOGE(LOG_TAG, "Failed to add IScanEvent service");
    s_ScanEvent = nullptr;
    return nullptr;
  }
  android::ProcessState::self()->startThreadPool();
  android::ProcessState::self()->giveThreadPoolName();
  return s_ScanEvent;
}

/**
 * Implement IScanEvent
 */
android::binder::Status ScanEventService::OnScanResultReady() {
  MutexAutoLock lock(s_Lock);

  nsCString iface(g_InterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SCAN_RESULT_READY));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
  return android::binder::Status::ok();
}

android::binder::Status ScanEventService::OnScanFailed() {
  MutexAutoLock lock(s_Lock);

  nsCString iface(g_InterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SCAN_RESULT_FAILED));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
  return android::binder::Status::ok();
}

/**
 * PnoScanEventService
 */
mozilla::Mutex PnoScanEventService::s_Lock("pno_lock");
android::sp<PnoScanEventService> PnoScanEventService::s_PnoScanEvent = nullptr;

android::sp<PnoScanEventService> PnoScanEventService::CreateService(
    const std::string& aInterfaceName) {
  if (s_PnoScanEvent) {
    return s_PnoScanEvent;
  }
  g_InterfaceName = aInterfaceName;

  // Create new instance
  s_PnoScanEvent = new PnoScanEventService();
  ClearOnShutdown(&s_PnoScanEvent);

  if (BinderService<PnoScanEventService>::publish() != android::OK) {
    s_PnoScanEvent = nullptr;
    return nullptr;
  }
  android::ProcessState::self()->startThreadPool();
  android::ProcessState::self()->giveThreadPoolName();
  return s_PnoScanEvent;
}

/**
 * Implement IPnoScanEvent
 */
android::binder::Status PnoScanEventService::OnPnoNetworkFound() {
  MutexAutoLock lock(s_Lock);

  nsCString iface(g_InterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_PNO_SCAN_FOUND));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
  return android::binder::Status::ok();
}

android::binder::Status PnoScanEventService::OnPnoScanFailed() {
  MutexAutoLock lock(s_Lock);

  nsCString iface(g_InterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_PNO_SCAN_FAILED));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
  return android::binder::Status::ok();
}

android::binder::Status PnoScanEventService::OnPnoScanOverOffloadStarted() {
  return android::binder::Status::ok();
}

android::binder::Status PnoScanEventService::OnPnoScanOverOffloadFailed(
    int32_t reason) {
  return android::binder::Status::ok();
}
