/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_Hal_h
#define mozilla_Hal_h

#include "base/basictypes.h"
#include "base/platform_thread.h"
#include "nsTArray.h"
#include "mozilla/dom/FlashlightManager.h"
#include "mozilla/hal_sandbox/PHal.h"
#include "mozilla/HalBatteryInformation.h"
#include "mozilla/HalNetworkInformation.h"
#include "mozilla/HalScreenConfiguration.h"
#include "mozilla/HalWakeLockInformation.h"
#include "mozilla/HalTypes.h"
#include "mozilla/Types.h"

/*
 * Hal.h contains the public Hal API.
 *
 * By default, this file defines its functions in the hal namespace, but if
 * MOZ_HAL_NAMESPACE is defined, we'll define our functions in that namespace.
 *
 * This is used by HalImpl.h and HalSandbox.h, which define copies of all the
 * functions here in the hal_impl and hal_sandbox namespaces.
 */

class nsPIDOMWindowInner;

#ifndef MOZ_HAL_NAMESPACE
#  define MOZ_HAL_NAMESPACE hal
#  define MOZ_DEFINED_HAL_NAMESPACE 1
#endif

namespace mozilla {

namespace hal {

typedef Observer<void_t> AlarmObserver;

class WindowIdentifier;

}  // namespace hal

namespace MOZ_HAL_NAMESPACE {

/**
 * Initializes the HAL. This must be called before any other HAL function.
 */
void Init();

/**
 * Shuts down the HAL. Besides freeing all the used resources this will check
 * that all observers have been properly deregistered and assert if not.
 */
void Shutdown();

/**
 * Turn the default vibrator device on/off per the pattern specified
 * by |pattern|.  Each element in the pattern is the number of
 * milliseconds to turn the vibrator on or off.  The first element in
 * |pattern| is an "on" element, the next is "off", and so on.
 *
 * If |pattern| is empty, any in-progress vibration is canceled.
 *
 * Only an active window within an active tab may call Vibrate; calls
 * from inactive windows and windows on inactive tabs do nothing.
 *
 * If you're calling hal::Vibrate from the outside world, pass an
 * nsIDOMWindow* in place of the WindowIdentifier parameter.
 * The method with WindowIdentifier will be called automatically.
 */
void Vibrate(const nsTArray<uint32_t>& pattern, nsPIDOMWindowInner* aWindow);
void Vibrate(const nsTArray<uint32_t>& pattern,
             const hal::WindowIdentifier& id);

/**
 * Cancel a vibration started by the content window identified by
 * WindowIdentifier.
 *
 * If the window was the last window to start a vibration, the
 * cancellation request will go through even if the window is not
 * active.
 *
 * As with hal::Vibrate(), if you're calling hal::CancelVibrate from the outside
 * world, pass an nsIDOMWindow*. The method with WindowIdentifier will be called
 * automatically.
 */
void CancelVibrate(nsPIDOMWindowInner* aWindow);
void CancelVibrate(const hal::WindowIdentifier& id);

#define MOZ_DEFINE_HAL_OBSERVER(name_)                             \
  /**                                                              \
   * Inform the backend there is a new |name_| observer.           \
   * @param aObserver The observer that should be added.           \
   */                                                              \
  void Register##name_##Observer(hal::name_##Observer* aObserver); \
  /**                                                              \
   * Inform the backend a |name_| observer unregistered.           \
   * @param aObserver The observer that should be removed.         \
   */                                                              \
  void Unregister##name_##Observer(hal::name_##Observer* aObserver);

MOZ_DEFINE_HAL_OBSERVER(Battery);

/**
 * Returns the current battery information.
 */
void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);

/**
 * Notify of a change in the battery state.
 * @param aBatteryInfo The new battery information.
 */
void NotifyBatteryChange(const hal::BatteryInformation& aBatteryInfo);

/**
 * Inform the flashlightmanager backend there is a new flashlight observer.
 * @param aFlashlightObserver The observer that should be added.
 */
void RegisterFlashlightObserver(mozilla::dom::FlashlightObserver* aFlashlightObserver);

/**
 * Inform the flashlightmanager backend a flashlight observer unregistered.
 * @param aFlashlightObserver The observer that should be removed.
 */
void UnregisterFlashlightObserver(mozilla::dom::FlashlightObserver* aFlashlightObserver);

/**
 * Request the current Flashlight state. If the request is made by content process,
 * result will be returned asynchronously.
 */
void RequestCurrentFlashlightState();

/**
 * Update the Flashlight state in each Flashlight observers.
 * @param aFlashlightState The new state of Flashlight.
 */
void UpdateFlashlightState(const hal::FlashlightInformation& aFlashlightInfo);

/**
 * Get the flashlight state which is enabled or not
 */
bool GetFlashlightEnabled();

/**
 * Enable or disable flashlight.
 */
void SetFlashlightEnabled(bool aEnabled);

/**
 * Determine whether the device's screen is currently enabled.
 */
bool GetScreenEnabled();

/**
 * Enable or disable the device's screen.
 *
 * Note that it may take a few seconds for the screen to turn on or off.
 */
void SetScreenEnabled(bool aEnabled);

/**
 * Set the brightness of the device's screen's backlight, on a scale from 0
 * (very dimm) to 1 (full blast).  Values larger than 1 are treated like 1, and
 * values smaller than 0 are treated like 0.
 *
 * Note that we may reduce the resolution of the given brightness value before
 * sending it to the screen.  Therefore if you call SetScreenBrightness(x)
 * followed by GetScreenBrightness(), the value returned by
 * GetScreenBrightness() may not be exactly x.
 */
void SetScreenBrightness(double aBrightness);

/**
 * Register an observer for the sensor of given type.
 *
 * The observer will receive data whenever the data generated by the
 * sensor is avaiable.
 */
void RegisterSensorObserver(hal::SensorType aSensor,
                            hal::ISensorObserver* aObserver);

/**
 * Unregister an observer for the sensor of given type.
 */
void UnregisterSensorObserver(hal::SensorType aSensor,
                              hal::ISensorObserver* aObserver);

/**
 * Post a value generated by a sensor.
 *
 * This API is internal to hal; clients shouldn't call it directly.
 */
void NotifySensorChange(const hal::SensorData& aSensorData);

/**
 * Enable sensor notifications from the backend
 *
 * This method is only visible from implementation of sensor manager.
 * Rest of the system should not try this.
 */
void EnableSensorNotifications(hal::SensorType aSensor);

/**
 * Disable sensor notifications from the backend
 *
 * This method is only visible from implementation of sensor manager.
 * Rest of the system should not try this.
 */
void DisableSensorNotifications(hal::SensorType aSensor);

MOZ_DEFINE_HAL_OBSERVER(Network);

/**
 * Returns the current network information.
 */
void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo);

