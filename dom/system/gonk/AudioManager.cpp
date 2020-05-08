/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <cutils/properties.h>
#include <binder/IServiceManager.h>

#include "AudioChannelService.h"
#include "AudioManager.h"

#include "nsIObserverService.h"
#ifdef MOZ_B2G_RIL
#  include "nsIRadioInterfaceLayer.h"
#endif
//#include "nsISettingsService.h" //TODO FIXME
#ifdef MOZ_B2G_RIL
#  include "nsITelephonyService.h"
#endif
#include "nsPrintfCString.h"

#include "mozilla/Hal.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/MozPromise.h"
#include "mozilla/dom/ScriptSettings.h"
#include "base/message_loop.h"

#include "BluetoothCommon.h"
#include "BluetoothHfpManagerBase.h"

#include "nsJSUtils.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/dom/BindingUtils.h"
//#include "mozilla/dom/SettingChangeNotificationBinding.h" //TODO: FIXME
#include "mozilla/dom/power/PowerManagerService.h"

#include "mozilla/Preferences.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/ToJSValue.h"
#include "nsXULAppAPI.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::gonk;
using namespace mozilla::dom::bluetooth;
using android::AudioSystem;

#undef LOG
#define LOG(args...) \
  __android_log_print(ANDROID_LOG_INFO, "AudioManager", ##args)

#define HEADPHONES_STATUS_HEADSET u"headset"
#define HEADPHONES_STATUS_HEADPHONE u"headphone"
#define HEADPHONES_STATUS_LINEOUT u"lineout"
#define HEADPHONES_STATUS_OFF u"off"
#define HEADPHONES_STATUS_UNKNOWN u"unknown"
#define HEADPHONES_STATUS_CHANGED "headphones-status-changed"
#define MOZ_SETTINGS_CHANGE_ID "mozsettings-changed"
#define SCREEN_STATE_CHANGED "screen-state-changed"
#define AUDIO_POLICY_SERVICE_NAME "media.audio_policy"
#define SETTINGS_SERVICE "@mozilla.org/settingsService;1"

// Refer AudioService.java from Android
static const uint32_t sMaxStreamVolumeTbl[AUDIO_STREAM_CNT] = {
    5,   // voice call
    15,  // system
    15,  // ring
    15,  // music
    15,  // alarm
    15,  // notification
    15,  // BT SCO
    15,  // enforced audible
    15,  // DTMF
    15,  // TTS
};

static const uint32_t sDefaultStreamVolumeTbl[AUDIO_STREAM_CNT] = {
    3,   // voice call
    8,   // system
    8,   // ring
    8,   // music
    8,   // alarm
    8,   // notification
    8,   // BT SCO
    15,  // enforced audible  // XXX Handle as fixed maximum audio setting
    8,   // DTMF
    8,   // TTS
};

static const int32_t sStreamVolumeAliasTbl[AUDIO_STREAM_CNT] = {
    AUDIO_STREAM_VOICE_CALL,        // voice call
    AUDIO_STREAM_NOTIFICATION,      // system
    AUDIO_STREAM_NOTIFICATION,      // ring
    AUDIO_STREAM_MUSIC,             // music
    AUDIO_STREAM_ALARM,             // alarm
    AUDIO_STREAM_NOTIFICATION,      // notification
    AUDIO_STREAM_BLUETOOTH_SCO,     // BT SCO
    AUDIO_STREAM_ENFORCED_AUDIBLE,  // enforced audible
    AUDIO_STREAM_DTMF,              // DTMF
    AUDIO_STREAM_TTS,               // TTS
};

static const uint32_t sChannelStreamTbl[NUMBER_OF_AUDIO_CHANNELS] = {
    AUDIO_STREAM_MUSIC,             // AudioChannel::Normal
    AUDIO_STREAM_MUSIC,             // AudioChannel::Content
    AUDIO_STREAM_NOTIFICATION,      // AudioChannel::Notification
    AUDIO_STREAM_ALARM,             // AudioChannel::Alarm
    AUDIO_STREAM_VOICE_CALL,        // AudioChannel::Telephony
    AUDIO_STREAM_RING,              // AudioChannel::Ringer
    AUDIO_STREAM_ENFORCED_AUDIBLE,  // AudioChannel::Publicnotification
    AUDIO_STREAM_SYSTEM,            // AudioChannel::System
};

struct AudioDeviceInfo {
  /** The string the value maps to */
  const char* tag;
  /** The enum value that maps to this string */
  uint32_t value;
};

// Mappings audio output devices to strings.
static const AudioDeviceInfo kAudioDeviceInfos[] = {
    {"earpiece", AUDIO_DEVICE_OUT_EARPIECE},
    {"speaker", AUDIO_DEVICE_OUT_SPEAKER},
    {"wired_headset", AUDIO_DEVICE_OUT_WIRED_HEADSET},
    {"wired_headphone", AUDIO_DEVICE_OUT_WIRED_HEADPHONE},
    {"bt_scoheadset", AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET},
    {"bt_a2dp", AUDIO_DEVICE_OUT_BLUETOOTH_A2DP},
};

static const int kBtSampleRate = 8000;
static const int kBtWideBandSampleRate = 16000;

typedef MozPromise<bool, const char*, true> VolumeInitPromise;

namespace mozilla {
namespace dom {
namespace gonk {

/**
 * We have five sound volume settings from UX spec,
 * You can see more informations in Bug1068219.
 * (1) Media : music, video, FM ...
 * (2) Notification : ringer, notification ...
 * (3) Alarm : alarm
 * (4) Telephony : GSM call, WebRTC call
 * (5) Bluetooth SCO : SCO call
 **/
struct VolumeData {
  const char* mChannelName;
  int32_t mStreamType;
};

static const VolumeData gVolumeData[] = {
    {"audio.volume.content", AUDIO_STREAM_MUSIC},
    {"audio.volume.notification", AUDIO_STREAM_NOTIFICATION},
    {"audio.volume.alarm", AUDIO_STREAM_ALARM},
    {"audio.volume.telephony", AUDIO_STREAM_VOICE_CALL},
    {"audio.volume.bt_sco", AUDIO_STREAM_BLUETOOTH_SCO}};

// TODO FIXME
// nsCOMPtr<nsISettingsServiceLock>
// GetSettingServiceLock()
// {
//   nsresult rv;
//   nsCOMPtr<nsISettingsService> service = do_GetService(SETTINGS_SERVICE,
//   &rv); if (NS_WARN_IF(NS_FAILED(rv))) {
//     return nullptr;
//   }

//   nsCOMPtr<nsISettingsServiceLock> lock;
//   rv = service->CreateLock(nullptr, getter_AddRefs(lock));
//   if (NS_WARN_IF(NS_FAILED(rv))) {
//     return nullptr;
//   }
//   return lock.forget();
// }

class GonkAudioPortCallback : public AudioSystem::AudioPortCallback {
 public:
  virtual void onAudioPortListUpdate() {
    nsCOMPtr<nsIRunnable> runnable = NS_NewRunnableFunction(
        "GonkAudioPortCallback::onAudioPortListUpdate", []() {
          MOZ_ASSERT(NS_IsMainThread());
          RefPtr<AudioManager> audioManager = AudioManager::GetInstance();
          NS_ENSURE_TRUE(audioManager.get(), );
          audioManager->UpdateCachedActiveDevicesForStreams();
          audioManager->MaybeUpdateVolumeSettingToDatabase();
        });
    NS_DispatchToMainThread(runnable);
  }
  virtual void onAudioPatchListUpdate() {}
  virtual void onServiceDied() {}
};

// We need to store GonkAudioPortCallback instance in AudioManager class, but
// we don't want to expose android::sp in the header, so add a holder to hide
// it. If there are more similar cases in the future, we need a private
// AudioManager class instead.
class AudioPortCallbackHolder {
 public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AudioPortCallbackHolder);

  android::sp<AudioSystem::AudioPortCallback> Callback() {
    if (!mCallback) {
      mCallback = new GonkAudioPortCallback();
    }
    return mCallback;
  }

 private:
  ~AudioPortCallbackHolder(){};
  android::sp<AudioSystem::AudioPortCallback> mCallback;
};

void AudioManager::HandleAudioFlingerDied() {
  // Disable volume change notification
  mIsVolumeInited = false;

  uint32_t attempt;
  for (attempt = 0; attempt < 50; attempt++) {
    if (android::defaultServiceManager()->checkService(
            android::String16(AUDIO_POLICY_SERVICE_NAME)) != 0) {
      break;
    }
    LOG("AudioPolicyService is dead! attempt=%d", attempt);
    usleep(1000 * 200);
  }

  MOZ_RELEASE_ASSERT(attempt < 50);

  // Indicate to audio HAL that we start the reconfiguration phase after a media
  // server crash
  SetParameters("restarting=true");

  // Restore device connection states
  SetAllDeviceConnectionStates();

  // Restore call state
  AudioSystem::setPhoneState(static_cast<audio_mode_t>(mPhoneState));

  // Restore master volume
  AudioSystem::setMasterVolume(1.0);

  // Restore stream volumes
  for (uint32_t streamType = 0; streamType < AUDIO_STREAM_CNT; ++streamType) {
    mStreamStates[streamType]->InitStreamVolume();
    mStreamStates[streamType]->RestoreVolumeIndexToAllDevices();
  }

  // Indicate the end of reconfiguration phase to audio HAL
  SetParameters("restarting=true");

  // Enable volume change notification
  mIsVolumeInited = true;
  MaybeUpdateVolumeSettingToDatabase(true);

  AudioSystem::setAssistantUid(AUDIO_UID_INVALID);
}

// TODO FIXME
// class VolumeInitCallback final : public nsISettingsServiceCallback
// {
// public:
//   NS_DECL_ISUPPORTS

//   VolumeInitCallback()
//     : mInitCounter(0)
//   {
//     mPromise = mPromiseHolder.Ensure(__func__);
//   }

//   RefPtr<VolumeInitPromise> GetPromise() const
//   {
//     return mPromise;
//   }

//   NS_IMETHOD Handle(const nsAString& aName, JS::Handle<JS::Value> aResult)
//   {
//     RefPtr<AudioManager> audioManager = AudioManager::GetInstance();
//     MOZ_ASSERT(audioManager);
//     for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
//       NS_ConvertASCIItoUTF16 volumeType(gVolumeData[idx].mChannelName);
//       if (StringBeginsWith(aName, volumeType)) {
//         uint32_t device = GetDeviceFromSettingName(aName);
//         if (aResult.isInt32()) {
//           int32_t stream = gVolumeData[idx].mStreamType;
//           uint32_t volIndex = aResult.toInt32();
//           nsresult rv = NS_OK;
//           // The key from FE in the first booting would be like
//           "audio.volumes.content"
//           // (without device suffix). For such cases, we have to set the
//           volumes of
//           // all devices with this value. If not, the following stages will
//           set the
//           // volumes by Gecko's defaults that could conflict with the UX
//           specifications. if (device == AUDIO_DEVICE_NONE) {
//             rv = audioManager->SetStreamVolumeIndex(stream, volIndex);
//           } else {
//             rv = audioManager->SetStreamVolumeForDevice(stream, volIndex,
//             device);
//           }
//           if (rv != NS_OK) {
//             mPromiseHolder.RejectIfExists("Error : set volume failed",
//             __func__); return rv;
//           }
//         }

//         if (++mInitCounter == audioManager->GetSpecificVolumeCount()) {
//           mPromiseHolder.ResolveIfExists(true, __func__);
//         }
//         return NS_OK;
//       }
//     }
//     mPromiseHolder.RejectIfExists("Error : unexpected audio init event.",
//     __func__); return NS_OK;
//   }

//   NS_IMETHOD HandleError(const nsAString& aName)
//   {
//     mPromiseHolder.Reject(NS_ConvertUTF16toUTF8(aName).get(), __func__);
//     return NS_OK;
//   }

// protected:
//   ~VolumeInitCallback() {}

//   uint32_t GetDeviceFromSettingName(const nsAString& aName) const
//   {
//     for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(kAudioDeviceInfos); ++idx)
//     {
//       NS_ConvertASCIItoUTF16 device(kAudioDeviceInfos[idx].tag);
//       if (StringEndsWith(aName, device)) {
//         return kAudioDeviceInfos[idx].value;
//       }
//     }
//     return AUDIO_DEVICE_NONE;
//   }

//   RefPtr<VolumeInitPromise> mPromise;
//   MozPromiseHolder<VolumeInitPromise> mPromiseHolder;
//   uint32_t mInitCounter;
// };

// NS_IMPL_ISUPPORTS(VolumeInitCallback, nsISettingsServiceCallback)

static void BinderDeadCallback(android::status_t aErr) {
  if (aErr != android::DEAD_OBJECT) {
    return;
  }

  nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableFunction("AudioManager::BinderDeadCallback", []() {
        MOZ_ASSERT(NS_IsMainThread());
        RefPtr<AudioManager> audioManager = AudioManager::GetInstance();
        NS_ENSURE_TRUE(audioManager.get(), );
        audioManager->HandleAudioFlingerDied();
      });

  NS_DispatchToMainThread(runnable);
}

bool AudioManager::IsFmOutConnected() {
#if defined(PRODUCT_MANUFACTURER_SPRD)
  return mConnectedDevices.Get(AUDIO_DEVICE_OUT_FM_HEADSET, nullptr) ||
         mConnectedDevices.Get(AUDIO_DEVICE_OUT_FM_SPEAKER, nullptr);
#elif defined(PRODUCT_MANUFACTURER_MTK)
  return mConnectedDevices.Get(AUDIO_DEVICE_IN_FM_TUNER, nullptr);
#else
  return mConnectedDevices.Get(AUDIO_DEVICE_OUT_FM, nullptr);
#endif
}

NS_IMPL_ISUPPORTS(AudioManager, nsIAudioManager, nsIObserver)

void AudioManager::UpdateHeadsetConnectionState(hal::SwitchState aState) {
  bool headphoneConnected =
      mConnectedDevices.Get(AUDIO_DEVICE_OUT_WIRED_HEADPHONE, nullptr);
  bool headsetConnected =
      mConnectedDevices.Get(AUDIO_DEVICE_OUT_WIRED_HEADSET, nullptr);
  bool lineoutConnected = mConnectedDevices.Get(AUDIO_DEVICE_OUT_LINE, nullptr);

  if (aState == hal::SWITCH_STATE_HEADSET) {
    UpdateDeviceConnectionState(true, AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                NS_LITERAL_CSTRING(""));
    UpdateDeviceConnectionState(true, AUDIO_DEVICE_IN_WIRED_HEADSET,
                                NS_LITERAL_CSTRING(""));

  } else if (aState == hal::SWITCH_STATE_HEADPHONE) {
    UpdateDeviceConnectionState(true, AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                NS_LITERAL_CSTRING(""));
  } else if (aState == hal::SWITCH_STATE_LINEOUT) {
    UpdateDeviceConnectionState(true, AUDIO_DEVICE_OUT_LINE,
                                NS_LITERAL_CSTRING(""));
  } else if (aState == hal::SWITCH_STATE_OFF) {
    if (headsetConnected) {
      UpdateDeviceConnectionState(false, AUDIO_DEVICE_OUT_WIRED_HEADSET,
                                  NS_LITERAL_CSTRING(""));
      UpdateDeviceConnectionState(false, AUDIO_DEVICE_IN_WIRED_HEADSET,
                                  NS_LITERAL_CSTRING(""));
    }
    if (headphoneConnected) {
      UpdateDeviceConnectionState(false, AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                  NS_LITERAL_CSTRING(""));
    }
    if (lineoutConnected) {
      UpdateDeviceConnectionState(false, AUDIO_DEVICE_OUT_LINE,
                                  NS_LITERAL_CSTRING(""));
    }
  }
}

static void setDeviceConnectionStateInternal(audio_devices_t device,
                                             audio_policy_dev_state_t state,
                                             const char* device_address) {
  AudioSystem::setDeviceConnectionState(device, state, device_address, "",
                                        AUDIO_FORMAT_DEFAULT);
}

uint32_t AudioManager::GetSpecificVolumeCount() {
  uint32_t count = 0;
  for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
    count++;
    uint32_t streamType = gVolumeData[idx].mStreamType;
    VolumeStreamState* streamState = mStreamStates[streamType].get();
    if (streamState->IsDeviceSpecificVolume()) {
      count += MOZ_ARRAY_LENGTH(kAudioDeviceInfos);
    }
  }
  return count;
}

void AudioManager::UpdateDeviceConnectionState(bool aIsConnected,
                                               uint32_t aDevice,
                                               const nsCString& aDeviceName) {
  bool isConnected = mConnectedDevices.Get(aDevice, nullptr);
  if (isConnected && !aIsConnected) {
    setDeviceConnectionStateInternal(static_cast<audio_devices_t>(aDevice),
                                     AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                                     aDeviceName.get());
    mConnectedDevices.Remove(aDevice);
  } else if (!isConnected && aIsConnected) {
    setDeviceConnectionStateInternal(static_cast<audio_devices_t>(aDevice),
                                     AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                                     aDeviceName.get());
    mConnectedDevices.Put(aDevice, aDeviceName);
  }
}

void AudioManager::SetAllDeviceConnectionStates() {
  for (auto iter = mConnectedDevices.Iter(); !iter.Done(); iter.Next()) {
    const uint32_t& device = iter.Key();
    nsCString& deviceAddress = iter.Data();
    setDeviceConnectionStateInternal(static_cast<audio_devices_t>(device),
                                     AUDIO_POLICY_DEVICE_STATE_AVAILABLE,
                                     deviceAddress.get());
  }
}

void AudioManager::HandleBluetoothStatusChanged(nsISupports* aSubject,
                                                const char* aTopic,
                                                const nsCString aAddress) {
#ifdef MOZ_B2G_BT
  bool isConnected = false;
  if (!strcmp(aTopic, BLUETOOTH_SCO_STATUS_CHANGED_ID)) {
    BluetoothHfpManagerBase* hfp =
        static_cast<BluetoothHfpManagerBase*>(aSubject);
    isConnected = hfp->IsScoConnected();
  } else {
    BluetoothProfileManagerBase* profile =
        static_cast<BluetoothProfileManagerBase*>(aSubject);
    isConnected = profile->IsConnected();
  }

  if (!strcmp(aTopic, BLUETOOTH_SCO_STATUS_CHANGED_ID)) {
    if (isConnected) {
      BluetoothHfpManagerBase* hfp =
          static_cast<BluetoothHfpManagerBase*>(aSubject);
      int btSampleRate =
          hfp->IsWbsEnabled() ? kBtWideBandSampleRate : kBtSampleRate;
      SetParameters("bt_samplerate=%d", btSampleRate);
      SetForceForUse(nsIAudioManager::USE_COMMUNICATION,
                     nsIAudioManager::FORCE_BT_SCO);
    } else {
      int32_t force;
      GetForceForUse(nsIAudioManager::USE_COMMUNICATION, &force);
      if (force == nsIAudioManager::FORCE_BT_SCO) {
        SetForceForUse(nsIAudioManager::USE_COMMUNICATION,
                       nsIAudioManager::FORCE_NONE);
      }
    }
  } else if (!strcmp(aTopic, BLUETOOTH_A2DP_STATUS_CHANGED_ID)) {
    if (!isConnected && mA2dpSwitchDone) {
      RefPtr<AudioManager> self = this;
      nsCOMPtr<nsIRunnable> runnable = NS_NewRunnableFunction(
          "AudioManager::HandleBluetoothStatusChanged",
          [self, isConnected, aAddress]() {
            if (self->mA2dpSwitchDone) {
              return;
            }
            self->UpdateDeviceConnectionState(
                isConnected, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP, aAddress);
            self->SetParameters("bluetooth_enabled=false");
            self->SetParameters("A2dpSuspended=true");
            self->mA2dpSwitchDone = true;
          });
      MessageLoop::current()->PostDelayedTask(runnable.forget(), 1000);

      mA2dpSwitchDone = false;
    } else {
      UpdateDeviceConnectionState(isConnected, AUDIO_DEVICE_OUT_BLUETOOTH_A2DP,
                                  aAddress);
      SetParameters("bluetooth_enabled=true");
      SetParameters("A2dpSuspended=false");
      mA2dpSwitchDone = true;
      if (AudioSystem::getForceUse(AUDIO_POLICY_FORCE_FOR_MEDIA) ==
          AUDIO_POLICY_FORCE_NO_BT_A2DP) {
        SetForceForUse(AUDIO_POLICY_FORCE_FOR_MEDIA, AUDIO_POLICY_FORCE_NONE);
      }
    }
    mBluetoothA2dpEnabled = isConnected;
  } else if (!strcmp(aTopic, BLUETOOTH_HFP_STATUS_CHANGED_ID)) {
    UpdateDeviceConnectionState(
        isConnected, AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET, aAddress);
    UpdateDeviceConnectionState(
        isConnected, AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET, aAddress);
  } else if (!strcmp(aTopic, BLUETOOTH_HFP_NREC_STATUS_CHANGED_ID)) {
    BluetoothHfpManagerBase* hfp =
        static_cast<BluetoothHfpManagerBase*>(aSubject);
    if (hfp->IsNrecEnabled()) {
      // TODO: (Bug 880785) Replace <unknown> with remote Bluetooth device name
      SetParameters("bt_headset_name=<unknown>;bt_headset_nrec=on");
    } else {
      SetParameters("bt_headset_name=<unknown>;bt_headset_nrec=off");
    }
  } else if (!strcmp(aTopic, BLUETOOTH_HFP_WBS_STATUS_CHANGED_ID)) {
    BluetoothHfpManagerBase* hfp =
        static_cast<BluetoothHfpManagerBase*>(aSubject);
    if (hfp->IsWbsEnabled()) {
      SetParameters("bt_wbs=on");
    } else {
      SetParameters("bt_wbs=off");
    }
  }
#endif
}

nsresult AudioManager::Observe(nsISupports* aSubject, const char* aTopic,
                               const char16_t* aData) {
  if ((strcmp(aTopic, BLUETOOTH_SCO_STATUS_CHANGED_ID) == 0) ||
      (strcmp(aTopic, BLUETOOTH_HFP_STATUS_CHANGED_ID) == 0) ||
      (strcmp(aTopic, BLUETOOTH_HFP_NREC_STATUS_CHANGED_ID) == 0) ||
      (strcmp(aTopic, BLUETOOTH_HFP_WBS_STATUS_CHANGED_ID) == 0) ||
      (strcmp(aTopic, BLUETOOTH_A2DP_STATUS_CHANGED_ID) == 0)) {
    nsCString address = NS_ConvertUTF16toUTF8(nsDependentString(aData));
    if (address.IsEmpty()) {
      NS_WARNING(nsPrintfCString("Invalid address of %s", aTopic).get());
      return NS_ERROR_FAILURE;
    }

    HandleBluetoothStatusChanged(aSubject, aTopic, address);
    return NS_OK;
  }

  // To process the volume control on each volume categories according to
  // change of settings
  // else if (!strcmp(aTopic, MOZ_SETTINGS_CHANGE_ID)) {//TODO FIXME
  //   RootedDictionary<dom::SettingChangeNotification>
  //   setting(nsContentUtils::RootingCxForThread()); if
  //   (!WrappedJSToDictionary(aSubject, setting)) {
  //     return NS_OK;
  //   }
  //   if (!StringBeginsWith(setting.mKey, NS_LITERAL_STRING("audio.volume.")))
  //   {
  //     return NS_OK;
  //   }
  //   if (!setting.mValue.isNumber()) {
  //     return NS_OK;
  //   }

  //   uint32_t volIndex = setting.mValue.toNumber();
  //   for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
  //     if (setting.mKey.EqualsASCII(gVolumeData[idx].mChannelName)) {
  //       SetStreamVolumeIndex(gVolumeData[idx].mStreamType, volIndex);
  //       return NS_OK;
  //     }
  //   }
  // }
#ifdef PRODUCT_MANUFACTURER_MTK
  // Notify screen state to audio HAL in order to save power when screen is off.
  else if (!strcmp(aTopic, SCREEN_STATE_CHANGED)) {
    if (NS_LITERAL_STRING("on").Equals(aData)) {
      SetParameters("screen_state=on");
    } else {
      SetParameters("screen_state=off");
    }
    return NS_OK;
  }
#endif

  NS_WARNING("Unexpected topic in AudioManager");
  return NS_ERROR_FAILURE;
}

static void NotifyHeadphonesStatus(hal::SwitchState aState) {
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    if (aState == hal::SWITCH_STATE_HEADSET) {
      obs->NotifyObservers(nullptr, HEADPHONES_STATUS_CHANGED,
                           HEADPHONES_STATUS_HEADSET);
    } else if (aState == hal::SWITCH_STATE_HEADPHONE) {
      obs->NotifyObservers(nullptr, HEADPHONES_STATUS_CHANGED,
                           HEADPHONES_STATUS_HEADPHONE);
    } else if (aState == hal::SWITCH_STATE_OFF) {
      obs->NotifyObservers(nullptr, HEADPHONES_STATUS_CHANGED,
                           HEADPHONES_STATUS_OFF);
    } else if (aState == hal::SWITCH_STATE_LINEOUT) {
      obs->NotifyObservers(nullptr, HEADPHONES_STATUS_CHANGED,
                           HEADPHONES_STATUS_LINEOUT);
    } else {
      obs->NotifyObservers(nullptr, HEADPHONES_STATUS_CHANGED,
                           HEADPHONES_STATUS_UNKNOWN);
    }
  }
}

class HeadphoneSwitchObserver : public hal::SwitchObserver {
 public:
  void Notify(const hal::SwitchEvent& aEvent) {
    RefPtr<AudioManager> audioManager = AudioManager::GetInstance();
    MOZ_ASSERT(audioManager);
    audioManager->HandleHeadphoneSwitchEvent(aEvent);
  }
};

void AudioManager::HandleHeadphoneSwitchEvent(const hal::SwitchEvent& aEvent) {
  // For more information, see bug 29237.
  // Holds the wakelock for making sure that gecko can do all the needed things
  // before system sleeps.
  //
  // Because of the switch events will be passed from
  // InputReaderThread -> main thread -> IO thread -> main
  // thread(HeadphoneSwitchObserver) I think adding wakelock on every thread is
  // not a good way. So that I decide to hold the wakelock here first. If we
  // still get problems, we may need to add more wakelocks on other threads.
  CreateWakeLock();

  NotifyHeadphonesStatus(aEvent.status());
  // When user pulled out the headset, a delay of routing here can avoid the
  // leakage of audio from speaker.
  if (aEvent.status() == hal::SWITCH_STATE_OFF && mSwitchDone) {
    // When system is in sleep mode and user unplugs the headphone, we need to
    // hold the wakelock here, or the delayed task will not be executed.
    RefPtr<AudioManager> self = this;
    nsCOMPtr<nsIRunnable> runnable = NS_NewRunnableFunction(
        "AudioManager::HandleHeadphoneSwitchEvent", [self]() {
          if (self->mSwitchDone) {
            // The headset was inserted again, so skip this task.
            return;
          }
          self->UpdateHeadsetConnectionState(hal::SWITCH_STATE_OFF);
          self->mSwitchDone = true;
          self->ReleaseWakeLock();
        });
    MessageLoop::current()->PostDelayedTask(runnable.forget(), 1000);
    mSwitchDone = false;
  } else if (aEvent.status() != hal::SWITCH_STATE_OFF) {
    UpdateHeadsetConnectionState(aEvent.status());
    mSwitchDone = true;
  }
  // Handle the coexistence of a2dp / headset device, latest one wins.
  int32_t forceUse = 0;
  GetForceForUse(AUDIO_POLICY_FORCE_FOR_MEDIA, &forceUse);
  if (aEvent.status() != hal::SWITCH_STATE_OFF && mBluetoothA2dpEnabled) {
    SetForceForUse(AUDIO_POLICY_FORCE_FOR_MEDIA, AUDIO_POLICY_FORCE_NO_BT_A2DP);
  } else if (forceUse == AUDIO_POLICY_FORCE_NO_BT_A2DP) {
    SetForceForUse(AUDIO_POLICY_FORCE_FOR_MEDIA, AUDIO_POLICY_FORCE_NONE);
  }
  if (aEvent.status() != hal::SWITCH_STATE_OFF) {
    ReleaseWakeLock();
  }
}

void AudioManager::CreateWakeLock() {
  RefPtr<dom::power::PowerManagerService> pmService =
      dom::power::PowerManagerService::GetInstance();
  NS_ENSURE_TRUE_VOID(pmService);

  if (mWakeLock) {
    // If the user inserts the headset before the SWITCH_STATE_OFF runnable is
    // executed, we may hit his case.
    return;
  }

  ErrorResult rv;
  mWakeLock = pmService->NewWakeLock(NS_LITERAL_STRING("cpu"), nullptr, rv);
}

void AudioManager::ReleaseWakeLock() {
  if (!mWakeLock) {
    NS_WARNING("ReleaseWakeLock mWakeLock is null");
    return;
  }

  ErrorResult rv;
  mWakeLock->Unlock(rv);
  mWakeLock = nullptr;
}

static StaticRefPtr<AudioManager> sAudioManager;

AudioManager::AudioManager()
    : mObserver(new HeadphoneSwitchObserver()),
      mAudioPortCallbackHolder(new AudioPortCallbackHolder()) {
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sAudioManager);

  for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(kAudioDeviceInfos); ++idx) {
    mAudioDeviceTableIdMaps.Put(kAudioDeviceInfos[idx].value, idx);
  }

  AudioSystem::setErrorCallback(BinderDeadCallback);
  AudioSystem::addAudioPortCallback(mAudioPortCallbackHolder->Callback());

  // Create VolumeStreamStates
  for (uint32_t loop = 0; loop < AUDIO_STREAM_CNT; ++loop) {
    VolumeStreamState* streamState =
        new VolumeStreamState(*this, static_cast<audio_stream_type_t>(loop));
    mStreamStates.AppendElement(streamState);
  }
  // Initialize stream volumes with default values
  for (int32_t streamType = 0; streamType < AUDIO_STREAM_CNT; streamType++) {
    uint32_t volIndex = sDefaultStreamVolumeTbl[streamType];
    SetStreamVolumeForDevice(streamType, volIndex, AUDIO_DEVICE_OUT_DEFAULT);
  }
  UpdateCachedActiveDevicesForStreams();

  RegisterSwitchObserver(hal::SWITCH_HEADPHONES, mObserver.get());
  // Initialize headhone/heaset status
  UpdateHeadsetConnectionState(
      hal::GetCurrentSwitchState(hal::SWITCH_HEADPHONES));
  NotifyHeadphonesStatus(hal::GetCurrentSwitchState(hal::SWITCH_HEADPHONES));

  RegisterSwitchObserver(hal::SWITCH_LINEOUT, mObserver.get());
  // Initialize lineout status
  UpdateHeadsetConnectionState(hal::GetCurrentSwitchState(hal::SWITCH_LINEOUT));
  NotifyHeadphonesStatus(hal::GetCurrentSwitchState(hal::SWITCH_LINEOUT));

  // Get the initial volume index from settings DB during boot up.
  InitVolumeFromDatabase();

  // Gecko only control stream volume not master so set to default value
  // directly.
  AudioSystem::setMasterVolume(1.0);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(
          obs->AddObserver(this, BLUETOOTH_SCO_STATUS_CHANGED_ID, false))) {
    NS_WARNING("Failed to add bluetooth sco status changed observer!");
  }
  if (NS_FAILED(
          obs->AddObserver(this, BLUETOOTH_A2DP_STATUS_CHANGED_ID, false))) {
    NS_WARNING("Failed to add bluetooth a2dp status changed observer!");
  }
  if (NS_FAILED(
          obs->AddObserver(this, BLUETOOTH_HFP_STATUS_CHANGED_ID, false))) {
    NS_WARNING("Failed to add bluetooth hfp status changed observer!");
  }
  if (NS_FAILED(obs->AddObserver(this, BLUETOOTH_HFP_NREC_STATUS_CHANGED_ID,
                                 false))) {
    NS_WARNING("Failed to add bluetooth hfp NREC status changed observer!");
  }
  if (NS_FAILED(
          obs->AddObserver(this, BLUETOOTH_HFP_WBS_STATUS_CHANGED_ID, false))) {
    NS_WARNING("Failed to add bluetooth hfp WBS status changed observer!");
  }
  // TODO FIXME
  // if (NS_FAILED(obs->AddObserver(this, MOZ_SETTINGS_CHANGE_ID, false))) {
  //  NS_WARNING("Failed to add mozsettings-changed observer!");
  // }
