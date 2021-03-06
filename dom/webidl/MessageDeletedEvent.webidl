/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.sms.enabled",
 Func="B2G::HasMobileMessageSupport",
 Exposed=Window]
interface MessageDeletedEvent : Event
{
  constructor(DOMString type, optional MessageDeletedEventInit eventInitDict={});
  // Array of deleted message ids.
  [Cached, Constant] readonly attribute sequence<long>? deletedMessageIds;
  // Array of deleted thread ids.
  [Cached, Constant] readonly attribute sequence<unsigned long long>? deletedThreadIds;
};

dictionary MessageDeletedEventInit : EventInit
{
  sequence<long>? deletedMessageIds = null;
  sequence<unsigned long long>? deletedThreadIds = null;
};
