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

  virtual mozilla::ipc::IPCResult
  Recv__delete__(const BluetoothReply& aReply) override;

protected:
  virtual ~BluetoothRequestChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;
};

END_BLUETOOTH_NAMESPACE

#endif // mozilla_dom_bluetooth_ipc_BluetoothRequestChild_h
