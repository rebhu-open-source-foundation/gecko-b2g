/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "AutoMounter.h"
#include "AutoMounterSetting.h"

#include "base/message_loop.h"
#include "jsapi.h"
#include "mozilla/Services.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsIObserverService.h"
#include "nsJSUtils.h"
#include "nsPrintfCString.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "xpcpublic.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/SVGContentUtils.h"

#undef LOG
#undef ERR
#define LOG(args...) \
  __android_log_print(ANDROID_LOG_INFO, "AutoMounterSetting", ##args)
#define ERR(args...) \
  __android_log_print(ANDROID_LOG_ERROR, "AutoMounterSetting", ##args)

static const auto kSettingUmsMode = u"ums.mode"_ns;
static const auto kSettingUmsStatus = u"ums.status"_ns;
static const auto kSettingUmsVolumeSdcardEnabled =
    u"ums.volume.sdcard.enabled"_ns;
static const auto kSettingUmsVolumeSdcard1Enabled =
    u"ums.volume.sdcard1.enabled"_ns;
static const auto kSettingUmsVolumeExtsdcardEnabled =
    u"ums.volume.extsdcard.enabled"_ns;
static const auto kSettingUmsVolumeExternalEnabled =
    u"ums.volume.external.enabled"_ns;

#define UMS_MODE "ums.mode"
#define UMS_STATUS "ums.status"
#define UMS_VOLUME_ENABLED_PREFIX "ums.volume."
#define UMS_VOLUME_ENABLED_SUFFIX ".enabled"

using namespace mozilla::dom;

namespace mozilla {
namespace system {
class SettingInfo final : public nsISettingInfo {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISETTINGINFO

  explicit SettingInfo(nsAString& aName, nsAString& aValue);

 protected:
  ~SettingInfo() = default;

 private:
  nsString mName;
  nsString mValue;
};

NS_IMPL_ISUPPORTS(SettingInfo, nsISettingInfo)
SettingInfo::SettingInfo(nsAString& aName, nsAString& aValue)
    : mName(aName), mValue(aValue) {}

NS_IMETHODIMP
SettingInfo::GetName(nsAString& aName) {
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::SetName(const nsAString& aName) {
  mName = aName;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::GetValue(nsAString& aValue) {
  aValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::SetValue(const nsAString& aValue) {
  mValue = aValue;
  return NS_OK;
}

class SettingsServiceCallback final : public nsISidlDefaultResponse {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIDLDEFAULTRESPONSE

 protected:
  ~SettingsServiceCallback() = default;
};

NS_IMPL_ISUPPORTS(SettingsServiceCallback, nsISidlDefaultResponse)

NS_IMETHODIMP
SettingsServiceCallback::Resolve() { return NS_OK; }

NS_IMETHODIMP
SettingsServiceCallback::Reject() { return NS_ERROR_FAILURE; }

class CheckVolumeSettingsCallback final : public nsISettingsGetResponse {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISETTINGSGETRESPONSE

  CheckVolumeSettingsCallback(const nsACString& aVolumeName)
      : mVolumeName(aVolumeName) {}

 protected:
  ~CheckVolumeSettingsCallback() = default;

 private:
  nsCString mVolumeName;
};

NS_IMPL_ISUPPORTS(CheckVolumeSettingsCallback, nsISettingsGetResponse)

NS_IMETHODIMP
CheckVolumeSettingsCallback::Resolve(nsISettingInfo* info) {
  MOZ_ASSERT(NS_IsMainThread());
  if (!info) {
    return NS_OK;
  }
  nsString name;
  info->GetName(name);

  nsString value;
  info->GetValue(value);

  bool isSharingEnabled = value.EqualsLiteral("true");
  SetAutoMounterSharingMode(mVolumeName, isSharingEnabled);
  return NS_OK;
}

NS_IMETHODIMP
CheckVolumeSettingsCallback::Reject(nsISettingError* error) {
  if (error) {
    nsString name;
    unsigned short reason;
    error->GetName(name);
    error->GetReason(&reason);
    ERR("CheckVolumeSettingsCallback failed to read %s, reason: %s",
        NS_ConvertUTF16toUTF8(name).get(),
        (reason == nsISettingError::UNKNOWN_ERROR) ? "UNKNOWN_ERROR"
                                                   : "NON_EXISTING_SETTING");
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(AutoMounterSetting, nsISettingsGetResponse,
                  nsISettingsObserver, nsISidlDefaultResponse)

AutoMounterSetting::AutoMounterSetting()
    : mStatus(AUTOMOUNTER_STATUS_DISABLED) {
  MOZ_ASSERT(NS_IsMainThread());
  // Force ums.mode to be 0 initially. We do this because settings are
  // persisted. We don't want UMS to be enabled until such time as the phone is
  // unlocked, and gaia/apps/system/js/storage.js takes care of detecting when
  // the phone becomes unlocked and changes ums.mode appropriately.
  nsCOMPtr<nsISettingsManager> settingsManager =
      do_GetService("@mozilla.org/sidl-native/settings;1");
  if (settingsManager) {
    settingsManager->Get(kSettingUmsMode, this);
    settingsManager->AddObserver(kSettingUmsMode, this, this);
    settingsManager->Get(kSettingUmsVolumeSdcardEnabled, this);
    settingsManager->AddObserver(kSettingUmsVolumeSdcardEnabled, this, this);
    settingsManager->Get(kSettingUmsVolumeSdcard1Enabled, this);
    settingsManager->AddObserver(kSettingUmsVolumeSdcard1Enabled, this, this);
    settingsManager->Get(kSettingUmsVolumeExtsdcardEnabled, this);
    settingsManager->AddObserver(kSettingUmsVolumeExtsdcardEnabled, this, this);
    settingsManager->Get(kSettingUmsVolumeExternalEnabled, this);
    settingsManager->AddObserver(kSettingUmsVolumeExternalEnabled, this, this);
  } else {
    ERR("Failed to get settingmanager service!");
  }

  nsTArray<RefPtr<nsISettingInfo>> settings;
  RefPtr<SettingsServiceCallback> callback = new SettingsServiceCallback();
  NS_ConvertASCIItoUTF16 settingValue(
      std::to_string(AUTOMOUNTER_DISABLE).c_str());
  NS_ConvertASCIItoUTF16 settingNameUmsMode(UMS_MODE);
  NS_ConvertASCIItoUTF16 settingNameUmsStatus(UMS_STATUS);
  RefPtr<nsISettingInfo> settingInfoUmsMode =
      new SettingInfo(settingNameUmsMode, settingValue);
  RefPtr<nsISettingInfo> settingInfoUmsStatus =
      new SettingInfo(settingNameUmsStatus, settingValue);
  settings.AppendElement(settingInfoUmsMode);
  settings.AppendElement(settingInfoUmsStatus);

  if (settingsManager) {
    RefPtr<SettingsServiceCallback> callback = new SettingsServiceCallback();
    settingsManager->Set(settings, callback);
  } else {
    ERR("Failed to get settingmanager service to set the mtp");
  }
}

AutoMounterSetting::~AutoMounterSetting() {
  nsCOMPtr<nsISettingsManager> settingsManager =
      do_GetService("@mozilla.org/sidl-native/settings;1");
  if (settingsManager) {
    settingsManager->RemoveObserver(kSettingUmsMode, this, this);
    settingsManager->RemoveObserver(kSettingUmsVolumeSdcardEnabled, this, this);
    settingsManager->RemoveObserver(kSettingUmsVolumeSdcard1Enabled, this,
                                    this);
    settingsManager->RemoveObserver(kSettingUmsVolumeExtsdcardEnabled, this,
                                    this);
    settingsManager->RemoveObserver(kSettingUmsVolumeExternalEnabled, this,
                                    this);
  } else {
    ERR("Failed to get settingmanager to remove the observer");
  }
}

const char* AutoMounterSetting::StatusStr(int32_t aStatus) {
  switch (aStatus) {
    case AUTOMOUNTER_STATUS_DISABLED:
      return "Disabled";
    case AUTOMOUNTER_STATUS_ENABLED:
      return "Enabled";
    case AUTOMOUNTER_STATUS_FILES_OPEN:
      return "FilesOpen";
  }
  return "??? Unknown ???";
}

class CheckVolumeSettingsRunnable : public Runnable {
 public:
  explicit CheckVolumeSettingsRunnable(const nsACString& aVolumeName)
      : Runnable("AutoMounter::CheckVolumeSettings"),
        mVolumeName(aVolumeName) {}

  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    nsCOMPtr<nsISettingsManager> settingsManager =
        do_GetService("@mozilla.org/sidl-native/settings;1");

    nsPrintfCString setting(UMS_VOLUME_ENABLED_PREFIX
                            "%s" UMS_VOLUME_ENABLED_SUFFIX,
                            mVolumeName.get());
    NS_ConvertASCIItoUTF16 settingInfo(setting.get());

    if (settingsManager) {
      RefPtr<CheckVolumeSettingsCallback> callback =
          new CheckVolumeSettingsCallback(mVolumeName);
      settingsManager->Get(settingInfo, callback);
    } else {
      ERR("Failed to get settingmanager service to CheckVolumeSettings");
    }
    return NS_OK;
  }

 private:
  nsCString mVolumeName;
};

// static
void AutoMounterSetting::CheckVolumeSettings(const nsACString& aVolumeName) {
  NS_DispatchToMainThread(new CheckVolumeSettingsRunnable(aVolumeName));
}

class SetStatusRunnable : public Runnable {
 public:
  explicit SetStatusRunnable(int32_t aStatus)
      : Runnable("AutoMounter::SetStatus"), mStatus(aStatus) {}

  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    nsCOMPtr<nsISettingsManager> settingsManager =
        do_GetService("@mozilla.org/sidl-native/settings;1");

    nsTArray<RefPtr<nsISettingInfo>> settings;
    RefPtr<SettingsServiceCallback> callback = new SettingsServiceCallback();
    NS_ConvertASCIItoUTF16 settingValue(std::to_string(mStatus).c_str());
    NS_ConvertASCIItoUTF16 settingName(UMS_STATUS);
    RefPtr<nsISettingInfo> settingInfo =
        new SettingInfo(settingName, settingValue);
    settings.AppendElement(settingInfo);

    if (settingsManager) {
      RefPtr<SettingsServiceCallback> callback = new SettingsServiceCallback();
      settingsManager->Set(settings, callback);
    } else {
      ERR("Failed to get settingmanager service to set status");
    }

    return NS_OK;
  }

 private:
  int32_t mStatus;
};

// static
void AutoMounterSetting::SetStatus(int32_t aStatus) {
  if (aStatus != mStatus) {
    LOG("Changing status from '%s' to '%s'", StatusStr(mStatus),
        StatusStr(aStatus));
    mStatus = aStatus;
    NS_DispatchToMainThread(new SetStatusRunnable(aStatus));
  }
}

// Implements nsISettingsGetResponse::Resolve
NS_IMETHODIMP AutoMounterSetting::Resolve(nsISettingInfo* info) {
  return NS_OK;
}

// Implements nsISettingsGetResponse::Reject
NS_IMETHODIMP AutoMounterSetting::Reject(nsISettingError* error) {
  if (error) {
    nsString name;
    unsigned short reason;
    error->GetName(name);
    error->GetReason(&reason);
    ERR("Failed to read %s, reason: %s", NS_ConvertUTF16toUTF8(name).get(),
        (reason == nsISettingError::UNKNOWN_ERROR) ? "UNKNOWN_ERROR"
                                                   : "NON_EXISTING_SETTING");
  }
  return NS_OK;
}

// Implements nsISettingsObserver::ObserveSetting
NS_IMETHODIMP AutoMounterSetting::ObserveSetting(nsISettingInfo* info) {
  MOZ_ASSERT(NS_IsMainThread());
  if (!info) {
    return NS_OK;
  }
  nsString name;
  info->GetName(name);
  nsString value;
  info->GetValue(value);
  LOG("Success to read %s, value is %s", NS_ConvertUTF16toUTF8(name).get(),
      NS_ConvertUTF16toUTF8(value).get());
  if (name.Equals(kSettingUmsMode)) {
    int32_t mode = AUTOMOUNTER_DISABLE;
    if (SVGContentUtils::ParseInteger(value, mode) == false) {
      ERR("ums.mode is not a number!");
      return NS_ERROR_UNEXPECTED;
    }
    SetAutoMounterMode(mode);
  }

  // Check for ums.volume.NAME.enabled
  if (StringBeginsWith(
          name, NS_LITERAL_STRING_FROM_CSTRING(UMS_VOLUME_ENABLED_PREFIX)) &&
      StringEndsWith(
          name, NS_LITERAL_STRING_FROM_CSTRING(UMS_VOLUME_ENABLED_SUFFIX))) {
    bool isSharingEnabled = value.EqualsLiteral("true");
    const size_t prefixLen = sizeof(UMS_VOLUME_ENABLED_PREFIX) - 1;
    const size_t suffixLen = sizeof(UMS_VOLUME_ENABLED_SUFFIX) - 1;
    nsDependentSubstring volumeName =
        Substring(name, prefixLen, name.Length() - prefixLen - suffixLen);
    SetAutoMounterSharingMode(NS_LossyConvertUTF16toASCII(volumeName),
                              isSharingEnabled);
  }

  return NS_OK;
}

// Implements nsISidlDefaultResponse
NS_IMETHODIMP AutoMounterSetting::Resolve() { return NS_OK; }
NS_IMETHODIMP AutoMounterSetting::Reject() { return NS_OK; }

}  // namespace system
}  // namespace mozilla
