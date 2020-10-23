/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethodService_h
#define mozilla_dom_InputMethodService_h

#include "mozilla/ClearOnShutdown.h"

class nsIInputMethodListener;
class nsIEditableSupportListener;
class nsIInputContext;
namespace mozilla {

namespace dom {

class ContentParent;

class InputMethodService final : public nsISupports {
 public:
  NS_DECL_ISUPPORTS

  static already_AddRefed<InputMethodService> GetInstance();
  void SetComposition(const nsAString& aText,
                      nsIInputMethodListener* aListener);
  void EndComposition(const nsAString& aText,
                      nsIInputMethodListener* aListener);
  void Keydown(const nsAString& aKey, nsIInputMethodListener* aListener);
  void Keyup(const nsAString& aKey, nsIInputMethodListener* aListener);
  void SendKey(const nsAString& aKey, nsIInputMethodListener* aListener);
  void DeleteBackward(nsIInputMethodListener* aListener);
  void SetSelectedOption(int32_t optionIndex);
  void SetSelectedOptions(const nsTArray<int32_t>& optionIndexes);
  void RemoveFocus();
  void HandleFocus(nsIEditableSupportListener* aListener,
                   nsIInputContext* aPropBag);
  void HandleBlur(nsIEditableSupportListener* aListener);
  void RegisterEditableSupport(nsIEditableSupportListener* aSupport) {
    mEditableSupportListener = aSupport;
  }

  void UnregisterEditableSupport(nsIEditableSupportListener* aSupport) {
    mEditableSupportListener = nullptr;
  }

 private:
  InputMethodService() = default;
  ~InputMethodService() = default;
  nsCOMPtr<nsIEditableSupportListener> mEditableSupportListener;
};

}  // namespace dom
}  // namespace mozilla
#endif  // mozilla_dom_InputMethodService_h
