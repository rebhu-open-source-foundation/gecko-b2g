/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
