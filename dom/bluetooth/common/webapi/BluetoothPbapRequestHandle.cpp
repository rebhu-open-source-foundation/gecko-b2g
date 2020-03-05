/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothCommon.h"
#include "BluetoothPbapRequestHandle.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"

#include "mozilla/dom/BluetoothPbapRequestHandleBinding.h"
#include "mozilla/dom/ContentChild.h"

using namespace mozilla;
using namespace dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(BluetoothPbapRequestHandle, mOwner)
NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothPbapRequestHandle)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothPbapRequestHandle)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothPbapRequestHandle)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothPbapRequestHandle::BluetoothPbapRequestHandle(nsPIDOMWindowInner* aOwner)
  : mOwner(aOwner)
{
  MOZ_ASSERT(aOwner);
}

BluetoothPbapRequestHandle::~BluetoothPbapRequestHandle()
{
}

already_AddRefed<BluetoothPbapRequestHandle>
BluetoothPbapRequestHandle::Create(nsPIDOMWindowInner* aOwner)
{
  MOZ_ASSERT(aOwner);

  RefPtr<BluetoothPbapRequestHandle> handle =
    new BluetoothPbapRequestHandle(aOwner);

  return handle.forget();
}

already_AddRefed<DOMRequest>
BluetoothPbapRequestHandle::ReplyTovCardPulling(Blob& aBlob,
                                                ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowInner> win = GetParentObject();
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<DOMRequest> request = new DOMRequest(win);
  RefPtr<BluetoothVoidReplyRunnable> result =
    new BluetoothVoidReplyRunnable(request);

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  bs->ReplyTovCardPulling(aBlob.Impl(), result);

  return request.forget();
}

already_AddRefed<DOMRequest>
BluetoothPbapRequestHandle::ReplyToPhonebookPulling(Blob& aBlob,
                                                    uint16_t phonebookSize,
                                                    ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowInner> win = GetParentObject();
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<DOMRequest> request = new DOMRequest(win);
  RefPtr<BluetoothVoidReplyRunnable> result =
    new BluetoothVoidReplyRunnable(request);

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  bs->ReplyToPhonebookPulling(aBlob.Impl(), phonebookSize, result);

  return request.forget();
}

already_AddRefed<DOMRequest>
BluetoothPbapRequestHandle::ReplyTovCardListing(Blob& aBlob,
                                                uint16_t phonebookSize,
                                                ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowInner> win = GetParentObject();
  if (!win) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<DOMRequest> request = new DOMRequest(win);
  RefPtr<BluetoothVoidReplyRunnable> result =
    new BluetoothVoidReplyRunnable(request);

  BluetoothService* bs = BluetoothService::Get();
  if (!bs) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  bs->ReplyTovCardListing(aBlob.Impl(), phonebookSize, result);

  return request.forget();
}

JSObject*
BluetoothPbapRequestHandle::WrapObject(JSContext* aCx,
                                   JS::Handle<JSObject*> aGivenProto)
{
  return BluetoothPbapRequestHandle_Binding::Wrap(aCx, this, aGivenProto);
}
