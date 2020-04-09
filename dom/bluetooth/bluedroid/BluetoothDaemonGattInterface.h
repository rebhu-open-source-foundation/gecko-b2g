/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluedroid_BluetoothDaemonGattInterface_h
#define mozilla_dom_bluetooth_bluedroid_BluetoothDaemonGattInterface_h

#include "BluetoothDaemonHelpers.h"
#include "BluetoothInterface.h"
#include "mozilla/ipc/DaemonRunnables.h"

BEGIN_BLUETOOTH_NAMESPACE

using mozilla::ipc::DaemonSocketPDU;
using mozilla::ipc::DaemonSocketPDUHeader;
using mozilla::ipc::DaemonSocketResultHandler;

class BluetoothDaemonGattModule {
 public:
  enum { SERVICE_ID = 0x09 };

  enum {
    OPCODE_ERROR = 0x00,

    // TODO: Relace the following withdrawal opcodes with new opcodes
    // replaced by OPCODE_SCANNER_SCAN
    OPCODE_CLIENT_SCAN = OPCODE_ERROR,

    // replaced by OPCODE_ADVERTISER_ENABLE
    OPCODE_CLIENT_LISTEN = OPCODE_ERROR,

    // replaced by OPCODE_CLIENT_GET_GATT_DB
    // (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_srvc_id_t
    // *start_incl_srvc_id);
    OPCODE_CLIENT_GET_INCLUDED_SERVICE = OPCODE_ERROR,
    // (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t
    // *start_char_id);
    OPCODE_CLIENT_GET_CHARACTERISTIC = OPCODE_ERROR,
    // (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t *char_id,
    // btgatt_gatt_id_t *start_descr_id);
    OPCODE_CLIENT_GET_DESCRIPTOR = OPCODE_ERROR,

    // replaced by OPCODE_ADVERTISER_SET_DATA
    // (int client_if, bool set_scan_rsp, bool include_name,
    //  bool include_txpower, int min_interval, int max_interval,
    //  int appearance, uint16_t manufacturer_len, char *manufacturer_data,
    //  uint16_t service_data_len, char *service_data,
    //  uint16_t service_uuid_len, char *service_uuid);
    OPCODE_CLIENT_SET_ADV_DATA = OPCODE_ERROR,

    // replaced by OPCODE_SERVER_ADD_SERVICE
    // (int server_if, int service_handle, int included_handle);
    OPCODE_SERVER_ADD_INCLUDED_SERVICE = OPCODE_ERROR,
    // (int server_if, int service_handle, bt_uuid_t *uuid, int properties, int
    // permissions);
    OPCODE_SERVER_ADD_CHARACTERISTIC = OPCODE_ERROR,
    // (int server_if, int service_handle, bt_uuid_t *uuid, int permissions);
    OPCODE_SERVER_ADD_DESCRIPTOR = OPCODE_ERROR,
    // (int server_if, int service_handle, int transport);
    OPCODE_SERVER_START_SERVICE = OPCODE_ERROR,

    // ----- GATT client, 0x01 - 0x20 -----
    OPCODE_CLIENT_REGISTER_CLIENT = 0x01,
    OPCODE_CLIENT_UNREGISTER_CLIENT = 0x02,
    // TODO: supports new args (..., bool opportunistic, int initiating_phys)
    OPCODE_CLIENT_CONNECT = 0x03,
    OPCODE_CLIENT_DISCONNECT = 0x04,
    OPCODE_CLIENT_REFRESH = 0x05,
    OPCODE_CLIENT_SEARCH_SERVICE = 0x06,
    // TODO: (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t *char_id,
    // int auth_req)
    //    -> (int conn_id, const bluetooth::Uuid &uuid, uint16_t s_handle,
    //    uint16_t e_handle, int auth_req)
    OPCODE_CLIENT_READ_CHARACTERISTIC = 0x08,
    // TODO: (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t *char_id,
    // int write_type, int len, int auth_req, char *p_value)
    //    -> (int conn_id, uint16_t handle, int write_type, int auth_req,
    //    std::vector<uint8_t> value)
    OPCODE_CLIENT_WRITE_CHARACTERISTIC = 0x0a,
    // TODO: (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t *char_id,
    // btgatt_gatt_id_t *descr_id, int auth_req)
    //    -> (int conn_id, uint16_t handle, int auth_req)
    OPCODE_CLIENT_READ_DESCRIPTOR = 0x0b,
    // TODO: (int conn_id, btgatt_srvc_id_t *srvc_id, btgatt_gatt_id_t *char_id,
    // btgatt_gatt_id_t *descr_id, int write_type, int len, int auth_req, char
    // *p_value)
    //    -> (int conn_id, uint16_t handle, int auth_req, std::vector<uint8_t>
    //    value)
    OPCODE_CLIENT_WRITE_DESCRIPTOR = 0x0c,
    OPCODE_CLIENT_EXECUTE_WRITE = 0x0d,
    // TODO: (int client_if, const bt_bdaddr_t *bd_addr, btgatt_srvc_id_t
    // *srvc_id, btgatt_gatt_id_t *char_id)
    //    -> (int client_if, const RawAddress &bd_addr, uint16_t handle)
    OPCODE_CLIENT_REGISTER_FOR_NOTIFICATION = 0x0e,
    // TODO: (int client_if, const bt_bdaddr_t *bd_addr, btgatt_srvc_id_t
    // *srvc_id, btgatt_gatt_id_t *char_id)
    //    -> (int client_if, const RawAddress &bd_addr, uint16_t handle)
    OPCODE_CLIENT_DEREGISTER_FOR_NOTIFICATION = 0x0f,
    OPCODE_CLIENT_READ_REMOTE_RSSI = 0x10,
    OPCODE_CLIENT_GET_DEVICE_TYPE = 0x11,
    // TODO: (int command, btgatt_test_params_t *params)
    //    -> (int command, const btgatt_test_params_t &params)
    OPCODE_CLIENT_TEST_COMMAND = 0x16,
    // TODO: Support GET_GATT_DB for getting characteristic anddescriptor
    OPCODE_CLIENT_GET_GATT_DB = 0x17,

