/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef GonkGPSGeolocationProvider_h
#define GonkGPSGeolocationProvider_h

#include "android/hardware/gnss/1.1/IGnss.h"
#include "android/hardware/gnss/2.0/IGnss.h"
#include "android/hardware/gnss/1.0/IGnss.h"
#include "android/hardware/gnss/2.0/IAGnss.h"
#include "android/hardware/gnss/2.0/IAGnssRil.h"
#include "android/hardware/gnss/visibility_control/1.0/IGnssVisibilityControl.h"

#include "nsCOMPtr.h"
#include "nsIDOMGeoPosition.h"
#include "nsIGeolocationProvider.h"
#include "nsISettings.h"
#ifdef MOZ_B2G_RIL
#  include "nsIObserver.h"
#  include "nsIRadioInterfaceLayer.h"
#endif

class nsIThread;

#define GONK_GPS_GEOLOCATION_PROVIDER_CID            \
  {                                                  \
    0x48525ec5, 0x5a7f, 0x490a, {                    \
      0x92, 0x77, 0xba, 0x66, 0xe0, 0xd2, 0x2c, 0x8b \
    }                                                \
  }

#define GONK_GPS_GEOLOCATION_PROVIDER_CONTRACTID \
  "@mozilla.org/gonk-gps-geolocation-provider;1"

class GonkGPSGeolocationProvider : public nsIGeolocationProvider,
#ifdef MOZ_B2G_RIL
                                   public nsIObserver,
#endif
                                   public nsISettingsGetResponse,
                                   public nsISettingsObserver,
                                   public nsISidlDefaultResponse {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER
#ifdef MOZ_B2G_RIL
  NS_DECL_NSIOBSERVER
#endif
  NS_DECL_NSISETTINGSGETRESPONSE
  NS_DECL_NSISETTINGSOBSERVER
  NS_DECL_NSISIDLDEFAULTRESPONSE

  static already_AddRefed<GonkGPSGeolocationProvider> GetSingleton();

 private:
  class UpdateLocationEvent;
  class UpdateCapabilitiesEvent;
  friend class UpdateLocationEvent;
  friend class UpdateCapabilitiesEvent;
  friend struct GnssCallback;
  friend struct AGnssCallback_V2_0;

  /* Client should use GetSingleton() to get the provider instance. */
  GonkGPSGeolocationProvider();
  GonkGPSGeolocationProvider(const GonkGPSGeolocationProvider&);
  GonkGPSGeolocationProvider& operator=(const GonkGPSGeolocationProvider&);
  virtual ~GonkGPSGeolocationProvider();

  static GonkGPSGeolocationProvider* sSingleton;
  void Init();
  void StartGPS();
  void ShutdownGPS();

  void InjectLocation(double latitude, double longitude, float accuracy);

  NS_IMETHOD HandleSettings(nsISettingInfo* const info, bool isObserved);

#ifdef MOZ_B2G_RIL
  void UpdateRadioInterface();
  bool IsValidRilServiceId(uint32_t aServiceId);
  void SetupAGPS();
  int32_t GetDataConnectionState();
  void SetAGpsDataConn(const nsAString& aApn);
  void RequestDataConnection();
  void ReleaseDataConnection();
#endif  // MOZ_B2G_RIL

  // Whether the GPS HAL has been initialized
  bool mInitialized;
  bool mStarted;
  bool mSupportsScheduling;
  bool mSupportsSingleShot;
  bool mSupportsTimeInjection;

  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;
  nsCOMPtr<nsIThread> mInitThread;
  nsCOMPtr<nsIDOMGeoPosition> mLastGPSPosition;
  nsCOMPtr<nsIGeolocationProvider> mNetworkLocationProvider;

#ifdef MOZ_B2G_RIL
  bool mSupportsMSB;
  bool mSupportsMSA;
  uint32_t mRilDataServiceId;
  // mNumberOfRilServices indicates how many SIM slots supported on device, and
  // RadioInterfaceLayer.js takes responsibility to set up the corresponding
  // preference value.
  uint32_t mNumberOfRilServices;
  bool mObservingNetworkConnStateChange;
  nsCOMPtr<nsIRadioInterface> mRadioInterface;
#endif

  struct GnssDeathRecipient
      : virtual public android::hardware::hidl_death_recipient {
    // hidl_death_recipient interface
    virtual void serviceDied(
        uint64_t cookie,
        const android::wp<android::hidl::base::V1_0::IBase>& who) override;
  };

  void InitGnssHal();

  android::sp<GnssDeathRecipient> mGnssHalDeathRecipient;
  android::sp<android::hardware::gnss::V1_0::IGnss> mGnssHal;
  android::sp<android::hardware::gnss::V1_1::IGnss> mGnssHal_V1_1;
  android::sp<android::hardware::gnss::V2_0::IGnss> mGnssHal_V2_0;
  android::sp<android::hardware::gnss::V2_0::IAGnss> mAGnssHal_V2_0;
  android::sp<android::hardware::gnss::V2_0::IAGnssRil> mAGnssRilHal_V2_0;
  android::sp<
      android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl>
      mGnssVisibilityControlHal;

  class NetworkLocationUpdate : public nsIGeolocationUpdate {
   public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONUPDATE

    NetworkLocationUpdate() {}

   private:
    virtual ~NetworkLocationUpdate() {}
  };
};

#endif /* GonkGPSGeolocationProvider_h */
