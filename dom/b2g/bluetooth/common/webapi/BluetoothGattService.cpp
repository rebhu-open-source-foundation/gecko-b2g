/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "mozilla/dom/BluetoothGattServiceBinding.h"
#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "mozilla/dom/bluetooth/BluetoothGattCharacteristic.h"
#include "mozilla/dom/bluetooth/BluetoothGattService.h"
#include "mozilla/dom/bluetooth/BluetoothTypes.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(BluetoothGattService, mOwner,
                                      mIncludedServices, mCharacteristics)

NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattService)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattService)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattService)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

const uint16_t BluetoothGattService::sHandleCount = 1;

// Constructor of BluetoothGattService in ATT client role
BluetoothGattService::BluetoothGattService(nsPIDOMWindowInner* aOwner,
                                           const nsAString& aAppUuid,
                                           const BluetoothGattDbElement& aDb)
    : mOwner(aOwner),
      mAppUuid(aAppUuid),
      mInstanceId(aDb.mId),
      mIsPrimary(aDb.mType == GATT_DB_TYPE_PRIMARY_SERVICE),
      mAttRole(ATT_CLIENT_ROLE),
      mActive(true),
      mServiceHandle(aDb.mHandle) {
  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(!mAppUuid.IsEmpty());

  UuidToString(aDb.mUuid, mUuidStr);
}

// Constructor of BluetoothGattService in ATT server role
BluetoothGattService::BluetoothGattService(
    nsPIDOMWindowInner* aOwner, const BluetoothGattServiceInit& aInit)
    : mOwner(aOwner),
      mUuidStr(aInit.mUuid),
      mAttRole(ATT_SERVER_ROLE),
      mActive(false) {
  mIsPrimary = aInit.mIsPrimary;
}

BluetoothGattService::~BluetoothGattService() {}

void BluetoothGattService::AppendIncludedService(
    BluetoothGattService* aService) {
  mIncludedServices.AppendElement(aService);

  BluetoothGattService_Binding::ClearCachedIncludedServicesValue(this);
}

void BluetoothGattService::AppendCharacteristic(
    BluetoothGattCharacteristic* aChar) {
  mCharacteristics.AppendElement(aChar);
  BluetoothGattService_Binding::ClearCachedCharacteristicsValue(this);
}

void BluetoothGattService::AssignAppUuid(const nsAString& aAppUuid) {
  MOZ_ASSERT(mAttRole == ATT_SERVER_ROLE);

  mAppUuid = aAppUuid;
}

void BluetoothGattService::AssignServiceHandle(
    const BluetoothAttributeHandle& aServiceHandle) {
  MOZ_ASSERT(mAttRole == ATT_SERVER_ROLE);
  MOZ_ASSERT(!mActive);
  MOZ_ASSERT(!mServiceHandle.mHandle);

  mServiceHandle = aServiceHandle;
}

void BluetoothGattService::AssignCharacteristicHandle(
    const BluetoothUuid& aCharacteristicUuid,
    const BluetoothAttributeHandle& aCharacteristicHandle) {
  MOZ_ASSERT(mAttRole == ATT_SERVER_ROLE);
  MOZ_ASSERT(mActive);

  size_t index = mCharacteristics.IndexOf(aCharacteristicUuid);
  NS_ENSURE_TRUE_VOID(index != mCharacteristics.NoIndex);
  mCharacteristics[index]->AssignCharacteristicHandle(aCharacteristicHandle);
}

void BluetoothGattService::AssignDescriptorHandle(
    const BluetoothUuid& aDescriptorUuid,
    const BluetoothAttributeHandle& aCharacteristicHandle,
    const BluetoothAttributeHandle& aDescriptorHandle) {
  MOZ_ASSERT(mAttRole == ATT_SERVER_ROLE);
  MOZ_ASSERT(mActive);

  size_t index = mCharacteristics.IndexOf(aCharacteristicHandle);
  NS_ENSURE_TRUE_VOID(index != mCharacteristics.NoIndex);
  mCharacteristics[index]->AssignDescriptorHandle(aDescriptorUuid,
                                                  aDescriptorHandle);
}

