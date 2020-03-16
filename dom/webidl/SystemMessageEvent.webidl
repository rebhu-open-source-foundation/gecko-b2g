/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.systemMessage.enabled",
 Exposed=ServiceWorker]
interface SystemMessageEvent : ExtendableEvent {
  [Throws]
  constructor(DOMString type, DOMString name,
              optional SystemMessageEventInit eventInitDict = {});
  readonly attribute DOMString name;
  readonly attribute SystemMessageData? data;
};

typedef object SystemMessageDataInit;

dictionary SystemMessageEventInit : ExtendableEventInit {
  SystemMessageDataInit data;
};
