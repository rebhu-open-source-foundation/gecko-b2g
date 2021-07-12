/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bluedroid/BluetoothDaemonHelpers.h"
#include "gtest/gtest.h"

BEGIN_BLUETOOTH_NAMESPACE

//
// Test fixture
//
class BluetoothDaemonHelpers : public testing::Test {
 protected:
  void SetUp() override {
    mPDU = MakeUnique<DaemonSocketPDU>(DaemonSocketPDU::PDU_MAX_PAYLOAD_LENGTH);
    mAddr = BluetoothAddress(11, 22, 33, 44, 55, 66);
  }

  void TearDown() override { mPDU = nullptr; }

  UniquePtr<DaemonSocketPDU> mPDU;
  BluetoothAddress mAddr;
  const uint16_t mBatteryService = 0x180F;
};

//
// Bluetooth GAP and GATT
//
TEST_F(BluetoothDaemonHelpers, TestBluetoothAddress) {
  EXPECT_EQ(PackPDU(mAddr, *mPDU), NS_OK);

  BluetoothAddress addr;
  EXPECT_EQ(UnpackPDU(*mPDU, addr), NS_OK);

  EXPECT_EQ(mAddr, addr);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, TestBluetoothAttributeHandle) {
  BluetoothAttributeHandle handle1, handle2;
  handle1.mHandle = 0x0123;
  EXPECT_EQ(PackPDU(handle1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, handle2), NS_OK);

  EXPECT_EQ(handle1, handle2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothConfigurationParameter) {
  BluetoothConfigurationParameter parameter;
  parameter.mType = 0x03;  // an arbitrary uint8_t value
  parameter.mLength = 2;
  parameter.mValue = MakeUnique<uint8_t[]>(parameter.mLength);
  parameter.mValue[0] = 17;
  parameter.mValue[1] = 29;
  EXPECT_EQ(PackPDU(parameter, *mPDU), NS_OK);

  uint8_t type;
  uint16_t length;
  EXPECT_EQ(UnpackPDU(*mPDU, type), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, length), NS_OK);

  uint8_t value[length];
  EXPECT_EQ(UnpackPDU(*mPDU, UnpackArray<uint8_t>(value, length)), NS_OK);

  EXPECT_EQ(parameter.mType, type);
  EXPECT_EQ(parameter.mLength, length);
  EXPECT_EQ(parameter.mValue[0], value[0]);
  EXPECT_EQ(parameter.mValue[1], value[1]);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothProperty) {
  BluetoothProperty p1, p2;
  p1.mType = PROPERTY_CLASS_OF_DEVICE;
  p1.mUint32 = 0x040400;  // Rendering service, Audio/Video major device class
  EXPECT_EQ(PackPDU(p1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, p2), NS_OK);

  EXPECT_EQ(p1.mType, p2.mType);
  EXPECT_EQ(p1.mUint32, p2.mUint32);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothPinCode) {
  BluetoothPinCode pin1, pin2;
  pin1.mLength = 4;
  pin1.mPinCode[0] = 'p';
  pin1.mPinCode[1] = 'a';
  pin1.mPinCode[2] = 's';
  pin1.mPinCode[3] = 's';
  EXPECT_EQ(PackPDU(pin1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, pin2.mLength), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, UnpackArray<uint8_t>(pin2.mPinCode, pin2.mLength)),
            NS_OK);

  EXPECT_EQ(pin1, pin2);
  EXPECT_EQ(mPDU->GetSize(),
            sizeof(pin1.mPinCode) - pin1.mLength * sizeof(pin1.mPinCode[0]));
}

TEST_F(BluetoothDaemonHelpers, BluetoothPropertyType) {
  BluetoothPropertyType type1 = PROPERTY_TYPE_OF_DEVICE,
                        type2 = PROPERTY_UNKNOWN;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, type2), NS_OK);

  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothServiceName) {
  BluetoothServiceName name1, name2;
  name1.mName[0] = 'h';
  name1.mName[1] = 'f';
  name1.mName[2] = 'p';
  EXPECT_EQ(PackPDU(name1, *mPDU), NS_OK);

  EXPECT_EQ(
      UnpackPDU(*mPDU, UnpackArray<uint8_t>(name2.mName, sizeof(name2.mName))),
      NS_OK);

  EXPECT_EQ(name1.mName[0], name2.mName[0]);
  EXPECT_EQ(name1.mName[1], name2.mName[1]);
  EXPECT_EQ(name1.mName[2], name2.mName[2]);
  EXPECT_EQ(mPDU->GetSize(), 1u);  // additional '\0' remains
}

