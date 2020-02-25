/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
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

typedef void (*EventCallback)(nsWifiEvent* aEvent,
                              const nsACString& aInterface);

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
    COPY_OPT_FIELD(mPsk, EmptyString())
    COPY_OPT_FIELD(mWepKey, EmptyString())
    COPY_OPT_FIELD(mWepTxKeyIndex, 0)
    COPY_OPT_FIELD(mScanSsid, false)
    COPY_OPT_FIELD(mPmf, false)
    COPY_OPT_FIELD(mKeyManagement, 0)
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
  nsString mPsk;
  nsString mWepKey;
  int32_t  mWepTxKeyIndex;
  bool     mScanSsid;
  bool     mPmf;
  int32_t  mKeyManagement;
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
    COPY_OPT_FIELD(mKeyManagement, 0)
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
  uint32_t mKeyManagement;
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

  ScanSettingsOptions(const mozilla::dom::ScanSettings& aOther){
  COPY_OPT_FIELD(mScanType, 2)
  COPY_SEQUENCE_FIELD(mChannels, int32_t)
  COPY_SEQUENCE_FIELD(mHiddenNetworks, nsString)}

  uint32_t mScanType;
  nsTArray<int32_t> mChannels;
  nsTArray<nsString> mHiddenNetworks;
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
    ConfigurationOptions config(aOther.mConfig);
    SoftapConfigurationOptions softapConfig(aOther.mSoftapConfig);
    SupplicantDebugLevelOptions debugLevel(aOther.mDebugLevel);
    ScanSettingsOptions scanSettings(aOther.mScanSettings);
    mConfig = config;
    mSoftapConfig = softapConfig;
    mDebugLevel = debugLevel;
    mScanSettings = scanSettings;
  }

  // All the fields, not Optional<> anymore to get copy constructors.
  uint32_t mId;
  uint32_t mCmd;

  bool mEnabled;
  nsString mCountryCode;
  nsString mSoftapCountryCode;
  uint8_t mBtCoexistenceMode;
  uint32_t mBandMask;
  ConfigurationOptions mConfig;
  SoftapConfigurationOptions mSoftapConfig;
  SupplicantDebugLevelOptions mDebugLevel;
  ScanSettingsOptions mScanSettings;
};

#undef COPY_OPT_FIELD
#undef COPY_FIELD

template <typename T>
std::string ConvertMacToString(const T& mac);

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

#endif /* WifiCommon_H */
