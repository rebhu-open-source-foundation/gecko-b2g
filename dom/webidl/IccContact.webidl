/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.telephony.enabled",
 Exposed=Window]
interface IccContact {
  [Throws]
  constructor(DOMString id,
              DOMString number,
              DOMString name,
              DOMString email);
  readonly attribute DOMString? id;
  readonly attribute DOMString? number;
  readonly attribute DOMString? name;
  readonly attribute DOMString? email;

};

dictionary IccContactInit {
    DOMString id;
    DOMString number;
    DOMString name;
    DOMString email;
};
