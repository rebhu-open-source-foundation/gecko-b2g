/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothParent.h"

#include "mozilla/Unused.h"
#include "BluetoothRequestParent.h"
#include "BluetoothService.h"


using mozilla::Unused;
USING_BLUETOOTH_NAMESPACE

/*******************************************************************************
 * BluetoothParent
 ******************************************************************************/

BluetoothParent::BluetoothParent()
: mShutdownState(Running)
{
  MOZ_COUNT_CTOR(BluetoothParent);
}

BluetoothParent::~BluetoothParent()
{
  MOZ_COUNT_DTOR(BluetoothParent);
  MOZ_ASSERT(!mService);
  MOZ_ASSERT(mShutdownState == Dead);
}

void
BluetoothParent::BeginShutdown()
{
  // Only do something here if we haven't yet begun the shutdown sequence.
  if (mShutdownState == Running) {
    Unused << SendBeginShutdown();
    mShutdownState = SentBeginShutdown;
  }
}

bool
BluetoothParent::InitWithService(BluetoothService* aService)
{
  MOZ_ASSERT(aService);
  MOZ_ASSERT(!mService);

  if (!SendEnabled(aService->IsEnabled())) {
    return false;
  }

  mService = aService;
  return true;
}

void
BluetoothParent::UnregisterAllSignalHandlers()
{
  MOZ_ASSERT(mService);
  mService->UnregisterAllSignalHandlers(this);
}

void
BluetoothParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mService) {
    UnregisterAllSignalHandlers();
#ifdef DEBUG
    mService = nullptr;
#endif
  }

#ifdef DEBUG
  mShutdownState = Dead;
#endif
}

bool
BluetoothParent::RecvRegisterSignalHandler(const nsString& aNode)
{
  MOZ_ASSERT(mService);
  mService->RegisterBluetoothSignalHandler(aNode, this);
  return true;
}

bool
BluetoothParent::RecvUnregisterSignalHandler(const nsString& aNode)
{
  MOZ_ASSERT(mService);
  mService->UnregisterBluetoothSignalHandler(aNode, this);
  return true;
}

bool
BluetoothParent::RecvStopNotifying()
{
  MOZ_ASSERT(mService);

  if (mShutdownState != Running && mShutdownState != SentBeginShutdown) {
    MOZ_ASSERT(false, "Bad state!");
    return false;
  }

  mShutdownState = ReceivedStopNotifying;

  UnregisterAllSignalHandlers();

  if (SendNotificationsStopped()) {
    mShutdownState = SentNotificationsStopped;
    return true;
  }

  return false;
}

