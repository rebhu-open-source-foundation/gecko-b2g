/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.telephony.enabled",
 Exposed=Window]
interface RttModifyResponseReceivedEvent : Event
{
  constructor(DOMString type, optional RttModifyResponseReceivedEventInit eventInitDict={});
  readonly attribute unsigned short status;
};

dictionary RttModifyResponseReceivedEventInit : EventInit
{
  unsigned short status = 0;
};
