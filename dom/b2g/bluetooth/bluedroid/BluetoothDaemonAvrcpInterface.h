/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluedroid_BluetoothDaemonAvrcpInterface_h
#define mozilla_dom_bluetooth_bluedroid_BluetoothDaemonAvrcpInterface_h

#include "BluetoothDaemonHelpers.h"
#include "BluetoothInterface.h"
#include "mozilla/ipc/DaemonRunnables.h"

BEGIN_BLUETOOTH_NAMESPACE

using mozilla::ipc::DaemonSocketPDU;
using mozilla::ipc::DaemonSocketPDUHeader;
using mozilla::ipc::DaemonSocketResultHandler;

class BluetoothDaemonAvrcpModule {
 public:
  enum { SERVICE_ID = 0x08 };

  enum {
    OPCODE_ERROR = 0x00,
    OPCODE_GET_PLAY_STATUS_RSP = 0x01,
    OPCODE_LIST_PLAYER_APP_ATTR_RSP = 0x02,
    OPCODE_LIST_PLAYER_APP_VALUE_RSP = 0x03,
    OPCODE_GET_PLAYER_APP_VALUE_RSP = 0x04,
    OPCODE_GET_PLAYER_APP_ATTR_TEXT_RSP = 0x05,
    OPCODE_GET_PLAYER_APP_VALUE_TEXT_RSP = 0x06,
    OPCODE_GET_ELEMENT_ATTR_RSP = 0x07,
    OPCODE_SET_PLAYER_APP_VALUE_RSP = 0x08,
    OPCODE_REGISTER_NOTIFICATION_RSP = 0x09,
    OPCODE_SET_VOLUME = 0x0a,
    OPCODE_SET_ADDRESSED_PLAYER_RSP = 0x0b,
    OPCODE_SET_BROWSED_PLAYER_RSP = 0x0c,  // TODO: Support browsed player
    OPCODE_GET_FOLDER_ITEMS_LIST_RSP = 0x0d,

    OPCODE_REMOTE_FEATURES_NTF = 0x81,
    OPCODE_GET_PLAY_STATUS_NTF = 0x82,
    OPCODE_LIST_PLAYER_APP_ATTR_NTF = 0x83,
    OPCODE_LIST_PLAYER_APP_VALUES_NTF = 0x84,
    OPCODE_GET_PLAYER_APP_VALUE_NTF = 0x85,
    OPCODE_GET_PLAYER_APP_ATTRS_TEXT_NTF = 0x86,
    OPCODE_GET_PLAYER_APP_VALUES_TEXT_NTF = 0x87,
    OPCODE_SET_PLAYER_APP_VALUE_NTF = 0x88,
    OPCODE_GET_ELEMENT_ATTR_NTF = 0x89,
    OPCODE_REGISTER_NOTIFICATION_NTF = 0x8a,
    OPCODE_VOLUME_CHANGE_NTF = 0x8b,
    OPCODE_PASSTHROUGH_CMD_NTF = 0x8c,
    OPCODE_SET_ADDRESSED_PLAYER_NTF = 0x8d,
    OPCODE_SET_BROWSED_PLAYER_NTF = 0x8e,  // TODO: Support browsed player
    OPCODE_GET_FOLDER_ITEMS_NTF = 0x8f,
  };

  virtual nsresult Send(DaemonSocketPDU* aPDU,
                        DaemonSocketResultHandler* aRes) = 0;

  void SetNotificationHandler(
      BluetoothAvrcpNotificationHandler* aNotificationHandler);

  //
  // Commands
  //

  nsresult GetPlayStatusRspCmd(ControlPlayStatus aPlayStatus, uint32_t aSongLen,
                               uint32_t aSongPos,
                               BluetoothAvrcpResultHandler* aRes);

  nsresult ListPlayerAppAttrRspCmd(int aNumAttr,
                                   const BluetoothAvrcpPlayerAttribute* aPAttrs,
                                   BluetoothAvrcpResultHandler* aRes);

