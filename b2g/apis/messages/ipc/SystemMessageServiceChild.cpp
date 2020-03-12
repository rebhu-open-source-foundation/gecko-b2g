/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SystemMessageServiceChild.h"

#include "mozilla/dom/ContentChild.h"
#include "nsISystemMessageListener.h"

namespace mozilla {
namespace dom {

SystemMessageServiceChild::SystemMessageServiceChild(
    nsISystemMessageListener* aListener)
    : mListener(aListener) {}

SystemMessageServiceChild::~SystemMessageServiceChild() {}

mozilla::ipc::IPCResult SystemMessageServiceChild::RecvResponse(
    const SystemMessageServiceResponse& aResponse) {
  switch (aResponse.type()) {
    case SystemMessageServiceResponse::TSubscribeResponse: {
      const SubscribeResponse& response = aResponse;
      mListener->OnSubscribe(response.status());
      break;
    }
    default: {
      return IPC_FAIL(this, "Unknown SystemMessageService action type.");
    }
  }

  return IPC_OK();
}

}  // namespace dom
}  // namespace mozilla
