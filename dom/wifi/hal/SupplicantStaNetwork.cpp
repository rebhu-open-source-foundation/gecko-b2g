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

Result_t SupplicantStaNetwork::SetConfiguration(
    const NetworkConfiguration& aConfig) {
  NetworkConfiguration config(aConfig);

  // ssid
  if (!config.mSsid.empty()) {
    if (SetSsid(config.mSsid) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }

  // bssid
  if (!config.mBssid.empty()) {
    if (SetBssid(config.mBssid) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }

  // key management
  uint32_t keyMgmtMask;
  if (config.mKeyMgmt.empty()) {
    keyMgmtMask = key_mgmt_none;
  } else {
    // remove quotation marks
    Dequote(config.mKeyMgmt);
    keyMgmtMask = ConvertKeyMgmtToMask(config.mKeyMgmt);
  }
  if (SetKeyMgmt(keyMgmtMask) != SupplicantStatusCode::SUCCESS) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  // psk
  if (!config.mPsk.empty()) {
    if (SetPsk(config.mPsk) != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaNetwork::EnableNetwork() {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  bool noConnect = false;
  HIDL_SET(mNetwork, enable, SupplicantStatus, response, noConnect);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::DisableNetwork() {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  HIDL_CALL(mNetwork, disable, SupplicantStatus, response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SelectNetwork() {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  HIDL_CALL(mNetwork, select, SupplicantStatus, response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

/**
 * Internal functions to set wifi configuration.
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

SupplicantStatusCode SupplicantStaNetwork::GetSsid(std::string& aSsid) {
  MOZ_ASSERT(mNetwork);

  SupplicantStatus response;
  mNetwork->getSsid([&](const SupplicantStatus& status,
                        const ::android::hardware::hidl_vec<uint8_t>& ssid) {
    response = status;
    if (response.code == SupplicantStatusCode::SUCCESS) {
      std::string ssid_str(ssid.begin(), ssid.end());
      aSsid = ssid_str.empty() ? "" : ssid_str;
    }
  });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::GetBssid(std::string& aBssid) {
  MOZ_ASSERT(mNetwork);

  SupplicantStatus response;
  mNetwork->getBssid(
      [&](const SupplicantStatus& status,
          const ::android::hardware::hidl_array<uint8_t, 6>& bssid) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          std::string bssid_str = ConvertMacToString(bssid);
          aBssid = bssid_str.empty() ? "" : bssid_str;
        }
      });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::GetKeyMgmt(uint32_t& aKeyMgmtMask) {
  MOZ_ASSERT(mNetwork);

  SupplicantStatus response;
  mNetwork->getKeyMgmt(
      [&](const SupplicantStatus& status,
          hidl_bitfield<ISupplicantStaNetwork::KeyMgmtMask> keyMgmtMask) {
        response = status;
        aKeyMgmtMask = (uint32_t)keyMgmtMask;
      });
  return response.code;
}

/**
 * Internal helper functions.
 */
uint32_t SupplicantStaNetwork::ConvertKeyMgmtToMask(
    const std::string& aKeyMgmt) const {
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

std::string SupplicantStaNetwork::ConvertMaskToKeyMgmt(uint32_t aMask) const {
  std::string keyMgmt;

  if (aMask == key_mgmt_none) {
    keyMgmt.assign("NONE");
  } else if (aMask == key_mgmt_wpa_psk) {
    keyMgmt.assign("WPA-PSK");
  } else if (key_mgmt_wpa_eap) {
    keyMgmt.assign("WPA-EAP");
  } else if (key_mgmt_ft_psk) {
    keyMgmt.assign("FT-PSK");
  } else if (key_mgmt_ft_eap) {
    keyMgmt.assign("FT_EAP");
  } else if (key_mgmt_osen) {
    keyMgmt.assign("OSEN");
  } else {
    keyMgmt.assign("UNKNOWN");
  }
  return keyMgmt;
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