  nsresult ListPlayerAppValueRspCmd(int aNumVal, uint8_t* aPVals,
                                    BluetoothAvrcpResultHandler* aRes);

  nsresult GetPlayerAppValueRspCmd(uint8_t aNumAttrs, const uint8_t* aIds,
                                   const uint8_t* aValues,
                                   BluetoothAvrcpResultHandler* aRes);

  nsresult GetPlayerAppAttrTextRspCmd(int aNumAttr, const uint8_t* aIds,
                                      const char** aTexts,
                                      BluetoothAvrcpResultHandler* aRes);

  nsresult GetPlayerAppValueTextRspCmd(int aNumVal, const uint8_t* aIds,
                                       const char** aTexts,
                                       BluetoothAvrcpResultHandler* aRes);

  nsresult GetElementAttrRspCmd(uint8_t aNumAttr,
                                const BluetoothAvrcpElementAttribute* aAttr,
                                BluetoothAvrcpResultHandler* aRes);

  nsresult SetPlayerAppValueRspCmd(BluetoothAvrcpStatus aRspStatus,
                                   BluetoothAvrcpResultHandler* aRes);

  nsresult RegisterNotificationRspCmd(
      BluetoothAvrcpEvent aEvent, BluetoothAvrcpNotification aType,
      const BluetoothAvrcpNotificationParam& aParam,
      BluetoothAvrcpResultHandler* aRes);

  nsresult SetVolumeCmd(uint8_t aVolume, BluetoothAvrcpResultHandler* aRes);

  nsresult SetAddressedPlayerRspCmd(BluetoothAvrcpStatus aRspStatus,
                                    BluetoothAvrcpResultHandler* aRes);

  nsresult GetFolderItemsListRspCmd(
      BluetoothAvrcpStatus aRspStatus, uint16_t aUidCounter, uint8_t aNumItems,
      const nsTArray<BluetoothAvrcpItemPlayer>& aPlayers,
      BluetoothAvrcpResultHandler* aRes);

 protected:
  void HandleSvc(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Responses
  //

  typedef mozilla::ipc::DaemonResultRunnable0<BluetoothAvrcpResultHandler, void>
      ResultRunnable;

  typedef mozilla::ipc::DaemonResultRunnable1<BluetoothAvrcpResultHandler, void,
                                              BluetoothStatus, BluetoothStatus>
      ErrorRunnable;

  void ErrorRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                BluetoothAvrcpResultHandler* aRes);

  void GetPlayStatusRspRsp(const DaemonSocketPDUHeader& aHeader,
                           DaemonSocketPDU& aPDU,
                           BluetoothAvrcpResultHandler* aRes);

  void ListPlayerAppAttrRspRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothAvrcpResultHandler* aRes);

  void ListPlayerAppValueRspRsp(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU,
                                BluetoothAvrcpResultHandler* aRes);

  void GetPlayerAppValueRspRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothAvrcpResultHandler* aRes);

  void GetPlayerAppAttrTextRspRsp(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU,
                                  BluetoothAvrcpResultHandler* aRes);

  void GetPlayerAppValueTextRspRsp(const DaemonSocketPDUHeader& aHeader,
                                   DaemonSocketPDU& aPDU,
                                   BluetoothAvrcpResultHandler* aRes);

  void GetElementAttrRspRsp(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU,
                            BluetoothAvrcpResultHandler* aRes);

  void SetPlayerAppValueRspRsp(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU,
                               BluetoothAvrcpResultHandler* aRes);

  void RegisterNotificationRspRsp(const DaemonSocketPDUHeader& aHeader,
                                  DaemonSocketPDU& aPDU,
                                  BluetoothAvrcpResultHandler* aRes);

  void SetVolumeRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                    BluetoothAvrcpResultHandler* aRes);

  void SetAddressedPlayerRspRsp(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU,
                                BluetoothAvrcpResultHandler* aRes);

  void GetFolderItemsListRspRsp(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU,
                                BluetoothAvrcpResultHandler* aRes);

  void HandleRsp(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  //
  // Notifications
  //

  class NotificationHandlerWrapper;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, BluetoothAddress, unsigned long,
      const BluetoothAddress&>
      RemoteFeatureNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable0<NotificationHandlerWrapper,
                                                    void>
      GetPlayStatusNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable0<NotificationHandlerWrapper,
                                                    void>
      ListPlayerAppAttrNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable1<
      NotificationHandlerWrapper, void, BluetoothAvrcpPlayerAttribute>
      ListPlayerAppValuesNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, uint8_t,
      UniquePtr<BluetoothAvrcpPlayerAttribute[]>, uint8_t,
      const BluetoothAvrcpPlayerAttribute*>
      GetPlayerAppValueNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, uint8_t,
      UniquePtr<BluetoothAvrcpPlayerAttribute[]>, uint8_t,
      const BluetoothAvrcpPlayerAttribute*>
      GetPlayerAppAttrsTextNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable3<
      NotificationHandlerWrapper, void, uint8_t, uint8_t, UniquePtr<uint8_t[]>,
      uint8_t, uint8_t, const uint8_t*>
      GetPlayerAppValuesTextNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable1<
      NotificationHandlerWrapper, void, BluetoothAvrcpPlayerSettings,
      const BluetoothAvrcpPlayerSettings&>
      SetPlayerAppValueNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, uint8_t,
      UniquePtr<BluetoothAvrcpMediaAttribute[]>, uint8_t,
      const BluetoothAvrcpMediaAttribute*>
      GetElementAttrNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, BluetoothAvrcpEvent, uint32_t>
      RegisterNotificationNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<NotificationHandlerWrapper,
                                                    void, uint8_t, uint8_t>
      VolumeChangeNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable2<
      NotificationHandlerWrapper, void, uint8_t, uint8_t, uint8_t, uint8_t>
      PassthroughCmdNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable1<NotificationHandlerWrapper,
                                                    void, uint16_t>
      SetAddressedPlayerNotification;

  typedef mozilla::ipc::DaemonNotificationRunnable5<
      NotificationHandlerWrapper, void, uint8_t, uint32_t, uint32_t, uint8_t,
      UniquePtr<uint32_t[]>, uint8_t, uint32_t, uint32_t, uint8_t,
      const uint32_t*>
      GetFolderItemsNotification;

  class GetElementAttrInitOp;
  class GetPlayerAppAttrsTextInitOp;
  class GetPlayerAppValueInitOp;
  class GetPlayerAppValuesTextInitOp;
  class RemoteFeatureInitOp;
  class GetFolderItemsInitOp;

  void RemoteFeatureNtf(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU);

  void GetPlayStatusNtf(const DaemonSocketPDUHeader& aHeader,
                        DaemonSocketPDU& aPDU);

  void ListPlayerAppAttrNtf(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU);

  void ListPlayerAppValuesNtf(const DaemonSocketPDUHeader& aHeader,
                              DaemonSocketPDU& aPDU);

  void GetPlayerAppValueNtf(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU);

  void GetPlayerAppAttrsTextNtf(const DaemonSocketPDUHeader& aHeader,
                                DaemonSocketPDU& aPDU);

  void GetPlayerAppValuesTextNtf(const DaemonSocketPDUHeader& aHeader,
                                 DaemonSocketPDU& aPDU);

  void SetPlayerAppValueNtf(const DaemonSocketPDUHeader& aHeader,
                            DaemonSocketPDU& aPDU);

  void GetElementAttrNtf(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU);

  void RegisterNotificationNtf(const DaemonSocketPDUHeader& aHeader,
                               DaemonSocketPDU& aPDU);

  void VolumeChangeNtf(const DaemonSocketPDUHeader& aHeader,
                       DaemonSocketPDU& aPDU);

  void PassthroughCmdNtf(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU);

  void SetAddressedPlayerNtf(const DaemonSocketPDUHeader& aHeader,
                             DaemonSocketPDU& aPDU);

  void GetFolderItemsNtf(const DaemonSocketPDUHeader& aHeader,
                         DaemonSocketPDU& aPDU);

  void HandleNtf(const DaemonSocketPDUHeader& aHeader, DaemonSocketPDU& aPDU,
                 DaemonSocketResultHandler* aRes);

  static BluetoothAvrcpNotificationHandler* sNotificationHandler;
};

