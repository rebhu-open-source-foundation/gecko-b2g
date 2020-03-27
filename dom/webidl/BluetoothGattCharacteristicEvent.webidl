/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * [B2G only GATT client API]
 * BluetoothGattCharacteristicEvent interface is exposed only if
 * "dom.bluetooth.webbluetooth.enabled" preference is false.
 */
[Func="mozilla::dom::bluetooth::BluetoothManager::B2GGattClientEnabled",
 Exposed=Window]
interface BluetoothGattCharacteristicEvent : Event
{
  constructor(DOMString type,
              optional BluetoothGattCharacteristicEventInit eventInitDict = {});

  readonly attribute BluetoothGattCharacteristic? characteristic;
};

dictionary BluetoothGattCharacteristicEventInit : EventInit
{
  BluetoothGattCharacteristic? characteristic = null;
};
