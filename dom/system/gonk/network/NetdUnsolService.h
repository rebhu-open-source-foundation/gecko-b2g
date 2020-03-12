/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _NETD_UNSOLSERVICE_H_
#define _NETD_UNSOLSERVICE_H_

#include <binder/BinderService.h>
#include "mozilla/dom/NetworkOptionsBinding.h"
#include <android/net/BnNetdUnsolicitedEventListener.h>

static const uint32_t BUF_SIZE = 1024;

enum UnsolEvent : uint32_t {
  InterfaceClassActivity = 1 << 0,
  QuotaLimitReached = 1 << 1,
  InterfaceDnsServersAdded = 1 << 2,
  InterfaceAddressUpdated = 1 << 3,
  InterfaceAddressRemoved = 1 << 4,
  InterfaceAdded = 1 << 5,
  InterfaceRemoved = 1 << 6,
  InterfaceChanged = 1 << 7,
  InterfaceLinkStatusChanged = 1 << 8,
  RouteChanged = 1 << 9,
  StrictCleartextDetected = 1 << 10,
};

typedef void (*NetdEventCallback)(
    mozilla::dom::NetworkResultOptions& aResult);

class NetdUnsolService : public android::BinderService<NetdUnsolService>,
                         public android::net::BnNetdUnsolicitedEventListener {
 public:
  NetdUnsolService(NetdEventCallback aCallback);
  ~NetdUnsolService() = default;

  void updateDebug(bool aEnable);

  static char const* getServiceName() { return "netdUnsolService"; }
  android::binder::Status onInterfaceClassActivityChanged(bool isActive,
                                                          int label,
                                                          int64_t timestamp,
                                                          int uid) override;
  android::binder::Status onQuotaLimitReached(
      const std::string& alertName, const std::string& ifName) override;
  android::binder::Status onInterfaceDnsServerInfo(
      const std::string& ifName, int64_t lifetime,
      const std::vector<std::string>& servers) override;
  android::binder::Status onInterfaceAddressUpdated(const std::string& addr,
                                                    const std::string& ifName,
                                                    int flags,
                                                    int scope) override;
  android::binder::Status onInterfaceAddressRemoved(const std::string& addr,
                                                    const std::string& ifName,
                                                    int flags,
                                                    int scope) override;
  android::binder::Status onInterfaceAdded(const std::string& ifName) override;
  android::binder::Status onInterfaceRemoved(
      const std::string& ifName) override;
  android::binder::Status onInterfaceChanged(const std::string& ifName,
                                             bool status) override;
  android::binder::Status onInterfaceLinkStateChanged(const std::string& ifName,
                                                      bool status) override;
  android::binder::Status onRouteChanged(bool updated, const std::string& route,
                                         const std::string& gateway,
                                         const std::string& ifName) override;
  android::binder::Status onStrictCleartextDetected(
      int uid, const std::string& hex) override;

 private:
  NetdEventCallback mNetdEventCallback;
  /**
   * Notify broadcast message to main thread.
   */
  void sendBroadcast(UnsolEvent evt, char* reason);
};

#endif  // _NETD_UNSOLSERVICE_H_