    // TODO: Support the following commands as new feature
    OPCODE_CLIENT_BTIF_GATTC_DISCOVER_SERVICE_BY_UUID = 0x07,
    OPCODE_CLIENT_READ_USING_CHARACTERISTIC_UUID = 0x09,
    OPCODE_CLIENT_CONFIGURE_MTU = 0x12,
    OPCODE_CLIENT_CONN_PARAMETER_UPDATE = 0x13,
    OPCODE_CLIENT_SET_PREFERRED_PHY = 0x14,
    OPCODE_CLIENT_READ_PHY = 0x15,

    // ----- GATT server, 0x21 - 0x40 -----
    OPCODE_SERVER_REGISTER_SERVER = 0x21,
    OPCODE_SERVER_UNREGISTER_SERVER = 0x22,
    OPCODE_SERVER_CONNECT = 0x23,
    OPCODE_SERVER_DISCONNECT = 0x24,
    // TODO: (int server_if, btgatt_srvc_id_t *srvc_id, int num_handles)
    //    -> (int server_if, std::vector<btgatt_db_element_t> service)
    OPCODE_SERVER_ADD_SERVICE = 0x25,
    OPCODE_SERVER_STOP_SERVICE = 0x26,
    OPCODE_SERVER_DELETE_SERVICE = 0x27,
    // TODO: (int server_if, int attribute_handle, int conn_id, int len, int
    // confirm, char *p_value);
    //    -> (int server_if, int attribute_handle, int conn_id, int confirm,
    //    std::vector<uint8_t> value)
    OPCODE_SERVER_SEND_INDICATION = 0x28,

    // TODO: (int conn_id, int trans_id, int status, btgatt_response_t
    // *response)
    //    -> (int conn_id, int trans_id, int status, const btgatt_response_t
    //    &response)
    OPCODE_SERVER_SEND_RESPONSE = 0x29,

    // TODO: Support the following commands as new feature
    // OPCODE_SERVER_SET_PREFERRED_PHY = 0x2a,
    // OPCODE_SERVER_READ_PHY = 0x2b,

    // ----- LE scanner, 0x41 - 0x60 -----
    // TODO: Support LE scanner
    OPCODE_SCANNER_REGISTER_SCANNER = 0x41,
    OPCODE_SCANNER_UNREGISTER = 0x42,
    OPCODE_SCANNER_SCAN = 0x43,  // was OPCODE_CLIENT_SCAN

    // TODO: Support the following commands as new feature
    // OPCODE_SCANNER_SCAN_FILTER_PARAMS_SETUP = 0x44,
    // OPCODE_SCANNER_SCAN_FILTER_ADD_REMOVE = 0x45,
    // OPCODE_SCANNER_SCAN_FILTER_CLEAR = 0x46,
    // OPCODE_SCANNER_SCAN_FILTER_ENABLE = 0x47,
    // OPCODE_SCANNER_SET_SCAN_PARAMETERS = 0x48,
    // OPCODE_SCANNER_CONFIGURE_BATCHSCAN = 0x49,
    // OPCODE_SCANNER_ENABLE_BATCHSCAN = 0x4a,
    // OPCODE_SCANNER_DISABLE_BATCHSCAN = 0x4b,
    // OPCODE_SCANNER_READ_BATCHSCAN_REPORTS = 0x4c,
    // OPCODE_SCANNER_START_SYNC = 0x4d,
    // OPCODE_SCANNER_STOP_SYNC = 0x4e,

    // ----- LE advertiser, 0x61 - 0x80 -----
    // TODO: Support LE advertiser
    OPCODE_ADVERTISER_REGISTER_ADVERTISER = 0x61,
    OPCODE_ADVERTISER_UNREGISTER = 0x62,
    OPCODE_ADVERTISER_SET_DATA = 0x65,
    OPCODE_ADVERTISER_ENABLE = 0x66,  // similar with OPCODE_CLIENT_LISTEN

