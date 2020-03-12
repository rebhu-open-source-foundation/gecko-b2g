/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SupplicantStaNetwork_H
#define SupplicantStaNetwork_H

#include "WifiCommon.h"

#include <string.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantNetwork.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>
#include <android/hardware/wifi/supplicant/1.0/types.h>

#include "mozilla/Mutex.h"

using ::android::hardware::wifi::supplicant::V1_0::ISupplicantNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetworkCallback;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;

class SupplicantStaNetwork
    : virtual public android::RefBase,
      virtual public android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetworkCallback {
 public:
  SupplicantStaNetwork(ISupplicantStaNetwork* aNetwork);

  bool SetConfiguration(ConfigurationOptions* aConfig);
  bool GetConfiguration();
  bool SelectNetwork();

 private:
  virtual ~SupplicantStaNetwork();

  //..................... ISupplicantStaNetworkCallback ......................./
  /**
   * Used to request EAP GSM SIM authentication for this particular network.
   *
   * The response for the request must be sent using the corresponding
   * |ISupplicantNetwork.sendNetworkEapSimGsmAuthResponse| call.
   *
   * @param params Params associated with the request.
   */
  Return<void> onNetworkEapSimGsmAuthRequest(
      const ISupplicantStaNetworkCallback::NetworkRequestEapSimGsmAuthParams& params) override;
  /**
   * Used to request EAP UMTS SIM authentication for this particular network.
   *
   * The response for the request must be sent using the corresponding
   * |ISupplicantNetwork.sendNetworkEapSimUmtsAuthResponse| call.
   *
   * @param params Params associated with the request.
   */
  Return<void> onNetworkEapSimUmtsAuthRequest(
      const ISupplicantStaNetworkCallback::NetworkRequestEapSimUmtsAuthParams& params) override;
  /**
   * Used to request EAP Identity for this particular network.
   *
   * The response for the request must be sent using the corresponding
   * |ISupplicantNetwork.sendNetworkEapIdentityResponse| call.
   */
  Return<void> onNetworkEapIdentityRequest() override;

  SupplicantStatusCode SetSsid(const std::string& aSsid);
  SupplicantStatusCode SetBssid(const std::string& aBssid);
  SupplicantStatusCode SetKeyMgmt(int32_t aKeyMgmtMask);
  SupplicantStatusCode SetPsk(const std::string& aPsk);

  static mozilla::Mutex s_Lock;

  android::sp<ISupplicantStaNetwork> mNetwork;
};

#endif /* SupplicantStaNetwork_H */