mozilla::ipc::IPCResult
BluetoothParent::RecvPBluetoothRequestConstructor(
                                                PBluetoothRequestParent* aActor,
                                                const Request& aRequest)
{
  BluetoothRequestParent* actor = static_cast<BluetoothRequestParent*>(aActor);

#ifdef DEBUG
  actor->mRequestType = aRequest.type();
#endif

  switch (aRequest.type()) {
    case Request::TGetAdaptersRequest:
      return actor->DoRequest(aRequest.get_GetAdaptersRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TStartBluetoothRequest:
      return actor->DoRequest(aRequest.get_StartBluetoothRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TStopBluetoothRequest:
      return actor->DoRequest(aRequest.get_StopBluetoothRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TSetPropertyRequest:
    return actor->DoRequest(aRequest.get_SetPropertyRequest())
      ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TStartDiscoveryRequest:
      return actor->DoRequest(aRequest.get_StartDiscoveryRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TStopDiscoveryRequest:
    return actor->DoRequest(aRequest.get_StopDiscoveryRequest())
      ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    // case Request::TStartLeScanRequest:
    //   return actor->DoRequest(aRequest.get_StartLeScanRequest());
    // case Request::TStopLeScanRequest:
    //   return actor->DoRequest(aRequest.get_StopLeScanRequest());
    // case Request::TStartAdvertisingRequest:
    //   return actor->DoRequest(aRequest.get_StartAdvertisingRequest());
    // case Request::TStopAdvertisingRequest:
    //   return actor->DoRequest(aRequest.get_StopAdvertisingRequest());
    case Request::TPairRequest:
      return actor->DoRequest(aRequest.get_PairRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TUnpairRequest:
      return actor->DoRequest(aRequest.get_UnpairRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TPairedDevicePropertiesRequest:
      return actor->DoRequest(aRequest.get_PairedDevicePropertiesRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TConnectedDevicePropertiesRequest:
      return actor->DoRequest(aRequest.get_ConnectedDevicePropertiesRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TFetchUuidsRequest:
      return actor->DoRequest(aRequest.get_FetchUuidsRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TPinReplyRequest:
      return actor->DoRequest(aRequest.get_PinReplyRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TSspReplyRequest:
      return actor->DoRequest(aRequest.get_SspReplyRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TConnectRequest:
      return actor->DoRequest(aRequest.get_ConnectRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TDisconnectRequest:
      return actor->DoRequest(aRequest.get_DisconnectRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TAcceptConnectionRequest:
      return actor->DoRequest(aRequest.get_AcceptConnectionRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TRejectConnectionRequest:
      return actor->DoRequest(aRequest.get_RejectConnectionRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TSendFileRequest:
      return actor->DoRequest(aRequest.get_SendFileRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TStopSendingFileRequest:
      return actor->DoRequest(aRequest.get_StopSendingFileRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TConfirmReceivingFileRequest:
      return actor->DoRequest(aRequest.get_ConfirmReceivingFileRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TDenyReceivingFileRequest:
      return actor->DoRequest(aRequest.get_DenyReceivingFileRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    // case Request::TConnectScoRequest:
    //   return actor->DoRequest(aRequest.get_ConnectScoRequest());
    // case Request::TDisconnectScoRequest:
    //   return actor->DoRequest(aRequest.get_DisconnectScoRequest());
    // case Request::TIsScoConnectedRequest:
    //   return actor->DoRequest(aRequest.get_IsScoConnectedRequest());
    case Request::TSetObexPasswordRequest:
      return actor->DoRequest(aRequest.get_SetObexPasswordRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TRejectObexAuthRequest:
      return actor->DoRequest(aRequest.get_RejectObexAuthRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyTovCardPullingRequest:
      return actor->DoRequest(aRequest.get_ReplyTovCardPullingRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToPhonebookPullingRequest:
      return actor->DoRequest(aRequest.get_ReplyToPhonebookPullingRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyTovCardListingRequest:
      return actor->DoRequest(aRequest.get_ReplyTovCardListingRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToFolderListingRequest:
      return actor->DoRequest(aRequest.get_ReplyToFolderListingRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToMessagesListingRequest:
      return actor->DoRequest(aRequest.get_ReplyToMessagesListingRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToGetMessageRequest:
      return actor->DoRequest(aRequest.get_ReplyToGetMessageRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToSetMessageStatusRequest:
      return actor->DoRequest(aRequest.get_ReplyToSetMessageStatusRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToSendMessageRequest:
      return actor->DoRequest(aRequest.get_ReplyToSendMessageRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TReplyToMessageUpdateRequest:
      return actor->DoRequest(aRequest.get_ReplyToMessageUpdateRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
// #ifdef MOZ_B2G_RIL
//     case Request::TAnswerWaitingCallRequest:
//       return actor->DoRequest(aRequest.get_AnswerWaitingCallRequest());
//     case Request::TIgnoreWaitingCallRequest:
//       return actor->DoRequest(aRequest.get_IgnoreWaitingCallRequest());
//     case Request::TToggleCallsRequest:
//       return actor->DoRequest(aRequest.get_ToggleCallsRequest());
// #endif
    case Request::TSendMetaDataRequest:
      return actor->DoRequest(aRequest.get_SendMetaDataRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TSendPlayStatusRequest:
      return actor->DoRequest(aRequest.get_SendPlayStatusRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    case Request::TSendMessageEventRequest:
      return actor->DoRequest(aRequest.get_SendMessageEventRequest())
        ? IPC_OK() : IPC_FAIL_NO_REASON(this);
    // case Request::TConnectGattClientRequest:
    //   return actor->DoRequest(aRequest.get_ConnectGattClientRequest());
    // case Request::TDisconnectGattClientRequest:
    //   return actor->DoRequest(aRequest.get_DisconnectGattClientRequest());
    // case Request::TDiscoverGattServicesRequest:
    //   return actor->DoRequest(aRequest.get_DiscoverGattServicesRequest());
    // case Request::TGattClientStartNotificationsRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientStartNotificationsRequest());
    // case Request::TGattClientStopNotificationsRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientStopNotificationsRequest());
    // case Request::TUnregisterGattClientRequest:
    //   return actor->DoRequest(aRequest.get_UnregisterGattClientRequest());
    // case Request::TGattClientReadRemoteRssiRequest:
    //   return actor->DoRequest(aRequest.get_GattClientReadRemoteRssiRequest());
    // case Request::TGattClientReadCharacteristicValueRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientReadCharacteristicValueRequest());
    // case Request::TGattClientWriteCharacteristicValueRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientWriteCharacteristicValueRequest());
    // case Request::TGattClientReadDescriptorValueRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientReadDescriptorValueRequest());
    // case Request::TGattClientWriteDescriptorValueRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattClientWriteDescriptorValueRequest());
    // case Request::TGattServerRegisterRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattServerRegisterRequest());
    // case Request::TGattServerConnectPeripheralRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattServerConnectPeripheralRequest());
    // case Request::TGattServerDisconnectPeripheralRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattServerDisconnectPeripheralRequest());
    // case Request::TUnregisterGattServerRequest:
    //   return actor->DoRequest(aRequest.get_UnregisterGattServerRequest());
    // case Request::TGattServerAddServiceRequest:
    //   return actor->DoRequest(aRequest.get_GattServerAddServiceRequest());
    // case Request::TGattServerAddIncludedServiceRequest:
    //   return actor->DoRequest(
    //            aRequest.get_GattServerAddIncludedServiceRequest());
    // case Request::TGattServerAddCharacteristicRequest:
    //   return actor->DoRequest(
    //            aRequest.get_GattServerAddCharacteristicRequest());
    // case Request::TGattServerAddDescriptorRequest:
    //   return actor->DoRequest(aRequest.get_GattServerAddDescriptorRequest());
    // case Request::TGattServerRemoveServiceRequest:
    //   return actor->DoRequest(aRequest.get_GattServerRemoveServiceRequest());
    // case Request::TGattServerStartServiceRequest:
    //   return actor->DoRequest(aRequest.get_GattServerStartServiceRequest());
    // case Request::TGattServerStopServiceRequest:
    //   return actor->DoRequest(aRequest.get_GattServerStopServiceRequest());
    // case Request::TGattServerSendResponseRequest:
    //   return actor->DoRequest(aRequest.get_GattServerSendResponseRequest());
    // case Request::TGattServerSendIndicationRequest:
    //   return actor->DoRequest(
    //     aRequest.get_GattServerSendIndicationRequest());
    default:
      MOZ_CRASH("Unknown type!");
  }

  MOZ_CRASH("Should never get here!");
}

PBluetoothRequestParent*
BluetoothParent::AllocPBluetoothRequestParent(const Request& aRequest)
{
  MOZ_ASSERT(mService);
  return new BluetoothRequestParent(mService);
}

bool
BluetoothParent::DeallocPBluetoothRequestParent(PBluetoothRequestParent* aActor)
{
  delete aActor;
  return true;
}

void
BluetoothParent::Notify(const BluetoothSignal& aSignal)
{
  Unused << SendNotify(aSignal);
}