    // TODO: Support the following commands as new feature
    // OPCODE_ADVERTISER_GET_OWN_ADDRESS = 0x63,
    // OPCODE_ADVERTISER_SET_PARAMETERS = 0x64,
    // OPCODE_ADVERTISER_START_ADVERTISING = 0x67,
    // OPCODE_ADVERTISER_START_ADVERTISING_SET = 0x68,
    // OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_PARAMETERS = 0x69,
    // OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_DATA = 0x6a,
    // OPCODE_ADVERTISER_SET_PERIODIC_ADVERTISING_ENABLE = 0x6b,

  };

  virtual nsresult Send(DaemonSocketPDU* aPDU,
                        DaemonSocketResultHandler* aRes) = 0;

  void SetNotificationHandler(
      BluetoothGattNotificationHandler* aNotificationHandler);

  //
  // Commands
  //

  /* Register / Unregister */
  nsresult ClientRegisterClientCmd(const BluetoothUuid& aUuid,
                                   BluetoothGattResultHandler* aRes);

  nsresult ClientUnregisterClientCmd(int aClientIf,
                                     BluetoothGattResultHandler* aRes);

  /* Start / Stop LE Scan */
  nsresult ClientScanCmd(int aClientIf, bool aStart,
                         BluetoothGattResultHandler* aRes);

  /* Connect / Disconnect */
  nsresult ClientConnectCmd(int aClientIf, const BluetoothAddress& aBdAddr,
                            bool aIsDirect, /* auto connect */
                            BluetoothTransport aTransport,
                            BluetoothGattResultHandler* aRes);

  nsresult ClientDisconnectCmd(int aClientIf, const BluetoothAddress& aBdAddr,
                               int aConnId, BluetoothGattResultHandler* aRes);

  /* Start / Stop advertisements to listen for incoming connections */
  nsresult ClientListenCmd(int aClientIf, bool aIsStart,
                           BluetoothGattResultHandler* aRes);

  /* Clear the attribute cache for a given device*/
  nsresult ClientRefreshCmd(int aClientIf, const BluetoothAddress& aBdAddr,
                            BluetoothGattResultHandler* aRes);

  /* Enumerate Attributes */
  nsresult ClientSearchServiceCmd(int aConnId, bool aFiltered,
                                  const BluetoothUuid& aUuid,
                                  BluetoothGattResultHandler* aRes);

  nsresult ClientGetIncludedServiceCmd(
      int aConnId, const BluetoothGattServiceId& aServiceId, bool aContinuation,
      const BluetoothGattServiceId& aStartServiceId,
      BluetoothGattResultHandler* aRes);

  nsresult ClientGetCharacteristicCmd(int aConnId,
                                      const BluetoothGattServiceId& aServiceId,
                                      bool aContinuation,
                                      const BluetoothGattId& aStartCharId,
                                      BluetoothGattResultHandler* aRes);

  nsresult ClientGetDescriptorCmd(int aConnId,
                                  const BluetoothGattServiceId& aServiceId,
                                  const BluetoothGattId& aCharId,
                                  bool aContinuation,
                                  const BluetoothGattId& aDescriptorId,
                                  BluetoothGattResultHandler* aRes);

  /* Read / Write An Attribute */
  nsresult ClientReadCharacteristicCmd(int aConnId,
                                       const BluetoothGattServiceId& aServiceId,
                                       const BluetoothGattId& aCharId,
                                       BluetoothGattAuthReq aAuthReq,
                                       BluetoothGattResultHandler* aRes);

  nsresult ClientWriteCharacteristicCmd(
      int aConnId, const BluetoothGattServiceId& aServiceId,
      const BluetoothGattId& aCharId, BluetoothGattWriteType aWriteType,
      int aLength, BluetoothGattAuthReq aAuthReq, char* aValue,
      BluetoothGattResultHandler* aRes);

  nsresult ClientReadDescriptorCmd(int aConnId,
                                   const BluetoothGattServiceId& aServiceId,
                                   const BluetoothGattId& aCharId,
                                   const BluetoothGattId& aDescriptorId,
                                   BluetoothGattAuthReq aAuthReq,
                                   BluetoothGattResultHandler* aRes);

  nsresult ClientWriteDescriptorCmd(int aConnId,
                                    const BluetoothGattServiceId& aServiceId,
                                    const BluetoothGattId& aCharId,
                                    const BluetoothGattId& aDescriptorId,
                                    BluetoothGattWriteType aWriteType,
                                    int aLength, BluetoothGattAuthReq aAuthReq,
                                    char* aValue,
                                    BluetoothGattResultHandler* aRes);

  /* Execute / Abort Prepared Write*/
  nsresult ClientExecuteWriteCmd(int aConnId, int aIsExecute,
                                 BluetoothGattResultHandler* aRes);