#ifdef PRODUCT_MANUFACTURER_MTK
  if (NS_FAILED(obs->AddObserver(this, SCREEN_STATE_CHANGED, false))) {
    NS_WARNING("Failed to add screen-state-changed observer!");
  }
#endif

#ifdef MOZ_B2G_RIL
  char value[PROPERTY_VALUE_MAX];
  property_get("ro.moz.mute.call.to_ril", value, "false");
  if (!strcmp(value, "true")) {
    mMuteCallToRIL = true;
  }
#endif

  // Android 10 introduces new rules for sharing audio input. This call can
  // prevent AudioPolicyService from treating us as assistant app and
  // incorrectly muting our audio input because we don't meet some criteria of
  // assistant app.
  AudioSystem::setAssistantUid(AUDIO_UID_INVALID);
}

AudioManager::~AudioManager() {
  MOZ_ASSERT(!sAudioManager);

  AudioSystem::setErrorCallback(nullptr);
  AudioSystem::removeAudioPortCallback(mAudioPortCallbackHolder->Callback());
  hal::UnregisterSwitchObserver(hal::SWITCH_HEADPHONES, mObserver.get());
  hal::UnregisterSwitchObserver(hal::SWITCH_LINEOUT, mObserver.get());

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(obs->RemoveObserver(this, BLUETOOTH_SCO_STATUS_CHANGED_ID))) {
    NS_WARNING("Failed to remove bluetooth sco status changed observer!");
  }
  if (NS_FAILED(obs->RemoveObserver(this, BLUETOOTH_A2DP_STATUS_CHANGED_ID))) {
    NS_WARNING("Failed to remove bluetooth a2dp status changed observer!");
  }
  if (NS_FAILED(obs->RemoveObserver(this, BLUETOOTH_HFP_STATUS_CHANGED_ID))) {
    NS_WARNING("Failed to remove bluetooth hfp status changed observer!");
  }
  if (NS_FAILED(
          obs->RemoveObserver(this, BLUETOOTH_HFP_NREC_STATUS_CHANGED_ID))) {
    NS_WARNING("Failed to remove bluetooth hfp NREC status changed observer!");
  }
  if (NS_FAILED(
          obs->RemoveObserver(this, BLUETOOTH_HFP_WBS_STATUS_CHANGED_ID))) {
    NS_WARNING("Failed to remove bluetooth hfp WBS status changed observer!");
  }
  if (NS_FAILED(obs->RemoveObserver(this, MOZ_SETTINGS_CHANGE_ID))) {
    NS_WARNING("Failed to remove mozsettings-changed observer!");
  }
