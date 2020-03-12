/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface WifiOpenNetworkEvent : Event
{
  constructor(DOMString type, optional WifiOpenNetworkEventInit eventInitDict = {});

  /**
   * Open network notification availability.
   */
  readonly attribute boolean availability;
};

dictionary WifiOpenNetworkEventInit : EventInit
{
  boolean availability = false;
};
