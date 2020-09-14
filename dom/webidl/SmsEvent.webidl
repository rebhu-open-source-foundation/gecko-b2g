/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */


[Exposed=Window, Pref="dom.sms.enabled", Func="B2G::HasMobileMessageSupport"]
interface SmsEvent : Event
{
  constructor(DOMString type, optional SmsEventInit eventInitDict={});
  readonly attribute SmsMessage? message;
};

dictionary SmsEventInit : EventInit
{
  SmsMessage? message = null;
};