#ifdef PRODUCT_MANUFACTURER_MTK
  if (NS_FAILED(obs->RemoveObserver(this, SCREEN_STATE_CHANGED))) {
    NS_WARNING("Failed to remove screen-state-changed observer!");
  }
#endif
}

already_AddRefed<AudioManager> AudioManager::GetInstance() {
  // Avoid createing AudioManager from content process.
  if (!XRE_IsParentProcess()) {
    MOZ_CRASH("Non-chrome processes should not get here.");
  }

  MOZ_ASSERT(NS_IsMainThread());

  // Avoid createing multiple AudioManager instance inside main process.
  if (!sAudioManager) {
    sAudioManager = new AudioManager();
    ClearOnShutdown(&sAudioManager);
  }

  RefPtr<AudioManager> audioMgr = sAudioManager.get();
  return audioMgr.forget();
}

NS_IMETHODIMP
AudioManager::GetMicrophoneMuted(bool* aMicrophoneMuted) {
#ifdef MOZ_B2G_RIL
  if (mMuteCallToRIL) {
    // Simply return cached mIsMicMuted if mute call go via RIL.
    *aMicrophoneMuted = mIsMicMuted;
    return NS_OK;
  }
#endif

  if (AudioSystem::isMicrophoneMuted(aMicrophoneMuted)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetMicrophoneMuted(bool aMicrophoneMuted) {
#ifdef MOZ_B2G_RIL
  if (mMuteCallToRIL) {
    // Extra mute request to RIL for specific platform.
    nsCOMPtr<nsIRadioInterfaceLayer> ril = do_GetService("@mozilla.org/ril;1");
    NS_ENSURE_TRUE(ril, NS_ERROR_FAILURE);
    ril->SetMicrophoneMuted(aMicrophoneMuted);
    mIsMicMuted = aMicrophoneMuted;
    return NS_OK;
  }
#endif

  AudioSystem::muteMicrophone(aMicrophoneMuted);

  bool micMuted;
  AudioSystem::isMicrophoneMuted(&micMuted);
  if (micMuted != aMicrophoneMuted) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetPhoneState(int32_t* aState) {
  *aState = mPhoneState;
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetPhoneState(int32_t aState) {
  if (mPhoneState == aState) {
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    nsString state;
    state.AppendInt(aState);
    obs->NotifyObservers(nullptr, "phone-state-changed", state.get());
  }

  if (AudioSystem::setPhoneState(static_cast<audio_mode_t>(aState))) {
    return NS_ERROR_FAILURE;
  }

  mPhoneState = aState;
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetHeadsetState(int32_t* aHeadsetState) {
  MOZ_ASSERT(aHeadsetState);

  hal::SwitchState state = GetCurrentSwitchState(hal::SWITCH_HEADPHONES);
  if (state == hal::SWITCH_STATE_HEADSET) {
    *aHeadsetState = nsIAudioManager::HEADSET_STATE_HEADSET;
  } else if (state == hal::SWITCH_STATE_HEADPHONE) {
    *aHeadsetState = nsIAudioManager::HEADSET_STATE_HEADPHONE;
  } else if (state == hal::SWITCH_STATE_OFF) {
    *aHeadsetState = nsIAudioManager::HEADSET_STATE_OFF;
  } else {
    *aHeadsetState = nsIAudioManager::HEADSET_STATE_UNKNOWN;
  }

  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetTtyMode(uint16_t aTtyMode) {
  if (aTtyMode == nsIAudioManager::TTY_MODE_FULL) {
    SetParameters("tty_mode=tty_full");
  } else if (aTtyMode == nsIAudioManager::TTY_MODE_HCO) {
    SetParameters("tty_mode=tty_hco");
  } else if (aTtyMode == nsIAudioManager::TTY_MODE_VCO) {
    SetParameters("tty_mode=tty_vco");
  } else {
    SetParameters("tty_mode=tty_off");
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetForceForUse(int32_t aUsage, int32_t aForce) {
  android::status_t status = AudioSystem::setForceUse(
      (audio_policy_force_use_t)aUsage, (audio_policy_forced_cfg_t)aForce);
#ifdef PRODUCT_MANUFACTURER_SPRD
  // Sync force use between MEDIA and FM
  if (aUsage == USE_MEDIA && status == NO_ERROR) {
    // Mute FM when switching output paths. This can avoid pop noise caused
    // by applying incorrect volume index, which is from the original path,
    // to the new path.
    SetVendorFmVolumeIndex(true);
    status = AudioSystem::setForceUse(AUDIO_POLICY_FORCE_FOR_FM,
                                      (audio_policy_forced_cfg_t)aForce);
  }
#elif defined(PRODUCT_MANUFACTURER_MTK)
  // Sync force use between MEDIA and FM
  if (aUsage == USE_MEDIA && status == NO_ERROR) {
    status = AudioSystem::setForceUse(AUDIO_POLICY_FORCE_FOR_PROPRIETARY,
                                      (audio_policy_forced_cfg_t)aForce);
  }
#endif

  bool enableRadio = false;
  GetFmRadioAudioEnabled(&enableRadio);
  // AudioPortListUpdate will trigger only when there is a device change(adding
  // or removing) SetForcespeaker() only switch between devices (ex: FM switch
  // to speaker) so still have to manually call it.
  if (enableRadio == true && aUsage == AUDIO_POLICY_FORCE_FOR_MEDIA) {
    mAudioPortCallbackHolder->Callback()->onAudioPortListUpdate();
  }
  return status ? NS_ERROR_FAILURE : NS_OK;
}

NS_IMETHODIMP
AudioManager::GetForceForUse(int32_t aUsage, int32_t* aForce) {
  *aForce = AudioSystem::getForceUse((audio_policy_force_use_t)aUsage);
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::GetFmRadioAudioEnabled(bool* aFmRadioAudioEnabled) {
  *aFmRadioAudioEnabled = IsFmOutConnected();
  return NS_OK;
}

void AudioManager::SetVendorFmVolumeIndex(bool aMute) {
#if defined(PRODUCT_MANUFACTURER_SPRD)
  uint32_t device = GetDeviceForSprdFm();
  uint32_t volIndex =
      aMute ? 0 : mStreamStates[AUDIO_STREAM_MUSIC]->GetVolumeIndex(device);
  SetParameters("FM_Volume=%d", volIndex);
#endif
}

NS_IMETHODIMP
AudioManager::SetFmRadioAudioEnabled(bool aFmRadioAudioEnabled) {
#if defined(PRODUCT_MANUFACTURER_SPRD)
  UpdateDeviceConnectionState(aFmRadioAudioEnabled, AUDIO_DEVICE_OUT_FM_HEADSET,
                              NS_LITERAL_CSTRING(""));

  UpdateDeviceConnectionState(aFmRadioAudioEnabled, AUDIO_DEVICE_OUT_FM_SPEAKER,
                              NS_LITERAL_CSTRING(""));
#elif defined(PRODUCT_MANUFACTURER_MTK)
  UpdateDeviceConnectionState(aFmRadioAudioEnabled, AUDIO_DEVICE_IN_FM_TUNER,
                              NS_LITERAL_CSTRING(""));
#else
  UpdateDeviceConnectionState(aFmRadioAudioEnabled, AUDIO_DEVICE_OUT_FM,
                              NS_LITERAL_CSTRING(""));
#endif

  if (aFmRadioAudioEnabled) {
    SetVendorFmVolumeIndex(false);
  }

  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetAudioChannelVolume(uint32_t aChannel, uint32_t aIndex) {
  if (aChannel >= NUMBER_OF_AUDIO_CHANNELS) {
    return NS_ERROR_INVALID_ARG;
  }

  return SetStreamVolumeIndex(sChannelStreamTbl[aChannel], aIndex);
}

NS_IMETHODIMP
AudioManager::GetAudioChannelVolume(uint32_t aChannel, uint32_t* aIndex) {
  if (aChannel >= NUMBER_OF_AUDIO_CHANNELS) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!aIndex) {
    return NS_ERROR_NULL_POINTER;
  }

  return GetStreamVolumeIndex(sChannelStreamTbl[aChannel], aIndex);
}

NS_IMETHODIMP
AudioManager::GetMaxAudioChannelVolume(uint32_t aChannel, uint32_t* aMaxIndex) {
  if (aChannel >= NUMBER_OF_AUDIO_CHANNELS) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!aMaxIndex) {
    return NS_ERROR_NULL_POINTER;
  }

  *aMaxIndex = mStreamStates[sChannelStreamTbl[aChannel]]->GetMaxIndex();
  return NS_OK;
}

nsresult AudioManager::ValidateVolumeIndex(int32_t aStream,
                                           uint32_t aIndex) const {
  if (aStream <= AUDIO_STREAM_DEFAULT || aStream >= AUDIO_STREAM_CNT) {
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t maxIndex = mStreamStates[aStream]->GetMaxIndex();
  if (aIndex > maxIndex) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult AudioManager::SetStreamVolumeForDevice(int32_t aStream,
                                                uint32_t aIndex,
                                                uint32_t aDevice) {
  if (ValidateVolumeIndex(aStream, aIndex) != NS_OK) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t streamAlias = sStreamVolumeAliasTbl[aStream];
  VolumeStreamState* streamState = mStreamStates[streamAlias].get();
  return streamState->SetVolumeIndexToAliasStreams(aIndex, aDevice);
}

nsresult AudioManager::SetStreamVolumeIndex(int32_t aStream, uint32_t aIndex) {
  if (ValidateVolumeIndex(aStream, aIndex) != NS_OK) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t streamAlias = sStreamVolumeAliasTbl[aStream];

  nsresult rv;
  for (int32_t streamType = 0; streamType < AUDIO_STREAM_CNT; streamType++) {
    if (streamAlias == sStreamVolumeAliasTbl[streamType]) {
      rv = mStreamStates[streamType]->SetVolumeIndexToActiveDevices(aIndex);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  // Bug 17613, Sync FM volume with MUSIC volume for SPRD.
  if (streamAlias == AUDIO_STREAM_MUSIC && IsFmOutConnected()) {
    SetVendorFmVolumeIndex(false);
  }

  MaybeUpdateVolumeSettingToDatabase();
  return NS_OK;
}

nsresult AudioManager::GetStreamVolumeIndex(int32_t aStream, uint32_t* aIndex) {
  if (!aIndex) {
    return NS_ERROR_INVALID_ARG;
  }

  if (aStream <= AUDIO_STREAM_DEFAULT || aStream >= AUDIO_STREAM_CNT) {
    return NS_ERROR_INVALID_ARG;
  }

  *aIndex = mStreamStates[aStream]->GetVolumeIndex();
  return NS_OK;
}

nsAutoCString AudioManager::AppendDeviceToVolumeSetting(const char* aName,
                                                        uint32_t aDevice) {
  nsAutoCString topic;
  topic.Assign(aName);
  topic.Append(".");
  uint32_t index = 0;
  DebugOnly<bool> exist = mAudioDeviceTableIdMaps.Get(aDevice, &index);
  MOZ_ASSERT(exist);
  topic.Append(kAudioDeviceInfos[index].tag);
  return topic;
}

void AudioManager::InitVolumeFromDatabase() {
  // nsresult rv; //TODO: FIXME
  // nsCOMPtr<nsISettingsService> service = do_GetService(SETTINGS_SERVICE,
  // &rv); if (NS_WARN_IF(NS_FAILED(rv))) {
  //   return;
  // }

  // nsCOMPtr<nsISettingsServiceLock> lock;
  // rv = service->CreateLock(nullptr, getter_AddRefs(lock));
  // if (NS_WARN_IF(NS_FAILED(rv))) {
  //   return;
  // }

  // RefPtr<VolumeInitCallback> callback = new VolumeInitCallback();
  // MOZ_ASSERT(callback);
  // callback->GetPromise()->Then(AbstractThread::MainThread(), __func__, this,
  //                              &AudioManager::InitDeviceVolumeSucceeded,
  //                              &AudioManager::InitDeviceVolumeFailed);

  // for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
  //   // We also need to get the value with mChannelName. FE use mChannelName
  //   only
  //   // for the key.
  //   lock->Get(gVolumeData[idx].mChannelName, callback);

  //   VolumeStreamState* streamState =
  //   mStreamStates[gVolumeData[idx].mStreamType].get(); if
  //   (!streamState->IsDeviceSpecificVolume()) {
  //     //volume type has no specific volume for different device
  //     continue;
  //   }

  //   for (uint32_t idx2 = 0; idx2 < MOZ_ARRAY_LENGTH(kAudioDeviceInfos);
  //   ++idx2) {
  //     lock->Get(AppendDeviceToVolumeSetting(gVolumeData[idx].mChannelName,
  //                                           kAudioDeviceInfos[idx2].value).get(),
  //               callback);
  //   }
  // }
}

void AudioManager::InitDeviceVolumeSucceeded() {
  mIsVolumeInited = true;
  MaybeUpdateVolumeSettingToDatabase(true);
}

void AudioManager::InitDeviceVolumeFailed(const char* aError) {
  // Default volume of AUDIO_DEVICE_OUT_DEFAULT is already set.
  mIsVolumeInited = true;
  MaybeUpdateVolumeSettingToDatabase(true);
  NS_WARNING(aError);
}

void AudioManager::MaybeUpdateVolumeSettingToDatabase(bool aForce) {
  if (!mIsVolumeInited) {
    return;
  }

  // TODO FIXME
  // nsCOMPtr<nsISettingsServiceLock> lock = GetSettingServiceLock();
  // if (NS_WARN_IF(!lock)) {
  //   return;
  // }

  // // Send events to update the Gaia volumes
  // JS::Rooted<JS::Value> value(nsContentUtils::RootingCx());
  // uint32_t volume = 0;
  // for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
  //   int32_t streamType = gVolumeData[idx].mStreamType;
  //   VolumeStreamState* streamState = mStreamStates[streamType].get();

  //   bool isVolumeUpdated = streamState->IsDevicesChanged() &&
  //   streamState->IsDeviceSpecificVolume(); if (!aForce && !isVolumeUpdated) {
  //     continue;
  //   }
  //   // Get volume index of active device.
  //   volume = streamState->GetVolumeIndex();
  //   value.setInt32(volume);
  //   lock->Set(gVolumeData[idx].mChannelName, value, nullptr, nullptr);
  // }

  // // For reducing the code dependency, Gaia doesn't need to know the
  // // device volume, it only need to care about different volume categories.
  // // However, we need to send the setting volume to the permanent database,
  // // so that we can store the volume setting even if the phone reboots.

  // for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
  //   int32_t  streamType = gVolumeData[idx].mStreamType;
  //   VolumeStreamState* streamState = mStreamStates[streamType].get();

  //   uint32_t remainingDevices = streamState->GetDevicesWithVolumeChange();
  //   for (uint32_t i = 0; remainingDevices != 0; i++) {
  //     uint32_t device = (1 << i);
  //     if ((device & remainingDevices) == 0) {
  //       continue;
  //     }
  //     remainingDevices &= ~device;
  //     if (!mAudioDeviceTableIdMaps.Get(device, nullptr)) {
  //       continue;
  //     }
  //     volume = streamState->GetVolumeIndex(device);
  //     value.setInt32(volume);
  //     lock->Set(AppendDeviceToVolumeSetting(gVolumeData[idx].mChannelName,
  //                                           device).get(),
  //               value, nullptr, nullptr);
  //   }
  // }

  // // Clear changed flags
  // for (uint32_t idx = 0; idx < MOZ_ARRAY_LENGTH(gVolumeData); ++idx) {
  //   int32_t  streamType = gVolumeData[idx].mStreamType;
  //   mStreamStates[streamType]->ClearDevicesChanged();
  //   mStreamStates[streamType]->ClearDevicesWithVolumeChange();
  // }
}

void AudioManager::UpdateCachedActiveDevicesForStreams() {
  // This function updates cached active devices for streams.
  // It is used for optimization of GetDevicesForStream() since L.
  // AudioManager could know when active devices
  // are changed in AudioPolicyManager by onAudioPortListUpdate().
  // Except it, AudioManager normally do not need to ask AuidoPolicyManager
  // about current active devices of streams and could use cached values.
  // Before L, onAudioPortListUpdate() does not exist and GetDevicesForStream()
  // does not use the cache. Therefore this function do nothing.
  for (int32_t streamType = 0; streamType < AUDIO_STREAM_CNT; streamType++) {
    // Update cached active devices of stream
    mStreamStates[streamType]->IsDevicesChanged(false /* aFromCache */);
  }
}

uint32_t AudioManager::GetDevicesForStream(int32_t aStream, bool aFromCache) {
  // Since Lollipop, devices update could be notified by AudioPortCallback.
  // Cached values can be used if there is no update.
  if (aFromCache) {
    return mStreamStates[aStream]->GetLastDevices();
  }

  audio_devices_t devices = AudioSystem::getDevicesForStream(
      static_cast<audio_stream_type_t>(aStream));

  return static_cast<uint32_t>(devices);
}

uint32_t AudioManager::GetDeviceForStream(int32_t aStream) {
  uint32_t devices =
      GetDevicesForStream(static_cast<audio_stream_type_t>(aStream));
  uint32_t device = SelectDeviceFromDevices(devices);
  return device;
}

#ifdef PRODUCT_MANUFACTURER_SPRD
uint32_t AudioManager::GetDeviceForSprdFm() {
  uint32_t device = AUDIO_POLICY_FORCE_SPEAKER;

  int32_t force = AudioSystem::getForceUse(AUDIO_POLICY_FORCE_FOR_FM);
  if (force == AUDIO_POLICY_FORCE_SPEAKER) {
    device = AUDIO_DEVICE_OUT_SPEAKER;
  } else {
    if (mConnectedDevices.Get(AUDIO_DEVICE_OUT_WIRED_HEADSET, nullptr)) {
      device = AUDIO_DEVICE_OUT_WIRED_HEADSET;
    } else if (mConnectedDevices.Get(AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
                                     nullptr)) {
      device = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
    }
  }

  return device;
}
#endif

/* static */
uint32_t AudioManager::SelectDeviceFromDevices(uint32_t aOutDevices) {
  uint32_t device = aOutDevices;

  // Consider force use speaker case.
  int32_t force = AudioSystem::getForceUse(
      (audio_policy_force_use_t)nsIAudioManager::USE_MEDIA);
  if (force == nsIAudioManager::FORCE_SPEAKER) {
    device = AUDIO_DEVICE_OUT_SPEAKER;
  }

  // See android AudioService.getDeviceForStream().
  // AudioPolicyManager expects it.
  // See also android AudioPolicyManager::getDeviceForVolume().
  if ((device & (device - 1)) != 0) {
    // Multiple device selection.
    if ((device & AUDIO_DEVICE_OUT_SPEAKER) != 0) {
      device = AUDIO_DEVICE_OUT_SPEAKER;
    } else if ((device & AUDIO_DEVICE_OUT_HDMI_ARC) != 0) {
      device = AUDIO_DEVICE_OUT_HDMI_ARC;
    } else if ((device & AUDIO_DEVICE_OUT_SPDIF) != 0) {
      device = AUDIO_DEVICE_OUT_SPDIF;
    } else if ((device & AUDIO_DEVICE_OUT_AUX_LINE) != 0) {
      device = AUDIO_DEVICE_OUT_AUX_LINE;
    } else {
      device &= AUDIO_DEVICE_OUT_ALL_A2DP;
    }
  }
  MOZ_ASSERT(audio_is_output_device(device));
  return device;
}

AudioManager::VolumeStreamState::VolumeStreamState(AudioManager& aManager,
                                                   int32_t aStreamType)
    : mManager(aManager), mStreamType(aStreamType) {
  switch (mStreamType) {
    case AUDIO_STREAM_SYSTEM:
    case AUDIO_STREAM_RING:
    case AUDIO_STREAM_NOTIFICATION:
    case AUDIO_STREAM_ALARM:
    case AUDIO_STREAM_ENFORCED_AUDIBLE:
      mIsDeviceSpecificVolume = false;
      break;
    default:
      break;
  }

  InitStreamVolume();
}

bool AudioManager::VolumeStreamState::IsDevicesChanged(bool aFromCache) {
  uint32_t devices = mManager.GetDevicesForStream(mStreamType, aFromCache);
  if (devices != mLastDevices) {
    mLastDevices = devices;
    mIsDevicesChanged = true;
  }
  return mIsDevicesChanged;
}

void AudioManager::VolumeStreamState::ClearDevicesChanged() {
  mIsDevicesChanged = false;
}

void AudioManager::VolumeStreamState::ClearDevicesWithVolumeChange() {
  mDevicesWithVolumeChange = 0;
}

uint32_t AudioManager::VolumeStreamState::GetDevicesWithVolumeChange() {
  return mDevicesWithVolumeChange;
}

void AudioManager::VolumeStreamState::InitStreamVolume() {
  AudioSystem::initStreamVolume(static_cast<audio_stream_type_t>(mStreamType),
                                0, GetMaxIndex());
}

uint32_t AudioManager::VolumeStreamState::GetMaxIndex() {
  return sMaxStreamVolumeTbl[mStreamType];
}

uint32_t AudioManager::VolumeStreamState::GetDefaultIndex() {
  return sDefaultStreamVolumeTbl[mStreamType];
}

uint32_t AudioManager::VolumeStreamState::GetVolumeIndex() {
  uint32_t device = mManager.GetDeviceForStream(mStreamType);
  return GetVolumeIndex(device);
}

uint32_t AudioManager::VolumeStreamState::GetVolumeIndex(uint32_t aDevice) {
  uint32_t index = 0;
  bool ret = mVolumeIndexes.Get(aDevice, &index);
  if (!ret) {
    index = mVolumeIndexes.Get(AUDIO_DEVICE_OUT_DEFAULT);
  }
  return index;
}

nsresult AudioManager::VolumeStreamState::SetVolumeIndexToActiveDevices(
    uint32_t aIndex) {
  uint32_t device = mManager.GetDeviceForStream(mStreamType);

  // Update volume index for device
  uint32_t oldVolumeIndex = 0;
  bool exist = mVolumeIndexes.Get(device, &oldVolumeIndex);
  if (exist && aIndex == oldVolumeIndex) {
    // No update
    return NS_OK;
  }

  // AudioPolicyManager::setStreamVolumeIndex() set volumes of all active
  // devices for stream.
  nsresult rv;
  rv = SetVolumeIndexToConsistentDeviceIfNeeded(aIndex, device);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult AudioManager::VolumeStreamState::SetVolumeIndexToAliasStreams(
    uint32_t aIndex, uint32_t aDevice) {
  uint32_t oldVolumeIndex = 0;
  bool exist = mVolumeIndexes.Get(aDevice, &oldVolumeIndex);
  if (exist && aIndex == oldVolumeIndex) {
    // No update
    return NS_OK;
  }

  nsresult rv = SetVolumeIndexToConsistentDeviceIfNeeded(aIndex, aDevice);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (int32_t streamType = 0; streamType < AUDIO_STREAM_CNT; streamType++) {
    if ((streamType != mStreamType) &&
        sStreamVolumeAliasTbl[streamType] == mStreamType) {
      // Rescaling of index is not necessary.
      rv = mManager.mStreamStates[streamType]->SetVolumeIndexToAliasStreams(
          aIndex, aDevice);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }
    }
  }

  return NS_OK;
}

nsresult
AudioManager::VolumeStreamState::SetVolumeIndexToConsistentDeviceIfNeeded(
    uint32_t aIndex, uint32_t aDevice) {
  nsresult rv;

  if (!IsDeviceSpecificVolume()) {
    rv = SetVolumeIndex(aIndex, AUDIO_DEVICE_OUT_DEFAULT);
  } else if (aDevice == AUDIO_DEVICE_OUT_SPEAKER ||
             aDevice == AUDIO_DEVICE_OUT_EARPIECE) {
    // Set AUDIO_DEVICE_OUT_SPEAKER and AUDIO_DEVICE_OUT_EARPIECE to same
    // volume.
    rv = SetVolumeIndex(aIndex, AUDIO_DEVICE_OUT_SPEAKER);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    rv = SetVolumeIndex(aIndex, AUDIO_DEVICE_OUT_EARPIECE);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else {
    // No alias device
    rv = SetVolumeIndex(aIndex, aDevice);
  }
  return rv;
}

nsresult AudioManager::VolumeStreamState::SetVolumeIndex(uint32_t aIndex,
                                                         uint32_t aDevice,
                                                         bool aUpdateCache) {
  android::status_t rv;
  if (aUpdateCache) {
    mVolumeIndexes.Put(aDevice, aIndex);
    mDevicesWithVolumeChange |= aDevice;
  }

  rv = AudioSystem::setStreamVolumeIndex(
      static_cast<audio_stream_type_t>(mStreamType), aIndex, aDevice);

  // when changing music volume,  also set FMradio volume.Just for SPRD FMradio.
  if ((AUDIO_STREAM_MUSIC == mStreamType) && mManager.IsFmOutConnected()) {
    mManager.SetVendorFmVolumeIndex(false);
  }

  return rv ? NS_ERROR_FAILURE : NS_OK;
}

void AudioManager::VolumeStreamState::RestoreVolumeIndexToAllDevices() {
  for (auto iter = mVolumeIndexes.Iter(); !iter.Done(); iter.Next()) {
    const uint32_t& key = iter.Key();
    uint32_t& index = iter.Data();
    SetVolumeIndex(key, index, /* aUpdateCache */ false);
  }
}

NS_IMETHODIMP
AudioManager::SetHacMode(bool aHacMode) {
  if (aHacMode) {
    SetParameters("HACSetting=ON");
  } else {
    SetParameters("HACSetting=OFF");
  }
  return NS_OK;
}

static nsresult SetAudioSystemParameters(
    audio_io_handle_t aIoHandle, const android::String8& aKeyValuePairs) {
  LOG("Set audio system parameter: %s", aKeyValuePairs.string());
  android::status_t status =
      AudioSystem::setParameters(aIoHandle, aKeyValuePairs);
  if (status != android::OK) {
    LOG("Failed to set parameter: %s, error status: %d",
        aKeyValuePairs.string(), status);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
AudioManager::SetParameters(const nsACString& aKeyValuePairs) {
  android::String8 cmd(aKeyValuePairs.Data(), aKeyValuePairs.Length());
  return SetAudioSystemParameters(0, cmd);
}

nsresult AudioManager::SetParameters(const char* aFormat, ...) {
  va_list args;
  va_start(args, aFormat);
  android::String8 cmd;
  android::status_t status = cmd.appendFormatV(aFormat, args);
  va_end(args);

  if (status != android::OK) {
    LOG("Invalid parameter, error status: %d", status);
    return NS_ERROR_FAILURE;
  }
  return SetAudioSystemParameters(0, cmd);
}

} /* namespace gonk */
} /* namespace dom */
} /* namespace mozilla */

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(AudioManager,
                                         AudioManager::GetInstance);

NS_DEFINE_NAMED_CID(NS_AUDIOMANAGER_CID);

static const mozilla::Module::CIDEntry kAudioManagerCIDs[] = {
    {&kNS_AUDIOMANAGER_CID, false, nullptr, AudioManagerConstructor},
    {nullptr}};

static const mozilla::Module::ContractIDEntry kAudioManagerContracts[] = {
    {"@mozilla.org/telephony/audiomanager;1", &kNS_AUDIOMANAGER_CID},
    {nullptr}};

extern const mozilla::Module kAudioManagerModule = {
    mozilla::Module::kVersion, kAudioManagerCIDs, kAudioManagerContracts,
    nullptr};
