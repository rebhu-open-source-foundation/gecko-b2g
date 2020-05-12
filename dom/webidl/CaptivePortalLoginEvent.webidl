/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface CaptivePortalLoginEvent : Event
{
  constructor(DOMString type, optional CaptivePortalLoginEventInit eventInitDict = {});

  /**
   * Returns whether or not captive portal login success or not.
   */
  readonly attribute boolean loginSuccess;

  /**
   * Network object with a SSID field describing the network affected by
   * this change.
   */
  readonly attribute WifiNetwork? network;
};

dictionary CaptivePortalLoginEventInit : EventInit
{
  boolean loginSuccess = false;
  WifiNetwork? network = null;
};
