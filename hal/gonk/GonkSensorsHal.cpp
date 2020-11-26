/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/thread.h"
#include "base/task.h"

#include "Hal.h"
#include "HalLog.h"

#include "GonkSensorsHal.h"


inline double radToDeg(double aRad) {
  return aRad * (180.0 / M_PI);
}

SensorType getSensorType(hidl_sensors::SensorType aHidlSensorType) {
  switch (aHidlSensorType) {
    case hidl_sensors::SensorType::ORIENTATION:
      return SENSOR_ORIENTATION;
    case hidl_sensors::SensorType::ACCELEROMETER:
      return SENSOR_ACCELERATION;
    case hidl_sensors::SensorType::PROXIMITY:
      return SENSOR_PROXIMITY;
    case hidl_sensors::SensorType::LIGHT:
      return SENSOR_LIGHT;
    case hidl_sensors::SensorType::GYROSCOPE:
      return SENSOR_GYROSCOPE;
    case hidl_sensors::SensorType::LINEAR_ACCELERATION:
      return SENSOR_LINEAR_ACCELERATION;
    case hidl_sensors::SensorType::ROTATION_VECTOR:
      return SENSOR_ROTATION_VECTOR;
    case hidl_sensors::SensorType::GAME_ROTATION_VECTOR:
      return SENSOR_GAME_ROTATION_VECTOR;
    case hidl_sensors::SensorType::PRESSURE:
      return SENSOR_PRESSURE;
    default:
      return SENSOR_UNKNOWN;
  }
}

namespace mozilla {
namespace hal_impl {


class GonkSensorsHal::SensorDataNotifier : public Runnable {
public:
  SensorDataNotifier(const SensorData aSensorData, const SensorDataCallback aCallback)
  : mozilla::Runnable("GonkSensors::SensorDataNotifier"),
    mSensorData(aSensorData),
    mCallback(aCallback) {}

