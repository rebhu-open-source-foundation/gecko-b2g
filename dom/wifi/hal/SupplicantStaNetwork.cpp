/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#define LOG_TAG "SupplicantStaNetwork"

#include "SupplicantStaNetwork.h"
#include <mozilla/ClearOnShutdown.h>

constexpr uint32_t key_mgmt_none =
    (ISupplicantStaNetwork::KeyMgmtMask::NONE | 0x0);
constexpr uint32_t key_mgmt_wpa_psk =
    (ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK | 0x0);

mozilla::Mutex SupplicantStaNetwork::s_Lock("supplicant-network");

SupplicantStaNetwork::SupplicantStaNetwork(ISupplicantStaNetwork* aNetwork) {
  mNetwork = aNetwork;
}

SupplicantStaNetwork::~SupplicantStaNetwork() {}

bool SupplicantStaNetwork::SetConfiguration(ConfigurationOptions* aConfig) {
  if (aConfig == nullptr) {
    WIFI_LOGE(LOG_TAG, "configuration is null");
    return false;
  }

  // ssid
  if (!aConfig->mSsid.IsEmpty()) {
    std::string ssid(NS_ConvertUTF16toUTF8(aConfig->mSsid).get());
    if (SetSsid(ssid) != SupplicantStatusCode::SUCCESS) {
      return false;
    }
  }

  // bssid
  if (!aConfig->mBssid.IsEmpty()) {
    std::string bssid(NS_ConvertUTF16toUTF8(aConfig->mBssid).get());
    if (SetBssid(bssid) != SupplicantStatusCode::SUCCESS) {
      return false;
    }
  }

  // key management
  if (SetKeyMgmt(aConfig->mKeyManagement) != SupplicantStatusCode::SUCCESS) {
    return false;
  }

  // psk
  if (!aConfig->mPsk.IsEmpty()) {
    std::string psk(NS_ConvertUTF16toUTF8(aConfig->mPsk).get());
    if (SetPsk(psk) != SupplicantStatusCode::SUCCESS) {
      return false;
    }
  }
  return true;
}

bool SupplicantStaNetwork::GetConfiguration() {
  MOZ_ASSERT(mNetwork);
  return true;
}

bool SupplicantStaNetwork::SelectNetwork() {
  MOZ_ASSERT(mNetwork);

  mNetwork->select([](const SupplicantStatus& status) {
    WIFI_LOGD(LOG_TAG, "select network: %d", status.code);
  });
  return true;
}

/**
 * Internal function to set wifi configuration.
 */
SupplicantStatusCode SupplicantStaNetwork::SetSsid(const std::string& aSsid) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "ssid => %s", aSsid.c_str());

  SupplicantStatus response;
  std::vector<uint8_t> ssid(aSsid.begin(), aSsid.end());

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

  std::sscanf(aBssid.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0],
              bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

  mNetwork->setBssid(bssid, [&response](const SupplicantStatus& status) {
    response = status;
    WIFI_LOGD(LOG_TAG, "set bssid return: %d", response.code);
  });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetKeyMgmt(int32_t aKeyMgmtMask) {
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
