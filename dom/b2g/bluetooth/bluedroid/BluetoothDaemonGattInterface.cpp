/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothDaemonGattInterface.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Unused.h"

BEGIN_BLUETOOTH_NAMESPACE

using namespace mozilla::ipc;

//
// GATT module
//

BluetoothGattNotificationHandler*
    BluetoothDaemonGattModule::sNotificationHandler;

void BluetoothDaemonGattModule::SetNotificationHandler(
    BluetoothGattNotificationHandler* aNotificationHandler) {
  sNotificationHandler = aNotificationHandler;
}

void BluetoothDaemonGattModule::HandleSvc(const DaemonSocketPDUHeader& aHeader,
                                          DaemonSocketPDU& aPDU,
                                          DaemonSocketResultHandler* aRes) {
  static void (BluetoothDaemonGattModule::*const HandleOp[])(
      const DaemonSocketPDUHeader&, DaemonSocketPDU&,
      DaemonSocketResultHandler*) = {
      [0] = &BluetoothDaemonGattModule::HandleRsp,
      [1] = &BluetoothDaemonGattModule::HandleNtf};

  MOZ_ASSERT(!NS_IsMainThread());

  // Negate twice to map bit to 0/1
  unsigned long isNtf = !!(aHeader.mOpcode & 0x80);

  (this->*(HandleOp[isNtf]))(aHeader, aPDU, aRes);
}

// Commands
//

