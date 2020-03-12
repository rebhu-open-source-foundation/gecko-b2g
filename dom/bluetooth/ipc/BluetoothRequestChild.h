/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef mozilla_dom_bluetooth_ipc_BluetoothRequestChild_h
#define mozilla_dom_bluetooth_ipc_BluetoothRequestChild_h

#include "mozilla/dom/bluetooth/PBluetoothRequestChild.h"


BEGIN_BLUETOOTH_NAMESPACE

class BluetoothChild;
class BluetoothReplyRunnable;

/*******************************************************************************
 * BluetoothRequestChild
 ******************************************************************************/

class BluetoothRequestChild : public PBluetoothRequestChild
{
  friend class mozilla::dom::bluetooth::BluetoothChild;

  RefPtr<BluetoothReplyRunnable> mReplyRunnable;

public:
  BluetoothRequestChild(BluetoothReplyRunnable* aReplyRunnable);

  mozilla::ipc::IPCResult
  Recv__delete__(const BluetoothReply& aReply);

protected:
  virtual ~BluetoothRequestChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;
};

END_BLUETOOTH_NAMESPACE

#endif // mozilla_dom_bluetooth_ipc_BluetoothRequestChild_h
