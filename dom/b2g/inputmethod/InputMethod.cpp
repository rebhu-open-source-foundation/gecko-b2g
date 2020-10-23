/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/InputMethod.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/InputMethodListener.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/IMELog.h"

using namespace mozilla::widget;

namespace mozilla {
namespace dom {

InputMethod::InputMethod(nsIGlobalObject* aGlobal) : mGlobal(aGlobal) {
  MOZ_ASSERT(aGlobal);
}

nsresult InputMethod::PermissionCheck() {
  // TODO file bug 103458 to track
  return NS_OK;
}

already_AddRefed<Promise> InputMethod::SetComposition(const nsAString& aText) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::SetComposition");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->SetComposition(aText);
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }
  return promise.forget();
}

already_AddRefed<Promise> InputMethod::EndComposition(
    const Optional<nsAString>& aText) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::EndComposition");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->EndComposition(aText.WasPassed() ? aText.Value()
                                                               : VoidString());
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }

  return promise.forget();
}

already_AddRefed<Promise> InputMethod::Keydown(const nsAString& aKey) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::Keydown");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->Keydown(aKey);
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }

  return promise.forget();
}

already_AddRefed<Promise> InputMethod::Keyup(const nsAString& aKey) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::Keyup");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->Keyup(aKey);
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }

  return promise.forget();
}

already_AddRefed<Promise> InputMethod::SendKey(const nsAString& aKey) {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::SendKey");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->SendKey(aKey);
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }

  return promise.forget();
}

already_AddRefed<Promise> InputMethod::DeleteBackward() {
  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  IME_LOGD("-- InputMethod::DeleteBackward");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create(promise);
  nsresult result = listener->DeleteBackward();
  if (NS_FAILED(result)) {
    promise->MaybeReject(result);
  }

  return promise.forget();
}

void InputMethod::SetSelectedOption(int32_t aOptionIndex) {
  IME_LOGD("-- InputMethod::SetSelectedOption [%ld]", aOptionIndex);

  RefPtr<InputMethodListener> listener = InputMethodListener::Create();
  listener->SetSelectedOption(aOptionIndex);
}

void InputMethod::SetSelectedOptions(const nsTArray<int32_t>& aOptionIndexes) {
  IME_LOGD("-- InputMethod::SetSelectedOptions length:[%d]",
           aOptionIndexes.Length());

  RefPtr<InputMethodListener> listener = InputMethodListener::Create();
  listener->SetSelectedOptions(aOptionIndexes);
}

void InputMethod::RemoveFocus() {
  IME_LOGD("-- InputMethod::RemoveFocus");

  RefPtr<InputMethodListener> listener = InputMethodListener::Create();
  listener->RemoveFocus();
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(InputMethod, mGlobal)
NS_IMPL_CYCLE_COLLECTING_ADDREF(InputMethod)
NS_IMPL_CYCLE_COLLECTING_RELEASE(InputMethod)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InputMethod)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject* InputMethod::WrapObject(JSContext* aCx,
                                  JS::Handle<JSObject*> aGivenProto) {
  return InputMethod_Binding::Wrap(aCx, this, aGivenProto);
}

}  // namespace dom
}  // namespace mozilla
