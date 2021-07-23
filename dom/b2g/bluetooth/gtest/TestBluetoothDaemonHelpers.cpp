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
    mAttributeHandle.mHandle = mArbitraryUint16;
  }

  void TearDown() override { mPDU = nullptr; }

  UniquePtr<DaemonSocketPDU> mPDU;
  BluetoothAddress mAddr;
  BluetoothAttributeHandle mAttributeHandle;
  const uint16_t mBatteryService = 0x180F;
  const uint8_t mArbitraryUint8 = 12;
  const uint16_t mArbitraryUint16 = 1234;
  const uint32_t mArbitraryUint32 = 1234567;
};

//
// Bluetooth GAP and GATT
//
TEST_F(BluetoothDaemonHelpers, BluetoothAddress) {
  EXPECT_EQ(PackPDU(mAddr, *mPDU), NS_OK);

  BluetoothAddress addr;
  EXPECT_EQ(UnpackPDU(*mPDU, addr), NS_OK);

  EXPECT_EQ(mAddr, addr);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAttributeHandle) {
  EXPECT_EQ(PackPDU(mAttributeHandle, *mPDU), NS_OK);

  BluetoothAttributeHandle handle;
  EXPECT_EQ(UnpackPDU(*mPDU, handle), NS_OK);

  EXPECT_EQ(mAttributeHandle, handle);
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
  element.mId = 7;
  element.mUuid = BluetoothUuid(mBatteryService);
  element.mType = GATT_DB_TYPE_SECONDARY_SERVICE;
  element.mHandle = mAttributeHandle;
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

TEST_F(BluetoothDaemonHelpers, BluetoothAclState) {
  const uint8_t val = static_cast<uint8_t>(ACL_STATE_DISCONNECTED);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothAclState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothAclState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothBondState) {
  const uint8_t val = static_cast<uint8_t>(BOND_STATE_BONDING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothBondState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothBondState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothTypeOfDevice) {
  const int32_t val = static_cast<int32_t>(TYPE_OF_DEVICE_BLE);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothTypeOfDevice type1;
  EXPECT_EQ(UnpackPDU(*mPDU, type1), NS_OK);

  BluetoothTypeOfDevice type2;
  EXPECT_EQ(Convert(val, type2), NS_OK);
  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothRemoteInfo) {
  const uint32_t verMajor = 3;
  const uint32_t verMinor = 7;
  const uint32_t manufacturer = 0x000000b9;
  EXPECT_EQ(PackPDU(verMajor, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(verMinor, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(manufacturer, *mPDU), NS_OK);

  BluetoothRemoteInfo info;
  EXPECT_EQ(UnpackPDU(*mPDU, info), NS_OK);

  EXPECT_EQ(static_cast<int>(verMajor), info.mVerMajor);
  EXPECT_EQ(static_cast<int>(verMinor), info.mVerMinor);
  EXPECT_EQ(static_cast<int>(manufacturer), info.mManufacturer);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothRemoteName) {
  BluetoothRemoteName bdName;
  const int bufferSize = sizeof(bdName.mName) + 1;
  const uint8_t name[bufferSize] = {'b', '2', 'g'};
  const uint8_t length = 3;
  EXPECT_EQ(PackPDU(PackArray<uint8_t>(name, sizeof(name)), *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, bdName), NS_OK);

  EXPECT_EQ(length, bdName.mLength);
  for (uint8_t i = 0; i < bdName.mLength; ++i) {
    EXPECT_EQ(name[i], bdName.mName[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

// TODO: Revise UnpackPDU(DaemonSocketPDU& aPDU, BluetoothServiceRecord& aOut)
// by deducing buffer size of aPDU.Read(aOut.mName, sizeof(aOut.mName));
TEST_F(BluetoothDaemonHelpers, BluetoothServiceRecord) {
  const BluetoothUuid uuid(AVRCP_TARGET);
  const uint16_t channel = DEFAULT_RFCOMM_CHANNEL_MAS;
  const char name[256] = {"SMS Message Access"};
  EXPECT_EQ(PackPDU(uuid, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(channel, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(PackArray<char>(name, sizeof(name)), *mPDU), NS_OK);

  BluetoothServiceRecord record;
  EXPECT_EQ(UnpackPDU(*mPDU, record), NS_OK);

  EXPECT_EQ(uuid, record.mUuid);
  EXPECT_EQ(channel, record.mChannel);
  for (unsigned int i = 0; i < sizeof(record.mName); ++i) {
    EXPECT_EQ(name[i], record.mName[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothStatus) {
  const uint8_t val = static_cast<uint8_t>(STATUS_FAIL);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothStatus status1;
  EXPECT_EQ(UnpackPDU(*mPDU, status1), NS_OK);

  BluetoothStatus status2;
  EXPECT_EQ(Convert(val, status2), NS_OK);
  EXPECT_EQ(status1, status2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattStatus) {
  const int32_t val = static_cast<int32_t>(GATT_STATUS_INVALID_HANDLE);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothGattStatus status1;
  EXPECT_EQ(UnpackPDU(*mPDU, status1), NS_OK);

  BluetoothGattStatus status2;
  EXPECT_EQ(Convert(val, status2), NS_OK);
  EXPECT_EQ(status1, status2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattDbType) {
  const int32_t val = static_cast<int32_t>(GATT_STATUS_INVALID_HANDLE);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothGattDbType type1;
  EXPECT_EQ(UnpackPDU(*mPDU, type1), NS_OK);

  BluetoothGattDbType type2;
  EXPECT_EQ(Convert(val, type2), NS_OK);
  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattReadParam) {
  const uint8_t status = mArbitraryUint8;
  const uint16_t valueType = mArbitraryUint16;
  const uint8_t value[] = {1, 2, 3};
  const uint16_t valueLength = sizeof(value);
  EXPECT_EQ(PackPDU(mAttributeHandle, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(status, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(valueType, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(valueLength, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(PackArray<uint8_t>(value, valueLength), *mPDU), NS_OK);

  BluetoothGattReadParam param;
  EXPECT_EQ(UnpackPDU(*mPDU, param), NS_OK);

  EXPECT_EQ(mAttributeHandle, param.mHandle);
  EXPECT_EQ(status, param.mStatus);
  EXPECT_EQ(valueType, param.mValueType);
  EXPECT_EQ(valueLength, param.mValueLength);
  for (uint16_t i = 0; i < param.mValueLength; ++i) {
    EXPECT_EQ(value[i], param.mValue[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothGattNotifyParam) {
  const bool isNotify = true;
  const uint8_t value[] = {1, 2, 3};
  const uint16_t length = sizeof(value);
  EXPECT_EQ(PackPDU(mAddr, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(mAttributeHandle, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(isNotify, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(length, *mPDU), NS_OK);
  EXPECT_EQ(PackPDU(PackArray<uint8_t>(value, length), *mPDU), NS_OK);

  BluetoothGattNotifyParam param;
  EXPECT_EQ(UnpackPDU(*mPDU, param), NS_OK);

  EXPECT_EQ(mAddr, param.mBdAddr);
  EXPECT_EQ(mAttributeHandle, param.mHandle);
  EXPECT_EQ(isNotify, param.mIsNotify);
  EXPECT_EQ(length, param.mLength);
  for (uint16_t i = 0; i < param.mLength; ++i) {
    EXPECT_EQ(value[i], param.mValue[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

//
// Bluetooth profiles
//
TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpAttributeTextPairs) {
  const uint8_t length1 = 4;
  const uint8_t attrs[length1] = {1, 2, 3, 4};
  const char* texts[length1] = {"text1", "text2", "text3", "text4"};
  BluetoothAvrcpAttributeTextPairs attrTexts(attrs, texts, length1);
  EXPECT_EQ(PackPDU(length1, attrTexts, *mPDU), NS_OK);

  uint8_t length2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, length2), NS_OK);

  EXPECT_EQ(length1, length2);
  for (uint8_t i = 0; i < length2; ++i) {
    uint8_t att = 0, clen = 0;
    EXPECT_EQ(UnpackPDU(*mPDU, att), NS_OK);
    EXPECT_EQ(UnpackPDU(*mPDU, clen), NS_OK);

    nsDependentCString cstring;
    EXPECT_EQ(UnpackPDU(*mPDU, UnpackCString0(cstring)), NS_OK);

    EXPECT_EQ(attrs[i], att);
    EXPECT_EQ(strlen(texts[i]) + 1, clen);
    EXPECT_EQ(nsCString(texts[i], clen - 1), cstring);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpAttributeValuePairs) {
  const uint8_t length1 = 4;
  const uint8_t attr[length1] = {1, 2, 3, 4};
  const uint8_t values[length1] = {1, 2, 3, 4};
  BluetoothAvrcpAttributeValuePairs attrVal(attr, values, length1);
  EXPECT_EQ(PackPDU(length1, attrVal, *mPDU), NS_OK);

  uint8_t length2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, length2), NS_OK);
  EXPECT_EQ(length1, length2);
  for (uint8_t i = 0; i < length2; ++i) {
    uint8_t att = 0, val = 0;
    EXPECT_EQ(UnpackPDU(*mPDU, att), NS_OK);
    EXPECT_EQ(UnpackPDU(*mPDU, val), NS_OK);

    EXPECT_EQ(att, attr[i]);
    EXPECT_EQ(val, values[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpElementAttribute) {
  // The maximum value of mId can only be 255 in Gecko, but the HAL could accept
  // a wider range up to uint32
  const BluetoothAvrcpElementAttribute att1 = {
      .mId = static_cast<uint32_t>(mArbitraryUint8),
      .mValue = u"element value"_ns};
  EXPECT_EQ(PackPDU(att1, *mPDU), NS_OK);

  uint8_t id = 0, clen = 0;
  nsDependentCString cstring;
  EXPECT_EQ(UnpackPDU(*mPDU, id), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, clen), NS_OK);
  EXPECT_EQ(UnpackPDU(*mPDU, cstring), NS_OK);

  EXPECT_EQ(clen, static_cast<uint8_t>(att1.mValue.Length()) + 1);
  EXPECT_EQ(att1.mId, id);
  EXPECT_EQ(att1.mValue, NS_ConvertUTF8toUTF16(cstring));
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpEvent) {
  const BluetoothAvrcpEvent evt1 = AVRCP_EVENT_PLAY_POS_CHANGED;
  EXPECT_EQ(PackPDU(evt1, *mPDU), NS_OK);

  BluetoothAvrcpEvent evt2 = AVRCP_EVENT_APP_SETTINGS_CHANGED;
  EXPECT_EQ(UnpackPDU(*mPDU, evt2), NS_OK);

  EXPECT_EQ(evt1, evt2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpEventParamPair) {
  const BluetoothAvrcpNotificationParam param = {
      .mSongPos = 1234,
  };
  const BluetoothAvrcpEventParamPair evtParam(AVRCP_EVENT_PLAY_POS_CHANGED,
                                              param);
  EXPECT_EQ(PackPDU(evtParam, *mPDU), NS_OK);

  uint32_t songPos = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, songPos), NS_OK);

  EXPECT_EQ(evtParam.mParam.mSongPos, songPos);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpNotification) {
  const BluetoothAvrcpNotification ntf1 = AVRCP_NTF_CHANGED;
  EXPECT_EQ(PackPDU(ntf1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(ntf1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpPlayerAttribute) {
  const BluetoothAvrcpPlayerAttribute att1 = AVRCP_PLAYER_ATTRIBUTE_SHUFFLE;
  EXPECT_EQ(PackPDU(att1, *mPDU), NS_OK);

  BluetoothAvrcpPlayerAttribute att2 = AVRCP_PLAYER_ATTRIBUTE_REPEAT;
  EXPECT_EQ(UnpackPDU(*mPDU, att2), NS_OK);

  EXPECT_EQ(att2, att2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpStatus) {
  const BluetoothAvrcpStatus status1 = AVRCP_STATUS_NOT_FOUND;
  EXPECT_EQ(PackPDU(status1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(status1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeAtResponse) {
  const BluetoothHandsfreeAtResponse resp1 = HFP_AT_RESPONSE_OK;
  EXPECT_EQ(PackPDU(resp1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(resp1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallAddressType) {
  const BluetoothHandsfreeCallAddressType type1 =
      HFP_CALL_ADDRESS_TYPE_INTERNATIONAL;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(type1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallDirection) {
  const BluetoothHandsfreeCallDirection direction1 =
      HFP_CALL_DIRECTION_INCOMING;
  EXPECT_EQ(PackPDU(direction1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(direction1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallMode) {
  const BluetoothHandsfreeCallMode mode1 = HFP_CALL_MODE_DATA;
  EXPECT_EQ(PackPDU(mode1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(mode1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallMptyType) {
  const BluetoothHandsfreeCallMptyType type1 = HFP_CALL_MPTY_TYPE_MULTI;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(type1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallState) {
  const BluetoothHandsfreeCallState state1 = HFP_CALL_STATE_HELD;
  EXPECT_EQ(PackPDU(state1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(state1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeNetworkState) {
  const BluetoothHandsfreeNetworkState state1 = HFP_NETWORK_STATE_AVAILABLE;
  EXPECT_EQ(PackPDU(state1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(state1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeServiceType) {
  const BluetoothHandsfreeServiceType type1 = HFP_SERVICE_TYPE_ROAMING;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(type1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeVolumeType) {
  const BluetoothHandsfreeVolumeType type1 = HFP_VOLUME_TYPE_MICROPHONE;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);

  BluetoothHandsfreeVolumeType type2 = HFP_VOLUME_TYPE_SPEAKER;
  EXPECT_EQ(UnpackPDU(*mPDU, type2), NS_OK);

  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeWbsConfig) {
  const BluetoothHandsfreeWbsConfig conf1 = HFP_WBS_YES;
  EXPECT_EQ(PackPDU(conf1, *mPDU), NS_OK);

  BluetoothHandsfreeWbsConfig conf2 = HFP_WBS_NONE;
  EXPECT_EQ(UnpackPDU(*mPDU, conf2), NS_OK);

  EXPECT_EQ(conf1, conf2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, ControlPlayStatus) {
  const ControlPlayStatus status1 = PLAYSTATUS_FWD_SEEK;
  EXPECT_EQ(PackPDU(status1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(status1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidInfoParam) {
  const BluetoothHidInfoParam param1 = {
      .mAttributeMask = mArbitraryUint16,
      .mSubclass = mArbitraryUint8,
      .mApplicationId = mArbitraryUint8,
      .mVendorId = mArbitraryUint16,
      .mProductId = mArbitraryUint16,
      .mVersion = mArbitraryUint16,
      .mCountryCode = mArbitraryUint8,
      .mDescriptorLength = 6,
      .mDescriptorValue = {1, 7, 11, 8, 9, 13}  // arbitrary uint8_t array
  };
  EXPECT_EQ(PackPDU(param1, *mPDU), NS_OK);

  BluetoothHidInfoParam param2;
  EXPECT_EQ(UnpackPDU(*mPDU, param2), NS_OK);

  EXPECT_EQ(param1.mAttributeMask, param2.mAttributeMask);
  EXPECT_EQ(param1.mSubclass, param2.mSubclass);
  EXPECT_EQ(param1.mApplicationId, param2.mApplicationId);
  EXPECT_EQ(param1.mVendorId, param2.mVendorId);
  EXPECT_EQ(param1.mProductId, param2.mProductId);
  EXPECT_EQ(param1.mVersion, param2.mVersion);
  EXPECT_EQ(param1.mCountryCode, param2.mCountryCode);
  EXPECT_EQ(param1.mDescriptorLength, param2.mDescriptorLength);
  for (uint16_t i = 0; i < param1.mDescriptorLength; ++i) {
    EXPECT_EQ(param1.mDescriptorValue[i], param2.mDescriptorValue[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidReport) {
  BluetoothHidReport report1, report2;
  // fill a nsTArray<uint8_t> with arbitrary length and values
  for (int i = 0; i < 13; ++i) {
    report1.mReportData.AppendElement(i);
  }
  EXPECT_EQ(PackPDU(report1, *mPDU), NS_OK);

  EXPECT_EQ(UnpackPDU(*mPDU, report2), NS_OK);

  EXPECT_EQ(report1.mReportData, report2.mReportData);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidProtocolMode) {
  const BluetoothHidProtocolMode mode1 = HID_PROTOCOL_MODE_BOOT;
  EXPECT_EQ(PackPDU(mode1, *mPDU), NS_OK);

  BluetoothHidProtocolMode mode2 = HID_PROTOCOL_MODE_REPORT;
  EXPECT_EQ(UnpackPDU(*mPDU, mode2), NS_OK);

  EXPECT_EQ(mode1, mode2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidReportType) {
  const BluetoothHidReportType type1 = HID_REPORT_TYPE_OUTPUT;
  EXPECT_EQ(PackPDU(type1, *mPDU), NS_OK);
  uint8_t val1 = 0;
  EXPECT_EQ(Convert(type1, val1), NS_OK);

  uint8_t val2 = 0;
  EXPECT_EQ(UnpackPDU(*mPDU, val2), NS_OK);

  EXPECT_EQ(val1, val2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothA2dpAudioState) {
  const uint8_t val = static_cast<uint8_t>(A2DP_AUDIO_STATE_STOPPED);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothA2dpAudioState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothA2dpAudioState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothA2dpConnectionState) {
  const uint8_t val = static_cast<uint8_t>(A2DP_CONNECTION_STATE_CONNECTING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothA2dpConnectionState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothA2dpConnectionState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpMediaAttribute) {
  const uint8_t val = static_cast<uint8_t>(AVRCP_MEDIA_ATTRIBUTE_ARTIST);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothAvrcpMediaAttribute att1;
  EXPECT_EQ(UnpackPDU(*mPDU, att1), NS_OK);

  BluetoothAvrcpMediaAttribute att2;
  EXPECT_EQ(Convert(val, att2), NS_OK);
  EXPECT_EQ(att1, att2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpPlayerSettings) {
  const uint8_t numAttr = 4;
  const uint8_t ids[256] = {1, 2, 3, 4};
  const uint8_t values[256] = {1, 2, 3, 4};
  EXPECT_EQ(PackPDU(numAttr, *mPDU), NS_OK);
  for (uint8_t i = 0; i < numAttr; ++i) {
    EXPECT_EQ(PackPDU(ids[i], *mPDU), NS_OK);
    EXPECT_EQ(PackPDU(values[i], *mPDU), NS_OK);
  }

  BluetoothAvrcpPlayerSettings setting;
  EXPECT_EQ(UnpackPDU(*mPDU, setting), NS_OK);

  EXPECT_EQ(numAttr, setting.mNumAttr);
  for (uint8_t i = 0; i < setting.mNumAttr; ++i) {
    EXPECT_EQ(ids[i], setting.mIds[i]);
    EXPECT_EQ(values[i], setting.mValues[i]);
  }
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothAvrcpRemoteFeatureBits) {
  const uint8_t val = static_cast<uint8_t>(AVRCP_REMOTE_FEATURE_METADATA);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothAvrcpRemoteFeatureBits bits1;
  EXPECT_EQ(UnpackPDU(*mPDU, bits1), NS_OK);

  BluetoothAvrcpRemoteFeatureBits bits2;
  EXPECT_EQ(Convert(val, bits2), NS_OK);
  EXPECT_EQ(bits1, bits2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeAudioState) {
  const uint8_t val = static_cast<uint8_t>(HFP_AUDIO_STATE_CONNECTING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeAudioState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothHandsfreeAudioState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeCallHoldType) {
  const uint8_t val = static_cast<uint8_t>(HFP_AUDIO_STATE_CONNECTING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeCallHoldType type1;
  EXPECT_EQ(UnpackPDU(*mPDU, type1), NS_OK);

  BluetoothHandsfreeCallHoldType type2;
  EXPECT_EQ(Convert(val, type2), NS_OK);
  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeConnectionState) {
  const uint8_t val = static_cast<uint8_t>(HFP_AUDIO_STATE_CONNECTING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeConnectionState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothHandsfreeConnectionState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeNRECState) {
  const uint8_t val = static_cast<uint8_t>(HFP_NREC_STARTED);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeNRECState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothHandsfreeNRECState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeHfIndType) {
  const uint8_t val = static_cast<uint8_t>(HFP_HF_IND_ENHANCED_DRIVER_SAFETY);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeHfIndType type1;
  EXPECT_EQ(UnpackPDU(*mPDU, type1), NS_OK);

  BluetoothHandsfreeHfIndType type2;
  EXPECT_EQ(Convert(val, type2), NS_OK);
  EXPECT_EQ(type1, type2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHandsfreeVoiceRecognitionState) {
  const uint8_t val = static_cast<uint8_t>(HFP_VOICE_RECOGNITION_STARTED);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHandsfreeVoiceRecognitionState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothHandsfreeVoiceRecognitionState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidConnectionState) {
  const uint8_t val = static_cast<uint8_t>(HID_CONNECTION_STATE_CONNECTING);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHidConnectionState state1;
  EXPECT_EQ(UnpackPDU(*mPDU, state1), NS_OK);

  BluetoothHidConnectionState state2;
  EXPECT_EQ(Convert(val, state2), NS_OK);
  EXPECT_EQ(state1, state2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

TEST_F(BluetoothDaemonHelpers, BluetoothHidStatus) {
  const uint8_t val =
      static_cast<uint8_t>(HID_STATUS_HANDSHAKE_DEVICE_NOT_READY);
  EXPECT_EQ(PackPDU(val, *mPDU), NS_OK);

  BluetoothHidStatus status1;
  EXPECT_EQ(UnpackPDU(*mPDU, status1), NS_OK);

  BluetoothHidStatus status2;
  EXPECT_EQ(Convert(val, status2), NS_OK);
  EXPECT_EQ(status1, status2);
  EXPECT_EQ(mPDU->GetSize(), 0u);
}

END_BLUETOOTH_NAMESPACE