nsresult BluetoothDaemonGattModule::ClientRegisterClientCmd(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_REGISTER_CLIENT,
                                  16);  // Service UUID

  nsresult rv = PackPDU(aUuid, *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientUnregisterClientCmd(
    int aClientIf, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_UNREGISTER_CLIENT,
                                  4);  // Client Interface

  nsresult rv = PackPDU(aClientIf, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ScannerRegisterScannerCmd(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SCANNER_REGISTER_SCANNER,
                                  16);  // scan UUID

  nsresult rv = PackPDU(aUuid, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ScannerUnregisterScannerCmd(
    int aScannerId, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SCANNER_UNREGISTER,
                                  4);  // Scanner Interface

  nsresult rv = PackPDU(aScannerId, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::AdvertiserRegisterAdvertiserCmd(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_ADVERTISER_REGISTER_ADVERTISER,
      16);  // advertise UUID

  nsresult rv = PackPDU(aUuid, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::AdvertiserUnregisterCmd(
    uint8_t aAdvertiserId, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_ADVERTISER_UNREGISTER,
                                  1);  // advertiser ID

  nsresult rv = PackPDU(aAdvertiserId, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ScannerScanCmd(
    bool aStart, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SCANNER_SCAN,
                                  1);  // Start

  nsresult rv = PackPDU(PackConversion<bool, uint8_t>(aStart), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientConnectCmd(
    int aClientIf, const BluetoothAddress& aBdAddr, bool aIsDirect,
    BluetoothTransport aTransport, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_CONNECT,
                                  4 +      // Client Interface
                                      6 +  // Remote Address
                                      1 +  // Is Direct
                                      4);  // Transport

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr,
              PackConversion<bool, uint8_t>(aIsDirect),
              PackConversion<BluetoothTransport, int32_t>(aTransport), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientDisconnectCmd(
    int aClientIf, const BluetoothAddress& aBdAddr, int aConnId,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_DISCONNECT,
                                  4 +      // Client Interface
                                      6 +  // Remote Address
                                      4);  // Connection ID

  nsresult rv;
  rv = PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr,
               PackConversion<int, int32_t>(aConnId), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::AdvertiserStartAdvertisingCmd(
    uint8_t aAdvertiserId, const nsTArray<uint8_t>& aAdvData,
    const nsTArray<uint8_t>& aScanResponse, int aTimeout,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_ADVERTISER_START_ADVERTISING, 0);

  const int kAdvertisingChannel37 = (1 << 0);
  const int kAdvertisingChannel38 = (1 << 1);
  const int kAdvertisingChannel39 = (1 << 2);
  const int kAdvertisingChannelAll =
      (kAdvertisingChannel37 | kAdvertisingChannel38 | kAdvertisingChannel39);

  uint16_t advertisingEventProperties = aScanResponse.IsEmpty()
                                            ? 0x0013   // ADV_IND
                                            : 0x0012;  // ADV_SCAN_IND
  uint32_t minInterval = 1600;
  uint32_t maxInterval = 1700;
  uint8_t channelMap = kAdvertisingChannelAll;
  int8_t txPower = kDefaultTxPower;
  uint8_t primaryAdvertisingPhy = 0x01;
  uint8_t secondaryAdvertisingPhy = 0x01;
  uint8_t scanRequestNotificationEnable = 0;

  uint16_t advDataLen = aAdvData.Length() * sizeof(uint8_t);
  uint8_t* advData = const_cast<uint8_t*>(aAdvData.Elements());

  uint16_t scanResponseLen = aScanResponse.Length() * sizeof(uint8_t);
  uint8_t* scanResponse = const_cast<uint8_t*>(aScanResponse.Elements());

  // TODO: support AdvertiseParameters and scanResponse
  nsresult rv = PackPDU(
      aAdvertiserId, advertisingEventProperties, minInterval, maxInterval,
      channelMap, txPower, primaryAdvertisingPhy, secondaryAdvertisingPhy,
      scanRequestNotificationEnable, advDataLen, scanResponseLen,
      PackArray<uint8_t>(advData, aAdvData.Length()),
      PackArray<uint8_t>(scanResponse, aScanResponse.Length()), aTimeout, *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientRefreshCmd(
    int aClientIf, const BluetoothAddress& aBdAddr,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_REFRESH,
                                  4 +      // Client Interface
                                      6);  // Remote Address

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientSearchServiceCmd(
    int aConnId, bool aFiltered, const BluetoothUuid& aUuid,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_SEARCH_SERVICE,
                                  4 +       // Connection ID
                                      1 +   // Filtered
                                      16);  // UUID

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aConnId),
                        PackConversion<bool, uint8_t>(aFiltered),
                        PackReversed<BluetoothUuid>(aUuid), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientReadCharacteristicCmd(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattAuthReq aAuthReq, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_READ_CHARACTERISTIC,
                                  4 +      // Connection ID
                                      2 +  // Handle
                                      4);  // Authorization

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aConnId), aHandle, aAuthReq, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientWriteCharacteristicCmd(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattWriteType aWriteType, int aLength,
    BluetoothGattAuthReq aAuthReq, char* aValue,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_CLIENT_WRITE_CHARACTERISTIC, 0);

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aConnId), aHandle,
                        aWriteType, PackConversion<int, int32_t>(aLength),
                        aAuthReq, PackArray<char>(aValue, aLength), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientReadDescriptorCmd(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattAuthReq aAuthReq, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_READ_DESCRIPTOR,
                                  4 +      // Connection ID
                                      2 +  // Handle
                                      4);  // Authorization

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aConnId), aHandle, aAuthReq, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientWriteDescriptorCmd(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattWriteType aWriteType, int aLength,
    BluetoothGattAuthReq aAuthReq, char* aValue,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_CLIENT_WRITE_DESCRIPTOR, 0);

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aConnId), aHandle,
                        aWriteType, PackConversion<int, int32_t>(aLength),
                        aAuthReq, PackArray<char>(aValue, aLength), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientExecuteWriteCmd(
    int aConnId, int aIsExecute, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_EXECUTE_WRITE,
                                  4 +      // Connection ID
                                      4);  // Execute

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aConnId),
                        PackConversion<int, int32_t>(aIsExecute), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientRegisterForNotificationCmd(
    int aClientIf, const BluetoothAddress& aBdAddr,
    const BluetoothAttributeHandle& aHandle, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_CLIENT_REGISTER_FOR_NOTIFICATION,
      4 +      // Client Interface
          6 +  // Remote Address
          2);  // Handle

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr, aHandle, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientDeregisterForNotificationCmd(
    int aClientIf, const BluetoothAddress& aBdAddr,
    const BluetoothAttributeHandle& aHandle, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu = MakeUnique<DaemonSocketPDU>(
      SERVICE_ID, OPCODE_CLIENT_DEREGISTER_FOR_NOTIFICATION,
      4 +      // Client Interface
          6 +  // Remote Address
          2);  // Handle

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr, aHandle, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientReadRemoteRssiCmd(
    int aClientIf, const BluetoothAddress& aBdAddr,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_READ_REMOTE_RSSI,
                                  4 +      // Client Interface
                                      6);  // Remote Address

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aClientIf), aBdAddr, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientGetDeviceTypeCmd(
    const BluetoothAddress& aBdAddr, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_GET_DEVICE_TYPE,
                                  6);  // Remote Address

  nsresult rv = PackPDU(aBdAddr, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientTestCommandCmd(
    int aCommand, const BluetoothGattTestParam& aTestParam,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_TEST_COMMAND,
                                  4 +       // Command
                                      6 +   // Address
                                      16 +  // UUID
                                      2 +   // U1
                                      2 +   // U2
                                      2 +   // U3
                                      2 +   // U4
                                      2);   // U5

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aCommand),
                        aTestParam.mBdAddr, aTestParam.mU1, aTestParam.mU2,
                        aTestParam.mU3, aTestParam.mU4, aTestParam.mU5, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ClientGetGattDbCmd(
    int aConnId, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_CLIENT_GET_GATT_DB,
                                  4);  // Connection ID

  nsresult rv;
  rv = PackPDU(PackConversion<int, int32_t>(aConnId), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerRegisterServerCmd(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_REGISTER_SERVER,
                                  16);  // Uuid

  nsresult rv = PackPDU(aUuid, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerUnregisterServerCmd(
    int aServerIf, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_REGISTER_SERVER,
                                  4);  // Server Interface

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aServerIf), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerConnectCmd(
    int aServerIf, const BluetoothAddress& aBdAddr, bool aIsDirect,
    BluetoothTransport aTransport, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_CONNECT,
                                  4 +      // Server Interface
                                      6 +  // Remote Address
                                      1 +  // Is Direct
                                      4);  // Transport

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aServerIf), aBdAddr,
              PackConversion<bool, uint8_t>(aIsDirect),
              PackConversion<BluetoothTransport, int32_t>(aTransport), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerDisconnectCmd(
    int aServerIf, const BluetoothAddress& aBdAddr, int aConnId,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_DISCONNECT,
                                  4 +      // Server Interface
                                      6 +  // Remote Address
                                      4);  // Connection Id

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aServerIf), aBdAddr,
                        PackConversion<int, int32_t>(aConnId), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerAddServiceCmd(
    int aServerIf, const nsTArray<BluetoothGattDbElement>& aDb,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  size_t len = aDb.Length();

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_ADD_SERVICE,
                                  4 +             // Server Interface
                                      len * 31);  // DB elements

  nsresult rv = PackPDU(PackConversion<int, int32_t>(aServerIf), aDb, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerStopServiceCmd(
    int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_STOP_SERVICE,
                                  4 +      // Server Interface
                                      4);  // Service Handle

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aServerIf), aServiceHandle, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerDeleteServiceCmd(
    int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_DELETE_SERVICE,
                                  4 +      // Server Interface
                                      4);  // Service Handle

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aServerIf), aServiceHandle, *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerSendIndicationCmd(
    int aServerIf, const BluetoothAttributeHandle& aCharacteristicHandle,
    int aConnId, int aLength, bool aConfirm, uint8_t* aValue,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_SEND_INDICATION, 0);

  nsresult rv =
      PackPDU(PackConversion<int, int32_t>(aServerIf), aCharacteristicHandle,
              PackConversion<int, int32_t>(aConnId),
              PackConversion<int, int32_t>(aLength),
              PackConversion<bool, int32_t>(aConfirm),
              PackArray<uint8_t>(aValue, aLength), *pdu);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

