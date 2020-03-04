/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Func="B2G::HasWifiManagerSupport",
 Exposed=Window]
interface WifiP2pStatusChangeEvent : Event
{
  constructor(DOMString type, optional WifiP2pStatusChangeEventInit eventInitDict = {});

  /**
   * The mac address of the peer whose status has just changed.
   */
  readonly attribute DOMString peerAddress;
};

dictionary WifiP2pStatusChangeEventInit : EventInit
{
  DOMString peerAddress = "";
};
