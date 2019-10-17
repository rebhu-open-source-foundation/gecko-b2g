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

#ifndef mozilla_dom_SystemMessageServiceParent_h
#define mozilla_dom_SystemMessageServiceParent_h

#include "mozilla/dom/PSystemMessageServiceParent.h"
#include "nsISystemMessageListener.h"

class nsIURI;

namespace mozilla {
namespace dom {

class SystemMessageServiceParent final : public PSystemMessageServiceParent,
                                         public nsISystemMessageListener {
  friend class PSystemMessageServiceParent;

 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISYSTEMMESSAGELISTENER

  SystemMessageServiceParent();

 protected:
  mozilla::ipc::IPCResult RecvRequest(
      const SystemMessageServiceRequest& aRequest);

 private:
  ~SystemMessageServiceParent();
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_SystemMessageServiceParent_h