  /* Register / Deregister Characteristic Notifications or Indications */
  nsresult ClientRegisterForNotificationCmd(
      int aClientIf, const BluetoothAddress& aBdAddr,
      const BluetoothGattServiceId& aServiceId, const BluetoothGattId& aCharId,
      BluetoothGattResultHandler* aRes);

  nsresult ClientDeregisterForNotificationCmd(
      int aClientIf, const BluetoothAddress& aBdAddr,
      const BluetoothGattServiceId& aServiceId, const BluetoothGattId& aCharId,
      BluetoothGattResultHandler* aRes);

  nsresult ClientReadRemoteRssiCmd(int aClientIf,
                                   const BluetoothAddress& aBdAddr,
                                   BluetoothGattResultHandler* aRes);

  nsresult ClientGetDeviceTypeCmd(const BluetoothAddress& aBdAddr,
                                  BluetoothGattResultHandler* aRes);

  /* Set advertising data or scan response data */
  nsresult ClientSetAdvDataCmd(int aServerIf, bool aIsScanRsp,
                               bool aIsNameIncluded, bool aIsTxPowerIncluded,
                               int aMinInterval, int aMaxInterval,
                               int aApperance,
                               const nsTArray<uint8_t>& aManufacturerData,
                               const nsTArray<uint8_t>& aServiceData,
                               const nsTArray<BluetoothUuid>& aServiceUuids,
                               BluetoothGattResultHandler* aRes);

  nsresult ClientTestCommandCmd(int aCommand,
                                const BluetoothGattTestParam& aTestParam,
                                BluetoothGattResultHandler* aRes);

  /* Register / Unregister */
  nsresult ServerRegisterServerCmd(const BluetoothUuid& aUuid,
                                   BluetoothGattResultHandler* aRes);

  nsresult ServerUnregisterServerCmd(int aServerIf,
                                     BluetoothGattResultHandler* aRes);

  /* Connect / Disconnect */
  nsresult ServerConnectCmd(int aServerIf, const BluetoothAddress& aBdAddr,
                            bool aIsDirect, BluetoothTransport aTransport,
                            BluetoothGattResultHandler* aRes);

  nsresult ServerDisconnectCmd(int aServerIf, const BluetoothAddress& aBdAddr,
                               int aConnId, BluetoothGattResultHandler* aRes);

  /* Add a services / a characteristic / a descriptor */
  nsresult ServerAddServiceCmd(int aServerIf,
                               const BluetoothGattServiceId& aServiceId,
                               uint16_t aNumHandles,
                               BluetoothGattResultHandler* aRes);

  nsresult ServerAddIncludedServiceCmd(
      int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
      const BluetoothAttributeHandle& aIncludedServiceHandle,
      BluetoothGattResultHandler* aRes);

  nsresult ServerAddCharacteristicCmd(
      int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
      const BluetoothUuid& aUuid, BluetoothGattCharProp aProperties,
      BluetoothGattAttrPerm aPermissions, BluetoothGattResultHandler* aRes);

  nsresult ServerAddDescriptorCmd(
      int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
      const BluetoothUuid& aUuid, BluetoothGattAttrPerm aPermissions,
      BluetoothGattResultHandler* aRes);

  /* Start / Stop / Delete a service */
  nsresult ServerStartServiceCmd(int aServerIf,
                                 const BluetoothAttributeHandle& aServiceHandle,
                                 BluetoothTransport aTransport,
                                 BluetoothGattResultHandler* aRes);

  nsresult ServerStopServiceCmd(int aServerIf,
                                const BluetoothAttributeHandle& aServiceHandle,
                                BluetoothGattResultHandler* aRes);

  nsresult ServerDeleteServiceCmd(
      int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
      BluetoothGattResultHandler* aRes);

  /* Send an indication or a notification */
  nsresult ServerSendIndicationCmd(
      int aServerIf, const BluetoothAttributeHandle& aCharacteristicHandle,
      int aConnId, int aLength, bool aConfirm, uint8_t* aValue,
      BluetoothGattResultHandler* aRes);

  /* Send a response for an incoming indication */
  nsresult ServerSendResponseCmd(int aConnId, int aTransId, uint16_t aStatus,
                                 const BluetoothGattResponse& aResponse,
                                 BluetoothGattResultHandler* aRes);
  // TODO: Add L support

 protected:
  void HandleSvc(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Responses
  //

  typedef mozilla::ipc::DaemonResultRunnable0<BluetoothGattResultHandler, void>
      ResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<BluetoothGattResultHandler, void,
                                              BluetoothTypeOfDevice,
                                              BluetoothTypeOfDevice>
      ClientGetDeviceTypeResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<BluetoothGattResultHandler, void,
                                              BluetoothStatus, BluetoothStatus>
      ErrorRunnable;

  void ErrorRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                BluetoothGattResultHandler* aRes);

