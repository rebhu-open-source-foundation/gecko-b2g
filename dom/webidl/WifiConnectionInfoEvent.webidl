/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface WifiConnectionInfoEvent : Event
{
  constructor(DOMString type, optional WifiConnectionInfoEventInit eventInitDict = {});

  /**
   * Network object with an SSID field.
   */
  readonly attribute WifiNetwork? network;

  /**
   * Strength of the signal to network, in dBm between -55 and -100 dBm.
   */
  readonly attribute short signalStrength;

  /**
   * Relative signal strength between 0 and 100.
   */
  readonly attribute short relSignalStrength;

  /**
   * Link speed in Mb/s.
   */
  readonly attribute long linkSpeed;

  /**
   * IP address in the dotted quad format.
   */
  readonly attribute DOMString? ipAddress;
};

dictionary WifiConnectionInfoEventInit : EventInit
{
  WifiNetwork? network = null;
  short signalStrength = 0;
  short relSignalStrength = 0;
  long linkSpeed = 0;
  DOMString ipAddress = "";
};
