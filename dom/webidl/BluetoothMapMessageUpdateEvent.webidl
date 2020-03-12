/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface BluetoothMapMessageUpdateEvent : Event
{
  constructor(DOMString type,
              optional BluetoothMapMessageUpdateEventInit eventInitDict = {});

  readonly attribute unsigned long         instanceId;

  readonly attribute BluetoothMapRequestHandle? handle;
};

dictionary BluetoothMapMessageUpdateEventInit : EventInit
{
  unsigned long                   instanceId = 0;

  BluetoothMapRequestHandle? handle = null;
};