  void ClientRegisterClientRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothGattResultHandler* aRes);

  void ClientUnregisterClientRsp(const DaemonSocketPDUHeader& aHeader,
                                 DaemonSocketPDU& aPDU,
                                 BluetoothGattResultHandler* aRes);

  void ClientScanRsp(const DaemonSocketPDUHeader& aHeader,
                     DaemonSocketPDU& aPDU, BluetoothGattResultHandler* aRes);

  void ClientConnectRsp(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU,
                        BluetoothGattResultHandler* aRes);

  void ClientDisconnectRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           BluetoothGattResultHandler* aRes);

  void ClientListenRsp(const DaemonSocketPDUHeader& aHeader,
                       DaemonSocketPDU& aPDU, BluetoothGattResultHandler* aRes);

  void ClientRefreshRsp(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU,
                        BluetoothGattResultHandler* aRes);

  void ClientSearchServiceRsp(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              BluetoothGattResultHandler* aRes);

  void ClientGetIncludedServiceRsp(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU,
                                   BluetoothGattResultHandler* aRes);

  void ClientGetCharacteristicRsp(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU,
                                  BluetoothGattResultHandler* aRes);

  void ClientGetDescriptorRsp(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              BluetoothGattResultHandler* aRes);

  void ClientReadCharacteristicRsp(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU,
                                   BluetoothGattResultHandler* aRes);

  void ClientWriteCharacteristicRsp(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU,
                                    BluetoothGattResultHandler* aRes);

  void ClientReadDescriptorRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothGattResultHandler* aRes);

  void ClientWriteDescriptorRsp(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU,
                                BluetoothGattResultHandler* aRes);

  void ClientExecuteWriteRsp(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU,
                             BluetoothGattResultHandler* aRes);

  void ClientRegisterForNotificationRsp(const DaemonSocketPDUHeader& aHeader,
                                        DaemonSocketPDU& aPDU,
                                        BluetoothGattResultHandler* aRes);

  void ClientDeregisterForNotificationRsp(const DaemonSocketPDUHeader& aHeader,
                                          DaemonSocketPDU& aPDU,
                                          BluetoothGattResultHandler* aRes);

  void ClientReadRemoteRssiRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothGattResultHandler* aRes);

  void ClientGetDeviceTypeRsp(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              BluetoothGattResultHandler* aRes);

  void ClientSetAdvDataRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           BluetoothGattResultHandler* aRes);

  void ClientTestCommandRsp(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU,
                            BluetoothGattResultHandler* aRes);

  void ServerRegisterServerRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothGattResultHandler* aRes);

  void ServerUnregisterServerRsp(const DaemonSocketPDUHeader& aHeader,
                                 DaemonSocketPDU& aPDU,
                                 BluetoothGattResultHandler* aRes);

  void ServerConnectRsp(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU,
                        BluetoothGattResultHandler* aRes);

  void ServerDisconnectRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           BluetoothGattResultHandler* aRes);

  void ServerAddServiceRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           BluetoothGattResultHandler* aRes);

  void ServerAddIncludedServiceRsp(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU,
                                   BluetoothGattResultHandler* aRes);

  void ServerAddCharacteristicRsp(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU,
                                  BluetoothGattResultHandler* aRes);

  void ServerAddDescriptorRsp(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              BluetoothGattResultHandler* aRes);

  void ServerStartServiceRsp(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU,
                             BluetoothGattResultHandler* aRes);

  void ServerStopServiceRsp(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU,
                            BluetoothGattResultHandler* aRes);

  void ServerDeleteServiceRsp(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU,
                              BluetoothGattResultHandler* aRes);

  void ServerSendIndicationRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothGattResultHandler* aRes);

  void ServerSendResponseRsp(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU,
                             BluetoothGattResultHandler* aRes);

  // TODO: Add L support

  void HandleRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Notifications
  //

  class NotificationHandlerWrapper;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int, BluetoothUuid,
      BluetoothGattStatus, int, const BluetoothUuid&>
      ClientRegisterForNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothAddress, int,
      BluetoothGattAdvData, const BluetoothAddress&, int,
      const BluetoothGattAdvData&>
      ClientScanResultNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus, int,
      BluetoothAddress, int, BluetoothGattStatus, int, const BluetoothAddress&>
      ClientConnectNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus, int,
      BluetoothAddress, int, BluetoothGattStatus, int, const BluetoothAddress&>
      ClientDisconnectNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus>
      ClientSearchCompleteNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, int, BluetoothGattServiceId, int,
      const BluetoothGattServiceId&>
      ClientSearchResultNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattServiceId, BluetoothGattId, BluetoothGattCharProp, int,
      BluetoothGattStatus, const BluetoothGattServiceId&,
      const BluetoothGattId&, const BluetoothGattCharProp&>
      ClientGetCharacteristicNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattServiceId, BluetoothGattId, BluetoothGattId, int,
      BluetoothGattStatus, const BluetoothGattServiceId&,
      const BluetoothGattId&, const BluetoothGattId&>
      ClientGetDescriptorNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattServiceId, BluetoothGattServiceId, int, BluetoothGattStatus,
      const BluetoothGattServiceId&, const BluetoothGattServiceId&>
      ClientGetIncludedServiceNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, int, int, BluetoothGattStatus,
      BluetoothGattServiceId, BluetoothGattId, int, int, BluetoothGattStatus,
      const BluetoothGattServiceId&, const BluetoothGattId&>
      ClientRegisterForNotificationNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, int, BluetoothGattNotifyParam, int,
      const BluetoothGattNotifyParam&>
      ClientNotifyNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattReadParam, int, BluetoothGattStatus,
      const BluetoothGattReadParam&>
      ClientReadCharacteristicNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattWriteParam, int, BluetoothGattStatus,
      const BluetoothGattWriteParam&>
      ClientWriteCharacteristicNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattReadParam, int, BluetoothGattStatus,
      const BluetoothGattReadParam&>
      ClientReadDescriptorNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus,
      BluetoothGattWriteParam, int, BluetoothGattStatus,
      const BluetoothGattWriteParam&>
      ClientWriteDescriptorNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, int, BluetoothGattStatus>
      ClientExecuteWriteNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, BluetoothAddress, int,
      BluetoothGattStatus, int, const BluetoothAddress&, int,
      BluetoothGattStatus>
      ClientReadRemoteRssiNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int>
      ClientListenNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int, BluetoothUuid,
      BluetoothGattStatus, int, const BluetoothUuid&>
      ServerRegisterNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, int, bool, BluetoothAddress, int,
      int, bool, const BluetoothAddress&>
      ServerConnectionNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int,
      BluetoothGattServiceId, BluetoothAttributeHandle, BluetoothGattStatus,
      int, const BluetoothGattServiceId&, const BluetoothAttributeHandle&>
      ServerServiceAddedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int,
      BluetoothAttributeHandle, BluetoothAttributeHandle, BluetoothGattStatus,
      int, const BluetoothAttributeHandle&, const BluetoothAttributeHandle&>
      ServerIncludedServiceAddedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int, BluetoothUuid,
      BluetoothAttributeHandle, BluetoothAttributeHandle, BluetoothGattStatus,
      int, const BluetoothUuid&, const BluetoothAttributeHandle&,
      const BluetoothAttributeHandle&>
      ServerCharacteristicAddedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int, BluetoothUuid,
      BluetoothAttributeHandle, BluetoothAttributeHandle, BluetoothGattStatus,
      int, const BluetoothUuid&, const BluetoothAttributeHandle&,
      const BluetoothAttributeHandle&>
      ServerDescriptorAddedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int,
      BluetoothAttributeHandle, BluetoothGattStatus, int,
      const BluetoothAttributeHandle&>
      ServerServiceStartedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int,
      BluetoothAttributeHandle, BluetoothGattStatus, int,
      const BluetoothAttributeHandle&>
      ServerServiceStoppedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int,
      BluetoothAttributeHandle, BluetoothGattStatus, int,
      const BluetoothAttributeHandle&>
      ServerServiceDeletedNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable6<
      NotificationHandlerWrapper, void, int, int, BluetoothAddress,
      BluetoothAttributeHandle, int, bool, int, int, const BluetoothAddress&,
      const BluetoothAttributeHandle&, int, bool>
      ServerRequestReadNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable9<
      NotificationHandlerWrapper, void, int, int, BluetoothAddress,
      BluetoothAttributeHandle, int, int, UniquePtr<uint8_t[]>, bool, bool, int,
      int, const BluetoothAddress&, const BluetoothAttributeHandle&, int, int,
      const uint8_t*, bool, bool>
      ServerRequestWriteNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable4<
      NotificationHandlerWrapper, void, int, int, BluetoothAddress, bool, int,
      int, const BluetoothAddress&, bool>
      ServerRequestExecuteWriteNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, BluetoothGattStatus, int>
      ServerResponseConfirmationNotification;

  class ClientGetDeviceTypeInitOp;
  class ClientScanResultInitOp;
  class ServerConnectionInitOp;
  class ServerRequestWriteInitOp;
  class ServerCharacteristicAddedInitOp;
  class ServerDescriptorAddedInitOp;

  void ClientRegisterClientNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ClientScanResultNtf(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU);

  void ClientConnectNtf(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU);

  void ClientDisconnectNtf(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU);

  void ClientSearchCompleteNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ClientSearchResultNtf(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU);

  void ClientGetCharacteristicNtf(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU);

  void ClientGetDescriptorNtf(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU);

  void ClientGetIncludedServiceNtf(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU);

  void ClientRegisterForNotificationNtf(const DaemonSocketPDUHeader& aHeader,
                                        DaemonSocketPDU& aPDU);

  void ClientNotifyNtf(const DaemonSocketPDUHeader& aHeader,
                       DaemonSocketPDU& aPDU);

  void ClientReadCharacteristicNtf(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU);

  void ClientWriteCharacteristicNtf(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU);

  void ClientReadDescriptorNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ClientWriteDescriptorNtf(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU);

  void ClientExecuteWriteNtf(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU);

  void ClientReadRemoteRssiNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ClientListenNtf(const DaemonSocketPDUHeader& aHeader,
                       DaemonSocketPDU& aPDU);

  void ServerRegisterServerNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ServerConnectionNtf(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU);

  void ServerServiceAddedNtf(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU);

  void ServerIncludedServiceAddedNtf(const DaemonSocketPDUHeader& aHeader,
                                     DaemonSocketPDU& aPDU);

  void ServerCharacteristicAddedNtf(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU);

  void ServerDescriptorAddedNtf(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU);

  void ServerServiceStartedNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ServerServiceStoppedNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ServerServiceDeletedNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void ServerRequestReadNtf(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU);

  void ServerRequestWriteNtf(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU);

  void ServerRequestExecuteWriteNtf(const DaemonSocketPDUHeader& aHeader,
                                    DaemonSocketPDU& aPDU);

  void ServerResponseConfirmationNtf(const DaemonSocketPDUHeader& aHeader,
                                     DaemonSocketPDU& aPDU);

  // TODO: Add L support

  void HandleNtf(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  static BluetoothGattNotificationHandler* sNotificationHandler;
};