nsresult BluetoothDaemonGattModule::ServerSendResponseCmd(
    int aConnId, int aTransId, uint16_t aStatus,
    const BluetoothGattResponse& aResponse, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(NS_IsMainThread());

  UniquePtr<DaemonSocketPDU> pdu =
      MakeUnique<DaemonSocketPDU>(SERVICE_ID, OPCODE_SERVER_SEND_RESPONSE, 0);

  nsresult rv = PackPDU(
      PackConversion<int, int32_t>(aConnId),
      PackConversion<int, int32_t>(aTransId),
      PackConversion<BluetoothAttributeHandle, uint16_t>(aResponse.mHandle),
      aResponse.mOffset,
      PackConversion<BluetoothGattAuthReq, uint8_t>(aResponse.mAuthReq),
      PackConversion<uint16_t, int32_t>(aStatus), aResponse.mLength,
      PackArray<uint8_t>(aResponse.mValue, aResponse.mLength), *pdu);

  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = Send(pdu.get(), aRes);
  if (NS_FAILED(rv)) {
    return rv;
  }
  Unused << pdu.release();
  return NS_OK;
}

// Responses
//

void BluetoothDaemonGattModule::ErrorRsp(const DaemonSocketPDUHeader& aHeader,
                                         DaemonSocketPDU& aPDU,
                                         BluetoothGattResultHandler* aRes) {
  ErrorRunnable::Dispatch(aRes, &BluetoothGattResultHandler::OnError,
                          UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientRegisterClientRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::RegisterClient,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientUnregisterClientRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::UnregisterClient,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ScannerRegisterScannerRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::RegisterScanner,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ScannerUnregisterScannerRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::UnregisterScanner,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::AdvertiserRegisterAdvertiserRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::RegisterAdvertiser,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::AdvertiserUnregisterRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::UnregisterAdvertiser,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ScannerScanRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::Scan,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientConnectRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::Connect,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientDisconnectRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::Disconnect,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::AdvertiserStartAdvertisingRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::Advertise,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientRefreshRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::Refresh,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientSearchServiceRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::SearchService,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadCharacteristicRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::ReadCharacteristic,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientWriteCharacteristicRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::WriteCharacteristic,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadDescriptorRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::ReadDescriptor,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientWriteDescriptorRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::WriteDescriptor,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientExecuteWriteRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::ExecuteWrite,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientRegisterForNotificationRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::RegisterNotification,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientDeregisterForNotificationRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::DeregisterNotification,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadRemoteRssiRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::ReadRemoteRssi,
                           UnpackPDUInitOp(aPDU));
}

// Init operator class for ClientGetDeviceTypeRsp
class BluetoothDaemonGattModule::ClientGetDeviceTypeInitOp final
    : private PDUInitOp {
 public:
  explicit ClientGetDeviceTypeInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}

  nsresult operator()(BluetoothTypeOfDevice& aArg1) const {
    /* Read device type */
    nsresult rv = UnpackPDU(
        GetPDU(), UnpackConversion<uint8_t, BluetoothTypeOfDevice>(aArg1));
    if (NS_FAILED(rv)) {
      return rv;
    }
    WarnAboutTrailingData();
    return NS_OK;
  }
};

