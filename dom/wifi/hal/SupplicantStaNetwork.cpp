/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "SupplicantStaNetwork"

#include "SupplicantStaNetwork.h"
#include <mozilla/ClearOnShutdown.h>
#include <iomanip>

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

constexpr uint32_t protocol_wpa = (ISupplicantStaNetwork::ProtoMask::WPA | 0x0);
constexpr uint32_t protocol_rsn = (ISupplicantStaNetwork::ProtoMask::RSN | 0x0);
constexpr uint32_t protocol_osen =
    (ISupplicantStaNetwork::ProtoMask::OSEN | 0x0);

constexpr uint32_t auth_alg_open =
    (ISupplicantStaNetwork::AuthAlgMask::OPEN | 0x0);
constexpr uint32_t auth_alg_shared =
    (ISupplicantStaNetwork::AuthAlgMask::SHARED | 0x0);
constexpr uint32_t auth_alg_leap =
    (ISupplicantStaNetwork::AuthAlgMask::LEAP | 0x0);

constexpr uint32_t group_cipher_wep40 =
    (ISupplicantStaNetwork::GroupCipherMask::WEP40 | 0x0);
constexpr uint32_t group_cipher_wep104 =
    (ISupplicantStaNetwork::GroupCipherMask::WEP104 | 0x0);
constexpr uint32_t group_cipher_tkip =
    (ISupplicantStaNetwork::GroupCipherMask::TKIP | 0x0);
constexpr uint32_t group_cipher_ccmp =
    (ISupplicantStaNetwork::GroupCipherMask::CCMP | 0x0);
constexpr uint32_t group_cipher_gtk_not_used =
    (ISupplicantStaNetwork::GroupCipherMask::GTK_NOT_USED | 0x0);

constexpr uint32_t pairwise_cipher_none =
    (ISupplicantStaNetwork::PairwiseCipherMask::NONE | 0x0);
constexpr uint32_t pairwise_cipher_tkip =
    (ISupplicantStaNetwork::PairwiseCipherMask::TKIP | 0x0);
constexpr uint32_t pairwise_cipher_ccmp =
    (ISupplicantStaNetwork::PairwiseCipherMask::CCMP | 0x0);

#define EVENT_EAP_SIM_GSM_AUTH_REQUEST "EAP_SIM_GSM_AUTH_REQUEST"
#define EVENT_EAP_SIM_UMTS_AUTH_REQUEST "EAP_SIM_UMTS_AUTH_REQUEST"
#define EVENT_EAP_SIM_IDENTITY_REQUEST "EAP_SIM_IDENTITY_REQUEST"

mozilla::Mutex SupplicantStaNetwork::sLock("supplicant-network");

SupplicantStaNetwork::SupplicantStaNetwork(const std::string& aInterfaceName,
                                           WifiEventCallback* aCallback,
                                           ISupplicantStaNetwork* aNetwork)
    : mInterfaceName(aInterfaceName),
      mCallback(aCallback),
      mNetwork(aNetwork) {}

SupplicantStaNetwork::~SupplicantStaNetwork() {}