/**
 * Notify of a change in the network state.
 * @param aNetworkInfo The new network information.
 */
void NotifyNetworkChange(const hal::NetworkInformation& aNetworkInfo);

/**
 * Set active network connection type.
 * @param aType The network connection type.
 */
void SetNetworkType(int32_t aType);

/**
 * Enable wake lock notifications from the backend.
 *
 * This method is only used by WakeLockObserversManager.
 */
void EnableWakeLockNotifications();

/**
 * Disable wake lock notifications from the backend.
 *
 * This method is only used by WakeLockObserversManager.
 */
void DisableWakeLockNotifications();

MOZ_DEFINE_HAL_OBSERVER(WakeLock);

/**
 * Adjust a wake lock's counts on behalf of a given process.
 *
 * In most cases, you shouldn't need to pass the aProcessID argument; the
 * default of CONTENT_PROCESS_ID_UNKNOWN is probably what you want.
 *
 * @param aTopic        lock topic
 * @param aLockAdjust   to increase or decrease active locks
 * @param aHiddenAdjust to increase or decrease hidden locks
 * @param aProcessID    indicates which process we're modifying the wake lock
 *                      on behalf of.  It is interpreted as
 *
 *                      CONTENT_PROCESS_ID_UNKNOWN: The current process
 *                      CONTENT_PROCESS_ID_MAIN: The root process
 *                      X: The process with ContentChild::GetID() == X
 */
void ModifyWakeLock(const nsAString& aTopic, hal::WakeLockControl aLockAdjust,
                    hal::WakeLockControl aHiddenAdjust,
                    uint64_t aProcessID = hal::CONTENT_PROCESS_ID_UNKNOWN);

/**
 * Query the wake lock numbers of aTopic.
 * @param aTopic        lock topic
 * @param aWakeLockInfo wake lock numbers
 */
void GetWakeLockInfo(const nsAString& aTopic,
                     hal::WakeLockInformation* aWakeLockInfo);