class BluetoothDaemonAvrcpInterface final : public BluetoothAvrcpInterface {
  class CleanupResultHandler;
  class InitResultHandler;

 public:
  explicit BluetoothDaemonAvrcpInterface(BluetoothDaemonAvrcpModule* aModule);
  ~BluetoothDaemonAvrcpInterface();

  void SetNotificationHandler(
      BluetoothAvrcpNotificationHandler* aNotificationHandler) override;

  void GetPlayStatusRsp(ControlPlayStatus aPlayStatus, uint32_t aSongLen,
                        uint32_t aSongPos,
                        BluetoothAvrcpResultHandler* aRes) override;

  void ListPlayerAppAttrRsp(int aNumAttr,
                            const BluetoothAvrcpPlayerAttribute* aPAttrs,
                            BluetoothAvrcpResultHandler* aRes) override;

  void ListPlayerAppValueRsp(int aNumVal, uint8_t* aPVals,
                             BluetoothAvrcpResultHandler* aRes) override;

  void GetPlayerAppValueRsp(uint8_t aNumAttrs, const uint8_t* aIds,
                            const uint8_t* aValues,
                            BluetoothAvrcpResultHandler* aRes) override;

  void GetPlayerAppAttrTextRsp(int aNumAttr, const uint8_t* aIds,
                               const char** aTexts,
                               BluetoothAvrcpResultHandler* aRes) override;

  void GetPlayerAppValueTextRsp(int aNumVal, const uint8_t* aIds,
                                const char** aTexts,
                                BluetoothAvrcpResultHandler* aRes) override;

  void GetElementAttrRsp(uint8_t aNumAttr,
                         const BluetoothAvrcpElementAttribute* aAttr,
                         BluetoothAvrcpResultHandler* aRes) override;

  void SetPlayerAppValueRsp(BluetoothAvrcpStatus aRspStatus,
                            BluetoothAvrcpResultHandler* aRes) override;

  void RegisterNotificationRsp(BluetoothAvrcpEvent aEvent,
                               BluetoothAvrcpNotification aType,
                               const BluetoothAvrcpNotificationParam& aParam,
                               BluetoothAvrcpResultHandler* aRes) override;

  void SetVolume(uint8_t aVolume, BluetoothAvrcpResultHandler* aRes) override;

  void SetAddressedPlayerRsp(BluetoothAvrcpStatus aRspStatus,
                             BluetoothAvrcpResultHandler* aRes) override;

  // TODO: Support AVRCP_ITEM_FOLDER and AVRCP_ITEM_MEDIA
  void GetFolderItemsListRsp(BluetoothAvrcpStatus aRspStatus,
                             uint16_t aUidCounter, uint8_t aNumItems,
                             const nsTArray<BluetoothAvrcpItemPlayer>& aPlayers,
                             BluetoothAvrcpResultHandler* aRes) override;

 private:
  void DispatchError(BluetoothAvrcpResultHandler* aRes,
                     BluetoothStatus aStatus);
  void DispatchError(BluetoothAvrcpResultHandler* aRes, nsresult aRv);

  BluetoothDaemonAvrcpModule* mModule;
};

END_BLUETOOTH_NAMESPACE

#endif  // mozilla_dom_bluetooth_bluedroid_BluetoothDaemonAvrcpInterface_h
