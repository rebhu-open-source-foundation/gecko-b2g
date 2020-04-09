/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WifiCommon_H
#define WifiCommon_H

#include <android-base/macros.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include "mozilla/dom/WifiOptionsBinding.h"
#include "hidl/HidlSupport.h"

#if defined(MOZ_WIDGET_GONK)
#  include <utils/Log.h>
#  define WIFI_DEBUG 1

#  if WIFI_DEBUG
#    define WIFI_LOGD(tag, args...) \
      __android_log_print(ANDROID_LOG_DEBUG, tag, ##args)
#    define WIFI_LOGI(tag, args...) \
      __android_log_print(ANDROID_LOG_INFO, tag, ##args)
#    define WIFI_LOGW(tag, args...) \
      __android_log_print(ANDROID_LOG_WARN, tag, ##args)
#    define WIFI_LOGE(tag, args...) \
      __android_log_print(ANDROID_LOG_ERROR, tag, ##args)
#  else
#    define WIFI_LOGD(tag, args...)
#    define WIFI_LOGI(tag, args...)
#    define WIFI_LOGW(tag, args...)
#    define WIFI_LOGE(tag, args...) \
      __android_log_print(ANDROID_LOG_ERROR, tag, ##args)
#  endif /* WIFI_DEBUG */

#else

#  define WIFI_LOGD(tag, args...)
#  define WIFI_LOGI(tag, args...)
#  define WIFI_LOGW(tag, args...)
#  define WIFI_LOGE(tag, args...)

#endif /* MOZ_WIDGET_GONK */

typedef uint32_t Result_t;

#define COPY_SEQUENCE_FIELD(prop, type)                \
  if (aOther.prop.WasPassed()) {                       \
    mozilla::dom::Sequence<type> const& currentValue = \
        aOther.prop.InternalValue();                   \
    uint32_t length = currentValue.Length();           \
    for (uint32_t idx = 0; idx < length; idx++) {      \
      prop.AppendElement(currentValue[idx]);           \
    }                                                  \
  }

#define COPY_OPT_FIELD(prop, defaultValue) \
  if (aOther.prop.WasPassed()) {           \
    prop = aOther.prop.Value();            \
  } else {                                 \
    prop = defaultValue;                   \
  }

#define COPY_FIELD(prop) prop = aOther.prop;

struct ConfigurationOptions {
 public:
  ConfigurationOptions() = default;

  ConfigurationOptions(const mozilla::dom::WifiConfiguration& aOther) {
    COPY_OPT_FIELD(mSsid, EmptyString())
    COPY_OPT_FIELD(mBssid, EmptyString())
    COPY_OPT_FIELD(mKeyMgmt, EmptyString())
    COPY_OPT_FIELD(mPsk, EmptyString())
    COPY_OPT_FIELD(mWepKey0, EmptyString())
    COPY_OPT_FIELD(mWepKey1, EmptyString())
    COPY_OPT_FIELD(mWepKey2, EmptyString())
    COPY_OPT_FIELD(mWepKey3, EmptyString())
    COPY_OPT_FIELD(mWepTxKeyIndex, 0)
    COPY_OPT_FIELD(mScanSsid, false)
    COPY_OPT_FIELD(mPmf, false)
    COPY_OPT_FIELD(mProto, 0)
    COPY_OPT_FIELD(mAuthAlg, 0)
    COPY_OPT_FIELD(mGroupCipher, 0)
    COPY_OPT_FIELD(mPairwiseCipher, 0)
    COPY_OPT_FIELD(mEap, 0)
    COPY_OPT_FIELD(mEapPhase2, 0)
    COPY_OPT_FIELD(mIdentity, EmptyString())
    COPY_OPT_FIELD(mAnonymousId, EmptyString())
    COPY_OPT_FIELD(mPassword, EmptyString())
    COPY_OPT_FIELD(mClientCert, EmptyString())
    COPY_OPT_FIELD(mCaCert, EmptyString())
    COPY_OPT_FIELD(mCaPath, EmptyString())
    COPY_OPT_FIELD(mSubjectMatch, EmptyString())
    COPY_OPT_FIELD(mEngineId, EmptyString())
    COPY_OPT_FIELD(mEngine, false)
    COPY_OPT_FIELD(mPrivateKeyId, EmptyString())
    COPY_OPT_FIELD(mAltSubjectMatch, EmptyString())
    COPY_OPT_FIELD(mDomainSuffixMatch, EmptyString())
    COPY_OPT_FIELD(mProactiveKeyCaching, false)
  }

  nsString mSsid;
  nsString mBssid;
  nsString mKeyMgmt;
  nsString mPsk;
  nsString mWepKey0;
  nsString mWepKey1;
  nsString mWepKey2;
  nsString mWepKey3;
  int32_t  mWepTxKeyIndex;
  bool     mScanSsid;
  bool     mPmf;
  int32_t  mProto;
  int32_t  mAuthAlg;
  int32_t  mGroupCipher;
  int32_t  mPairwiseCipher;
  int32_t  mEap;
  int32_t  mEapPhase2;
  nsString mIdentity;
  nsString mAnonymousId;
  nsString mPassword;
  nsString mClientCert;
  nsString mCaCert;
  nsString mCaPath;
  nsString mSubjectMatch;
  nsString mEngineId;
  bool     mEngine;
  nsString mPrivateKeyId;
  nsString mAltSubjectMatch;
  nsString mDomainSuffixMatch;
  bool     mProactiveKeyCaching;
};

struct SoftapConfigurationOptions {
 public:
  SoftapConfigurationOptions() = default;

  SoftapConfigurationOptions(const mozilla::dom::SoftapConfiguration& aOther) {
    COPY_OPT_FIELD(mSsid, EmptyString())
    COPY_OPT_FIELD(mKeyMgmt, 0)
    COPY_OPT_FIELD(mKey, EmptyString())
    COPY_OPT_FIELD(mCountryCode, EmptyString())
    COPY_OPT_FIELD(mBand, 0)
    COPY_OPT_FIELD(mChannel, 0)
    COPY_OPT_FIELD(mHidden, false)
    COPY_OPT_FIELD(mEnable11N, false)
    COPY_OPT_FIELD(mEnable11AC, false)
    COPY_OPT_FIELD(mEnableACS, false)
    COPY_OPT_FIELD(mAcsExcludeDfs, false)
  }

  nsString mSsid;
  uint32_t mKeyMgmt;
  nsString mKey;
  nsString mCountryCode;
  uint32_t mBand;
  uint32_t mChannel;
  bool     mHidden;
  bool     mEnable11N;
  bool     mEnable11AC;
  bool     mEnableACS;
  bool     mAcsExcludeDfs;
};

struct SupplicantDebugLevelOptions {
 public:
  SupplicantDebugLevelOptions() = default;

  SupplicantDebugLevelOptions(const mozilla::dom::SupplicantDebugLevel& aOther) {
    COPY_OPT_FIELD(mLogLevel, 0)
    COPY_OPT_FIELD(mShowTimeStamp, false)
    COPY_OPT_FIELD(mShowKeys, false)
  }

  uint32_t mLogLevel;
  bool mShowTimeStamp;
  bool mShowKeys;
};

struct ScanSettingsOptions {
 public:
  ScanSettingsOptions() = default;

  ScanSettingsOptions(const mozilla::dom::ScanSettings& aOther) {
    COPY_OPT_FIELD(mScanType, 2)
    COPY_SEQUENCE_FIELD(mChannels, int32_t)
    COPY_SEQUENCE_FIELD(mHiddenNetworks, nsString)
  }

  uint32_t mScanType;
  nsTArray<int32_t> mChannels;
  nsTArray<nsString> mHiddenNetworks;
};

struct PnoNetworkOptions {
 public:
  PnoNetworkOptions() = default;

  PnoNetworkOptions(const mozilla::dom::PnoNetwork& aOther) {
    COPY_OPT_FIELD(mIsHidden, false)
    COPY_OPT_FIELD(mSsid, EmptyString())
    COPY_SEQUENCE_FIELD(mFrequencies, int32_t)
  }

  bool mIsHidden;
  nsString mSsid;
  nsTArray<int32_t> mFrequencies;
};

struct PnoScanSettingsOptions {
 public:
  PnoScanSettingsOptions() = default;

  PnoScanSettingsOptions(const mozilla::dom::PnoScanSettings& aOther) {
    COPY_OPT_FIELD(mInterval, 0)
    COPY_OPT_FIELD(mMin2gRssi, 0)
    COPY_OPT_FIELD(mMin5gRssi, 0)

    if (aOther.mPnoNetworks.WasPassed()) {
      mozilla::dom::Sequence<mozilla::dom::PnoNetwork> const& currentValue =
          aOther.mPnoNetworks.InternalValue();
      for (size_t i = 0; i < currentValue.Length(); i++) {
        PnoNetworkOptions pnoNetwork(currentValue[i]);
        mPnoNetworks.AppendElement(pnoNetwork);
      }
    }
  }

  int32_t mInterval;
  int32_t mMin2gRssi;
  int32_t mMin5gRssi;
  nsTArray<PnoNetworkOptions> mPnoNetworks;
};

// Needed to add a copy constructor to WifiCommandOptions.
struct CommandOptions {
 public:
  CommandOptions(const mozilla::dom::WifiCommandOptions& aOther) {
    COPY_FIELD(mId)
    COPY_FIELD(mCmd)
    COPY_FIELD(mEnabled)
    COPY_FIELD(mCountryCode)
    COPY_FIELD(mSoftapCountryCode)
    COPY_FIELD(mBtCoexistenceMode)
    COPY_FIELD(mBandMask)
    COPY_FIELD(mScanType)
    ConfigurationOptions config(aOther.mConfig);
    SoftapConfigurationOptions softapConfig(aOther.mSoftapConfig);
    SupplicantDebugLevelOptions debugLevel(aOther.mDebugLevel);
    ScanSettingsOptions scanSettings(aOther.mScanSettings);
    PnoScanSettingsOptions pnoScanSettings(aOther.mPnoScanSettings);
    mConfig = config;
    mSoftapConfig = softapConfig;
    mDebugLevel = debugLevel;
    mScanSettings = scanSettings;
    mPnoScanSettings = pnoScanSettings;
  }

  // All the fields, not Optional<> anymore to get copy constructors.
  uint32_t mId;
  uint32_t mCmd;

  bool mEnabled;
  nsString mCountryCode;
  nsString mSoftapCountryCode;
  uint8_t mBtCoexistenceMode;
  uint32_t mBandMask;
  uint32_t mScanType;

  ConfigurationOptions mConfig;
  SoftapConfigurationOptions mSoftapConfig;
  SupplicantDebugLevelOptions mDebugLevel;
  ScanSettingsOptions mScanSettings;
  PnoScanSettingsOptions mPnoScanSettings;
};

#undef COPY_OPT_FIELD
#undef COPY_FIELD

template <typename T>
std::string ConvertMacToString(const T& mac);

template <typename T>
int32_t ConvertMacToByteArray(const std::string& mac, T& out);

void Dequote(std::string& s);

#define HIDL_CALL(interface, method, responseType, response)       \
  do {                                                             \
    if (interface != nullptr) {                                    \
      interface->method(                                           \
          [&](const responseType& status) { response = status; }); \
    }                                                              \
  } while (0)

#define HIDL_SET(interface, method, responseType, response, ...)       \
  do {                                                                 \
    if (interface != nullptr) {                                        \
      interface->method(__VA_ARGS__, [&](const responseType& status) { \
        response = status;                                             \
      });                                                              \
    }                                                                  \
  } while (0)

#define CHECK_SUCCESS(condition) \
  (condition) ? nsIWifiResult::SUCCESS : nsIWifiResult::ERROR_COMMAND_FAILED

#endif /* WifiCommon_H */