class BluetoothDaemonGattInterface final : public BluetoothGattInterface {
  class CleanupResultHandler;
  class InitResultHandler;

 public:
  BluetoothDaemonGattInterface(BluetoothDaemonGattModule* aModule);
  ~BluetoothDaemonGattInterface();

  void SetNotificationHandler(
      BluetoothGattNotificationHandler* aNotificationHandler) override;

  /* Register / Unregister */
  void RegisterClient(const BluetoothUuid& aUuid,
                      BluetoothGattResultHandler* aRes) override;
  void UnregisterClient(int aClientIf,
                        BluetoothGattResultHandler* aRes) override;

  /* Start / Stop LE Scan */
  void Scan(int aClientIf, bool aStart,
            BluetoothGattResultHandler* aRes) override;

  /* Connect / Disconnect */
  void Connect(int aClientIf, const BluetoothAddress& aBdAddr,
               bool aIsDirect, /* auto connect */
               BluetoothTransport aTransport,
               BluetoothGattResultHandler* aRes) override;
  void Disconnect(int aClientIf, const BluetoothAddress& aBdAddr, int aConnId,
                  BluetoothGattResultHandler* aRes) override;

  /* Start / Stop advertisements to listen for incoming connections */
  void Listen(int aClientIf, bool aIsStart,
              BluetoothGattResultHandler* aRes) override;

