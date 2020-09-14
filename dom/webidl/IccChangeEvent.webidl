/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.icc.enabled",
 Func="B2G::HasMobileConnectionSupport",
 Exposed=Window]
interface IccChangeEvent : Event
{
  constructor(DOMString type, optional IccChangeEventInit eventInitDict={});
  readonly attribute DOMString iccId;
};

dictionary IccChangeEventInit : EventInit
{
  DOMString iccId = "";
};