  NS_IMETHOD Run() override {
    if (mCallback) {
      mCallback(mSensorData);
    }
    return NS_OK;
  }
private:
  SensorData mSensorData;
  SensorDataCallback mCallback;
};

SensorData
GonkSensorsHal::CreateSensorData(const hidl_sensors::Event aEvent) {
  AutoTArray<float, 4> values;

  SensorType sensorType = getSensorType(aEvent.sensorType);

  if (sensorType == SENSOR_UNKNOWN) {
    return SensorData(sensorType, aEvent.timestamp, values);
  }

  switch (sensorType) {
    case SENSOR_ORIENTATION:
      // Bug 938035, convert hidl orientation sensor data to meet the earth
      // coordinate frame defined by w3c spec
      values.AppendElement(-aEvent.u.vec3.x + 360);
      values.AppendElement(-aEvent.u.vec3.y);
      values.AppendElement(-aEvent.u.vec3.z);
      break;
    case SENSOR_ACCELERATION:
    case SENSOR_LINEAR_ACCELERATION:
      values.AppendElement(aEvent.u.vec3.x);
      values.AppendElement(aEvent.u.vec3.y);
      values.AppendElement(aEvent.u.vec3.z);
      break;
    case SENSOR_PROXIMITY:
      values.AppendElement(aEvent.u.scalar);
      values.AppendElement(0);
      values.AppendElement(mSensorInfoList[sensorType].maxRange);
      break;
    case SENSOR_GYROSCOPE:
      // convert hidl gyroscope sensor data from radians per second into
      // degrees per second that defined by w3c spec
      values.AppendElement(radToDeg(aEvent.u.vec3.x));
      values.AppendElement(radToDeg(aEvent.u.vec3.y));
      values.AppendElement(radToDeg(aEvent.u.vec3.z));
      break;
    case SENSOR_LIGHT:
      values.AppendElement(aEvent.u.scalar);
      break;
    case SENSOR_ROTATION_VECTOR:
    case SENSOR_GAME_ROTATION_VECTOR:
      values.AppendElement(aEvent.u.vec4.x);
      values.AppendElement(aEvent.u.vec4.y);
      values.AppendElement(aEvent.u.vec4.z);
      values.AppendElement(aEvent.u.vec4.w);
      break;
    case SENSOR_PRESSURE:
      values.AppendElement(aEvent.u.scalar);
      break;
    case SENSOR_UNKNOWN:
    default:
      NS_ERROR("invalid sensor type");
      break;
  }

  return SensorData(sensorType, aEvent.timestamp, values);
}

void
GonkSensorsHal::StartPollingThread() {
  if (mPollingThread == nullptr) {
    mPollingThread = new base::Thread("GonkSensors::Polling");
    MOZ_ASSERT(mPollingThread);
    mPollingThread->Start();
    mPollingThread->message_loop()->PostTask(
      NS_NewRunnableFunction("GonkSensors::PollEvents", [this]() {
        do {
          const int32_t numEventMax = 16;

          // poll sensors events from sensors hidl service
          android::hardware::hidl_vec<hidl_sensors::Event> events;
          if (!mSensors->poll(numEventMax,
            [&events](auto result, const auto &aEvents, const auto &) {
              if (result == hidl_sensors::Result::OK) {
                events = aEvents;
              }
            }).isOk()) {
            continue;
          }

          // create sensor data and dispatch to main thread
          const size_t count = events.size();
          for (size_t i=0; i<count; i++) {
            SensorData sensorData;

            sensorData = CreateSensorData(events[i]);
            if (sensorData.sensor() == SENSOR_UNKNOWN) {
              continue;
            }

            NS_DispatchToMainThread(
              new SensorDataNotifier(sensorData, mSensorDataCallback));
          }
        } while (true);
      })
    );
  }
}

void
GonkSensorsHal::Init() {
  android::hardware::hidl_vec<hidl_sensors::SensorInfo> list;
  // TODO: evaluate a proper retrying times if it's not enough
  size_t retry = 2;
  while (retry-- > 0) {
    mSensors = hidl_sensors::ISensors::getService();
    if (mSensors == nullptr) {
      HAL_ERR("no sensors hidl service found");
      continue;
    }

    // poke hidl service to check if it is alive
    if (!mSensors->poll(0, [](auto, const auto &, const auto &) {}).isOk()) {
      HAL_ERR("poke sensors hidl service failed");
      // sensors hidl service will kill and restart itself when it detects
      // double connections are calling poll()
      mSensors = nullptr;
      // TODO: add a proper delay waiting for service restarting
      continue;
    }

    // obtain hidl sensors list
    if (!mSensors->getSensorsList(
      [&list](const auto &aList) { list = aList; }).isOk()) {
      HAL_ERR("get sensors list failed");
      mSensors = nullptr;
      continue;
    }
  }

  if (mSensors == nullptr) {
    HAL_ERR("Init finally failed");
    return;
  }

  // setup the available sensors list and filter out unnecessary ones
  const size_t count = list.size();
  for (size_t i=0; i<count; i++) {
    const hidl_sensors::SensorInfo sensorInfo = list[i];

    SensorType sensorType = getSensorType(sensorInfo.type);
    SensorFlagBits mode = (SensorFlagBits)(sensorInfo.flags & SensorFlagBits::MASK_REPORTING_MODE);
    bool canWakeUp = sensorInfo.flags & SensorFlagBits::WAKE_UP;
    bool isValid = false;

    // check sensor reporting mode and wake-up capability
    switch (sensorType) {
      case SENSOR_PROXIMITY:
        if (mode == SensorFlagBits::ON_CHANGE_MODE && canWakeUp) {
          isValid = true;
        }
        break;
      case SENSOR_LIGHT:
        if (mode == SensorFlagBits::ON_CHANGE_MODE && !canWakeUp) {
          isValid = true;
        }
        break;
      case SENSOR_ORIENTATION:
      case SENSOR_ACCELERATION:
      case SENSOR_GYROSCOPE:
      case SENSOR_LINEAR_ACCELERATION:
      case SENSOR_ROTATION_VECTOR:
      case SENSOR_GAME_ROTATION_VECTOR:
      case SENSOR_PRESSURE:
        if (mode == SensorFlagBits::CONTINUOUS_MODE && !canWakeUp) {
          isValid = true;
        }
        break;
      default:
        break;
    }

    if (isValid) {
      mSensorInfoList[sensorType] = sensorInfo;
      HAL_LOG("a valid sensor type=%d handle=%d is initialized", sensorType, sensorInfo.sensorHandle);
    }
  }

  // start a polling thread reading sensors events
  StartPollingThread();

  HAL_LOG("sensors init completed");
}

bool
GonkSensorsHal::RegisterSensorDataCallback(const SensorDataCallback aCallback) {
  mSensorDataCallback = aCallback;
  return true;
};

bool
GonkSensorsHal::ActivateSensor(const SensorType aSensorType) {
  if (mSensors == nullptr) {
    return false;
  }

  const int32_t handle = mSensorInfoList[aSensorType].sensorHandle;

  // check if specified sensor is supported
  if (!handle) {
    HAL_LOG("device unsupported sensor aSensorType=%d", aSensorType);
    return false;
  }

  int64_t samplingPeriodNs;
  switch (aSensorType) {
    // no sampling period for on-change sensors
    case SENSOR_PROXIMITY:
    case SENSOR_LIGHT:
      samplingPeriodNs = 0;
      break;
    // specific sampling period for pressure sensor
    case SENSOR_PRESSURE:
      samplingPeriodNs = kPressureSamplingPeriodNs;
      break;
    // default sampling period for most continuous sensors
    case SENSOR_ORIENTATION:
    case SENSOR_ACCELERATION:
    case SENSOR_LINEAR_ACCELERATION:
    case SENSOR_GYROSCOPE:
    case SENSOR_ROTATION_VECTOR:
    case SENSOR_GAME_ROTATION_VECTOR:
    default:
      samplingPeriodNs = kDefaultSamplingPeriodNs;
      break;
  }

  int64_t minDelayNs = mSensorInfoList[aSensorType].minDelay * 1000;
  if (samplingPeriodNs < minDelayNs) {
    samplingPeriodNs = minDelayNs;
  }

  // config sampling period and reporting latency to specified sensor
  if (!mSensors->batch(handle, samplingPeriodNs, kReportLatencyNs).isOk()) {
    HAL_ERR("sensors batch failed aSensorType=%d", aSensorType);
    return false;
  }

  // activate specified sensor
  if (!mSensors->activate(handle, true).isOk()) {
    HAL_ERR("sensors activate failed aSensorType=%d", aSensorType);
    return false;
  }

  return true;
}

bool
GonkSensorsHal::DeactivateSensor(const SensorType aSensorType) {
  if (mSensors == nullptr) {
    return false;
  }

  const int32_t handle = mSensorInfoList[aSensorType].sensorHandle;

  // check if specified sensor is supported
  if (!handle) {
    HAL_LOG("device unsupported sensor aSensorType=%d", aSensorType);
    return false;
  }

  // deactivate specified sensor
  if (!mSensors->activate(handle, false).isOk()) {
    HAL_ERR("sensors deactivate failed aSensorType=%d", aSensorType);
    return false;
  }

  return true;
}


} // hal_impl
} // mozilla
