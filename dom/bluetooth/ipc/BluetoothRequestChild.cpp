/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
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