  /* Clear the attribute cache for a given device*/
  void Refresh(int aClientIf, const BluetoothAddress& aBdAddr,
               BluetoothGattResultHandler* aRes) override;

  /* Enumerate Attributes */
  void SearchService(int aConnId, bool aSearchAll, const BluetoothUuid& aUuid,
                     BluetoothGattResultHandler* aRes) override;
  void GetIncludedService(int aConnId, const BluetoothGattServiceId& aServiceId,
                          bool aFirst,
                          const BluetoothGattServiceId& aStartServiceId,
                          BluetoothGattResultHandler* aRes) override;
  void GetCharacteristic(int aConnId, const BluetoothGattServiceId& aServiceId,
                         bool aFirst, const BluetoothGattId& aStartCharId,
                         BluetoothGattResultHandler* aRes) override;
  void GetDescriptor(int aConnId, const BluetoothGattServiceId& aServiceId,
                     const BluetoothGattId& aCharId, bool aFirst,
                     const BluetoothGattId& aDescriptorId,
                     BluetoothGattResultHandler* aRes) override;

  /* Read / Write An Attribute */
  void ReadCharacteristic(int aConnId, const BluetoothGattServiceId& aServiceId,
                          const BluetoothGattId& aCharId,
                          BluetoothGattAuthReq aAuthReq,
                          BluetoothGattResultHandler* aRes) override;
  void WriteCharacteristic(int aConnId,
                           const BluetoothGattServiceId& aServiceId,
                           const BluetoothGattId& aCharId,
                           BluetoothGattWriteType aWriteType,
                           BluetoothGattAuthReq aAuthReq,
                           const nsTArray<uint8_t>& aValue,
                           BluetoothGattResultHandler* aRes) override;
  void ReadDescriptor(int aConnId, const BluetoothGattServiceId& aServiceId,
                      const BluetoothGattId& aCharId,
                      const BluetoothGattId& aDescriptorId,
                      BluetoothGattAuthReq aAuthReq,
                      BluetoothGattResultHandler* aRes) override;
  void WriteDescriptor(int aConnId, const BluetoothGattServiceId& aServiceId,
                       const BluetoothGattId& aCharId,
                       const BluetoothGattId& aDescriptorId,
                       BluetoothGattWriteType aWriteType,
                       BluetoothGattAuthReq aAuthReq,
                       const nsTArray<uint8_t>& aValue,
                       BluetoothGattResultHandler* aRes) override;

