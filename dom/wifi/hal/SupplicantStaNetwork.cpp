/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "SupplicantStaNetwork"

#include "SupplicantStaNetwork.h"
#include <mozilla/ClearOnShutdown.h>

constexpr uint32_t key_mgmt_none =
    (ISupplicantStaNetwork::KeyMgmtMask::NONE | 0x0);
constexpr uint32_t key_mgmt_wpa_psk =
    (ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK | 0x0);
constexpr uint32_t key_mgmt_wpa_eap =
    (ISupplicantStaNetwork::KeyMgmtMask::WPA_EAP | 0x0);
constexpr uint32_t key_mgmt_ieee8021x =
    (ISupplicantStaNetwork::KeyMgmtMask::IEEE8021X | 0x0);
constexpr uint32_t key_mgmt_ft_psk =
    (ISupplicantStaNetwork::KeyMgmtMask::FT_PSK | 0x0);
constexpr uint32_t key_mgmt_ft_eap =
    (ISupplicantStaNetwork::KeyMgmtMask::FT_EAP | 0x0);
constexpr uint32_t key_mgmt_osen =
    (ISupplicantStaNetwork::KeyMgmtMask::OSEN | 0x0);

mozilla::Mutex SupplicantStaNetwork::s_Lock("supplicant-network");

SupplicantStaNetwork::SupplicantStaNetwork(ISupplicantStaNetwork* aNetwork) {
  mNetwork = aNetwork;
}

SupplicantStaNetwork::~SupplicantStaNetwork() {}

Result_t SupplicantStaNetwork::SetConfiguration(ConfigurationOptions* aConfig) {
  if (aConfig == nullptr) {
    WIFI_LOGE(LOG_TAG, "configuration is null");
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }

  // ssid
  if (!aConfig->mSsid.IsEmpty()) {
    std::string ssid(NS_ConvertUTF16toUTF8(aConfig->mSsid).get());
    if (SetSsid(ssid) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }

  // bssid
  if (!aConfig->mBssid.IsEmpty()) {
    std::string bssid(NS_ConvertUTF16toUTF8(aConfig->mBssid).get());
    if (SetBssid(bssid) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }

  // key management
  uint32_t keyMgmtMask;
  if (aConfig->mKeyMgmt.IsEmpty()) {
    keyMgmtMask = key_mgmt_none;
  } else {
    std::string keyMgmt(NS_ConvertUTF16toUTF8(aConfig->mKeyMgmt).get());
    // remove quotation marks
    Dequote(keyMgmt);
    keyMgmtMask = ConvertKeyMgmtToMask(keyMgmt);
  }
  if (SetKeyMgmt(keyMgmtMask) != SupplicantStatusCode::SUCCESS) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  // psk
  if (!aConfig->mPsk.IsEmpty()) {
    std::string psk(NS_ConvertUTF16toUTF8(aConfig->mPsk).get());
    if (SetPsk(psk) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaNetwork::GetConfiguration() {
  MOZ_ASSERT(mNetwork);
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaNetwork::SelectNetwork() {
  MOZ_ASSERT(mNetwork);
  mNetwork->select([](const SupplicantStatus& status) {
    WIFI_LOGD(LOG_TAG, "select network: %d", status.code);
  });
  return nsIWifiResult::SUCCESS;
}

/**
 * Internal function to set wifi configuration.
 */
SupplicantStatusCode SupplicantStaNetwork::SetSsid(const std::string& aSsid) {
  MOZ_ASSERT(mNetwork);
  std::string ssid_str(aSsid);
  Dequote(ssid_str);

  WIFI_LOGD(LOG_TAG, "ssid => %s", ssid_str.c_str());

  SupplicantStatus response;
  std::vector<uint8_t> ssid(ssid_str.begin(), ssid_str.end());

  mNetwork->setSsid(ssid, [&response](const SupplicantStatus& status) {
    response = status;
    WIFI_LOGD(LOG_TAG, "set ssid return: %d", response.code);
  });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetBssid(const std::string& aBssid) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "bssid => %s", aBssid.c_str());

  SupplicantStatus response;
  std::array<uint8_t, 6> bssid;
  ConvertMacToByteArray(aBssid, bssid);

  mNetwork->setBssid(bssid, [&response](const SupplicantStatus& status) {
    response = status;
    WIFI_LOGD(LOG_TAG, "set bssid return: %d", response.code);
  });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetKeyMgmt(uint32_t aKeyMgmtMask) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "key_mgmt => %d", aKeyMgmtMask);

  SupplicantStatus response;
  mNetwork->setKeyMgmt(
      aKeyMgmtMask, [&response](const SupplicantStatus& status) {
        WIFI_LOGD(LOG_TAG, "set key_mgmt return: %d", response.code);
        response = status;
      });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetPsk(const std::string& aPsk) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "psk => %s", aPsk.c_str());

  SupplicantStatus response;
  mNetwork->setPskPassphrase(aPsk, [&response](const SupplicantStatus& status) {
    WIFI_LOGD(LOG_TAG, "set psk return: %d", response.code);
    response = status;
  });
  return response.code;
}

uint32_t SupplicantStaNetwork::ConvertKeyMgmtToMask(
    const std::string& aKeyMgmt) {
  uint32_t mask;
  if (aKeyMgmt.compare("NONE") == 0) {
    mask = key_mgmt_none;
  } else if (aKeyMgmt.compare("WPA-PSK") == 0 ||
             aKeyMgmt.compare("WPA2-PSK") == 0) {
    mask = key_mgmt_wpa_psk;
  } else if (aKeyMgmt.compare("WPA-EAP") == 0) {
    mask = key_mgmt_wpa_eap;
  } else if (aKeyMgmt.compare("FT-PSK") == 0) {
    mask = key_mgmt_ft_psk;
  } else if (aKeyMgmt.compare("FT_EAP") == 0) {
    mask = key_mgmt_ft_eap;
  } else if (aKeyMgmt.compare("OSEN") == 0) {
    mask = key_mgmt_osen;
  } else {
    WIFI_LOGD(LOG_TAG, "Unknown key management, use default NONE");
    mask = key_mgmt_none;
  }
  return mask;
}

/**
 * ISupplicantStaNetworkCallback implementation
 */
Return<void> SupplicantStaNetwork::onNetworkEapSimGsmAuthRequest(
    const ISupplicantStaNetworkCallback::NetworkRequestEapSimGsmAuthParams& params) {
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapSimGsmAuthRequest()");
  return android::hardware::Void();
}

Return<void> SupplicantStaNetwork::onNetworkEapSimUmtsAuthRequest(
    const ISupplicantStaNetworkCallback::NetworkRequestEapSimUmtsAuthParams& params) {
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapSimUmtsAuthRequest()");
  return android::hardware::Void();
}

Return<void> SupplicantStaNetwork::onNetworkEapIdentityRequest() {
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapIdentityRequest()");
  return android::hardware::Void();
}
