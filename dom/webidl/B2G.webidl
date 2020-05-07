/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=(Window,Worker)]
interface B2G {
  // objects implementing this interface also implement the interfaces given
  // below
};

partial interface B2G {
  [Throws, Exposed=(Window,Worker), Pref="dom.alarm.enabled"]
  readonly attribute AlarmManager alarmManager;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.flashlight.enabled"]
  Promise<FlashlightManager> getFlashlightManager();
};

partial interface B2G {
  [Throws, Exposed=Window]
  readonly attribute TetheringManager tetheringManager;
};

#ifdef MOZ_B2G_RIL
[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.mobileconnection.enabled"]
  readonly attribute MobileConnectionArray mobileConnections;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.telephony.enabled"]
  readonly attribute Telephony telephony;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.icc.enabled"]
  readonly attribute IccManager? iccManager;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.datacall.enabled"]
  readonly attribute DataCallManager? dataCallManager;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.subsidylock.enabled"]
  readonly attribute SubsidyLockManager? subsidyLockManager;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.voicemail.enabled"]
  readonly attribute Voicemail voicemail;
};

[Exposed=Window]
partial interface B2G {
  [Throws, Pref="dom.cellbroadcast.enabled"]
  readonly attribute CellBroadcast cellBroadcast;
};
#endif //MOZ_B2G_RIL

#ifdef HAS_KOOST_MODULES
[Exposed=(Window,Worker)]
partial interface B2G {
  [Throws]
  readonly attribute ExternalAPI externalapi;
};
#endif

#ifdef MOZ_B2G_BT
[Exposed=Window]
partial interface B2G {
  [Throws]
  readonly attribute BluetoothManager bluetooth;
};
#endif // MOZ_B2G_BT

#ifdef MOZ_B2G_CAMERA
// nsIDOMB2GCamera
partial interface B2G {
  [Throws, Func="B2G::HasCameraSupport"]
  readonly attribute CameraManager cameras;
};
#endif // MOZ_B2G_CAMERA

#ifndef DISABLE_WIFI
partial interface B2G {
  [Throws, Func="B2G::HasWifiManagerSupport", Exposed=Window]
  readonly attribute WifiManager wifiManager;
};
#endif // DISABLE_WIFI

partial interface B2G {
  [Throws, Pref="dom.downloads.enabled", Exposed=Window]
  readonly attribute DownloadManager downloadManager;
};

#ifdef MOZ_AUDIO_CHANNEL_MANAGER
[Exposed=Window]
partial interface B2G {
  [Throws]
  readonly attribute AudioChannelManager audioChannelManager;
};
#endif // MOZ_AUDIO_CHANNEL_MANAGER

partial interface B2G {
  /**
   * Request a wake lock for a resource.
   *
   * A page holds a wake lock to request that a resource not be turned
   * off (or otherwise made unavailable).
   *
   * The topic is the name of a resource that might be made unavailable for
   * various reasons. For example, on a mobile device the power manager might
   * decide to turn off the screen after a period of idle time to save power.
   *
   * The resource manager checks the lock state of a topic before turning off
   * the associated resource. For example, a page could hold a lock on the
   * "screen" topic to prevent the screensaver from appearing or the screen
   * from turning off.
   *
   * The resource manager defines what each topic means and sets policy.  For
   * example, the resource manager might decide to ignore 'screen' wake locks
   * held by pages which are not visible.
   *
   * One topic can be locked multiple times; it is considered released only when
   * all locks on the topic have been released.
   *
   * The returned WakeLock object is a token of the lock.  You can
   * unlock the lock via the object's |unlock| method.  The lock is released
   * automatically when its associated window is unloaded.
   *
   * @param aTopic resource name
   */
  [Throws, Pref="dom.wakelock.enabled", Func="B2G::HasWakeLockSupport", Exposed=Window]
  WakeLock requestWakeLock(DOMString aTopic);
};
