/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 * (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved.
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

#if ANDROID_VERSION >= 27
#  include "android/hardware/gnss/1.0/IGnss.h"
#else
#  include <hardware/gps.h>  // for GpsInterface
#endif

#include "nsCOMPtr.h"
#include "nsIDOMGeoPosition.h"
#include "nsIGeolocationProvider.h"

class nsIThread;

#define GONK_GPS_GEOLOCATION_PROVIDER_CID            \
  {                                                  \
    0x48525ec5, 0x5a7f, 0x490a, {                    \
      0x92, 0x77, 0xba, 0x66, 0xe0, 0xd2, 0x2c, 0x8b \
    }                                                \
  }

#define GONK_GPS_GEOLOCATION_PROVIDER_CONTRACTID \
  "@mozilla.org/gonk-gps-geolocation-provider;1"

class GonkGPSGeolocationProvider : public nsIGeolocationProvider {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  static already_AddRefed<GonkGPSGeolocationProvider> GetSingleton();

 private:
  class UpdateLocationEvent;
  class UpdateCapabilitiesEvent;
  friend class UpdateLocationEvent;
  friend class UpdateCapabilitiesEvent;

  /* Client should use GetSingleton() to get the provider instance. */
  GonkGPSGeolocationProvider();
  GonkGPSGeolocationProvider(const GonkGPSGeolocationProvider&);
  GonkGPSGeolocationProvider& operator=(const GonkGPSGeolocationProvider&);
  virtual ~GonkGPSGeolocationProvider();

  static GonkGPSGeolocationProvider* sSingleton;
  void Init();
  void StartGPS();
  void ShutdownGPS();

  // Whether the GPS HAL has been initialized
  bool mInitialized;
  bool mStarted;
  bool mSupportsScheduling;
  bool mSupportsSingleShot;
  bool mSupportsTimeInjection;

  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;
  nsCOMPtr<nsIThread> mInitThread;
  nsCOMPtr<nsIDOMGeoPosition> mLastGPSPosition;

#if ANDROID_VERSION >= 27  // struct GnssCallback;
  friend struct GnssCallback;

  struct GnssDeathRecipient
      : virtual public android::hardware::hidl_death_recipient {
    // hidl_death_recipient interface
    virtual void serviceDied(
        uint64_t cookie,
        const android::wp<android::hidl::base::V1_0::IBase>& who) override;
  };

  android::sp<GnssDeathRecipient> mGnssHalDeathRecipient;
  android::sp<android::hardware::gnss::V1_0::IGnss> mGnssHal;
#else
  const GpsInterface* GetGPSInterface();

  static void LocationCallback(GpsLocation* location);
  static void StatusCallback(GpsStatus* status);
  static void SvStatusCallback(GpsSvStatus* sv_info);
  static void NmeaCallback(GpsUtcTime timestamp, const char* nmea, int length);
  static void AcquireWakelockCallback();
  static void ReleaseWakelockCallback();
  static void SetCapabilitiesCallback(uint32_t capabilities);
  static pthread_t CreateThreadCallback(const char* name, void (*start)(void*),
                                        void* arg);
  static void RequestUtcTimeCallback();

  const GpsInterface* mGpsInterface = nullptr;
  static GpsCallbacks mCallbacks;
#endif
};

#endif /* GonkGPSGeolocationProvider_h */
