/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

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
      service->DoSubscribe(request.messageName(), request.spec(),
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