  /* Execute / Abort Prepared Write*/
  void ExecuteWrite(int aConnId, int aIsExecute,
                    BluetoothGattResultHandler* aRes) override;

  /* Register / Deregister Characteristic Notifications or Indications */
  void RegisterNotification(int aClientIf, const BluetoothAddress& aBdAddr,
                            const BluetoothGattServiceId& aServiceId,
                            const BluetoothGattId& aCharId,
                            BluetoothGattResultHandler* aRes) override;
  void DeregisterNotification(int aClientIf, const BluetoothAddress& aBdAddr,
                              const BluetoothGattServiceId& aServiceId,
                              const BluetoothGattId& aCharId,
                              BluetoothGattResultHandler* aRes) override;

  void ReadRemoteRssi(int aClientIf, const BluetoothAddress& aBdAddr,
                      BluetoothGattResultHandler* aRes) override;

  void GetDeviceType(const BluetoothAddress& aBdAddr,
                     BluetoothGattResultHandler* aRes) override;

  /* Set advertising data or scan response data */
  void SetAdvData(int aServerIf, bool aIsScanRsp, bool aIsNameIncluded,
                  bool aIsTxPowerIncluded, int aMinInterval, int aMaxInterval,
                  int aApperance, const nsTArray<uint8_t>& aManufacturerData,
                  const nsTArray<uint8_t>& aServiceData,
                  const nsTArray<BluetoothUuid>& aServiceUuids,
                  BluetoothGattResultHandler* aRes) override;

  void TestCommand(int aCommand, const BluetoothGattTestParam& aTestParam,
                   BluetoothGattResultHandler* aRes) override;

  /* Register / Unregister */
  void RegisterServer(const BluetoothUuid& aUuid,
                      BluetoothGattResultHandler* aRes) override;
  void UnregisterServer(int aServerIf,
                        BluetoothGattResultHandler* aRes) override;

  /* Connect / Disconnect */
  void ConnectPeripheral(int aServerIf, const BluetoothAddress& aBdAddr,
                         bool aIsDirect, /* auto connect */
                         BluetoothTransport aTransport,
                         BluetoothGattResultHandler* aRes) override;
  void DisconnectPeripheral(int aServerIf, const BluetoothAddress& aBdAddr,
                            int aConnId,
                            BluetoothGattResultHandler* aRes) override;

  /* Add a services / a characteristic / a descriptor */
  void AddService(int aServerIf, const BluetoothGattServiceId& aServiceId,
                  uint16_t aNumHandles,
                  BluetoothGattResultHandler* aRes) override;
  void AddIncludedService(
      int aServerIf, const BluetoothAttributeHandle& aServiceHandle,
      const BluetoothAttributeHandle& aIncludedServiceHandle,
      BluetoothGattResultHandler* aRes) override;
  void AddCharacteristic(int aServerIf,
                         const BluetoothAttributeHandle& aServiceHandle,
                         const BluetoothUuid& aUuid,
                         BluetoothGattCharProp aProperties,
                         BluetoothGattAttrPerm aPermissions,
                         BluetoothGattResultHandler* aRes) override;
  void AddDescriptor(int aServerIf,
                     const BluetoothAttributeHandle& aServiceHandle,
                     const BluetoothUuid& aUuid,
                     BluetoothGattAttrPerm aPermissions,
                     BluetoothGattResultHandler* aRes) override;

  /* Start / Stop / Delete a service */
  void StartService(int aServerIf,
                    const BluetoothAttributeHandle& aServiceHandle,
                    BluetoothTransport aTransport,
                    BluetoothGattResultHandler* aRes) override;
  void StopService(int aServerIf,
                   const BluetoothAttributeHandle& aServiceHandle,
                   BluetoothGattResultHandler* aRes) override;
  void DeleteService(int aServerIf,
                     const BluetoothAttributeHandle& aServiceHandle,
                     BluetoothGattResultHandler* aRes) override;

  /* Send an indication or a notification */
  void SendIndication(int aServerIf,
                      const BluetoothAttributeHandle& aCharacteristicHandle,
                      int aConnId, const nsTArray<uint8_t>& aValue,
                      bool aConfirm, /* true: indication, false: notification */
                      BluetoothGattResultHandler* aRes) override;

  /* Send a response for an incoming indication */
  void SendResponse(int aConnId, int aTransId, uint16_t aStatus,
                    const BluetoothGattResponse& aResponse,
                    BluetoothGattResultHandler* aRes) override;

 private:
  void DispatchError(BluetoothGattResultHandler* aRes, BluetoothStatus aStatus);
  void DispatchError(BluetoothGattResultHandler* aRes, nsresult aRv);

  BluetoothDaemonGattModule* mModule;
};

END_BLUETOOTH_NAMESPACE

#endif  // mozilla_dom_bluetooth_bluedroid_BluetoothDaemonGattInterface_h
