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

#define COPY_FIELD_STRING(configure, field)                \
  if (!configure->field.IsEmpty()) {                       \
    field = NS_ConvertUTF16toUTF8(configure->field).get(); \
  } else {                                                 \
    field = std::string();                                 \
  }

/**
 * Class that contains the network configurations.
 */
class NetworkConfiguration {
 public:
  NetworkConfiguration() = default;

  NetworkConfiguration(const NetworkConfiguration& aConfig) {
    mNetworkId = aConfig.mNetworkId;
    mSsid = aConfig.mSsid;
    mBssid = aConfig.mBssid;
    mKeyMgmt = aConfig.mKeyMgmt;
    mPsk = aConfig.mPsk;
    mWepKey0 = aConfig.mWepKey0;
    mWepKey1 = aConfig.mWepKey1;
    mWepKey2 = aConfig.mWepKey2;
    mWepKey3 = aConfig.mWepKey3;
    mWepTxKeyIndex = aConfig.mWepTxKeyIndex;
    mScanSsid = aConfig.mScanSsid;
    mPmf = aConfig.mPmf;
    mProto = aConfig.mProto;
    mAuthAlg = aConfig.mAuthAlg;
    mGroupCipher = aConfig.mGroupCipher;
    mPairwiseCipher = aConfig.mPairwiseCipher;
    mEap = aConfig.mEap;
    mEapPhase2 = aConfig.mEapPhase2;
    mIdentity = aConfig.mIdentity;
    mAnonymousId = aConfig.mAnonymousId;
    mPassword = aConfig.mPassword;
    mClientCert = aConfig.mClientCert;
    mCaCert = aConfig.mCaCert;
    mCaPath = aConfig.mCaPath;
    mSubjectMatch = aConfig.mSubjectMatch;
    mEngineId = aConfig.mEngineId;
    mEngine = aConfig.mEngine;
    mPrivateKeyId = aConfig.mPrivateKeyId;
    mAltSubjectMatch = aConfig.mAltSubjectMatch;
    mDomainSuffixMatch = aConfig.mDomainSuffixMatch;
    mProactiveKeyCaching = aConfig.mProactiveKeyCaching;
  }

  NetworkConfiguration(const ConfigurationOptions* aConfig) {
    COPY_FIELD_STRING(aConfig, mSsid)
    COPY_FIELD_STRING(aConfig, mBssid)
    COPY_FIELD_STRING(aConfig, mKeyMgmt)
    COPY_FIELD_STRING(aConfig, mPsk)
    COPY_FIELD_STRING(aConfig, mWepKey0)
    COPY_FIELD_STRING(aConfig, mWepKey1)
    COPY_FIELD_STRING(aConfig, mWepKey2)
    COPY_FIELD_STRING(aConfig, mWepKey3)
    COPY_FIELD_STRING(aConfig, mIdentity)
    COPY_FIELD_STRING(aConfig, mAnonymousId)
    COPY_FIELD_STRING(aConfig, mPassword)
    COPY_FIELD_STRING(aConfig, mClientCert)
    COPY_FIELD_STRING(aConfig, mCaCert)
    COPY_FIELD_STRING(aConfig, mCaPath)
    COPY_FIELD_STRING(aConfig, mSubjectMatch)
    COPY_FIELD_STRING(aConfig, mEngineId)
    COPY_FIELD_STRING(aConfig, mPrivateKeyId)
    COPY_FIELD_STRING(aConfig, mAltSubjectMatch)
    COPY_FIELD_STRING(aConfig, mDomainSuffixMatch)

    mNetworkId = aConfig->mNetId;
    mWepTxKeyIndex = aConfig->mWepTxKeyIndex;
    mScanSsid = aConfig->mScanSsid;
    mPmf = aConfig->mPmf;
    mProto = aConfig->mProto;
    mAuthAlg = aConfig->mAuthAlg;
    mGroupCipher = aConfig->mGroupCipher;
    mPairwiseCipher = aConfig->mPairwiseCipher;
    mEap = aConfig->mEap;
    mEapPhase2 = aConfig->mEapPhase2;
    mEngine = aConfig->mEngine;
    mProactiveKeyCaching = aConfig->mProactiveKeyCaching;
  }

  std::string GetNetworkKey() { return mSsid + mKeyMgmt; }

  int32_t     mNetworkId;
  std::string mSsid;
  std::string mBssid;
  std::string mKeyMgmt;
  std::string mPsk;
  std::string mWepKey0;
  std::string mWepKey1;
  std::string mWepKey2;
  std::string mWepKey3;
  int32_t     mWepTxKeyIndex;
  bool        mScanSsid;
  bool        mPmf;
  int32_t     mProto;
  int32_t     mAuthAlg;
  int32_t     mGroupCipher;
  int32_t     mPairwiseCipher;
  int32_t     mEap;
  int32_t     mEapPhase2;
  std::string mIdentity;
  std::string mAnonymousId;
  std::string mPassword;
  std::string mClientCert;
  std::string mCaCert;
  std::string mCaPath;
  std::string mSubjectMatch;
  std::string mEngineId;
  bool        mEngine;
  std::string mPrivateKeyId;
  std::string mAltSubjectMatch;
  std::string mDomainSuffixMatch;
  bool        mProactiveKeyCaching;
};

/**
 * A wrapper class that exports functions to configure each
 * ISupplicantStaNetwork.
 */
class SupplicantStaNetwork
    : virtual public android::RefBase,
      virtual public android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetworkCallback {
 public:
  explicit SupplicantStaNetwork(ISupplicantStaNetwork* aNetwork);

  Result_t SetConfiguration(const NetworkConfiguration& aConfig);
  Result_t EnableNetwork();
  Result_t DisableNetwork();
  Result_t SelectNetwork();

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
  SupplicantStatusCode SetKeyMgmt(uint32_t aKeyMgmtMask);
  SupplicantStatusCode SetPsk(const std::string& aPsk);

  SupplicantStatusCode GetSsid(std::string& aSsid);
  SupplicantStatusCode GetBssid(std::string& aBssid);
  SupplicantStatusCode GetKeyMgmt(uint32_t& aKeyMgmtMask);

  uint32_t ConvertKeyMgmtToMask(const std::string& aKeyMgmt) const;
  std::string ConvertMaskToKeyMgmt(uint32_t aMask) const;

  static mozilla::Mutex s_Lock;

  android::sp<ISupplicantStaNetwork> mNetwork;
};

#endif /* SupplicantStaNetwork_H */
