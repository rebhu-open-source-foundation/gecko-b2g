/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethodListener_h
#define mozilla_dom_InputMethodListener_h

#include "mozilla/AlreadyAddRefed.h"
#include "nsIInputMethodListener.h"

namespace mozilla {
namespace dom {

class Promise;

class InputMethodListener final : public nsIInputMethodListener {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODLISTENER

  static already_AddRefed<InputMethodListener> Create(Promise* aPromise);
  static already_AddRefed<InputMethodListener> Create();

  nsresult SetComposition(const nsAString& aText);
  nsresult EndComposition(const nsAString& aText);
  nsresult Keydown(const nsAString& aKey);
  nsresult Keyup(const nsAString& aKey);
  nsresult SendKey(const nsAString& aKey);
  nsresult DeleteBackward();

  void SetSelectedOption(int32_t optionIndex);
  void SetSelectedOptions(const nsTArray<int32_t>& optionIndexes);

 private:
  explicit InputMethodListener(Promise* aPromise);
  explicit InputMethodListener() = default;
  ~InputMethodListener() = default;

  void Initialize();

  RefPtr<Promise> mPromise;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_InputMethodListener_h
