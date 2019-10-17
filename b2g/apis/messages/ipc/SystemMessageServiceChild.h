/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_SystemMessageServiceChild_h
#define mozilla_dom_SystemMessageServiceChild_h

#include "mozilla/dom/PSystemMessageServiceChild.h"

class nsISystemMessageListener;

namespace mozilla {
namespace dom {

class SystemMessageServiceChild final : public PSystemMessageServiceChild {
  friend class PSystemMessageServiceChild;

 public:
  NS_INLINE_DECL_REFCOUNTING(SystemMessageServiceChild)

  SystemMessageServiceChild(nsISystemMessageListener* aListener);

 protected:
  mozilla::ipc::IPCResult RecvResponse(
      const SystemMessageServiceResponse& aResponse);

 private:
  ~SystemMessageServiceChild();

  RefPtr<nsISystemMessageListener> mListener;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_SystemMessageServiceChild_h
