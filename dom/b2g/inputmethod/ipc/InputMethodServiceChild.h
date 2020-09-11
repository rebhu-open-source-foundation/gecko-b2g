/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethodServiceChild_h
#define mozilla_dom_InputMethodServiceChild_h

#include "mozilla/dom/PInputMethodServiceChild.h"

class nsIInputMethodListener;
class nsIEditableSupportListener;

namespace mozilla {
namespace dom {

class InputMethodServiceChild final : public PInputMethodServiceChild {
  friend class PInputMethodServiceChild;

 public:
  NS_INLINE_DECL_REFCOUNTING(InputMethodServiceChild)

  explicit InputMethodServiceChild();
  void SetEditableSupportListener(nsIEditableSupportListener* aListener);
  void SetInputMethodListener(nsIInputMethodListener* aListener);

 protected:
  mozilla::ipc::IPCResult RecvResponse(
      const InputMethodServiceResponse& aResponse);

 private:
  ~InputMethodServiceChild();
  RefPtr<nsIInputMethodListener> mInputMethodListener;
  RefPtr<nsIEditableSupportListener> mEditableSupportListener;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_InputMethodServiceChild_h
