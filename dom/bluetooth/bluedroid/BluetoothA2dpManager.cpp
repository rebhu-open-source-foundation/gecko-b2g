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

#include "BluetoothA2dpManager.h"

#include "MainThreadUtils.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

namespace {
  StaticRefPtr<BluetoothA2dpManager> sBluetoothA2dpManager;
} // namespace

NS_IMETHODIMP
BluetoothA2dpManager::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
  return NS_OK;
}

void
BluetoothA2dpManager::Reset()
{
}

class BluetoothA2dpManager::InitProfileResultHandlerRunnable final
  : public Runnable
{
public:
  InitProfileResultHandlerRunnable(BluetoothProfileResultHandler* aRes,
                                   nsresult aRv)
    : Runnable("InitProfileResultHandlerRunnable")
    , mRes(aRes)
    , mRv(aRv)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (NS_SUCCEEDED(mRv)) {
      mRes->Init();
    } else {
      mRes->OnError(mRv);
    }
    return NS_OK;
  }

private:
  RefPtr<BluetoothProfileResultHandler> mRes;
  nsresult mRv;
};

// static
void
BluetoothA2dpManager::InitA2dpInterface(BluetoothProfileResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  // Fake the interface initialization for this dummy A2DP manager
  RefPtr<Runnable> r = new InitProfileResultHandlerRunnable(aRes, NS_OK);
  if (NS_FAILED(NS_DispatchToMainThread(r))) {
    BT_LOGR("Failed to dispatch A2DP Init runnable");
  }
}

//static
BluetoothA2dpManager*
BluetoothA2dpManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  // If sBluetoothA2dpManager already exists, exit early
  if (sBluetoothA2dpManager) {
    return sBluetoothA2dpManager;
  }

  // Create a new instance, register, and return
  RefPtr<BluetoothA2dpManager> manager = new BluetoothA2dpManager();

  sBluetoothA2dpManager = manager;

  return sBluetoothA2dpManager;
}

class BluetoothA2dpManager::DeinitProfileResultHandlerRunnable final
  : public Runnable
{
public:
  DeinitProfileResultHandlerRunnable(BluetoothProfileResultHandler* aRes,
                                     nsresult aRv)
    : Runnable("DeinitProfileResultHandlerRunnable")
    , mRes(aRes)
    , mRv(aRv)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (NS_SUCCEEDED(mRv)) {
      mRes->Deinit();
    } else {
      mRes->OnError(mRv);
    }
    return NS_OK;
  }

private:
  RefPtr<BluetoothProfileResultHandler> mRes;
  nsresult mRv;
};

// static
void
BluetoothA2dpManager::DeinitA2dpInterface(BluetoothProfileResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  // Fake the interface de-initialization for this dummy A2DP manager
  RefPtr<Runnable> r = new DeinitProfileResultHandlerRunnable(aRes, NS_OK);
  if (NS_FAILED(NS_DispatchToMainThread(r))) {
    BT_LOGR("Failed to dispatch A2DP Deinit runnable");
  }
}

void
BluetoothA2dpManager::Connect(const BluetoothAddress& aDeviceAddress,
                              BluetoothProfileController* aController)
{
}

void
BluetoothA2dpManager::Disconnect(BluetoothProfileController* aController)
{
}

void
BluetoothA2dpManager::OnConnect(const nsAString& aErrorStr)
{
}

void
BluetoothA2dpManager::OnDisconnect(const nsAString& aErrorStr)
{
}

void
BluetoothA2dpManager::OnGetServiceChannel(const BluetoothAddress& aDeviceAddress,
                                          const BluetoothUuid& aServiceUuid,
                                          int aChannel)
{
}

void
BluetoothA2dpManager::OnUpdateSdpRecords(const BluetoothAddress& aDeviceAddress)
{
}

void
BluetoothA2dpManager::GetAddress(BluetoothAddress& aDeviceAddress)
{
}

bool
BluetoothA2dpManager::IsConnected()
{
  return false;
}

bool
BluetoothA2dpManager::ReplyToConnectionRequest(bool aAccept)
{
  return false;
}

NS_IMPL_ISUPPORTS(BluetoothA2dpManager, nsIObserver)