TEST_F(BluetoothDaemonHelpers, BluetoothSetupServiceId) {
  BluetoothSetupServiceId serviceId = SETUP_SERVICE_ID_CORE;
  EXPECT_EQ(PackPDU(serviceId, *mPDU), NS_OK);

  uint8_t id1 = 0, id2 = 1;
  Convert(serviceId, id1);
  EXPECT_EQ(UnpackPDU(*mPDU, id2), NS_OK);

  EXPECT_EQ(id1, id2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothSocketType) {
  BluetoothSocketType socketType = RFCOMM;
  EXPECT_EQ(PackPDU(socketType, *mPDU), NS_OK);

  uint8_t type1 = 0, type2 = 1;
  Convert(socketType, type1);
  EXPECT_EQ(UnpackPDU(*mPDU, type2), NS_OK);

  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothSspVariant) {
  BluetoothSspVariant v1 = SSP_VARIANT_CONSENT,
                      v2 = SSP_VARIANT_PASSKEY_NOTIFICATION;
  EXPECT_EQ(PackPDU(v1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, v2), NS_OK);

  EXPECT_EQ(v1, v2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothScanMode) {
  BluetoothScanMode mode1 = SCAN_MODE_CONNECTABLE, mode2 = SCAN_MODE_NONE;
  EXPECT_EQ(PackPDU(mode1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, mode2), NS_OK);

  EXPECT_EQ(mode1, mode2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothUuid) {
  // Test PackPDU / UnpackPDU
  BluetoothUuid uuid1(AVRCP_TARGET), uuid2(UNKNOWN);
  EXPECT_EQ(PackPDU(uuid1, *mPDU), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, uuid2), NS_OK);
  EXPECT_EQ(uuid1, uuid2);
  EXPECT_EQ(mPDU->GetSize(), 0u);

  // Test PackReversed / UnpackReversed
  uuid2 = BluetoothUuid(UNKNOWN);
  EXPECT_EQ(PackPDU(PackReversed<BluetoothUuid>(uuid1), *mPDU), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, UnpackReversed<BluetoothUuid>(uuid2)), NS_OK);
  EXPECT_EQ(uuid1, uuid2);
  EXPECT_EQ(mPDU->GetSize(), 0u);

  // Test the reversing by reversing uuid1 twice
  BluetoothUuid uuidRev;
  EXPECT_EQ(PackPDU(PackReversed<BluetoothUuid>(uuid1), *mPDU), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, uuidRev), NS_OK);
  EXPECT_NE(uuid1, uuidRev);
  EXPECT_EQ(mPDU->GetSize(), 0u);

  BluetoothUuid uuidRevRev;
  EXPECT_EQ(PackPDU(PackReversed<BluetoothUuid>(uuidRev), *mPDU), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, uuidRevRev), NS_OK);
  EXPECT_NE(uuidRev, uuidRevRev);
  EXPECT_EQ(uuid1, uuidRevRev);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattId) {
  BluetoothGattId id1, id2;
  id1.mUuid = BluetoothUuid(mBatteryService);
  id1.mInstanceId = 7;
  EXPECT_EQ(PackPDU(id1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, id2), NS_OK);

  EXPECT_EQ(id1, id2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattDbElement) {
  nsTArray<BluetoothGattDbElement> db1, db2;
  BluetoothGattDbElement element;
  BluetoothAttributeHandle handle;
  handle.mHandle = 11;
  element.mId = 7;
  element.mUuid = BluetoothUuid(mBatteryService);
  element.mType = GATT_DB_TYPE_SECONDARY_SERVICE;
  element.mHandle = handle;
  element.mStartHandle = 5;
  element.mEndHandle = 9;
  element.mProperties = GATT_CHAR_PROP_BIT_READ | GATT_CHAR_PROP_BIT_NOTIFY;
  element.mPermissions =
      GATT_ATTR_PERM_BIT_READ | GATT_ATTR_PERM_BIT_READ_ENCRYPTED;
  db1.AppendElement(element);
  EXPECT_EQ(PackPDU(db1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, db2), NS_OK);

  EXPECT_EQ(db1, db2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattAuthReq) {
  BluetoothGattAuthReq req1 = GATT_AUTH_REQ_MITM;
  int32_t num1 = 0, num2 = 1;
  EXPECT_EQ(PackPDU(req1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, num2), NS_OK);
  Convert(req1, num1);

  EXPECT_EQ(num1, num2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattWriteType) {
  BluetoothGattWriteType type1 = GATT_WRITE_TYPE_NORMAL;
  int32_t num1 = 0, num2 = 1;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, num2), NS_OK);
  Convert(type1, num1);

  EXPECT_EQ(num1, num2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothTransport) {
  BluetoothTransport type1 = TRANSPORT_LE;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);

  uint8_t type2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, type2), NS_OK);

  EXPECT_EQ(static_cast<uint8_t>(type1), type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothSdpType) {
  BluetoothSdpType type1 = SDP_TYPE_PBAP_PSE, type2 = SDP_TYPE_RAW;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, type2), NS_OK);

  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothSdpRecord) {
  BluetoothSdpRecord r1 = BluetoothMasRecord(DEFAULT_RFCOMM_CHANNEL_MAS, 0);
  EXPECT_EQ(PackPDU(r1, *mPDU), NS_OK);

  BluetoothSdpRecord r2;
  EXPECT_EQ(UnpackPDU(*mPDU, r2), NS_OK);

  EXPECT_EQ(r1.mType, r2.mType);
  EXPECT_EQ(r1.mUuid, r2.mUuid);
  EXPECT_EQ(r1.mServiceName, r2.mServiceName);
  EXPECT_EQ(r1.mRfcommChannelNumber, r2.mRfcommChannelNumber);
  EXPECT_EQ(r1.mL2capPsm, r2.mL2capPsm);
  EXPECT_EQ(r1.mProfileVersion, r2.mProfileVersion);
  if (r1.mType == SDP_TYPE_MAP_MAS) {
    EXPECT_EQ(r1.mSupportedFeatures, r2.mSupportedFeatures);
    EXPECT_EQ(r1.mSupportedContentTypes, r2.mSupportedContentTypes);
    EXPECT_EQ(r1.mInstanceId, r2.mInstanceId);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

END_BLUETOOTH_NAMESPACE
