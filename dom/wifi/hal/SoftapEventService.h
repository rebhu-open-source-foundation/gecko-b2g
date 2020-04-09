/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SoftapEventService_H
#define SoftapEventService_H

#include <android/net/wifi/BnApInterfaceEventCallback.h>
#include <binder/BinderService.h>

class SoftapEventService
    : virtual public android::BinderService<SoftapEventService>,
      virtual public android::net::wifi::BnApInterfaceEventCallback {
 public:
  SoftapEventService(const std::string& aInterfaceName)
      : android::net::wifi::BnApInterfaceEventCallback(),
        mSoftapInterfaceName(aInterfaceName) {}
  virtual ~SoftapEventService() = default;

  static char const* GetServiceName() { return "softap.event"; }
  static SoftapEventService* CreateService(const std::string& aInterfaceName);

  void RegisterEventCallback(WifiEventCallback* aCallback);
  void UnregisterEventCallback();

  // IApInterfaceEventCallback
  android::binder::Status onNumAssociatedStationsChanged(
      int32_t numStations) override;
  android::binder::Status onSoftApChannelSwitched(
      int32_t frequency, int32_t bandwidth) override;

 private:
  static SoftapEventService* s_Instance;
  std::string mSoftapInterfaceName;

  android::sp<WifiEventCallback> mCallback;
};

#endif  // SoftapEventService_H
