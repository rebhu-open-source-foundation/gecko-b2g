/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BluetoothRequestParent.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "nsThreadUtils.h"
#include "mozilla/dom/IPCBlobUtils.h"

USING_BLUETOOTH_NAMESPACE

/*******************************************************************************
 * BluetoothRequestParent::ReplyRunnable
 ******************************************************************************/

class BluetoothRequestParent::ReplyRunnable final
    : public BluetoothReplyRunnable {
  BluetoothRequestParent* mRequest;

 public:
  explicit ReplyRunnable(BluetoothRequestParent* aRequest)
      : BluetoothReplyRunnable(nullptr), mRequest(aRequest) {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aRequest);
  }

  NS_IMETHOD
  Run() override {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mReply);

    if (mRequest) {
      // Must do this first because Send__delete__ will delete mRequest.
      mRequest->RequestComplete();

      if (!mRequest->Send__delete__(mRequest, *mReply)) {
        BT_WARNING("Failed to send response to child process!");
        return NS_ERROR_FAILURE;
      }
    }

    ReleaseMembers();
    return NS_OK;
  }

  void Revoke() { ReleaseMembers(); }

  virtual bool ParseSuccessfulReply(
      JS::MutableHandle<JS::Value> aValue) override {
    MOZ_CRASH("This should never be called!");
  }

  virtual void ReleaseMembers() override {
    MOZ_ASSERT(NS_IsMainThread());
    mRequest = nullptr;
    BluetoothReplyRunnable::ReleaseMembers();
  }
};

/*******************************************************************************
 * BluetoothRequestParent
 ******************************************************************************/

BluetoothRequestParent::BluetoothRequestParent(BluetoothService* aService)
    : mService(aService)
#ifdef DEBUG
      ,
      mRequestType(Request::T__None)
#endif
{
  MOZ_COUNT_CTOR(BluetoothRequestParent);
  MOZ_ASSERT(aService);

  mReplyRunnable = new ReplyRunnable(this);
}

BluetoothRequestParent::~BluetoothRequestParent() {
  MOZ_COUNT_DTOR(BluetoothRequestParent);

  // mReplyRunnable will be automatically revoked.
}

void BluetoothRequestParent::ActorDestroy(ActorDestroyReason aWhy) {
  mReplyRunnable.Revoke();
}

void BluetoothRequestParent::RequestComplete() {
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mReplyRunnable.IsPending());

  mReplyRunnable.Forget();
}

