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

#ifndef mozilla_dom_bluetooth_bluedroid_BluetoothA2dpManager_h
#define mozilla_dom_bluetooth_bluedroid_BluetoothA2dpManager_h

#include "BluetoothProfileManagerBase.h"

BEGIN_BLUETOOTH_NAMESPACE
class BluetoothA2dpManager : public BluetoothProfileManagerBase
{
public:
  BT_DECL_PROFILE_MGR_BASE
  virtual void GetName(nsACString& aName) override
  {
    aName.AssignLiteral("A2DP");
  }

  static BluetoothA2dpManager* Get();
  static void InitA2dpInterface(BluetoothProfileResultHandler* aRes);
  static void DeinitA2dpInterface(BluetoothProfileResultHandler* aRes);

  void HandleBackendError() {};

protected:
  virtual ~BluetoothA2dpManager() = default;

private:
  class DeinitProfileResultHandlerRunnable;
  class InitProfileResultHandlerRunnable;

  BluetoothA2dpManager() = default;
};

END_BLUETOOTH_NAMESPACE

#endif
