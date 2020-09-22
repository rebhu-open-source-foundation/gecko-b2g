/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SystemMessageServiceParent.h"
#include "mozilla/dom/SystemMessageService.h"
#include "nsIURI.h"

namespace mozilla {
namespace dom {

namespace {

SystemMessageService* GetSystemMessageService() {
  nsCOMPtr<nsISystemMessageService> service =
      do_GetService("@mozilla.org/systemmessage-service;1");
  MOZ_ASSERT(service);
  return static_cast<SystemMessageService*>(service.get());
}

}  // anonymous namespace

NS_IMPL_ISUPPORTS(SystemMessageServiceParent, nsISystemMessageListener)

SystemMessageServiceParent::SystemMessageServiceParent() {}

SystemMessageServiceParent::~SystemMessageServiceParent() {}

mozilla::ipc::IPCResult SystemMessageServiceParent::RecvRequest(
    const SystemMessageServiceRequest& aRequest) {
  SystemMessageService* service = GetSystemMessageService();
  MOZ_ASSERT(service);

  switch (aRequest.type()) {
    case SystemMessageServiceRequest::TSubscribeRequest: {
      const SubscribeRequest& request = aRequest;
      service->DoSubscribe(request.messageName(), request.origin(),
                           request.scope(), request.originSuffix(), this);
      break;
    }
    default: {
      return IPC_FAIL(this, "Unknown SystemMessageService action type.");
    }
  }

  return IPC_OK();
}

// nsISystemMessageListener methods.
NS_IMETHODIMP
SystemMessageServiceParent::OnSubscribe(nsresult aStatus) {
  SubscribeResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla
