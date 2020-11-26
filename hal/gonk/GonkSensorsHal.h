/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAL_GONK_GONKSENSORSHAL_H_
#define HAL_GONK_GONKSENSORSHAL_H_

#include "base/thread.h"
#include "HalSensor.h"

#include "android/hardware/sensors/1.0/ISensors.h"
#include "android/hardware/sensors/1.0/types.h"

using namespace mozilla::hal;
namespace hidl_sensors = android::hardware::sensors::V1_0;
using hidl_sensors::SensorFlagBits;

namespace mozilla {
namespace hal_impl {

typedef void (* SensorDataCallback)(const SensorData& aSensorData);

class GonkSensorsHal {
public:
  static GonkSensorsHal* GetInstance() {
    if (sInstance == nullptr) {
      sInstance = new GonkSensorsHal();
    }
    return sInstance;
  };

  bool RegisterSensorDataCallback(const SensorDataCallback aCallback);
  bool ActivateSensor(const SensorType aSensorType);
  bool DeactivateSensor(const SensorType aSensorType);
private:
  class SensorDataNotifier;

  static GonkSensorsHal* sInstance;

  GonkSensorsHal()
    : mSensors(nullptr),
      mPollingThread(nullptr),
      mSensorDataCallback(nullptr) {
        memset(mSensorInfoList, 0, sizeof(mSensorInfoList));
        Init();
  };
  ~GonkSensorsHal() {};

  void Init();
  void StartPollingThread();
  SensorData CreateSensorData(const hidl_sensors::Event aEvent);

  android::sp<hidl_sensors::ISensors> mSensors;
  hidl_sensors::SensorInfo mSensorInfoList[NUM_SENSOR_TYPE];
  base::Thread* mPollingThread;
  SensorDataCallback mSensorDataCallback;

  const int64_t kDefaultSamplingPeriodNs = 200000000;
  const int64_t kPressureSamplingPeriodNs = 1000000000;
  const int64_t kReportLatencyNs = 0;
};

GonkSensorsHal* GonkSensorsHal::sInstance = nullptr;


} // hal_impl
} // mozilla

#endif // HAL_GONK_GONKSENSORSHAL_H_