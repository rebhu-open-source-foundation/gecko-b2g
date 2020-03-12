/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WificondEventService_H
#define WificondEventService_H

#include <android/net/wifi/BnScanEvent.h>
#include <binder/BinderService.h>

class WificondEventService
    : virtual public android::BinderService<WificondEventService>,
      virtual public android::net::wifi::BnScanEvent {
 public:
  WificondEventService(const std::string& aInterfaceName)
      : android::net::wifi::BnScanEvent(), mStaInterfaceName(aInterfaceName) {}
  virtual ~WificondEventService() = default;

  static char const* GetServiceName() { return "wificond_event"; }
  static WificondEventService* CreateService(const std::string& aInterfaceName);

  void RegisterEventCallback(EventCallback aCallback);
  void UnregisterEventCallback();

  // IScanEvent
  android::binder::Status OnScanResultReady() override;
  android::binder::Status OnScanFailed() override;

 private:
  static WificondEventService* s_Instance;
  static mozilla::Mutex s_Lock;
  std::string mStaInterfaceName;

  EventCallback mEventCallback;
};

#endif  // WificondEventService_H