/**
 * Notify of a change in the wake lock state.
 * @param aWakeLockInfo The new wake lock information.
 */
void NotifyWakeLockChange(const hal::WakeLockInformation& aWakeLockInfo);

MOZ_DEFINE_HAL_OBSERVER(ScreenConfiguration);

/**
 * Returns the current screen configuration.
 */
void GetCurrentScreenConfiguration(
    hal::ScreenConfiguration* aScreenConfiguration);

/**
 * Notify of a change in the screen configuration.
 * @param aScreenConfiguration The new screen orientation.
 */
void NotifyScreenConfigurationChange(
    const hal::ScreenConfiguration& aScreenConfiguration);

/**
 * Lock the screen orientation to the specific orientation.
 * @return Whether the lock has been accepted.
 */
MOZ_MUST_USE bool LockScreenOrientation(
    const hal::ScreenOrientation& aOrientation);

/**
 * Unlock the screen orientation.
 */
void UnlockScreenOrientation();

/**
 * Register an observer for the switch of given SwitchDevice.
 *
 * The observer will receive data whenever the data generated by the
 * given switch.
 */
void RegisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aSwitchObserver);

/**
 * Unregister an observer for the switch of given SwitchDevice.
 */
void UnregisterSwitchObserver(hal::SwitchDevice aDevice, hal::SwitchObserver *aSwitchObserver);

/**
 * Notify the state of the switch.
 *
 * This API is internal to hal; clients shouldn't call it directly.
 */
void NotifySwitchChange(const hal::SwitchEvent& aEvent);

/**
 * Get current switch information.
 */
hal::SwitchState GetCurrentSwitchState(hal::SwitchDevice aDevice);

/**
 * Notify switch status change from input device.
 */
void NotifySwitchStateFromInputDevice(hal::SwitchDevice aDevice,
                                      hal::SwitchState aState);

/**
 * Return true if the current platform supports the setting of process
 * priority.
 */
bool SetProcessPrioritySupported();

/**
 * Register an observer that is notified when a programmed alarm
 * expires.
 *
 * Currently, there can only be 0 or 1 alarm observers.
 */
MOZ_MUST_USE bool RegisterTheOneAlarmObserver(hal::AlarmObserver* aObserver);

/**
 * Unregister the alarm observer.  Doing so will implicitly cancel any
 * programmed alarm.
 */
void UnregisterTheOneAlarmObserver();

/**
 * Notify that the programmed alarm has expired.
 *
 * This API is internal to hal; clients shouldn't call it directly.
 */
void NotifyAlarmFired();

/**
 * Program the real-time clock to expire at the time |aSeconds|,
 * |aNanoseconds|.  These specify a point in real time relative to the
 * UNIX epoch.  The alarm will fire at this time point even if the
 * real-time clock is changed; that is, this alarm respects changes to
 * the real-time clock.  Return true iff the alarm was programmed.
 *
 * The alarm can be reprogrammed at any time.
 *
 * This API is currently only allowed to be used from non-sandboxed
 * contexts.
 */
MOZ_MUST_USE bool SetAlarm(int32_t aSeconds, int32_t aNanoseconds);

/**
 * Set the priority of the given process.
 *
 * Exactly what this does will vary between platforms.  On *nix we might give
 * background processes higher nice values.  On other platforms, we might
 * ignore this call entirely.
 */
void SetProcessPriority(int aPid, hal::ProcessPriority aPriority);

/**
 * Get total system memory of device being run on in bytes.
 *
 * Returns 0 if we are unable to determine this information from /proc/meminfo.
 */
uint32_t GetTotalSystemMemory();

/**
 * Determine whether the headphone switch event is from input device
 */
bool IsHeadphoneEventFromInputDev();

/**
 * Start the system service with the specified name and arguments.
 */
nsresult StartSystemService(const char* aSvcName, const char* aArgs);

/**
 * Stop the system service with the specified name.
 */
void StopSystemService(const char* aSvcName);

/**
 * Determine whether the system service with the specified name is running.
 */
bool SystemServiceIsRunning(const char* aSvcName);
bool SystemServiceIsStopped(const char* aSvcName);

}  // namespace MOZ_HAL_NAMESPACE
}  // namespace mozilla

#ifdef MOZ_DEFINED_HAL_NAMESPACE
#  undef MOZ_DEFINED_HAL_NAMESPACE
#  undef MOZ_HAL_NAMESPACE
#endif

#endif  // mozilla_Hal_h
