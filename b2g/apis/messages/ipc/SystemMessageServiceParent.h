/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
