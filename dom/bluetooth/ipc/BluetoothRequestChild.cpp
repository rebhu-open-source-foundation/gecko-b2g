/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BluetoothRequestChild.h"

#include "BluetoothChild.h"
#include "BluetoothReplyRunnable.h"
#include "nsThreadUtils.h"


USING_BLUETOOTH_NAMESPACE

/*******************************************************************************
 * BluetoothRequestChild
 ******************************************************************************/

BluetoothRequestChild::BluetoothRequestChild(
                                         BluetoothReplyRunnable* aReplyRunnable)
: mReplyRunnable(aReplyRunnable)
{
  MOZ_COUNT_CTOR(BluetoothRequestChild);
  MOZ_ASSERT(aReplyRunnable);
}

BluetoothRequestChild::~BluetoothRequestChild()
{
  MOZ_COUNT_DTOR(BluetoothRequestChild);
}

void
BluetoothRequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  // Nothing needed here.
}

mozilla::ipc::IPCResult
BluetoothRequestChild::Recv__delete__(const BluetoothReply& aReply)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mReplyRunnable);

  RefPtr<BluetoothReplyRunnable> replyRunnable;
  mReplyRunnable.swap(replyRunnable);

  if (replyRunnable) {
    // XXXbent Need to fix this, it copies unnecessarily.
    replyRunnable->SetReply(new BluetoothReply(aReply));
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(replyRunnable))) {
      return IPC_OK();
    }
  }

  return IPC_FAIL_NO_REASON(this);
}
