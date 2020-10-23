/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/InputMethodListener.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/InputMethodServiceChild.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/IMELog.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(InputMethodListener, nsIInputMethodListener)

/*static*/
already_AddRefed<InputMethodListener> InputMethodListener::Create(
    Promise* aPromise) {
  IME_LOGD("--InputMethodListener::Create");
  RefPtr<InputMethodListener> listener = new InputMethodListener(aPromise);

  return listener.forget();
}

/*static*/
already_AddRefed<InputMethodListener> InputMethodListener::Create() {
  IME_LOGD("--InputMethodListener::Create without promise");
  RefPtr<InputMethodListener> listener = new InputMethodListener();

  return listener.forget();
}

InputMethodListener::InputMethodListener(Promise* aPromise)
    : mPromise(aPromise) {}

// nsIInputMethodListener methods.
NS_IMETHODIMP
InputMethodListener::OnSetComposition(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnSetComposition");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
InputMethodListener::OnEndComposition(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnEndComposition");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
InputMethodListener::OnKeydown(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnKeydown");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
InputMethodListener::OnKeyup(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnKeyup");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
InputMethodListener::OnSendKey(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnSendKey");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      mPromise->MaybeResolve(JS::UndefinedHandleValue);
    } else {
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
InputMethodListener::OnDeleteBackward(nsresult aStatus) {
  IME_LOGD("--InputMethodListener::OnDeleteBackward");
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      IME_LOGD("--InputMethodListener::OnDeleteBackward: true");
      mPromise->MaybeResolve(true);
    } else {
      IME_LOGD("--InputMethodListener::OnDeleteBackward: false");
      mPromise->MaybeReject(aStatus);
    }

    mPromise = nullptr;
  }

  return NS_OK;
}

nsresult InputMethodListener::SetComposition(const nsAString& aText) {
  nsString text(aText);
  // TODO use a pure interface, and make it point to either the remote version
  // or the local version at Listener's creation.
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::SetComposition content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    SetCompositionRequest request(text);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::SetComposition in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->SetComposition(aText, this);
  }
  return NS_OK;
}

nsresult InputMethodListener::EndComposition(const nsAString& aText) {
  nsString text(aText);
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::EndComposition content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    EndCompositionRequest request(text);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::EndComposition in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->EndComposition(aText, this);
  }
  return NS_OK;
}

nsresult InputMethodListener::Keydown(const nsAString& aKey) {
  nsString key(aKey);
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::Keydown content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    KeydownRequest request(key);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::Keydown in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->Keydown(aKey, this);
  }
  return NS_OK;
}

nsresult InputMethodListener::Keyup(const nsAString& aKey) {
  nsString key(aKey);
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::Keyup content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    KeyupRequest request(key);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::Keyup in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->Keyup(aKey, this);
  }
  return NS_OK;
}

nsresult InputMethodListener::SendKey(const nsAString& aKey) {
  nsString key(aKey);
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::SendKey content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    SendKeyRequest request(key);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::SendKey in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->SendKey(aKey, this);
  }
  return NS_OK;
}

nsresult InputMethodListener::DeleteBackward() {
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::DeleteBackward content process");
    // Call from content process.
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    DeleteBackwardRequest request;
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::DeleteBackward in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->DeleteBackward(this);
  }
  return NS_OK;
}

void InputMethodListener::SetSelectedOption(int32_t aOptionIndex) {
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::SetSelectedOption content process");
    // Call from content process.
    RefPtr<InputMethodServiceChild> child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    SetSelectedOptionRequest request(aOptionIndex);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::SetSelectedOption in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->SetSelectedOption(aOptionIndex);
  }
}
void InputMethodListener::SetSelectedOptions(
    const nsTArray<int32_t>& aOptionIndexes) {
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::SetSelectedOptions content process");
    // Call from content process.
    RefPtr<InputMethodServiceChild> child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    SetSelectedOptionsRequest request(aOptionIndexes);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::SetSelectedOptions in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->SetSelectedOptions(aOptionIndexes);
  }
}

void InputMethodListener::RemoveFocus() {
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    IME_LOGD("--InputMethodListener::RemoveFocus content process");
    // Call from content process.
    RefPtr<InputMethodServiceChild> child = new InputMethodServiceChild();
    child->SetInputMethodListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    CommonRequest request(u"RemoveFocus"_ns);
    child->SendRequest(request);
  } else {
    IME_LOGD("--InputMethodListener::RemoveFocus in-process");
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = InputMethodService::GetInstance();
    MOZ_ASSERT(service);
    service->RemoveFocus();
  }
}

}  // namespace dom
}  // namespace mozilla
