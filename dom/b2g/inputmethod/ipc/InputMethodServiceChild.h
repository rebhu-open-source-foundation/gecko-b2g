/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethodServiceChild_h
#define mozilla_dom_InputMethodServiceChild_h

#include "mozilla/dom/PInputMethodServiceChild.h"
#include "InputMethodServiceCommon.h"

class nsIEditableSupportListener;
class nsIEditableSupport;

namespace mozilla {
namespace dom {

using mozilla::ipc::IPCResult;

class InputMethodServiceChild final
    : public InputMethodServiceCommon<PInputMethodServiceChild> {
  friend class PInputMethodServiceChild;

 public:
  NS_DECL_ISUPPORTS

  explicit InputMethodServiceChild(nsIEditableSupportListener* aListener);
  InputMethodServiceChild();

  // setup the real 'EditableSupport' object to handle the requests
  void SetEditableSupport(nsIEditableSupport* aEditableSupport) {
    mEditableSupport = aEditableSupport;
  }

  // The real 'EditableSupport' object to handle the requests
  nsIEditableSupport* GetEditableSupport() override { return mEditableSupport; }

  // The 'requester' that should be informed once the requests are done.
  nsIEditableSupportListener* GetEditableSupportListener(
      uint32_t aId) override {
    return mRequester;
  }

 protected:
  IPCResult RecvResponse(const InputMethodResponse& aResponse) {
    IPCResult result =
        InputMethodServiceCommon<PInputMethodServiceChild>::RecvResponse(
            aResponse);
    Unused << Send__delete__(this);
    return result;
  }

 private:
  ~InputMethodServiceChild();
  RefPtr<nsIEditableSupportListener> mRequester;
  RefPtr<nsIEditableSupport> mEditableSupport;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_InputMethodServiceChild_h
