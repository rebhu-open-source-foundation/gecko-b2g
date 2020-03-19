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

#include "GonkGPSGeolocationProvider.h"

#include "GeolocationUtil.h"
#include "hardware_legacy/power.h"
#include "mozilla/dom/GeolocationPosition.h"
#include "mozilla/Preferences.h"
#include "nsComponentManagerUtils.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "nsIURLFormatter.h"
#include "prtime.h"  // for PR_Now()

#undef LOG
#undef ERR
#undef DBG
#define LOG(args...) \
  __android_log_print(ANDROID_LOG_INFO, "GonkGPS_GEO", ##args)
#define ERR(args...) \
  __android_log_print(ANDROID_LOG_ERROR, "GonkGPS_GEO", ##args)
#define DBG(args...)                                                 \
  do {                                                               \
    if (gDebug_isLoggingEnabled) {                                   \
      __android_log_print(ANDROID_LOG_DEBUG, "GonkGPS_GEO", ##args); \
    }                                                                \
  } while (0)

using namespace mozilla;
using namespace mozilla::dom;

using android::hardware::Return;
using android::hardware::Void;

using IGnss_V1_0 = android::hardware::gnss::V1_0::IGnss;
using IGnssCallback_V1_0 = android::hardware::gnss::V1_0::IGnssCallback;
using GnssLocation_V1_0 = android::hardware::gnss::V1_0::GnssLocation;

using android::hardware::hidl_vec;
using android::hardware::gnss::V2_0::IGnssCallback;
using IGnss_V1_1 = android::hardware::gnss::V1_1::IGnss;
using IGnss_V2_0 = android::hardware::gnss::V2_0::IGnss;
using GnssLocation_V2_0 = android::hardware::gnss::V2_0::GnssLocation;

// GnssCallback class implements the callback methods for IGnss interface.
struct GnssCallback : public IGnssCallback {
  Return<void> gnssLocationCb(const GnssLocation_V1_0& location) override;
  Return<void> gnssStatusCb(
      const IGnssCallback_V1_0::GnssStatusValue status) override;
  Return<void> gnssSvStatusCb(
      const IGnssCallback_V1_0::GnssSvStatus& svStatus) override;
  Return<void> gnssNmeaCb(int64_t timestamp,
                          const android::hardware::hidl_string& nmea) override;
  Return<void> gnssSetCapabilitesCb(uint32_t capabilities) override;
  Return<void> gnssAcquireWakelockCb() override;
  Return<void> gnssReleaseWakelockCb() override;
  Return<void> gnssRequestTimeCb() override;
  Return<void> gnssSetSystemInfoCb(
      const IGnssCallback::GnssSystemInfo& info) override;

  // New in gnss 1.1
  Return<void> gnssNameCb(const android::hardware::hidl_string& name) override;
  Return<void> gnssRequestLocationCb(const bool independentFromGnss) override;

  // New in gnss 2.0
  Return<void> gnssRequestLocationCb_2_0(const bool independentFromGnss,
                                         const bool isUserEmergency) override;
  Return<void> gnssSetCapabilitiesCb_2_0(uint32_t capabilities) override;
  Return<void> gnssLocationCb_2_0(const GnssLocation_V2_0& location) override;
  Return<void> gnssSvStatusCb_2_0(
      const hidl_vec<IGnssCallback::GnssSvInfo>& svInfoList) override;
};

static const int kDefaultPeriod = 1000;  // ms

static bool gDebug_isLoggingEnabled = false;

// Clean up GPS HAL when Geolocation setting is turned off.
static const char* kPrefOndemandCleanup = "geo.provider.ondemand_cleanup";

static const char* kWakeLockName = "GeckoGPS";

NS_IMPL_ISUPPORTS(GonkGPSGeolocationProvider::NetworkLocationUpdate,
                  nsIGeolocationUpdate)

void GonkGPSGeolocationProvider::InitGnssHal() {
  mGnssHal_V2_0 = IGnss_V2_0::getService();
  if (mGnssHal_V2_0 != nullptr) {
    mGnssHal = mGnssHal_V2_0;
    mGnssHal_V1_1 = mGnssHal_V2_0;
    return;
  }

  LOG("gnssHal 2.0 was null, trying 1.1");
  mGnssHal_V1_1 = IGnss_V1_1::getService();
  if (mGnssHal_V1_1 != nullptr) {
    mGnssHal = mGnssHal_V1_1;
    return;
  }

  LOG("gnssHal 1.1 was null, trying 1.0");
  mGnssHal = IGnss_V1_0::getService();
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::NetworkLocationUpdate::Update(
    nsIDOMGeoPosition* position) {
  RefPtr<GonkGPSGeolocationProvider> provider =
      GonkGPSGeolocationProvider::GetSingleton();

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  position->GetCoords(getter_AddRefs(coords));
  if (!coords) {
    return NS_ERROR_FAILURE;
  }

  double lat, lon, acc;
  coords->GetLatitude(&lat);
  coords->GetLongitude(&lon);
  coords->GetAccuracy(&acc);

  double delta = -1.0;

  static double sLastMLSPosLat = 0.0;
  static double sLastMLSPosLon = 0.0;

  if (0 != sLastMLSPosLon || 0 != sLastMLSPosLat) {
    delta = CalculateDeltaInMeter(lat, lon, sLastMLSPosLat, sLastMLSPosLon);
  }

  sLastMLSPosLat = lat;
  sLastMLSPosLon = lon;

  // if the MLS coord change is smaller than this arbitrarily small value
  // assume the MLS coord is unchanged, and stick with the GPS location
  const double kMinMLSCoordChangeInMeters = 10.0;

  DOMTimeStamp time_ms = 0;
  if (provider->mLastGPSPosition) {
    provider->mLastGPSPosition->GetTimestamp(&time_ms);
  }
  const int64_t diff_ms = (PR_Now() / PR_USEC_PER_MSEC) - time_ms;

  // We want to distinguish between the GPS being inactive completely
  // and temporarily inactive. In the former case, we would use a low
  // accuracy network location; in the latter, we only want a network
  // location that appears to updating with movement.

  const bool isGPSFullyInactive = diff_ms > 1000 * 60 * 2;  // two mins
  const bool isGPSTempInactive = diff_ms > 1000 * 10;       // 10 secs

  if (provider->mLocationCallback) {
    if (isGPSFullyInactive ||
        (isGPSTempInactive && delta > kMinMLSCoordChangeInMeters)) {
      DBG("Using MLS, GPS age:%fs, MLS Delta:%fm", diff_ms / 1000.0, delta);
      provider->mLocationCallback->Update(position);
    } else if (provider->mLastGPSPosition) {
      DBG("Using old GPS age:%fs", diff_ms / 1000.0);

      // This is a fallback case so that the GPS provider responds with its last
      // location rather than waiting for a more recent GPS or network location.
      // The service decides if the location is too old, not the provider.
      provider->mLocationCallback->Update(provider->mLastGPSPosition);
    }
  }

  provider->InjectLocation(lat, lon, acc);

  if (provider->mLocationCallback) {
    LOG("Got network position");
    provider->mLocationCallback->Update(position);
  }

  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::NetworkLocationUpdate::NotifyError(uint16_t error) {
  return NS_OK;
}

// While most methods of GonkGPSGeolocationProvider should only be
// called from main thread, we deliberately put the Init and ShutdownGPS
// methods off main thread to avoid blocking.
NS_IMPL_ISUPPORTS(GonkGPSGeolocationProvider, nsIGeolocationProvider)

/* static */ GonkGPSGeolocationProvider*
    GonkGPSGeolocationProvider::sSingleton = nullptr;

GonkGPSGeolocationProvider::GonkGPSGeolocationProvider()
    : mInitialized(false),
      mStarted(false),
      mSupportsScheduling(false),
      mSupportsSingleShot(false),
      mSupportsTimeInjection(false) {
  MOZ_ASSERT(NS_IsMainThread());
}

GonkGPSGeolocationProvider::~GonkGPSGeolocationProvider() {
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mStarted, "Must call Shutdown before destruction");

  if (mGnssHal != nullptr) {
    mGnssHal->cleanup();
  }

  sSingleton = nullptr;
}

already_AddRefed<GonkGPSGeolocationProvider>
GonkGPSGeolocationProvider::GetSingleton() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sSingleton) sSingleton = new GonkGPSGeolocationProvider();

  RefPtr<GonkGPSGeolocationProvider> provider = sSingleton;
  return provider.forget();
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Startup() {
  MOZ_ASSERT(NS_IsMainThread());

  if (mStarted) {
    return NS_OK;
  }

  // Setup NetworkLocationProvider if the API key and server URI are available
  nsAutoString serverUri;
  nsresult rv = Preferences::GetString("geo.wifi.uri", serverUri);
  if (NS_SUCCEEDED(rv) && !serverUri.IsEmpty()) {
    // nsresult rv;
    nsCOMPtr<nsIURLFormatter> formatter =
        do_CreateInstance("@mozilla.org/toolkit/URLFormatterService;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      nsString key;
      rv = formatter->FormatURLPref(NS_LITERAL_STRING("geo.fauthorization.key"),
                                    key);
      if (NS_SUCCEEDED(rv) && !key.IsEmpty()) {
        mNetworkLocationProvider =
            do_CreateInstance("@mozilla.org/geolocation/mls-provider;1");
        if (mNetworkLocationProvider) {
          rv = mNetworkLocationProvider->Startup();
          if (NS_SUCCEEDED(rv)) {
            RefPtr<NetworkLocationUpdate> update = new NetworkLocationUpdate();
          }
        }
      }
    }
  }

  if (mInitialized) {
    RefPtr<GonkGPSGeolocationProvider> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction(
        "GonkGPSGeolocationProvider::Startup", [self]() { self->StartGPS(); });
    NS_DispatchToMainThread(r);
  } else {
    // Initialize GPS HAL first and it would start GPS after then.
    if (!mInitThread) {
      nsresult rv = NS_NewNamedThread("GPS Init", getter_AddRefs(mInitThread));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    RefPtr<GonkGPSGeolocationProvider> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction(
        "GonkGPSGeolocationProvider::Startup", [self]() { self->Init(); });

    mInitThread->Dispatch(r, NS_DISPATCH_NORMAL);
  }

  mStarted = true;
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Shutdown() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!mStarted) {
    return NS_OK;
  }

  mStarted = false;

  if (mNetworkLocationProvider) {
    mNetworkLocationProvider->Shutdown();
    mNetworkLocationProvider = nullptr;
  }

  RefPtr<GonkGPSGeolocationProvider> self = this;
  nsCOMPtr<nsIRunnable> r =
      NS_NewRunnableFunction("GonkGPSGeolocationProvider::Shutdown",
                             [self]() { self->ShutdownGPS(); });
  mInitThread->Dispatch(r, NS_DISPATCH_NORMAL);

  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Watch(nsIGeolocationUpdate* aCallback) {
  MOZ_ASSERT(NS_IsMainThread());

  mLocationCallback = aCallback;
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::SetHighAccuracy(bool) { return NS_OK; }

void GonkGPSGeolocationProvider::GnssDeathRecipient::serviceDied(
    uint64_t cookie, const android::wp<android::hidl::base::V1_0::IBase>& who) {
  ERR("Abort due to IGNSS hidl service failure.");
}

void GonkGPSGeolocationProvider::Init() {
  // Must not be main thread. Some GPS driver's first init takes very long.
  MOZ_ASSERT(!NS_IsMainThread());

  InitGnssHal();
  if (mGnssHal == nullptr) {
    ERR("Failed to get Gnss HAL.");
    return;
  }

  mGnssHalDeathRecipient = new GnssDeathRecipient();
  Return<bool> linked =
      mGnssHal->linkToDeath(mGnssHalDeathRecipient, /*cookie*/ 0);
  if (!linked.isOk()) {
    ERR("Transaction error in linking to GnssHAL death: %s",
        linked.description().c_str());
  } else if (!linked) {
    ERR("Unable to link to GnssHal death notifications");
  } else {
    ERR("Link to death notification successful");
  }

  android::sp<IGnssCallback> gnssCbIface = new GnssCallback();

  Return<bool> result = false;
  if (mGnssHal_V2_0 != nullptr) {
    result = mGnssHal_V2_0->setCallback_2_0(gnssCbIface);
  } else if (mGnssHal_V1_1 != nullptr) {
    result = mGnssHal_V1_1->setCallback_1_1(gnssCbIface);
  } else {
    result = mGnssHal->setCallback(gnssCbIface);
  }

  if (!result.isOk() || !result) {
    ERR("SetCallback for Gnss Interface fails");
    return;
  }

  mInitialized = true;

  LOG("GPS HAL has been initialized");

  // If there is an ongoing location request, starts GPS navigating
  if (mStarted) {
    RefPtr<GonkGPSGeolocationProvider> self = this;
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction(
        "GonkGPSGeolocationProvider::Init", [self]() { self->StartGPS(); });
    NS_DispatchToMainThread(r);
  }
}

void GonkGPSGeolocationProvider::StartGPS() {
  MOZ_ASSERT(NS_IsMainThread());
  LOG("Starts GPS");

  int32_t update = Preferences::GetInt("geo.default.update", kDefaultPeriod);
  if (!mSupportsScheduling) {
    update = kDefaultPeriod;
  }

  MOZ_ASSERT(mGnssHal != nullptr);

  Return<bool> result = false;
  if (mGnssHal_V1_1 != nullptr) {
    result = mGnssHal_V1_1->setPositionMode_1_1(
        IGnss_V1_1::GnssPositionMode::STANDALONE,
        IGnss_V1_1::GnssPositionRecurrence::RECURRENCE_PERIODIC, update,
        0,       // preferred accuracy
        0,       // preferred time
        false);  // low power mode
  } else if (mGnssHal != nullptr) {
    result = mGnssHal->setPositionMode(
        IGnss_V1_0::GnssPositionMode::STANDALONE,
        IGnss_V1_0::GnssPositionRecurrence::RECURRENCE_PERIODIC, update,
        0,   // preferred accuracy
        0);  // preferred time
  }

  if (!result.isOk()) {
    ERR("failed to set IGnss position mode");
    return;
  }
  result = mGnssHal->start();
  if (!result.isOk()) {
    ERR("failed to start IGnss HAL");
  }
}

void GonkGPSGeolocationProvider::ShutdownGPS() {
  MOZ_ASSERT(!mStarted, "Should only be called after Shutdown");

  LOG("Stops GPS");

  if (mGnssHal != nullptr) {
    auto result = mGnssHal->stop();
    if (!result.isOk()) {
      ERR("failed to stop IGnss HAL");
    }
  }
}

void GonkGPSGeolocationProvider::InjectLocation(double latitude,
                                                double longitude,
                                                float accuracy) {
  MOZ_ASSERT(NS_IsMainThread());

  DBG("injecting location (%f, %f) accuracy: %f", latitude, longitude,
      accuracy);

  if (mGnssHal != nullptr) {
    auto result = mGnssHal->injectLocation(latitude, longitude, accuracy);
    if (!result.isOk() || !result) {
      ERR("%s: Gnss injectLocation() failed");
    }
  }
}

class GonkGPSGeolocationProvider::UpdateLocationEvent final
    : public mozilla::Runnable {
 public:
  UpdateLocationEvent(nsGeoPosition* aPosition)
      : mozilla::Runnable("UpdateLocationEvent"), mPosition(aPosition) {}
  NS_IMETHOD Run() {
    RefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();
    nsCOMPtr<nsIGeolocationUpdate> callback = provider->mLocationCallback;
    provider->mLastGPSPosition = mPosition;
    if (callback) {
      callback->Update(mPosition);
    }
    return NS_OK;
  }

 private:
  RefPtr<nsGeoPosition> mPosition;
};

class GonkGPSGeolocationProvider::UpdateCapabilitiesEvent final
    : public mozilla::Runnable {
 public:
  UpdateCapabilitiesEvent(uint32_t aCapabilities)
      : mozilla::Runnable("UpdateCapabilitiesEvent"),
        mCapabilities(aCapabilities) {}
  NS_IMETHOD Run() {
    RefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();

    provider->mSupportsScheduling =
        mCapabilities & IGnssCallback::Capabilities::SCHEDULING;
    provider->mSupportsSingleShot =
        mCapabilities & IGnssCallback::Capabilities::SINGLE_SHOT;
    provider->mSupportsTimeInjection =
        mCapabilities & IGnssCallback::Capabilities::ON_DEMAND_TIME;

    return NS_OK;
  }

 private:
  uint32_t mCapabilities;
};

Return<void> GnssCallback::gnssLocationCb(const GnssLocation_V1_0& location) {
  const float kImpossibleAccuracy_m = 0.001;
  if (location.horizontalAccuracyMeters < kImpossibleAccuracy_m) {
    return Void();
  }

  RefPtr<nsGeoPosition> somewhere = new nsGeoPosition(
      location.latitudeDegrees, location.longitudeDegrees,
      location.altitudeMeters, location.horizontalAccuracyMeters,
      location.verticalAccuracyMeters, location.bearingDegrees,
      location.speedMetersPerSec, PR_Now() / PR_USEC_PER_MSEC);
  // Note above: Can't use location->timestamp as the time from the satellite is
  // a minimum of 16 secs old (see http://leapsecond.com/java/gpsclock.htm). All
  // code from this point on expects the gps location to be timestamped with the
  // current time, most notably: the geolocation service which respects
  // maximumAge set in the DOM JS.

  DBG("geo: GPS got a fix (%f, %f). accuracy: %f", location.latitudeDegrees,
      location.longitudeDegrees, location.horizontalAccuracyMeters);

  nsCOMPtr<nsIRunnable> runnable =
      new GonkGPSGeolocationProvider::UpdateLocationEvent(somewhere);
  NS_DispatchToMainThread(runnable);

  return Void();
}

Return<void> GnssCallback::gnssStatusCb(
    const IGnssCallback_V1_0::GnssStatusValue status) {
  const char* msgStream = 0;
  switch (status) {
    case IGnssCallback::GnssStatusValue::NONE:
      msgStream = "geo: GnssStatusValue::NONE\n";
      break;
    case IGnssCallback::GnssStatusValue::SESSION_BEGIN:
      msgStream = "geo: GnssStatusValue::SESSION_BEGIN\n";
      break;
    case IGnssCallback::GnssStatusValue::SESSION_END:
      msgStream = "geo: GnssStatusValue::SESSION_END\n";
      break;
    case IGnssCallback::GnssStatusValue::ENGINE_ON:
      msgStream = "geo: GnssStatusValue::ENGINE_ON\n";
      break;
    case IGnssCallback::GnssStatusValue::ENGINE_OFF:
      msgStream = "geo: GnssStatusValue::ENGINE_OFF\n";
      break;
    default:
      msgStream = "geo: Unknown GNSS status\n";
      break;
  }
  DBG("%s", msgStream);
  return Void();
}

Return<void> GnssCallback::gnssSvStatusCb(
    const IGnssCallback_V1_0::GnssSvStatus& svStatus) {
  DBG("%s: numSvs: %u", __FUNCTION__, svStatus.numSvs);
  return Void();
}

Return<void> GnssCallback::gnssNmeaCb(
    int64_t timestamp, const ::android::hardware::hidl_string& nmea) {
  DBG("%s: timestamp: %ld", __FUNCTION__, timestamp);
  return Void();
}

Return<void> GnssCallback::gnssSetCapabilitesCb(uint32_t capabilities) {
  DBG("%s: capabilities: %u", __FUNCTION__, capabilities);
  NS_DispatchToMainThread(
      new GonkGPSGeolocationProvider::UpdateCapabilitiesEvent(capabilities));
  return Void();
}

Return<void> GnssCallback::gnssAcquireWakelockCb() {
  acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLockName);
  return Void();
}

Return<void> GnssCallback::gnssReleaseWakelockCb() {
  release_wake_lock(kWakeLockName);
  return Void();
}

Return<void> GnssCallback::gnssRequestTimeCb() {
  LOG("%s: Gonk doesn't support time injection.", __FUNCTION__);
  return Void();
}

Return<void> GnssCallback::gnssSetSystemInfoCb(
    const IGnssCallback::GnssSystemInfo& info) {
  DBG("%s: yearOfHw: %u", __FUNCTION__, info.yearOfHw);
  return Void();
}

Return<void> GnssCallback::gnssNameCb(
    const android::hardware::hidl_string& name) {
  LOG("%s: name=%s", __FUNCTION__, name.c_str());
  return Void();
}

Return<void> GnssCallback::gnssRequestLocationCb(
    const bool independentFromGnss) {
  LOG("%s: Gonk doesn't support gnssRequestLocationCb.", __FUNCTION__);
  return Void();
}

Return<void> GnssCallback::gnssRequestLocationCb_2_0(
    const bool independentFromGnss, const bool isUserEmergency) {
  LOG("%s: Gonk doesn't support gnssRequestLocationCb_2_0.", __FUNCTION__);
  return Void();
}

Return<void> GnssCallback::gnssSetCapabilitiesCb_2_0(uint32_t capabilities) {
  return gnssSetCapabilitesCb(capabilities);
}

Return<void> GnssCallback::gnssLocationCb_2_0(
    const GnssLocation_V2_0& location) {
  return gnssLocationCb(location.v1_0);
}

Return<void> GnssCallback::gnssSvStatusCb_2_0(
    const hidl_vec<IGnssCallback::GnssSvInfo>& svInfoList) {
  DBG("%s: numSvs: %u", __FUNCTION__, svInfoList.size());
  return Void();
}
