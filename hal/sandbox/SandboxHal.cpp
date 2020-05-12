/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Hal.h"
#include "HalLog.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "mozilla/dom/BrowserParent.h"
#include "mozilla/dom/BrowserChild.h"
#include "mozilla/fallback/FallbackScreenConfiguration.h"
#include "mozilla/EnumeratedRange.h"
#include "mozilla/Observer.h"
#include "mozilla/Unused.h"
#include "WindowIdentifier.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace mozilla {
namespace hal_sandbox {

static bool sHalChildDestroyed = false;

bool HalChildDestroyed() { return sHalChildDestroyed; }

static PHalChild* sHal;
static PHalChild* Hal() {
  if (!sHal) {
    sHal = ContentChild::GetSingleton()->SendPHalConstructor();
  }
  return sHal;
}

void Vibrate(const nsTArray<uint32_t>& pattern, WindowIdentifier&& id) {
  HAL_LOG("Vibrate: Sending to parent process.");

  WindowIdentifier newID(std::move(id));
  newID.AppendProcessID();
  Hal()->SendVibrate(pattern, newID.AsArray(),
                     BrowserChild::GetFrom(newID.GetWindow()));
}

void CancelVibrate(WindowIdentifier&& id) {
  HAL_LOG("CancelVibrate: Sending to parent process.");

  WindowIdentifier newID(std::move(id));
  newID.AppendProcessID();
  Hal()->SendCancelVibrate(newID.AsArray(),
                           BrowserChild::GetFrom(newID.GetWindow()));
}

void EnableBatteryNotifications() { Hal()->SendEnableBatteryNotifications(); }

void DisableBatteryNotifications() { Hal()->SendDisableBatteryNotifications(); }

void GetCurrentBatteryInformation(BatteryInformation* aBatteryInfo) {
  Hal()->SendGetCurrentBatteryInformation(aBatteryInfo);
}

void EnableNetworkNotifications() { Hal()->SendEnableNetworkNotifications(); }

void DisableNetworkNotifications() { Hal()->SendDisableNetworkNotifications(); }

void
SetNetworkType(int32_t aType) {}

void GetCurrentNetworkInformation(NetworkInformation* aNetworkInfo) {
  Hal()->SendGetCurrentNetworkInformation(aNetworkInfo);
}

void EnableScreenConfigurationNotifications() {
  Hal()->SendEnableScreenConfigurationNotifications();
}

void DisableScreenConfigurationNotifications() {
  Hal()->SendDisableScreenConfigurationNotifications();
}

void GetCurrentScreenConfiguration(ScreenConfiguration* aScreenConfiguration) {
  fallback::GetCurrentScreenConfiguration(aScreenConfiguration);
}

bool LockScreenOrientation(const hal::ScreenOrientation& aOrientation) {
  bool allowed;
  Hal()->SendLockScreenOrientation(aOrientation, &allowed);
  return allowed;
}

void UnlockScreenOrientation() { Hal()->SendUnlockScreenOrientation(); }

bool
GetScreenEnabled()
{
  bool enabled = false;
  Hal()->SendGetScreenEnabled(&enabled);
  return enabled;
}

void
SetScreenEnabled(bool aEnabled)
{
  Hal()->SendSetScreenEnabled(aEnabled);
}

void
SetScreenBrightness(double aBrightness)
{
  Hal()->SendSetScreenBrightness(aBrightness);
}

void EnableSensorNotifications(SensorType aSensor) {
  Hal()->SendEnableSensorNotifications(aSensor);
}

void DisableSensorNotifications(SensorType aSensor) {
  Hal()->SendDisableSensorNotifications(aSensor);
}

void EnableWakeLockNotifications() { Hal()->SendEnableWakeLockNotifications(); }

void DisableWakeLockNotifications() {
  Hal()->SendDisableWakeLockNotifications();
}

void ModifyWakeLock(const nsAString& aTopic, WakeLockControl aLockAdjust,
                    WakeLockControl aHiddenAdjust, uint64_t aProcessID) {
  MOZ_ASSERT(aProcessID != CONTENT_PROCESS_ID_UNKNOWN);
  Hal()->SendModifyWakeLock(nsString(aTopic), aLockAdjust, aHiddenAdjust,
                            aProcessID);
}

void GetWakeLockInfo(const nsAString& aTopic,
                     WakeLockInformation* aWakeLockInfo) {
  Hal()->SendGetWakeLockInfo(nsString(aTopic), aWakeLockInfo);
}

void EnableSwitchNotifications(SwitchDevice aDevice) {
  Hal()->SendEnableSwitchNotifications(aDevice);
}

void DisableSwitchNotifications(SwitchDevice aDevice) {
  Hal()->SendDisableSwitchNotifications(aDevice);
}

SwitchState GetCurrentSwitchState(SwitchDevice aDevice) {
  SwitchState state;
  Hal()->SendGetCurrentSwitchState(aDevice, &state);
  return state;
}

void NotifySwitchStateFromInputDevice(SwitchDevice aDevice, SwitchState aState) {
  Unused << aDevice;
  Unused << aState;
  MOZ_CRASH("Only the main process may notify switch state change.");
}

void
EnableFlashlightNotifications()
{
  Hal()->SendEnableFlashlightNotifications();
}

void
DisableFlashlightNotifications()
{
  Hal()->SendDisableFlashlightNotifications();
}

void
RequestCurrentFlashlightState()
{
  Hal()->SendGetFlashlightEnabled();
}

bool
GetFlashlightEnabled()
{
  MOZ_CRASH("GetFlashlightEnabled() can't be called from sandboxed contexts.");
  return true;
}

void
SetFlashlightEnabled(bool aEnabled)
{
  Hal()->SendSetFlashlightEnabled(aEnabled);
}

bool EnableAlarm() {
  MOZ_CRASH("Alarms can't be programmed from sandboxed contexts.  Yet.");
}

void DisableAlarm() {
  MOZ_CRASH("Alarms can't be programmed from sandboxed contexts.  Yet.");
}

bool SetAlarm(int32_t aSeconds, int32_t aNanoseconds) {
  MOZ_CRASH("Alarms can't be programmed from sandboxed contexts.  Yet.");
}

void SetProcessPriority(int aPid, ProcessPriority aPriority) {
  MOZ_CRASH("Only the main process may set processes' priorities.");
}

bool SetProcessPrioritySupported() {
  MOZ_CRASH("Only the main process may call SetProcessPrioritySupported().");
}

bool IsHeadphoneEventFromInputDev() {
  MOZ_CRASH(
      "IsHeadphoneEventFromInputDev() cannot be called from sandboxed "
      "contexts.");
  return false;
}

nsresult StartSystemService(const char* aSvcName, const char* aArgs) {
  MOZ_CRASH("System services cannot be controlled from sandboxed contexts.");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void StopSystemService(const char* aSvcName) {
  MOZ_CRASH("System services cannot be controlled from sandboxed contexts.");
}

bool SystemServiceIsRunning(const char* aSvcName) {
  MOZ_CRASH("System services cannot be controlled from sandboxed contexts.");
  return false;
}

bool SystemServiceIsStopped(const char* aSvcName) {
  MOZ_CRASH("System services cannot be controlled from sandboxed contexts.");
  return true;
}

class HalParent : public PHalParent,
                  public BatteryObserver,
                  public NetworkObserver,
                  public ISensorObserver,
                  public WakeLockObserver,
                  public ScreenConfigurationObserver,
                  public SwitchObserver,
                  public FlashlightObserver
{
 public:
  virtual void ActorDestroy(ActorDestroyReason aWhy) override {
    // NB: you *must* unconditionally unregister your observer here,
    // if it *may* be registered below.
    hal::UnregisterBatteryObserver(this);
    hal::UnregisterNetworkObserver(this);
    hal::UnregisterScreenConfigurationObserver(this);
    for (auto sensor : MakeEnumeratedRange(NUM_SENSOR_TYPE)) {
      hal::UnregisterSensorObserver(sensor, this);
    }
    hal::UnregisterWakeLockObserver(this);
    hal::UnregisterFlashlightObserver(this);
  }

  virtual mozilla::ipc::IPCResult RecvVibrate(
      nsTArray<unsigned int>&& pattern, nsTArray<uint64_t>&& id,
      PBrowserParent* browserParent) override {
    // We give all content vibration permission.
    //    BrowserParent *browserParent = BrowserParent::GetFrom(browserParent);
    /* xxxkhuey wtf
    nsCOMPtr<nsIDOMWindow> window =
      do_QueryInterface(browserParent->GetBrowserDOMWindow());
    */
    hal::Vibrate(pattern, WindowIdentifier(std::move(id), nullptr));
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvCancelVibrate(
      nsTArray<uint64_t>&& id, PBrowserParent* browserParent) override {
    // BrowserParent *browserParent = BrowserParent::GetFrom(browserParent);
    /* XXXkhuey wtf
    nsCOMPtr<nsIDOMWindow> window =
      browserParent->GetBrowserDOMWindow();
    */
    hal::CancelVibrate(WindowIdentifier(std::move(id), nullptr));
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvEnableFlashlightNotifications() override {
    hal::RegisterFlashlightObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableFlashlightNotifications() override {
    hal::UnregisterFlashlightObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvGetFlashlightEnabled() override {
    bool flashlightState = hal::GetFlashlightEnabled();
    FlashlightInformation flashlightInfo;
    flashlightInfo.enabled() = flashlightState;
    Unused << SendNotifyFlashlightState(flashlightInfo);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvSetFlashlightEnabled(
      const bool& aEnabled) override {
    hal::SetFlashlightEnabled(aEnabled);
    return IPC_OK();
  }

  void Notify(const FlashlightInformation& aFlashlightInfo) override {
    Unused << SendNotifyFlashlightState(aFlashlightInfo);
  }

  virtual mozilla::ipc::IPCResult RecvEnableBatteryNotifications() override {
    // We give all content battery-status permission.
    hal::RegisterBatteryObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableBatteryNotifications() override {
    hal::UnregisterBatteryObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvGetCurrentBatteryInformation(
      BatteryInformation* aBatteryInfo) override {
    // We give all content battery-status permission.
    hal::GetCurrentBatteryInformation(aBatteryInfo);
    return IPC_OK();
  }

  void Notify(const BatteryInformation& aBatteryInfo) override {
    Unused << SendNotifyBatteryChange(aBatteryInfo);
  }

  virtual mozilla::ipc::IPCResult RecvEnableNetworkNotifications() override {
    // We give all content access to this network-status information.
    hal::RegisterNetworkObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableNetworkNotifications() override {
    hal::UnregisterNetworkObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvGetCurrentNetworkInformation(
      NetworkInformation* aNetworkInfo) override {
    hal::GetCurrentNetworkInformation(aNetworkInfo);
    return IPC_OK();
  }

  void Notify(const NetworkInformation& aNetworkInfo) override {
    Unused << SendNotifyNetworkChange(aNetworkInfo);
  }

  virtual mozilla::ipc::IPCResult RecvEnableScreenConfigurationNotifications()
      override {
    // Screen configuration is used to implement CSS and DOM
    // properties, so all content already has access to this.
    hal::RegisterScreenConfigurationObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableScreenConfigurationNotifications()
      override {
    hal::UnregisterScreenConfigurationObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvLockScreenOrientation(
      const ScreenOrientation& aOrientation, bool* aAllowed) override {
    // FIXME/bug 777980: unprivileged content may only lock
    // orientation while fullscreen.  We should check whether the
    // request comes from an actor in a process that might be
    // fullscreen.  We don't have that information currently.
    *aAllowed = hal::LockScreenOrientation(aOrientation);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvUnlockScreenOrientation() override {
    hal::UnlockScreenOrientation();
    return IPC_OK();
  }

  void Notify(const ScreenConfiguration& aScreenConfiguration) override {
    Unused << SendNotifyScreenConfigurationChange(aScreenConfiguration);
  }

  virtual mozilla::ipc::IPCResult RecvGetScreenEnabled(
      bool* aEnabled) override {
#if 0 // TODO: FIXME
    if (!AssertAppProcessPermission(this, "power")) {
      return false;
    }
#endif
    *aEnabled = hal::GetScreenEnabled();
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvSetScreenEnabled(
      const bool& aEnabled) override {
#if 0 // TODO: FIXME
    if (!AssertAppProcessPermission(this, "power")) {
      return false;
    }
#endif
    hal::SetScreenEnabled(aEnabled);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvSetScreenBrightness(
      const double& aBrightness) override {
#if 0 // TODO: FIXME
    if (!AssertAppProcessPermission(this, "power")) {
      return false;
    }
#endif
    hal::SetScreenBrightness(aBrightness);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvEnableSensorNotifications(
      const SensorType& aSensor) override {
    // We currently allow any content to register device-sensor
    // listeners.
    hal::RegisterSensorObserver(aSensor, this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableSensorNotifications(
      const SensorType& aSensor) override {
    hal::UnregisterSensorObserver(aSensor, this);
    return IPC_OK();
  }

  void Notify(const SensorData& aSensorData) override {
    Unused << SendNotifySensorChange(aSensorData);
  }

  virtual mozilla::ipc::IPCResult RecvModifyWakeLock(
      const nsString& aTopic, const WakeLockControl& aLockAdjust,
      const WakeLockControl& aHiddenAdjust,
      const uint64_t& aProcessID) override {
    MOZ_ASSERT(aProcessID != CONTENT_PROCESS_ID_UNKNOWN);

    // We allow arbitrary content to use wake locks.
    hal::ModifyWakeLock(aTopic, aLockAdjust, aHiddenAdjust, aProcessID);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvEnableWakeLockNotifications() override {
    // We allow arbitrary content to use wake locks.
    hal::RegisterWakeLockObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableWakeLockNotifications() override {
    hal::UnregisterWakeLockObserver(this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvGetWakeLockInfo(
      const nsString& aTopic, WakeLockInformation* aWakeLockInfo) override {
    hal::GetWakeLockInfo(aTopic, aWakeLockInfo);
    return IPC_OK();
  }

  void Notify(const WakeLockInformation& aWakeLockInfo) override {
    Unused << SendNotifyWakeLockChange(aWakeLockInfo);
  }

  virtual mozilla::ipc::IPCResult RecvEnableSwitchNotifications(const SwitchDevice& aDevice) override {
    // Content has no reason to listen to switch events currently.
    hal::RegisterSwitchObserver(aDevice, this);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvDisableSwitchNotifications(const SwitchDevice& aDevice) override {
    hal::UnregisterSwitchObserver(aDevice, this);
    return IPC_OK();
  }

  virtual void Notify(const SwitchEvent& aSwitchEvent) override {
    Unused << SendNotifySwitchChange(aSwitchEvent);
  }

  virtual mozilla::ipc::IPCResult RecvGetCurrentSwitchState(const SwitchDevice& aDevice, hal::SwitchState *aState) override {
    // Content has no reason to listen to switch events currently.
    *aState = hal::GetCurrentSwitchState(aDevice);
    return IPC_OK();
  }
};

class HalChild : public PHalChild {
 public:
  virtual void ActorDestroy(ActorDestroyReason aWhy) override {
    sHalChildDestroyed = true;
  }

  virtual mozilla::ipc::IPCResult RecvNotifyBatteryChange(
      const BatteryInformation& aBatteryInfo) override {
    hal::NotifyBatteryChange(aBatteryInfo);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvNotifySensorChange(
      const hal::SensorData& aSensorData) override;

  virtual mozilla::ipc::IPCResult RecvNotifyNetworkChange(
      const NetworkInformation& aNetworkInfo) override {
    hal::NotifyNetworkChange(aNetworkInfo);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvNotifyWakeLockChange(
      const WakeLockInformation& aWakeLockInfo) override {
    hal::NotifyWakeLockChange(aWakeLockInfo);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvNotifyScreenConfigurationChange(
      const ScreenConfiguration& aScreenConfiguration) override {
    hal::NotifyScreenConfigurationChange(aScreenConfiguration);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvNotifySwitchChange(const mozilla::hal::SwitchEvent& aEvent) override {
    hal::NotifySwitchChange(aEvent);
    return IPC_OK();
  }

  mozilla::ipc::IPCResult RecvNotifyFlashlightState(const FlashlightInformation& aFlashlightState) override {
    hal::UpdateFlashlightState(aFlashlightState);
    return IPC_OK();
  }
};

mozilla::ipc::IPCResult HalChild::RecvNotifySensorChange(
    const hal::SensorData& aSensorData) {
  hal::NotifySensorChange(aSensorData);

  return IPC_OK();
}

PHalChild* CreateHalChild() { return new HalChild(); }

PHalParent* CreateHalParent() { return new HalParent(); }

}  // namespace hal_sandbox
}  // namespace mozilla