Result_t SupplicantStaNetwork::SetConfiguration(
    const NetworkConfiguration& aConfig) {
  NetworkConfiguration config(aConfig);

  SupplicantStatusCode stateCode = SupplicantStatusCode::FAILURE_UNKNOWN;

  // ssid
  if (!config.mSsid.empty()) {
    stateCode = SetSsid(config.mSsid);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // bssid
  if (!config.mBssid.empty()) {
    stateCode = SetBssid(config.mBssid);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
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
  stateCode = SetKeyMgmt(keyMgmtMask);
  if (stateCode != SupplicantStatusCode::SUCCESS) {
    return ConvertStatusToResult(stateCode);
  }

  // psk
  if (!config.mPsk.empty()) {
    stateCode = SetPsk(config.mPsk);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // wep key
  if (!config.mWepKey0.empty() || !config.mWepKey1.empty() ||
      !config.mWepKey2.empty() || !config.mWepKey3.empty()) {
    std::array<std::string, max_wep_key_num> wepKeys = {
        config.mWepKey0, config.mWepKey1, config.mWepKey2, config.mWepKey3};
    stateCode = SetWepKey(wepKeys, aConfig.mWepTxKeyIndex);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // proto
  if (!config.mProto.empty()) {
    stateCode = SetProto(config.mProto);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // auth algorithms
  if (!config.mAuthAlg.empty()) {
    stateCode = SetAuthAlg(config.mAuthAlg);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // group cipher
  if (!config.mGroupCipher.empty()) {
    stateCode = SetGroupCipher(config.mGroupCipher);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // proto
  if (!config.mPairwiseCipher.empty()) {
    stateCode = SetPairwiseCipher(config.mPairwiseCipher);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // eap configurations
  if (config.IsEapNetwork()) {
    stateCode = SetEapConfiguration(config);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return ConvertStatusToResult(stateCode);
    }
  }

  // Network is configured successfully, now we can try to
  // register event callback.
  stateCode = RegisterNetworkCallback();
  if (stateCode != SupplicantStatusCode::SUCCESS) {
    return ConvertStatusToResult(stateCode);
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

Result_t SupplicantStaNetwork::SendEapSimIdentityResponse(
    SimIdentityRespDataOptions* aIdentity) {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  std::string identityStr = NS_ConvertUTF16toUTF8(aIdentity->mIdentity).get();
  std::vector<uint8_t> identity(identityStr.begin(), identityStr.end());
  HIDL_SET(mNetwork, sendNetworkEapIdentityResponse, SupplicantStatus, response,
           identity);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SendEapSimGsmAuthResponse(
    const nsTArray<SimGsmAuthRespDataOptions>& aGsmAuthResp) {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  std::vector<ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams>
      gsmAuthParams;

  for (auto& item : aGsmAuthResp) {
    std::string kcStr = NS_ConvertUTF16toUTF8(item.mKc).get();
    std::string sresStr = NS_ConvertUTF16toUTF8(item.mSres).get();

    ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams params;
    std::array<uint8_t, 8> kc;
    std::array<uint8_t, 4> sres;
    if (ConvertHexStringToByteArray(kcStr, kc) < 0 ||
        ConvertHexStringToByteArray(sresStr, sres) < 0) {
      return nsIWifiResult::ERROR_INVALID_ARGS;
    }
    params.kc = kc;
    params.sres = sres;
    gsmAuthParams.push_back(params);
  }

  HIDL_SET(mNetwork, sendNetworkEapSimGsmAuthResponse, SupplicantStatus,
           response, gsmAuthParams);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SendEapSimGsmAuthFailure() {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  HIDL_CALL(mNetwork, sendNetworkEapSimGsmAuthFailure, SupplicantStatus,
            response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SendEapSimUmtsAuthResponse(
    SimUmtsAuthRespDataOptions* aUmtsAuthResp) {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams umtsAuthParams;

  std::string resStr = NS_ConvertUTF16toUTF8(aUmtsAuthResp->mRes).get();
  std::string ikStr = NS_ConvertUTF16toUTF8(aUmtsAuthResp->mIk).get();
  std::string ckStr = NS_ConvertUTF16toUTF8(aUmtsAuthResp->mCk).get();

  std::vector<uint8_t> res;
  std::array<uint8_t, 16> ik;
  std::array<uint8_t, 16> ck;
  if (ConvertHexStringToBytes(resStr, res) < 0 ||
      ConvertHexStringToByteArray(ikStr, ik) < 0 ||
      ConvertHexStringToByteArray(ckStr, ck) < 0) {
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }
  umtsAuthParams.res = res;
  umtsAuthParams.ik = ik;
  umtsAuthParams.ck = ck;

  HIDL_SET(mNetwork, sendNetworkEapSimUmtsAuthResponse, SupplicantStatus,
           response, umtsAuthParams);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SendEapSimUmtsAutsResponse(
    SimUmtsAutsRespDataOptions* aUmtsAutsResp) {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  std::string autsStr = NS_ConvertUTF16toUTF8(aUmtsAutsResp->mAuts).get();

  std::array<uint8_t, 14> auts;
  if (ConvertHexStringToByteArray(autsStr, auts) < 0) {
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }

  HIDL_SET(mNetwork, sendNetworkEapSimUmtsAutsResponse, SupplicantStatus,
           response, auts);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaNetwork::SendEapSimUmtsAuthFailure() {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response;
  HIDL_CALL(mNetwork, sendNetworkEapSimUmtsAuthFailure, SupplicantStatus,
            response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

/**
 * Internal functions to set wifi configuration.
 */
SupplicantStatusCode SupplicantStaNetwork::SetSsid(const std::string& aSsid) {
  MOZ_ASSERT(mNetwork);
  std::string ssidStr(aSsid);
  Dequote(ssidStr);

  WIFI_LOGD(LOG_TAG, "ssid => %s", ssidStr.c_str());

  uint32_t maxSsid = static_cast<uint32_t>(
      ISupplicantStaNetwork::ParamSizeLimits::SSID_MAX_LEN_IN_BYTES);

  SupplicantStatus response;
  std::vector<uint8_t> ssid(ssidStr.begin(), ssidStr.end());

  if (ssid.size() == 0 || ssid.size() > maxSsid) {
    return SupplicantStatusCode::FAILURE_ARGS_INVALID;
  }

  HIDL_SET(mNetwork, setSsid, SupplicantStatus, response, ssid);
  WIFI_LOGD(LOG_TAG, "set ssid return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetBssid(const std::string& aBssid) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "bssid => %s", aBssid.c_str());

  SupplicantStatus response;
  std::array<uint8_t, 6> bssid;
  ConvertMacToByteArray(aBssid, bssid);

  HIDL_SET(mNetwork, setBssid, SupplicantStatus, response, bssid);
  WIFI_LOGD(LOG_TAG, "set bssid return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetKeyMgmt(uint32_t aKeyMgmtMask) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "key_mgmt => %d", aKeyMgmtMask);

  SupplicantStatus response;
  HIDL_SET(mNetwork, setKeyMgmt, SupplicantStatus, response, aKeyMgmtMask);
  WIFI_LOGD(LOG_TAG, "set key_mgmt return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetPsk(const std::string& aPsk) {
  MOZ_ASSERT(mNetwork);
  std::string psk(aPsk);
  Dequote(psk);
  WIFI_LOGD(LOG_TAG, "psk => %s", psk.c_str());

  uint32_t minPskPassphrase = static_cast<uint32_t>(
      ISupplicantStaNetwork::ParamSizeLimits::PSK_PASSPHRASE_MIN_LEN_IN_BYTES);
  uint32_t maxPskPassphrase = static_cast<uint32_t>(
      ISupplicantStaNetwork::ParamSizeLimits::PSK_PASSPHRASE_MAX_LEN_IN_BYTES);

  if (psk.size() < minPskPassphrase || psk.size() > maxPskPassphrase) {
    return SupplicantStatusCode::FAILURE_ARGS_INVALID;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setPskPassphrase, SupplicantStatus, response, psk);
  WIFI_LOGD(LOG_TAG, "set psk return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetWepKey(
    const std::array<std::string, max_wep_key_num>& aWepKeys,
    int32_t aKeyIndex) {
  MOZ_ASSERT(mNetwork);
  SupplicantStatus response = {SupplicantStatusCode::FAILURE_UNKNOWN, ""};

  for (size_t i = 0; i < aWepKeys.size(); i++) {
    std::string keyStr = aWepKeys.at(i);

    if (!keyStr.empty()) {
      if (keyStr.size() > 1 && keyStr.front() == '"' && keyStr.back() == '"') {
        Dequote(keyStr);
      }

      uint32_t wep40Len = static_cast<uint32_t>(
          ISupplicantStaNetwork::ParamSizeLimits::WEP40_KEY_LEN_IN_BYTES);
      uint32_t wep104Len = static_cast<uint32_t>(
          ISupplicantStaNetwork::ParamSizeLimits::WEP104_KEY_LEN_IN_BYTES);

      std::vector<uint8_t> key;
      if (keyStr.size() == wep40Len || keyStr.size() == wep104Len) {
        // Key should be ASCII characters
        key = std::vector<uint8_t>(keyStr.begin(), keyStr.end());
      } else if (keyStr.size() == wep40Len * 2 ||
                 keyStr.size() == wep104Len * 2) {
        // Key should be a hexadecimal string
        if (ConvertHexStringToBytes(keyStr, key) < 0) {
          return SupplicantStatusCode::FAILURE_ARGS_INVALID;
        }
      }

      mNetwork->setWepKey(i, key, [&](const SupplicantStatus& status) {
        response = status;
        WIFI_LOGD(LOG_TAG, "set wep key return: %s",
                  ConvertStatusToString(response.code).c_str());
      });
    }
  }

  if (response.code == SupplicantStatusCode::SUCCESS) {
    HIDL_SET(mNetwork, setWepTxKeyIdx, SupplicantStatus, response, aKeyIndex);
    WIFI_LOGD(LOG_TAG, "set wep key index return: %s",
              ConvertStatusToString(response.code).c_str());
  }
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetProto(const std::string& aProto) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "proto => %s", aProto.c_str());

  uint32_t proto = 0;
  if (aProto.find("WPA") != std::string::npos) {
    proto |= protocol_wpa;
  }

  if (aProto.find("RSN") != std::string::npos) {
    proto |= protocol_rsn;
  }

  if (aProto.find("OSEN") != std::string::npos) {
    proto |= protocol_osen;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setProto, SupplicantStatus, response, proto);
  WIFI_LOGD(LOG_TAG, "set proto return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetAuthAlg(
    const std::string& aAuthAlg) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "authAlg => %s", aAuthAlg.c_str());

  uint32_t authAlg = 0;
  if (aAuthAlg.find("OPEN") != std::string::npos) {
    authAlg |= auth_alg_open;
  }

  if (aAuthAlg.find("SHARED") != std::string::npos) {
    authAlg |= auth_alg_shared;
  }

  if (aAuthAlg.find("LEAP") != std::string::npos) {
    authAlg |= auth_alg_leap;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setAuthAlg, SupplicantStatus, response, authAlg);
  WIFI_LOGD(LOG_TAG, "set auth alg return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetGroupCipher(
    const std::string& aGroupCipher) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "groupCipher => %s", aGroupCipher.c_str());

  uint32_t groupCipher = 0;
  if (aGroupCipher.find("WEP40") != std::string::npos) {
    groupCipher |= group_cipher_wep40;
  }

  if (aGroupCipher.find("WEP104") != std::string::npos) {
    groupCipher |= group_cipher_wep104;
  }

  if (aGroupCipher.find("TKIP") != std::string::npos) {
    groupCipher |= group_cipher_tkip;
  }

  if (aGroupCipher.find("CCMP") != std::string::npos) {
    groupCipher |= group_cipher_ccmp;
  }

  if (aGroupCipher.find("GTK_NOT_USED") != std::string::npos) {
    groupCipher |= group_cipher_gtk_not_used;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setGroupCipher, SupplicantStatus, response, groupCipher);
  WIFI_LOGD(LOG_TAG, "set groupCipher return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetPairwiseCipher(
    const std::string& aPairwiseCipher) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "pairwiseCipher => %s", aPairwiseCipher.c_str());

  uint32_t pairwiseCipher = 0;
  if (aPairwiseCipher.find("NONE") != std::string::npos) {
    pairwiseCipher |= pairwise_cipher_none;
  }

  if (aPairwiseCipher.find("TKIP") != std::string::npos) {
    pairwiseCipher |= pairwise_cipher_tkip;
  }

  if (aPairwiseCipher.find("CCMP") != std::string::npos) {
    pairwiseCipher |= pairwise_cipher_ccmp;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setPairwiseCipher, SupplicantStatus, response,
           pairwiseCipher);
  WIFI_LOGD(LOG_TAG, "set pairwiseCipher return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapConfiguration(
    const NetworkConfiguration& aConfig) {
  SupplicantStatusCode stateCode = SupplicantStatusCode::FAILURE_UNKNOWN;

  // eap method
  if (!aConfig.mEap.empty()) {
    stateCode = SetEapMethod(aConfig.mEap);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // eap phase2
  if (!aConfig.mPhase2.empty()) {
    stateCode = SetEapPhase2(aConfig.mPhase2);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // identity
  if (!aConfig.mIdentity.empty()) {
    stateCode = SetEapIdentity(aConfig.mIdentity);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // anonymous identity
  if (!aConfig.mAnonymousId.empty()) {
    stateCode = SetEapAnonymousId(aConfig.mAnonymousId);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // password
  if (!aConfig.mPassword.empty()) {
    stateCode = SetEapPassword(aConfig.mPassword);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // client certificate
  if (!aConfig.mClientCert.empty()) {
    stateCode = SetEapClientCert(aConfig.mClientCert);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // CA certificate
  if (!aConfig.mCaCert.empty()) {
    stateCode = SetEapCaCert(aConfig.mCaCert);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // CA path
  if (!aConfig.mCaPath.empty()) {
    stateCode = SetEapCaPath(aConfig.mCaPath);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // subject match
  if (!aConfig.mSubjectMatch.empty()) {
    stateCode = SetEapSubjectMatch(aConfig.mSubjectMatch);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // engine Id
  if (!aConfig.mEngineId.empty()) {
    stateCode = SetEapEngineId(aConfig.mEngineId);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // engine
  stateCode = SetEapEngine(aConfig.mEngine);
  if (stateCode != SupplicantStatusCode::SUCCESS) {
    return stateCode;
  }

  // private key Id
  if (!aConfig.mPrivateKeyId.empty()) {
    stateCode = SetEapPrivateKeyId(aConfig.mPrivateKeyId);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // Alt subject match
  if (!aConfig.mAltSubjectMatch.empty()) {
    stateCode = SetEapAltSubjectMatch(aConfig.mAltSubjectMatch);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // domain suffix match
  if (!aConfig.mDomainSuffixMatch.empty()) {
    stateCode = SetEapDomainSuffixMatch(aConfig.mDomainSuffixMatch);
    if (stateCode != SupplicantStatusCode::SUCCESS) {
      return stateCode;
    }
  }

  // engine
  stateCode = SetEapProactiveKeyCaching(aConfig.mProactiveKeyCaching);
  if (stateCode != SupplicantStatusCode::SUCCESS) {
    return stateCode;
  }

  return SupplicantStatusCode::SUCCESS;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapMethod(
    const std::string& aEapMethod) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap => %s", aEapMethod.c_str());

  ISupplicantStaNetwork::EapMethod eapMethod;
  if (aEapMethod.find("PEAP") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::PEAP;
  } else if (aEapMethod.find("TLS") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::TLS;
  } else if (aEapMethod.find("TTLS") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::TTLS;
  } else if (aEapMethod.find("PWD") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::PWD;
  } else if (aEapMethod.find("SIM") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::SIM;
  } else if (aEapMethod.find("AKA") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::AKA;
  } else if (aEapMethod.find("AKA_PRIME") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::AKA_PRIME;
  } else if (aEapMethod.find("WFA_UNAUTH_TLS") != std::string::npos) {
    eapMethod = ISupplicantStaNetwork::EapMethod::WFA_UNAUTH_TLS;
  } else {
    return SupplicantStatusCode::FAILURE_ARGS_INVALID;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapMethod, SupplicantStatus, response, eapMethod);
  WIFI_LOGD(LOG_TAG, "set eap method return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapPhase2(
    const std::string& aPhase2) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap phase2 => %s", aPhase2.c_str());

  ISupplicantStaNetwork::EapPhase2Method eapPhase2;
  if (aPhase2.find("NONE") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::NONE;
  } else if (aPhase2.find("PAP") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::PAP;
  } else if (aPhase2.find("MSCHAP") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::MSPAP;
  } else if (aPhase2.find("MSCHAPV2") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::MSPAPV2;
  } else if (aPhase2.find("GTC") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::GTC;
  } else if (aPhase2.find("SIM") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::SIM;
  } else if (aPhase2.find("AKA") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::AKA;
  } else if (aPhase2.find("AKA_PRIME") != std::string::npos) {
    eapPhase2 = ISupplicantStaNetwork::EapPhase2Method::AKA_PRIME;
  } else {
    return SupplicantStatusCode::FAILURE_ARGS_INVALID;
  }

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapPhase2Method, SupplicantStatus, response, eapPhase2);
  WIFI_LOGD(LOG_TAG, "set eap phase2 return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapIdentity(
    const std::string& aIdentity) {
  MOZ_ASSERT(mNetwork);
  std::string identityStr(aIdentity);
  Dequote(identityStr);
  WIFI_LOGD(LOG_TAG, "eap identity => %s", identityStr.c_str());

  std::vector<uint8_t> identity(identityStr.begin(), identityStr.end());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapIdentity, SupplicantStatus, response, identity);
  WIFI_LOGD(LOG_TAG, "set eap identity return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapAnonymousId(
    const std::string& aAnonymousId) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap anonymousId => %s", aAnonymousId.c_str());

  std::vector<uint8_t> anonymousId(aAnonymousId.begin(), aAnonymousId.end());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapAnonymousIdentity, SupplicantStatus, response,
           anonymousId);
  WIFI_LOGD(LOG_TAG, "set eap anonymousId return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapPassword(
    const std::string& aPassword) {
  MOZ_ASSERT(mNetwork);
  std::string passwordStr(aPassword);
  Dequote(passwordStr);
  WIFI_LOGD(LOG_TAG, "eap password => %s", passwordStr.c_str());

  std::vector<uint8_t> password(passwordStr.begin(), passwordStr.end());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapPassword, SupplicantStatus, response, password);
  WIFI_LOGD(LOG_TAG, "set eap password return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapClientCert(
    const std::string& aClientCert) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap client certificate => %s", aClientCert.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapClientCert, SupplicantStatus, response, aClientCert);
  WIFI_LOGD(LOG_TAG, "set eap client certificate return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapCaCert(
    const std::string& aCaCert) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap CA certificate => %s", aCaCert.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapCACert, SupplicantStatus, response, aCaCert);
  WIFI_LOGD(LOG_TAG, "set eap CA certificate return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapCaPath(
    const std::string& aCaPath) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap CA path => %s", aCaPath.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapCAPath, SupplicantStatus, response, aCaPath);
  WIFI_LOGD(LOG_TAG, "set eap CA path return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapSubjectMatch(
    const std::string& aSubjectMatch) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap subject match => %s", aSubjectMatch.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapSubjectMatch, SupplicantStatus, response,
           aSubjectMatch);
  WIFI_LOGD(LOG_TAG, "set eap subject match return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapEngineId(
    const std::string& aEngineId) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap engine id => %s", aEngineId.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapEngineID, SupplicantStatus, response, aEngineId);
  WIFI_LOGD(LOG_TAG, "set eap engine id return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapEngine(bool aEngine) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap engine => %d", aEngine);

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapEngine, SupplicantStatus, response, aEngine);
  WIFI_LOGD(LOG_TAG, "set eap engine return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapPrivateKeyId(
    const std::string& aPrivateKeyId) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap private key Id => %s", aPrivateKeyId.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapPrivateKeyId, SupplicantStatus, response,
           aPrivateKeyId);
  WIFI_LOGD(LOG_TAG, "set eap private key Id return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapAltSubjectMatch(
    const std::string& aAltSubjectMatch) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap Alt subject match => %s", aAltSubjectMatch.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapAltSubjectMatch, SupplicantStatus, response,
           aAltSubjectMatch);
  WIFI_LOGD(LOG_TAG, "set eap Alt subject match return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapDomainSuffixMatch(
    const std::string& aDomainSuffixMatch) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "eap domain suffix match => %s",
            aDomainSuffixMatch.c_str());

  SupplicantStatus response;
  HIDL_SET(mNetwork, setEapDomainSuffixMatch, SupplicantStatus, response,
           aDomainSuffixMatch);
  WIFI_LOGD(LOG_TAG, "set eap domain suffix match return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::SetEapProactiveKeyCaching(
    bool aProactiveKeyCaching) {
  MOZ_ASSERT(mNetwork);
  WIFI_LOGD(LOG_TAG, "proactive key caching => %d", aProactiveKeyCaching);

  SupplicantStatus response;
  HIDL_SET(mNetwork, setProactiveKeyCaching, SupplicantStatus, response,
           aProactiveKeyCaching);
  WIFI_LOGD(LOG_TAG, "set proactive key caching return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::GetSsid(std::string& aSsid) const {
  MOZ_ASSERT(mNetwork);

  SupplicantStatus response;
  mNetwork->getSsid([&](const SupplicantStatus& status,
                        const ::android::hardware::hidl_vec<uint8_t>& ssid) {
    response = status;
    if (response.code == SupplicantStatusCode::SUCCESS) {
      std::string ssidStr(ssid.begin(), ssid.end());
      aSsid = ssidStr.empty() ? "" : ssidStr;
    }
  });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::GetBssid(std::string& aBssid) const {
  MOZ_ASSERT(mNetwork);

  SupplicantStatus response;
  mNetwork->getBssid(
      [&](const SupplicantStatus& status,
          const ::android::hardware::hidl_array<uint8_t, 6>& bssid) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          std::string bssidStr = ConvertMacToString(bssid);
          aBssid = bssidStr.empty() ? "" : bssidStr;
        }
      });
  return response.code;
}

SupplicantStatusCode SupplicantStaNetwork::GetKeyMgmt(
    uint32_t& aKeyMgmtMask) const {
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

SupplicantStatusCode SupplicantStaNetwork::RegisterNetworkCallback() {
  SupplicantStatus response;
  HIDL_SET(mNetwork, registerCallback, SupplicantStatus, response, this);
  WIFI_LOGD(LOG_TAG, "register network callback return: %s",
            ConvertStatusToString(response.code).c_str());
  return response.code;
}

/**
 * Internal helper functions.
 */
uint32_t SupplicantStaNetwork::ConvertKeyMgmtToMask(
    const std::string& aKeyMgmt) {
  uint32_t mask;
  if (aKeyMgmt.compare("NONE") == 0) {
    mask = key_mgmt_none;
  } else if (aKeyMgmt.compare("WPA-PSK") == 0 ||
             aKeyMgmt.compare("WPA2-PSK") == 0) {
    mask = key_mgmt_wpa_psk;
  } else if (aKeyMgmt.find("WPA-EAP") != std::string::npos) {
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

std::string SupplicantStaNetwork::ConvertMaskToKeyMgmt(uint32_t aMask) {
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

std::string SupplicantStaNetwork::ConvertStatusToString(
    const SupplicantStatusCode& aCode) {
  switch (aCode) {
    case SupplicantStatusCode::SUCCESS:
      return "SUCCESS";
    case SupplicantStatusCode::FAILURE_UNKNOWN:
      return "FAILURE_UNKNOWN";
    case SupplicantStatusCode::FAILURE_ARGS_INVALID:
      return "FAILURE_ARGS_INVALID";
    case SupplicantStatusCode::FAILURE_IFACE_INVALID:
      return "FAILURE_IFACE_INVALID";
    case SupplicantStatusCode::FAILURE_IFACE_UNKNOWN:
      return "FAILURE_IFACE_UNKNOWN";
    case SupplicantStatusCode::FAILURE_IFACE_EXISTS:
      return "FAILURE_IFACE_EXISTS";
    case SupplicantStatusCode::FAILURE_IFACE_DISABLED:
      return "FAILURE_IFACE_DISABLED";
    case SupplicantStatusCode::FAILURE_IFACE_NOT_DISCONNECTED:
      return "FAILURE_IFACE_NOT_DISCONNECTED";
    case SupplicantStatusCode::FAILURE_NETWORK_INVALID:
      return "FAILURE_NETWORK_INVALID";
    case SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN:
      return "FAILURE_NETWORK_UNKNOWN";
  }
  return "FAILURE_UNKNOWN";
}

Result_t SupplicantStaNetwork::ConvertStatusToResult(
    const SupplicantStatusCode& aCode) {
  switch (aCode) {
    case SupplicantStatusCode::SUCCESS:
      return nsIWifiResult::SUCCESS;
    case SupplicantStatusCode::FAILURE_ARGS_INVALID:
      return nsIWifiResult::ERROR_INVALID_ARGS;
    case SupplicantStatusCode::FAILURE_IFACE_INVALID:
      return nsIWifiResult::ERROR_INVALID_INTERFACE;
    case SupplicantStatusCode::FAILURE_IFACE_EXISTS:
    case SupplicantStatusCode::FAILURE_IFACE_DISABLED:
    case SupplicantStatusCode::FAILURE_IFACE_NOT_DISCONNECTED:
    case SupplicantStatusCode::FAILURE_NETWORK_INVALID:
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    case SupplicantStatusCode::FAILURE_UNKNOWN:
    case SupplicantStatusCode::FAILURE_IFACE_UNKNOWN:
    case SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN:
    default:
      return nsIWifiResult::ERROR_UNKNOWN;
  }
}

void SupplicantStaNetwork::NotifyEapSimGsmAuthRequest(
    const RequestGsmAuthParams& aParams) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_EAP_SIM_GSM_AUTH_REQUEST));

  nsTArray<nsString> rands(aParams.rands.size());
  for (const auto& element : aParams.rands) {
    std::stringstream stream;
    stream << std::setw(2) << std::setfill('0') << std::hex;

    // each element is set as uint8_t[16]
    for (size_t i = 0; i < 16; i++) {
      stream << (uint32_t)element[i];
    }

    nsString gsmRand(NS_ConvertUTF8toUTF16(stream.str().c_str()));
    rands.AppendElement(gsmRand);
  }
  event->updateGsmRands(rands);

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaNetwork::NotifyEapSimUmtsAuthRequest(
    const RequestUmtsAuthParams& aParams) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_EAP_SIM_UMTS_AUTH_REQUEST));

  std::stringstream randStream, autnStream;
  for (size_t i = 0; i < 16; i++) {
    randStream << std::setw(2) << std::setfill('0') << std::hex;
    randStream << (uint32_t)aParams.rand[i];
    autnStream << std::setw(2) << std::setfill('0') << std::hex;
    autnStream << (uint32_t)aParams.autn[i];
  }

  event->mRand = NS_ConvertUTF8toUTF16(randStream.str().c_str());
  event->mAutn = NS_ConvertUTF8toUTF16(autnStream.str().c_str());

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaNetwork::NotifyEapIdentityRequest() {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_EAP_SIM_IDENTITY_REQUEST));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

/**
 * ISupplicantStaNetworkCallback implementation
 */
Return<void> SupplicantStaNetwork::onNetworkEapSimGsmAuthRequest(
    const ISupplicantStaNetworkCallback::NetworkRequestEapSimGsmAuthParams&
        params) {
  MutexAutoLock lock(sLock);
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapSimGsmAuthRequest()");

  if (params.rands.size() > 0) {
    NotifyEapSimGsmAuthRequest(params);
  }
  return android::hardware::Void();
}

Return<void> SupplicantStaNetwork::onNetworkEapSimUmtsAuthRequest(
    const ISupplicantStaNetworkCallback::NetworkRequestEapSimUmtsAuthParams&
        params) {
  MutexAutoLock lock(sLock);
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapSimUmtsAuthRequest()");

  NotifyEapSimUmtsAuthRequest(params);
  return android::hardware::Void();
}

Return<void> SupplicantStaNetwork::onNetworkEapIdentityRequest() {
  MutexAutoLock lock(sLock);
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaNetworkCallback.onNetworkEapIdentityRequest()");

  NotifyEapIdentityRequest();
  return android::hardware::Void();
}