bool BluetoothRequestParent::DoRequest(const GetAdaptersRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGetAdaptersRequest);

  nsresult rv = mService->GetAdaptersInternal(mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const StartBluetoothRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStartBluetoothRequest);

  nsresult rv = mService->StartInternal(mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const StopBluetoothRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopBluetoothRequest);

  nsresult rv = mService->StopInternal(mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const SetPropertyRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSetPropertyRequest);

  nsresult rv = mService->SetProperty(aRequest.type(), aRequest.value(),
                                      mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const StartDiscoveryRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStartDiscoveryRequest);

  mService->StartDiscoveryInternal(mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const StopDiscoveryRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopDiscoveryRequest);

  mService->StopDiscoveryInternal(mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const StartLeScanRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStartLeScanRequest);

  mService->StartLeScanInternal(aRequest.serviceUuids(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const StopLeScanRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopLeScanRequest);

  mService->StopLeScanInternal(aRequest.scanUuid(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const StartAdvertisingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStartAdvertisingRequest);

  mService->StartAdvertisingInternal(aRequest.appUuid(), aRequest.data(),
                                     mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const StopAdvertisingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopAdvertisingRequest);

  mService->StopAdvertisingInternal(aRequest.appUuid(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const PairRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TPairRequest);

  nsresult rv = mService->CreatePairedDeviceInternal(
      aRequest.address(), aRequest.timeoutMS(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const UnpairRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TUnpairRequest);

  nsresult rv =
      mService->RemoveDeviceInternal(aRequest.address(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const PairedDevicePropertiesRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TPairedDevicePropertiesRequest);

  nsresult rv = mService->GetPairedDevicePropertiesInternal(
      aRequest.addresses(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ConnectedDevicePropertiesRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectedDevicePropertiesRequest);
  nsresult rv = mService->GetConnectedDevicePropertiesInternal(
      aRequest.serviceUuid(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const FetchUuidsRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TFetchUuidsRequest);
  nsresult rv =
      mService->FetchUuidsInternal(aRequest.address(), mReplyRunnable.get());

  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool BluetoothRequestParent::DoRequest(const PinReplyRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TPinReplyRequest);

  mService->PinReplyInternal(aRequest.address(), aRequest.accept(),
                             aRequest.pinCode(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const SspReplyRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSspReplyRequest);

  mService->SspReplyInternal(aRequest.address(), aRequest.variant(),
                             aRequest.accept(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const ConnectRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectRequest);

  mService->Connect(aRequest.address(), aRequest.cod(), aRequest.serviceUuid(),
                    mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const DisconnectRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDisconnectRequest);

  mService->Disconnect(aRequest.address(), aRequest.serviceUuid(),
                       mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const AcceptConnectionRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TAcceptConnectionRequest);

  mService->AcceptConnection(aRequest.serviceUuid(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const RejectConnectionRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TRejectConnectionRequest);

  mService->RejectConnection(aRequest.serviceUuid(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const SendFileRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSendFileRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->SendFile(aRequest.address(), blobImpl.get(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const StopSendingFileRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopSendingFileRequest);

  mService->StopSendingFile(aRequest.address(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ConfirmReceivingFileRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConfirmReceivingFileRequest);

  mService->ConfirmReceivingFile(aRequest.address(), true,
                                 mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const DenyReceivingFileRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDenyReceivingFileRequest);

  mService->ConfirmReceivingFile(aRequest.address(), false,
                                 mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(const ConnectScoRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectScoRequest);

  mService->ConnectSco(mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(const DisconnectScoRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDisconnectScoRequest);

  mService->DisconnectSco(mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(const IsScoConnectedRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TIsScoConnectedRequest);

  mService->IsScoConnected(mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(const SetObexPasswordRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSetObexPasswordRequest);

  mService->SetObexPassword(aRequest.password(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const RejectObexAuthRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TRejectObexAuthRequest);

  mService->RejectObexAuth(mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyTovCardPullingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyTovCardPullingRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->ReplyTovCardPulling(blobImpl.get(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToPhonebookPullingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToPhonebookPullingRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->ReplyToPhonebookPulling(blobImpl.get(), aRequest.phonebookSize(),
                                    mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyTovCardListingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyTovCardListingRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->ReplyTovCardListing(blobImpl.get(), aRequest.phonebookSize(),
                                mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToFolderListingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToFolderListingRequest);

  mService->ReplyToMapFolderListing(aRequest.masId(), aRequest.folderList(),
                                    mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToMessagesListingRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToMessagesListingRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->ReplyToMapMessagesListing(
      aRequest.masId(), blobImpl.get(), aRequest.newMessage(),
      aRequest.timeStamp(), aRequest.size(), mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToGetMessageRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToGetMessageRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->ReplyToMapGetMessage(aRequest.masId(), blobImpl.get(),
                                 mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToSetMessageStatusRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToSetMessageStatusRequest);

  mService->ReplyToMapSetMessageStatus(
      aRequest.masId(), aRequest.messageStatus(), mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToSendMessageRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToSendMessageRequest);

  mService->ReplyToMapSendMessage(aRequest.masId(), aRequest.handleId(),
                                  aRequest.messageStatus(),
                                  mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ReplyToMessageUpdateRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TReplyToMessageUpdateRequest);

  mService->ReplyToMapMessageUpdate(aRequest.masId(), aRequest.messageStatus(),
                                    mReplyRunnable.get());
  return true;
}

#ifdef MOZ_B2G_RIL
bool BluetoothRequestParent::DoRequest(
    const AnswerWaitingCallRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TAnswerWaitingCallRequest);

  mService->AnswerWaitingCall(mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const IgnoreWaitingCallRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TIgnoreWaitingCallRequest);

  mService->IgnoreWaitingCall(mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(const ToggleCallsRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TToggleCallsRequest);

  mService->ToggleCalls(mReplyRunnable.get());

  return true;
}
#endif  // MOZ_B2G_RIL

bool BluetoothRequestParent::DoRequest(const SendMetaDataRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSendMetaDataRequest);

  mService->SendMetaData(aRequest.title(), aRequest.artist(), aRequest.album(),
                         aRequest.mediaNumber(), aRequest.totalMediaCount(),
                         aRequest.duration(), mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(const SendPlayStatusRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSendPlayStatusRequest);

  mService->SendPlayStatus(aRequest.duration(), aRequest.position(),
                           aRequest.playStatus(), mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const SendMessageEventRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSendMessageEventRequest);

  RefPtr<BlobImpl> blobImpl = IPCBlobUtils::Deserialize(aRequest.blob());
  mService->SendMessageEvent(aRequest.masId(), blobImpl.get(),
                             mReplyRunnable.get());
  return true;
}

bool BluetoothRequestParent::DoRequest(
    const ConnectGattClientRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectGattClientRequest);

  mService->ConnectGattClientInternal(
      aRequest.appUuid(), aRequest.deviceAddress(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const DisconnectGattClientRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDisconnectGattClientRequest);

  mService->DisconnectGattClientInternal(
      aRequest.appUuid(), aRequest.deviceAddress(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const DiscoverGattServicesRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDiscoverGattServicesRequest);

  mService->DiscoverGattServicesInternal(aRequest.appUuid(),
                                         mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientStartNotificationsRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattClientStartNotificationsRequest);

  mService->GattClientStartNotificationsInternal(
      aRequest.appUuid(), aRequest.handle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientStopNotificationsRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattClientStopNotificationsRequest);

  mService->GattClientStopNotificationsInternal(
      aRequest.appUuid(), aRequest.handle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const UnregisterGattClientRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TUnregisterGattClientRequest);

  mService->UnregisterGattClientInternal(aRequest.clientIf(),
                                         mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientReadRemoteRssiRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattClientReadRemoteRssiRequest);

  mService->GattClientReadRemoteRssiInternal(
      aRequest.clientIf(), aRequest.deviceAddress(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientReadCharacteristicValueRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType ==
             Request::TGattClientReadCharacteristicValueRequest);

  mService->GattClientReadCharacteristicValueInternal(
      aRequest.appUuid(), aRequest.handle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientWriteCharacteristicValueRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType ==
             Request::TGattClientWriteCharacteristicValueRequest);

  mService->GattClientWriteCharacteristicValueInternal(
      aRequest.appUuid(), aRequest.handle(), aRequest.writeType(),
      aRequest.value(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientReadDescriptorValueRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattClientReadDescriptorValueRequest);

  mService->GattClientReadDescriptorValueInternal(
      aRequest.appUuid(), aRequest.handle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattClientWriteDescriptorValueRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattClientWriteDescriptorValueRequest);

  mService->GattClientWriteDescriptorValueInternal(
      aRequest.appUuid(), aRequest.handle(), aRequest.value(),
      mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerRegisterRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerRegisterRequest);

  mService->GattServerRegisterInternal(aRequest.appUuid(),
                                       mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerConnectPeripheralRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerConnectPeripheralRequest);

  mService->GattServerConnectPeripheralInternal(
      aRequest.appUuid(), aRequest.address(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerDisconnectPeripheralRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerDisconnectPeripheralRequest);

  mService->GattServerDisconnectPeripheralInternal(
      aRequest.appUuid(), aRequest.address(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const UnregisterGattServerRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TUnregisterGattServerRequest);

  mService->UnregisterGattServerInternal(aRequest.serverIf(),
                                         mReplyRunnable.get());

  return true;
}
bool BluetoothRequestParent::DoRequest(
    const GattServerAddServiceRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerAddServiceRequest);

  mService->GattServerAddServiceInternal(aRequest.appUuid(), aRequest.gattDb(),
                                         mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerRemoveServiceRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerRemoveServiceRequest);

  mService->GattServerRemoveServiceInternal(
      aRequest.appUuid(), aRequest.serviceHandle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerStopServiceRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerStopServiceRequest);

  mService->GattServerStopServiceInternal(
      aRequest.appUuid(), aRequest.serviceHandle(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerSendIndicationRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerSendIndicationRequest);

  mService->GattServerSendIndicationInternal(
      aRequest.appUuid(), aRequest.address(), aRequest.characteristicHandle(),
      aRequest.confirm(), aRequest.value(), mReplyRunnable.get());

  return true;
}

bool BluetoothRequestParent::DoRequest(
    const GattServerSendResponseRequest& aRequest) {
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGattServerSendResponseRequest);

  mService->GattServerSendResponseInternal(
      aRequest.appUuid(), aRequest.address(), aRequest.status(),
      aRequest.requestId(), aRequest.response(), mReplyRunnable.get());

  return true;
}