void BluetoothGattService::UpdateAttributeHandles(
    const nsTArray<BluetoothGattDbElement>& aDbElements) {
  MOZ_ASSERT(mAttRole == ATT_SERVER_ROLE);

  BluetoothAttributeHandle currCharacteristicHandle;
  for (uint32_t i = 0; i < aDbElements.Length(); i++) {
    switch (aDbElements[i].mType) {
      case GATT_DB_TYPE_PRIMARY_SERVICE:
      case GATT_DB_TYPE_SECONDARY_SERVICE:
        AssignServiceHandle(aDbElements[i].mHandle);
        break;
      case GATT_DB_TYPE_CHARACTERISTIC:
        currCharacteristicHandle = aDbElements[i].mHandle;
        AssignCharacteristicHandle(aDbElements[i].mUuid,
                                   aDbElements[i].mHandle);
        break;
      case GATT_DB_TYPE_DESCRIPTOR:
        AssignDescriptorHandle(aDbElements[i].mUuid, currCharacteristicHandle,
                               aDbElements[i].mHandle);
        break;
      case GATT_DB_TYPE_INCLUDED_SERVICE: {
        // Do nothing, included service should be added first
        break;
      }
      case GATT_DB_TYPE_END_GUARD:
      default:
        BT_WARNING("Unhandled GATT DB type: %d", aDbElements[i].mType);
    }
  }

  // service is ready when attribute handles are assigned
  mActive = true;
}

uint16_t BluetoothGattService::GetHandleCount() const {
  uint16_t count = sHandleCount;
  for (size_t i = 0; i < mCharacteristics.Length(); ++i) {
    count += mCharacteristics[i]->GetHandleCount();
  }
  return count;
}

JSObject* BluetoothGattService::WrapObject(JSContext* aContext,
                                           JS::Handle<JSObject*> aGivenProto) {
  return BluetoothGattService_Binding::Wrap(aContext, this, aGivenProto);
}

already_AddRefed<BluetoothGattService> BluetoothGattService::Constructor(
    const GlobalObject& aGlobal, const BluetoothGattServiceInit& aInit,
    ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindowInner> window =
      do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<BluetoothGattService> service =
      new BluetoothGattService(window, aInit);

  return service.forget();
}

already_AddRefed<Promise> BluetoothGattService::AddCharacteristic(
    const nsAString& aCharacteristicUuid, const GattPermissions& aPermissions,
    const GattCharacteristicProperties& aProperties, const ArrayBuffer& aValue,
    ErrorResult& aRv) {
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  BT_ENSURE_TRUE_REJECT(mAttRole == ATT_SERVER_ROLE, promise,
                        NS_ERROR_UNEXPECTED);

  /* The service should not be actively acting with the Bluetooth backend.
   * Otherwise, characteristics cannot be added into the service. */
  BT_ENSURE_TRUE_REJECT(!mActive, promise, NS_ERROR_UNEXPECTED);

  RefPtr<BluetoothGattCharacteristic> characteristic =
      new BluetoothGattCharacteristic(GetParentObject(), this,
                                      aCharacteristicUuid, aPermissions,
                                      aProperties, aValue);

  mCharacteristics.AppendElement(characteristic);
  promise->MaybeResolve(characteristic);

  return promise.forget();
}

already_AddRefed<Promise> BluetoothGattService::AddIncludedService(
    BluetoothGattService& aIncludedService, ErrorResult& aRv) {
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  BT_ENSURE_TRUE_REJECT(mAttRole == ATT_SERVER_ROLE, promise,
                        NS_ERROR_UNEXPECTED);

  /* The service should not be actively acting with the Bluetooth backend.
   * Otherwise, included services cannot be added into the service. */
  BT_ENSURE_TRUE_REJECT(!mActive, promise, NS_ERROR_UNEXPECTED);

  /* The included service itself should be actively acting with the Bluetooth
   * backend. Otherwise, that service cannot be included by any services. */
  BT_ENSURE_TRUE_REJECT(aIncludedService.mActive, promise, NS_ERROR_UNEXPECTED);

  mIncludedServices.AppendElement(&aIncludedService);
  promise->MaybeResolve(JS::UndefinedHandleValue);

  return promise.forget();
}
