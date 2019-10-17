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
