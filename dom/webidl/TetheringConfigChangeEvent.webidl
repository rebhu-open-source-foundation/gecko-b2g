/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Func="B2G::HasTetheringManagerSupport",
 Exposed=(Window)]
interface TetheringConfigChangeEvent : Event
{
  constructor(DOMString type, optional TetheringConfigChangeEventInit eventInitDict = {});
  /**
   * wifi tethering config, refer to interface TetheringConfigInfo.
   */
  readonly attribute TetheringConfigInfo? wifiTetheringConfig;

  /**
   * usb tethering config, refer to interface TetheringConfigInfo.
   */
  readonly attribute TetheringConfigInfo? usbTetheringConfig;
};

dictionary TetheringConfigChangeEventInit : EventInit
{
  TetheringConfigInfo? wifiTetheringConfig = null;
  TetheringConfigInfo? usbTetheringConfig = null;
};
