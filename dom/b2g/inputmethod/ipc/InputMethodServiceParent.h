/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethodServiceParent_h
#define mozilla_dom_InputMethodServiceParent_h

#include "mozilla/dom/PInputMethodServiceParent.h"
#include "nsIInputMethodListener.h"
namespace mozilla {
namespace dom {

class InputMethodServiceParent final : public PInputMethodServiceParent,
                                       public nsIInputMethodListener,
                                       public nsIEditableSupportListener {
  friend class PInputMethodServiceParent;

 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODLISTENER
  NS_DECL_NSIEDITABLESUPPORTLISTENER

  InputMethodServiceParent();

 protected:
  mozilla::ipc::IPCResult RecvRequest(
      const InputMethodServiceRequest& aRequest);

 private:
  ~InputMethodServiceParent();
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_InputMethodServiceParent_h