void BluetoothDaemonGattModule::ClientGetDeviceTypeRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ClientGetDeviceTypeResultRunnable::Dispatch(
      aRes, &BluetoothGattResultHandler::GetDeviceType,
      ClientGetDeviceTypeInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientTestCommandRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::TestCommand,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientGetGattDbRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::GetGattDb,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerRegisterServerRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::RegisterServer,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerUnregisterServerRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::UnregisterServer,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerConnectRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::ConnectPeripheral,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerDisconnectRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes,
                           &BluetoothGattResultHandler::DisconnectPeripheral,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerAddServiceRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::AddService,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerStopServiceRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::StopService,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerDeleteServiceRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::DeleteService,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerSendIndicationRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::SendIndication,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerSendResponseRsp(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
    BluetoothGattResultHandler* aRes) {
  ResultRunnable::Dispatch(aRes, &BluetoothGattResultHandler::SendResponse,
                           UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::HandleRsp(const DaemonSocketPDUHeader& aHeader,
                                          DaemonSocketPDU& aPDU,
                                          DaemonSocketResultHandler* aRes) {
  static void (BluetoothDaemonGattModule::*const HandleRsp[])(
      const DaemonSocketPDUHeader&, DaemonSocketPDU&,
      BluetoothGattResultHandler*) = {
      [OPCODE_ERROR] = &BluetoothDaemonGattModule::ErrorRsp,
      [OPCODE_CLIENT_REGISTER_CLIENT] =
          &BluetoothDaemonGattModule::ClientRegisterClientRsp,
      [OPCODE_CLIENT_UNREGISTER_CLIENT] =
          &BluetoothDaemonGattModule::ClientUnregisterClientRsp,
      //  OPCODE_CLIENT_SCAN will be removed
      // [OPCODE_CLIENT_SCAN] = &BluetoothDaemonGattModule::ClientScanRsp,
      [OPCODE_CLIENT_CONNECT] = &BluetoothDaemonGattModule::ClientConnectRsp,
      [OPCODE_CLIENT_DISCONNECT] =
          &BluetoothDaemonGattModule::ClientDisconnectRsp,
      [OPCODE_CLIENT_REFRESH] = &BluetoothDaemonGattModule::ClientRefreshRsp,
      [OPCODE_CLIENT_SEARCH_SERVICE] =
          &BluetoothDaemonGattModule::ClientSearchServiceRsp,
      //  OPCODE_CLIENT_GET_INCLUDED_SERVICE will be removed
      // [OPCODE_CLIENT_GET_INCLUDED_SERVICE] =
      //     &BluetoothDaemonGattModule::ClientGetIncludedServiceRsp,
      //  OPCODE_CLIENT_GET_CHARACTERISTIC will be removed
      // [OPCODE_CLIENT_GET_CHARACTERISTIC] =
      //     &BluetoothDaemonGattModule::ClientGetCharacteristicRsp,
      //  OPCODE_CLIENT_GET_DESCRIPTOR will be removed
      // [OPCODE_CLIENT_GET_DESCRIPTOR] =
      //     &BluetoothDaemonGattModule::ClientGetDescriptorRsp,
      [OPCODE_CLIENT_READ_CHARACTERISTIC] =
          &BluetoothDaemonGattModule::ClientReadCharacteristicRsp,
      [OPCODE_CLIENT_WRITE_CHARACTERISTIC] =
          &BluetoothDaemonGattModule::ClientWriteCharacteristicRsp,
      [OPCODE_CLIENT_READ_DESCRIPTOR] =
          &BluetoothDaemonGattModule::ClientReadDescriptorRsp,
      [OPCODE_CLIENT_WRITE_DESCRIPTOR] =
          &BluetoothDaemonGattModule::ClientWriteDescriptorRsp,
      [OPCODE_CLIENT_EXECUTE_WRITE] =
          &BluetoothDaemonGattModule::ClientExecuteWriteRsp,
      [OPCODE_CLIENT_REGISTER_FOR_NOTIFICATION] =
          &BluetoothDaemonGattModule::ClientRegisterForNotificationRsp,
      [OPCODE_CLIENT_DEREGISTER_FOR_NOTIFICATION] =
          &BluetoothDaemonGattModule::ClientDeregisterForNotificationRsp,
      [OPCODE_CLIENT_READ_REMOTE_RSSI] =
          &BluetoothDaemonGattModule::ClientReadRemoteRssiRsp,
      [OPCODE_CLIENT_GET_DEVICE_TYPE] =
          &BluetoothDaemonGattModule::ClientGetDeviceTypeRsp,
      [OPCODE_CLIENT_TEST_COMMAND] =
          &BluetoothDaemonGattModule::ClientTestCommandRsp,
      [OPCODE_CLIENT_GET_GATT_DB] =
          &BluetoothDaemonGattModule::ClientGetGattDbRsp,
      [OPCODE_SERVER_REGISTER_SERVER] =
          &BluetoothDaemonGattModule::ServerRegisterServerRsp,
      [OPCODE_SERVER_UNREGISTER_SERVER] =
          &BluetoothDaemonGattModule::ServerUnregisterServerRsp,
      [OPCODE_SERVER_CONNECT] = &BluetoothDaemonGattModule::ServerConnectRsp,
      [OPCODE_SERVER_DISCONNECT] =
          &BluetoothDaemonGattModule::ServerDisconnectRsp,
      [OPCODE_SERVER_ADD_SERVICE] =
          &BluetoothDaemonGattModule::ServerAddServiceRsp,
      //  OPCODE_SERVER_ADD_INCLUDED_SERVICE will be removed
      // [OPCODE_SERVER_ADD_INCLUDED_SERVICE] =
      //     &BluetoothDaemonGattModule::ServerAddIncludedServiceRsp,
      //  OPCODE_SERVER_ADD_CHARACTERISTIC will be removed
      // [OPCODE_SERVER_ADD_CHARACTERISTIC] =
      //     &BluetoothDaemonGattModule::ServerAddCharacteristicRsp,
      //  OPCODE_SERVER_ADD_DESCRIPTOR will be removed
      // [OPCODE_SERVER_ADD_DESCRIPTOR] =
      //     &BluetoothDaemonGattModule::ServerAddDescriptorRsp,
      //  OPCODE_SERVER_START_SERVICE will be removed
      // [OPCODE_SERVER_START_SERVICE] =
      //     &BluetoothDaemonGattModule::ServerStartServiceRsp,
      [OPCODE_SERVER_STOP_SERVICE] =
          &BluetoothDaemonGattModule::ServerStopServiceRsp,
      [OPCODE_SERVER_DELETE_SERVICE] =
          &BluetoothDaemonGattModule::ServerDeleteServiceRsp,
      [OPCODE_SERVER_SEND_INDICATION] =
          &BluetoothDaemonGattModule::ServerSendIndicationRsp,
      [OPCODE_SERVER_SEND_RESPONSE] =
          &BluetoothDaemonGattModule::ServerSendResponseRsp,
      [OPCODE_SCANNER_REGISTER_SCANNER] =
          &BluetoothDaemonGattModule::ScannerRegisterScannerRsp,
      [OPCODE_SCANNER_UNREGISTER] =
          &BluetoothDaemonGattModule::ScannerUnregisterScannerRsp,
      [OPCODE_SCANNER_SCAN] = &BluetoothDaemonGattModule::ScannerScanRsp,

      [OPCODE_ADVERTISER_REGISTER_ADVERTISER] =
          &BluetoothDaemonGattModule::AdvertiserRegisterAdvertiserRsp,
      [OPCODE_ADVERTISER_UNREGISTER] =
          &BluetoothDaemonGattModule::AdvertiserUnregisterRsp,
      [OPCODE_ADVERTISER_START_ADVERTISING] =
          &BluetoothDaemonGattModule::AdvertiserStartAdvertisingRsp,
  };

  MOZ_ASSERT(!NS_IsMainThread());  // I/O thread

  if (NS_WARN_IF(!(aHeader.mOpcode < MOZ_ARRAY_LENGTH(HandleRsp))) ||
      NS_WARN_IF(!HandleRsp[aHeader.mOpcode])) {
    return;
  }

  RefPtr<BluetoothGattResultHandler> res =
      static_cast<BluetoothGattResultHandler*>(aRes);

  if (!res) {
    return;
  }  // Return early if no result handler has been set for response

  (this->*(HandleRsp[aHeader.mOpcode]))(aHeader, aPDU, res);
}

// Notifications
//

// Returns the current notification handler to a notification runnable
class BluetoothDaemonGattModule::NotificationHandlerWrapper final {
 public:
  typedef BluetoothGattNotificationHandler ObjectType;

  static ObjectType* GetInstance() {
    MOZ_ASSERT(NS_IsMainThread());

    return sNotificationHandler;
  }
};

void BluetoothDaemonGattModule::ClientRegisterClientNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientRegisterNotification::Dispatch(
      &BluetoothGattNotificationHandler::RegisterClientNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ScannerRegisterScannerNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ScannerRegisterNotification::Dispatch(
      &BluetoothGattNotificationHandler::RegisterScannerNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::AdvertiserRegisterAdvertiserNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ScannerRegisterNotification::Dispatch(
      &BluetoothGattNotificationHandler::RegisterAdvertiserNotification,
      UnpackPDUInitOp(aPDU));
}

// Init operator class for ScannerScanResultNotification
class BluetoothDaemonGattModule::ScannerScanResultInitOp final
    : private PDUInitOp {
 public:
  explicit ScannerScanResultInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}

  nsresult operator()(BluetoothAddress& aArg1, int& aArg2,
                      BluetoothGattAdvData& aArg3) const {
    DaemonSocketPDU& pdu = GetPDU();

    /* Read address */
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read RSSI */
    rv = UnpackPDU(pdu, aArg2);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read Length */
    uint16_t length;
    rv = UnpackPDU(pdu, length);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read Adv Data */
    rv = UnpackPDU(pdu, UnpackArray<uint8_t>(aArg3.mAdvData, length));
    if (NS_FAILED(rv)) {
      return rv;
    }

    WarnAboutTrailingData();
    return NS_OK;
  }
};

void BluetoothDaemonGattModule::ScannerScanResultNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ScannerScanResultNotification::Dispatch(
      &BluetoothGattNotificationHandler::ScanResultNotification,
      ScannerScanResultInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientConnectNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientConnectNotification::Dispatch(
      &BluetoothGattNotificationHandler::ConnectNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientDisconnectNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientDisconnectNotification::Dispatch(
      &BluetoothGattNotificationHandler::DisconnectNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientSearchCompleteNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientSearchCompleteNotification::Dispatch(
      &BluetoothGattNotificationHandler::SearchCompleteNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientRegisterForNotificationNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientRegisterForNotificationNotification::Dispatch(
      &BluetoothGattNotificationHandler::RegisterNotificationNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientNotifyNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientNotifyNotification::Dispatch(
      &BluetoothGattNotificationHandler::NotifyNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadCharacteristicNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientReadCharacteristicNotification::Dispatch(
      &BluetoothGattNotificationHandler::ReadCharacteristicNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientWriteCharacteristicNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientWriteCharacteristicNotification::Dispatch(
      &BluetoothGattNotificationHandler::WriteCharacteristicNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadDescriptorNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientReadDescriptorNotification::Dispatch(
      &BluetoothGattNotificationHandler::ReadDescriptorNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientWriteDescriptorNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientWriteDescriptorNotification::Dispatch(
      &BluetoothGattNotificationHandler::WriteDescriptorNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientExecuteWriteNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientExecuteWriteNotification::Dispatch(
      &BluetoothGattNotificationHandler::ExecuteWriteNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientReadRemoteRssiNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientReadRemoteRssiNotification::Dispatch(
      &BluetoothGattNotificationHandler::ReadRemoteRssiNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::AdvertiserStartAdvertisingNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  AdvertiseNotification::Dispatch(
      &BluetoothGattNotificationHandler::AdvertiseNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ClientGetGattDbNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ClientGetGattDbNotification::Dispatch(
      &BluetoothGattNotificationHandler::GetGattDbNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerRegisterServerNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerRegisterNotification::Dispatch(
      &BluetoothGattNotificationHandler::RegisterServerNotification,
      UnpackPDUInitOp(aPDU));
}

// Init operator class for ServerConnectionNotification
class BluetoothDaemonGattModule::ServerConnectionInitOp final
    : private PDUInitOp {
 public:
  explicit ServerConnectionInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}

  nsresult operator()(int& aArg1, int& aArg2, bool& aArg3,
                      BluetoothAddress& aArg4) const {
    DaemonSocketPDU& pdu = GetPDU();

    /* Read connection ID */
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read server interface */
    rv = UnpackPDU(pdu, aArg2);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read connected */
    rv = UnpackPDU(pdu, UnpackConversion<int32_t, bool>(aArg3));
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read address */
    rv = UnpackPDU(pdu, aArg4);
    if (NS_FAILED(rv)) {
      return rv;
    }

    WarnAboutTrailingData();
    return NS_OK;
  }
};

void BluetoothDaemonGattModule::ServerConnectionNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerConnectionNotification::Dispatch(
      &BluetoothGattNotificationHandler::ConnectionNotification,
      ServerConnectionInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerServiceAddedNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerServiceAddedNotification::Dispatch(
      &BluetoothGattNotificationHandler::ServiceAddedNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerServiceStoppedNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerServiceStoppedNotification::Dispatch(
      &BluetoothGattNotificationHandler::ServiceStoppedNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerServiceDeletedNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerServiceDeletedNotification::Dispatch(
      &BluetoothGattNotificationHandler::ServiceDeletedNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerRequestReadNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerRequestReadNotification::Dispatch(
      &BluetoothGattNotificationHandler::RequestReadNotification,
      UnpackPDUInitOp(aPDU));
}

// Init operator class for ServerRequestWriteNotification
class BluetoothDaemonGattModule::ServerRequestWriteInitOp final
    : private PDUInitOp {
 public:
  explicit ServerRequestWriteInitOp(DaemonSocketPDU& aPDU) : PDUInitOp(aPDU) {}

  nsresult operator()(int& aArg1, int& aArg2, BluetoothAddress& aArg3,
                      BluetoothAttributeHandle& aArg4, int& aArg5, int& aArg6,
                      UniquePtr<uint8_t[]>& aArg7, bool& aArg8,
                      bool& aArg9) const {
    DaemonSocketPDU& pdu = GetPDU();

    /* Read connection ID */
    nsresult rv = UnpackPDU(pdu, aArg1);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read trans ID */
    rv = UnpackPDU(pdu, aArg2);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read address */
    rv = UnpackPDU(pdu, aArg3);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read attribute handle */
    rv = UnpackPDU(pdu, aArg4);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read offset */
    rv = UnpackPDU(pdu, aArg5);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read Length */
    rv = UnpackPDU(pdu, aArg6);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read value */
    rv =
        UnpackPDU(pdu, UnpackArray<uint8_t>(aArg7, static_cast<size_t>(aArg6)));
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read need response */
    rv = UnpackPDU(pdu, aArg8);
    if (NS_FAILED(rv)) {
      return rv;
    }
    /* Read isPrepare */
    rv = UnpackPDU(pdu, aArg9);
    if (NS_FAILED(rv)) {
      return rv;
    }

    WarnAboutTrailingData();
    return NS_OK;
  }
};

void BluetoothDaemonGattModule::ServerRequestWriteNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerRequestWriteNotification::Dispatch(
      &BluetoothGattNotificationHandler::RequestWriteNotification,
      ServerRequestWriteInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerRequestExecuteWriteNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerRequestExecuteWriteNotification::Dispatch(
      &BluetoothGattNotificationHandler::RequestExecuteWriteNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::ServerResponseConfirmationNtf(
    const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU) {
  ServerResponseConfirmationNotification::Dispatch(
      &BluetoothGattNotificationHandler::ResponseConfirmationNotification,
      UnpackPDUInitOp(aPDU));
}

void BluetoothDaemonGattModule::HandleNtf(const DaemonSocketPDUHeader& aHeader,
                                          DaemonSocketPDU& aPDU,
                                          DaemonSocketResultHandler* aRes) {
  static void (BluetoothDaemonGattModule::*const HandleNtf[])(
      const DaemonSocketPDUHeader&, DaemonSocketPDU&) = {
      // ----- GATT client, [0] - [31] -----
      [0] = &BluetoothDaemonGattModule::ClientRegisterClientNtf,
      [1] = &BluetoothDaemonGattModule::ClientConnectNtf,
      [2] = &BluetoothDaemonGattModule::ClientDisconnectNtf,
      [3] = &BluetoothDaemonGattModule::ClientSearchCompleteNtf,
      // TODO: srvc_id, char_id -> handle
      [4] = &BluetoothDaemonGattModule::ClientRegisterForNotificationNtf,
      [5] = &BluetoothDaemonGattModule::ClientNotifyNtf,
      [6] = &BluetoothDaemonGattModule::ClientReadCharacteristicNtf,
      // TODO: btgatt_write_params_t -> handle
      [7] = &BluetoothDaemonGattModule::ClientWriteCharacteristicNtf,
      [8] = &BluetoothDaemonGattModule::ClientReadDescriptorNtf,
      // TODO: btgatt_write_params_t -> handle
      [9] = &BluetoothDaemonGattModule::ClientWriteDescriptorNtf,
      [10] = &BluetoothDaemonGattModule::ClientExecuteWriteNtf,
      [11] = &BluetoothDaemonGattModule::ClientReadRemoteRssiNtf,

      // TODO: Replace ClientScanResultNtf by LE scanner
      // [1] = &BluetoothDaemonGattModule::ClientScanResultNtf,

      // TODO: Replace the following NTFs by OPCODE_CLIENT_GET_GATT_DB_NTF
      // [5] = &BluetoothDaemonGattModule::ClientSearchResultNtf,
      // [6] = &BluetoothDaemonGattModule::ClientGetCharacteristicNtf,
      // [7] = &BluetoothDaemonGattModule::ClientGetDescriptorNtf,
      // [8] = &BluetoothDaemonGattModule::ClientGetIncludedServiceNtf,

      // TODO: Support the following NTF as new feature
      //   [12] OPCODE_CLIENT_CONFIGURE_MTU_NTF    (0x8d)
      //   [13] OPCODE_CLIENT_CONGESTION_NTF       (0x8e)
      //   [14] OPCODE_CLIENT_GET_GATT_DB_NTF      (0x8f)
      [14] = &BluetoothDaemonGattModule::ClientGetGattDbNtf,
      //   [15] OPCODE_CLIENT_SERVICES_REMOVED_NTF (0x90)
      //   [16] OPCODE_CLIENT_SERVICES_ADDED_NTF   (0x91)
      //   [17] OPCODE_CLIENT_PHY_UPDATED_NTF      (0x92)
      //   [18] OPCODE_CLIENT_CONN_UPDATED_NTF     (0x93)

      // ----- GATT server, [32] - [63] -----
      [32] = &BluetoothDaemonGattModule::ServerRegisterServerNtf,
      [33] = &BluetoothDaemonGattModule::ServerConnectionNtf,
      [34] = &BluetoothDaemonGattModule::ServerServiceAddedNtf,
      [35] = &BluetoothDaemonGattModule::ServerServiceStoppedNtf,
      [36] = &BluetoothDaemonGattModule::ServerServiceDeletedNtf,
      // TODO: replace it by OPCODE_SERVER_REQUEST_READ_CHARACTERISTIC_NTF
      [37] = &BluetoothDaemonGattModule::ServerRequestReadNtf,
      // TODO: replace it by OPCODE_SERVER_REQUEST_READ_DESCRIPTOR_NTF
      [38] = &BluetoothDaemonGattModule::ServerRequestReadNtf,
      // TODO: replace it by OPCODE_SERVER_REQUEST_WRITE_CHARACTERISTIC_NTF
      [39] = &BluetoothDaemonGattModule::ServerRequestWriteNtf,
      // TODO: replace it by OPCODE_SERVER_REQUEST_WRITE_DESCRIPTOR_NTF
      [40] = &BluetoothDaemonGattModule::ServerRequestWriteNtf,

      // TODO: Support the following NTF as new feature
      //   [40] OPCODE_SERVER_REQUEST_EXEC_WRITE_NTF    (0xaa)
      //   [41] OPCODE_SERVER_RESPONSE_CONFIRMATION_NTF (0xab)
      //   [42] OPCODE_SERVER_INDICATION_SENT_NTF       (0xac)
      //   [43] OPCODE_SERVER_CONGESTION_NTF            (0xad)
      //   [44] OPCODE_SERVER_MTU_CHANGED_NTF           (0xae)
      //   [45] OPCODE_SERVER_PHY_UPDATED_NTF           (0xaf)
      //   [46] OPCODE_SERVER_CONN_UPDATED_NTF          (0xb0)

      // ----- LE scanner, [64] - [95] -----
      // TODO: Support the following NTF as new feature
      //   [64] OPCODE_SCANNER_REGISTER_SCANNER_NTF         (0xc1)
      [64] = &BluetoothDaemonGattModule::ScannerRegisterScannerNtf,
      //   [65] OPCODE_SCANNER_SCAN_FILTER_PARAM_SETUP_NTF  (0xc2)
      //   [66] OPCODE_SCANNER_SCAN_FILTER_ADD_NTF          (0xc3)
      //   [67] OPCODE_SCANNER_SCAN_FILTER_CLEAR_NTF        (0xc4)
      //   [68] OPCODE_SCANNER_SCAN_FILTER_ENABLE_NTF       (0xc5)
      //   [69] OPCODE_SCANNER_SET_SCAN_PARAMETERS_NTF      (0xc6)
      //   [70] OPCODE_SCANNER_BATCHSCAN_CONFIG_STORAGE_NTF (0xc7)
      //   [71] OPCODE_SCANNER_BATCHSCAN_ENABLE_NTF         (0xc8)
      //   [72] OPCODE_SCANNER_BATCHSCAN_DISABLE_NTF        (0xc9)
      //   [73] OPCODE_SCANNER_START_SYNC_NTF               (0xca)
      //   [74] OPCODE_SCANNER_START_SYNC_REPORT_NTF        (0xcb)
      //   [75] OPCODE_SCANNER_START_SYNC_LOST_NTF          (0xcc)
      //   [76] OPCODE_SCANNER_SCAN_RESULT_NTF              (0xcd)
      [76] = &BluetoothDaemonGattModule::ScannerScanResultNtf,
      //   [77] OPCODE_SCANNER_BATCHSCAN_REPORTS_NTF        (0xce)
      //   [78] OPCODE_SCANNER_BATCHSCAN_THRESHOLD_NTF      (0xcf)
      //   [79] OPCODE_SCANNER_TRACK_ADV_EVENT_NTF          (0xd0)

      // ----- LE advertiser, [96] - [127] -----
      [96] = &BluetoothDaemonGattModule::AdvertiserRegisterAdvertiserNtf,
      [102] = &BluetoothDaemonGattModule::AdvertiserStartAdvertisingNtf,

      // TODO: Support the following NTF as new feature
      //   [97]  OPCODE_ADVERTISER_GET_OWN_ADDRESS_NTF                    (0xe2)
      //   [98]  OPCODE_ADVERTISER_SET_PARAMETERS_NTF                     (0xe3)
      //   [99]  OPCODE_ADVERTISER_SET_DATA_NTF                           (0xe4)
      //   [100] OPCODE_ADVERTISER_ENABLE_NTF                             (0xe5)
      //   [101] OPCODE_ADVERTISER_ENABLE_TIMEOUT_NTF                     (0xe6)
      //   [103] OPCODE_ADVERTISER_START_ADVERTISING_TIMEOUT_NTF          (0xe8)
      //   [104] OPCODE_ADVERTISER_START_ADVERTISING_SET_NTF              (0xe9)
      //   [105] OPCODE_ADVERTISER_START_ADVERTISING_SET_TIMEOUT_NTF      (0xea)
      //   [106] OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_PARAMETERS_NTF(0xeb)
      //   [107] OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_DATA_NTF      (0xec)
      //   [108] OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_ENABLE_NTF    (0xed)
  };
  MOZ_ASSERT(!NS_IsMainThread());

  // opcode 0x01 - 0x80 are reserved for daemon command/response
  // daemon notification starts from 0x81
  uint8_t index = aHeader.mOpcode - 0x81;

  if (NS_WARN_IF(!(index < MOZ_ARRAY_LENGTH(HandleNtf))) ||
      NS_WARN_IF(!HandleNtf[index])) {
    return;
  }

  (this->*(HandleNtf[index]))(aHeader, aPDU);
}

//
// Gatt interface
//

BluetoothDaemonGattInterface::BluetoothDaemonGattInterface(
    BluetoothDaemonGattModule* aModule)
    : mModule(aModule) {}

BluetoothDaemonGattInterface::~BluetoothDaemonGattInterface() {}

void BluetoothDaemonGattInterface::SetNotificationHandler(
    BluetoothGattNotificationHandler* aNotificationHandler) {
  MOZ_ASSERT(mModule);

  mModule->SetNotificationHandler(aNotificationHandler);
}

/* Register / Unregister */
void BluetoothDaemonGattInterface::RegisterClient(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientRegisterClientCmd(aUuid, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::UnregisterClient(
    int aClientIf, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientUnregisterClientCmd(aClientIf, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::RegisterScanner(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ScannerRegisterScannerCmd(aUuid, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::UnregisterScanner(
    int aScannerId, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ScannerUnregisterScannerCmd(aScannerId, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::RegisterAdvertiser(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->AdvertiserRegisterAdvertiserCmd(aUuid, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::UnregisterAdvertiser(
    uint8_t aAdvertiserId, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->AdvertiserUnregisterCmd(aAdvertiserId, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Start / Stop LE Scan */
void BluetoothDaemonGattInterface::Scan([[maybe_unused]] int aScannerId,
                                        bool aStart,
                                        BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ScannerScanCmd(aStart, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Connect / Disconnect */

void BluetoothDaemonGattInterface::Connect(int aClientIf,
                                           const BluetoothAddress& aBdAddr,
                                           bool aIsDirect,
                                           BluetoothTransport aTransport,
                                           BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientConnectCmd(aClientIf, aBdAddr, aIsDirect,
                                          aTransport, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::Disconnect(
    int aClientIf, const BluetoothAddress& aBdAddr, int aConnId,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientDisconnectCmd(aClientIf, aBdAddr, aConnId, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Start LE advertising */
void BluetoothDaemonGattInterface::Advertise(
    uint8_t aAdvertiserId, const BluetoothGattAdvertisingData& aData,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsTArray<uint8_t> advData, scanResponse;
  GattAdvertisingDataToBytes(aData, advData);

  // TODO: support scanResponse and timeout adjustment
  nsresult rv = mModule->AdvertiserStartAdvertisingCmd(
      aAdvertiserId, advData,
      scanResponse,  // scanResponse is empty
      0,             // timeout
      aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Clear the attribute cache for a given device*/
void BluetoothDaemonGattInterface::Refresh(int aClientIf,
                                           const BluetoothAddress& aBdAddr,
                                           BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientRefreshCmd(aClientIf, aBdAddr, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Enumerate Attributes */
void BluetoothDaemonGattInterface::SearchService(
    int aConnId, bool aSearchAll, const BluetoothUuid& aUuid,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientSearchServiceCmd(
      aConnId, !aSearchAll /* Filtered */, aUuid, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Read / Write An Attribute */
void BluetoothDaemonGattInterface::ReadCharacteristic(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattAuthReq aAuthReq, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv =
      mModule->ClientReadCharacteristicCmd(aConnId, aHandle, aAuthReq, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::WriteCharacteristic(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattWriteType aWriteType, BluetoothGattAuthReq aAuthReq,
    const nsTArray<uint8_t>& aValue, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientWriteCharacteristicCmd(
      aConnId, aHandle, aWriteType, aValue.Length() * sizeof(uint8_t), aAuthReq,
      reinterpret_cast<char*>(const_cast<uint8_t*>(aValue.Elements())), aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::ReadDescriptor(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattAuthReq aAuthReq, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv =
      mModule->ClientReadDescriptorCmd(aConnId, aHandle, aAuthReq, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::WriteDescriptor(
    int aConnId, const BluetoothAttributeHandle& aHandle,
    BluetoothGattWriteType aWriteType, BluetoothGattAuthReq aAuthReq,
    const nsTArray<uint8_t>& aValue, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientWriteDescriptorCmd(
      aConnId, aHandle, aWriteType, aValue.Length() * sizeof(uint8_t), aAuthReq,
      reinterpret_cast<char*>(const_cast<uint8_t*>(aValue.Elements())), aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Execute / Abort Prepared Write*/
void BluetoothDaemonGattInterface::ExecuteWrite(
    int aConnId, int aIsExecute, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientExecuteWriteCmd(aConnId, aIsExecute, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Register / Deregister Characteristic Notifications or Indications */
void BluetoothDaemonGattInterface::RegisterNotification(
    int aClientIf, const BluetoothAddress& aBdAddr,
    const BluetoothAttributeHandle& aHandle, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientRegisterForNotificationCmd(aClientIf, aBdAddr,
                                                          aHandle, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::DeregisterNotification(
    int aClientIf, const BluetoothAddress& aBdAddr,
    const BluetoothAttributeHandle& aHandle, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientDeregisterForNotificationCmd(aClientIf, aBdAddr,
                                                            aHandle, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::ReadRemoteRssi(
    int aClientIf, const BluetoothAddress& aBdAddr,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientReadRemoteRssiCmd(aClientIf, aBdAddr, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::GetDeviceType(
    const BluetoothAddress& aBdAddr, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientGetDeviceTypeCmd(aBdAddr, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::TestCommand(
    int aCommand, const BluetoothGattTestParam& aTestParam,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientTestCommandCmd(aCommand, aTestParam, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Get GATT DB elements */
void BluetoothDaemonGattInterface::GetGattDb(int aConnId,
                                             BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ClientGetGattDbCmd(aConnId, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Register / Unregister */
void BluetoothDaemonGattInterface::RegisterServer(
    const BluetoothUuid& aUuid, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerRegisterServerCmd(aUuid, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::UnregisterServer(
    int aServerIf, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerUnregisterServerCmd(aServerIf, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Connect / Disconnect */
void BluetoothDaemonGattInterface::ConnectPeripheral(
    int aServerIf, const BluetoothAddress& aBdAddr,
    bool aIsDirect, /* auto connect */
    BluetoothTransport aTransport, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerConnectCmd(aServerIf, aBdAddr, aIsDirect,
                                          aTransport, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::DisconnectPeripheral(
    int aServerIf, const BluetoothAddress& aBdAddr, int aConnId,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerDisconnectCmd(aServerIf, aBdAddr, aConnId, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Add a service */
void BluetoothDaemonGattInterface::AddService(
    int aServerIf, const nsTArray<BluetoothGattDbElement>& aDb,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerAddServiceCmd(aServerIf, aDb, aRes);
  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

/* Stop / Delete a service */
void BluetoothDaemonGattInterface::StopService(
    int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerStopServiceCmd(aServerIf, aServiceHandle, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::DeleteService(
    int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv =
      mModule->ServerDeleteServiceCmd(aServerIf, aServiceHandle, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::SendIndication(
    int aServerIf, const BluetoothAttributeHandle& aCharacteristicHandle,
    int aConnId, const nsTArray<uint8_t>& aValue, bool aConfirm,
    BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerSendIndicationCmd(
      aServerIf, aCharacteristicHandle, aConnId,
      aValue.Length() * sizeof(uint8_t), aConfirm,
      const_cast<uint8_t*>(aValue.Elements()), aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::SendResponse(
    int aConnId, int aTransId, uint16_t aStatus,
    const BluetoothGattResponse& aResponse, BluetoothGattResultHandler* aRes) {
  MOZ_ASSERT(mModule);

  nsresult rv = mModule->ServerSendResponseCmd(aConnId, aTransId, aStatus,
                                               aResponse, aRes);

  if (NS_FAILED(rv)) {
    DispatchError(aRes, rv);
  }
}

void BluetoothDaemonGattInterface::DispatchError(
    BluetoothGattResultHandler* aRes, BluetoothStatus aStatus) {
  DaemonResultRunnable1<
      BluetoothGattResultHandler, void, BluetoothStatus,
      BluetoothStatus>::Dispatch(aRes, &BluetoothGattResultHandler::OnError,
                                 ConstantInitOp1<BluetoothStatus>(aStatus));
}

void BluetoothDaemonGattInterface::DispatchError(
    BluetoothGattResultHandler* aRes, nsresult aRv) {
  BluetoothStatus status;

  if (NS_WARN_IF(NS_FAILED(Convert(aRv, status)))) {
    status = STATUS_FAIL;
  }
  DispatchError(aRes, status);
}

END_BLUETOOTH_NAMESPACE
