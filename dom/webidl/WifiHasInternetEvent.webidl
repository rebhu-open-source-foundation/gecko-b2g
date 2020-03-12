/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface WifiHasInternetEvent : Event
{
  constructor(DOMString type, optional WifiHasInternetEventInit eventInitDict = {});

  /**
   * Returns whether or not wifi has internet.
   */
  readonly attribute boolean hasInternet;

  /**
   * Network object with a SSID field describing the network affected by
   * this change.
   */
  readonly attribute any network;
};

dictionary WifiHasInternetEventInit : EventInit
{
  boolean hasInternet = false;
  any network = null;
};
