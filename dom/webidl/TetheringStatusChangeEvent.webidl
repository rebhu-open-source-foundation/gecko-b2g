/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=(Window)]
interface TetheringStatusChangeEvent : Event
{
  constructor(DOMString type, optional TetheringStatusChangeEventInit eventInitDict = {});
  /**
   * wifi tethering state. One of the TETHERING_STATE_* constants.
   */
  readonly attribute long wifiTetheringState;

  /**
   * usb tethering state. One of the TETHERING_STATE_* constants.
   */
  readonly attribute long usbTetheringState;
};

dictionary TetheringStatusChangeEventInit : EventInit
{
  long wifiTetheringState = 0;
  long usbTetheringState = 0;
};
