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
    COPY_OPT_FIELD(mKeyManagement, 0)
    COPY_OPT_FIELD(mPsk, EmptyString())
  }

  nsString mSsid;
  nsString mBssid;
  int32_t mKeyManagement;
  nsString mPsk;
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

// Needed to add a copy constructor to WifiCommandOptions.
struct CommandOptions {
 public:
  CommandOptions(const mozilla::dom::WifiCommandOptions& aOther) {
    COPY_FIELD(mId)
    COPY_FIELD(mCmd)
    COPY_FIELD(mPowerSave)
    COPY_FIELD(mSuspendMode)
    COPY_FIELD(mExternalSim)
    COPY_FIELD(mAutoReconnect)
    COPY_FIELD(mCountryCode)
    COPY_FIELD(mBtCoexistenceMode)
    COPY_FIELD(mBtCoexistenceScanMode)
    ConfigurationOptions config(aOther.mConfig);

    mConfig = config;
    SupplicantDebugLevelOptions debugLevel(aOther.mDebugLevel);
    mDebugLevel = debugLevel;
  }

  // All the fields, not Optional<> anymore to get copy constructors.
  uint32_t mId;
  uint32_t mCmd;

  bool mPowerSave;
  bool mSuspendMode;
  bool mExternalSim;
  bool mAutoReconnect;
  nsString mCountryCode;
  uint8_t mBtCoexistenceMode;
  bool mBtCoexistenceScanMode;
  ConfigurationOptions mConfig;
  SupplicantDebugLevelOptions mDebugLevel;
};

#undef COPY_OPT_FIELD
#undef COPY_FIELD

template <typename T>
std::string ConvertMacToString(const T& mac);

#define HIDL_CALL(interface, method, responseType, response, ...)      \
  do {                                                                 \
    if (interface != nullptr) {                                        \
      interface->method(__VA_ARGS__, [&](const responseType& status) { \
        response = status;                                             \
      });                                                              \
    }                                                                  \
  } while (0)

#endif /* WifiCommon_H */
