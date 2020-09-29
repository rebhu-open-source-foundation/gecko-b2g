/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/InputMethodServiceChild.h"
#include "nsIInputMethodListener.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/StaticPtr.h"
#include "js/JSON.h"
#include "mozilla/dom/IMELog.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

LogModule* GetIMELog() {
  static LazyLogModule sIMELog("IME");
  return sIMELog;
}

static StaticRefPtr<InputMethodService> sInputMethodService;

NS_IMPL_ISUPPORTS(InputMethodService, nsISupports)

InputMethodService::InputMethodService() {}

InputMethodService::~InputMethodService() {}

/*static*/
already_AddRefed<InputMethodService> InputMethodService::GetInstance() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  if (!sInputMethodService) {
    sInputMethodService = new InputMethodService();
    ClearOnShutdown(&sInputMethodService);
  }

  RefPtr<InputMethodService> service = sInputMethodService;
  return service.forget();
}

void InputMethodService::SetComposition(const nsAString& aText,
                                        nsIInputMethodListener* aListener) {
  IME_LOGD("InputMethodService::SetComposition");
  mEditableSupportListener->DoSetComposition(aText);
  // TODO resolve/reject on result of DoSetComposition.
  aListener->OnSetComposition(NS_OK);
  return;
}

void InputMethodService::EndComposition(const nsAString& aText,
                                        nsIInputMethodListener* aListener) {
  IME_LOGD("InputMethodService::EndComposition");
  mEditableSupportListener->DoEndComposition(aText);
  // TODO resolve/reject on result of DoEndComposition.
  aListener->OnEndComposition(NS_OK);
  return;
}

void InputMethodService::Keydown(const nsAString& aKey,
                                 nsIInputMethodListener* aListener) {
  IME_LOGD("InputMethodService::Keydown");
  mEditableSupportListener->DoKeydown(aKey);
  // TODO resolve/reject on result of DoKeydown.
  aListener->OnKeydown(NS_OK);
  return;
}

void InputMethodService::Keyup(const nsAString& aKey,
                               nsIInputMethodListener* aListener) {
  IME_LOGD("InputMethodService::Keyup");
  mEditableSupportListener->DoKeyup(aKey);
  // TODO resolve/reject on result of DoKeyup.
  aListener->OnKeyup(NS_OK);
  return;
}

void InputMethodService::SendKey(const nsAString& aKey,
                                 nsIInputMethodListener* aListener) {
  IME_LOGD("InputMethodService::SendKey");
  mEditableSupportListener->DoSendKey(aKey);
  // TODO resolve/reject on result of DoSendKey.
  aListener->OnSendKey(NS_OK);
  return;
}

void InputMethodService::HandleFocus(nsIEditableSupportListener* aListener,
                                     nsIPropertyBag2* aPropBag) {
  IME_LOGD("InputMethodService::HandleFocus");
  RegisterEditableSupport(aListener);
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->NotifyObservers(aPropBag, "inputmethod-contextchange", u"focus");
  }
  return;
}

void InputMethodService::HandleBlur(nsIEditableSupportListener* aListener) {
  IME_LOGD("InputMethodService::HandleBlur");
  UnregisterEditableSupport(aListener);
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    // Blur does not need input field information.
    obs->NotifyObservers(nullptr, "inputmethod-contextchange", u"blur");
  }
  return;
}

}  // namespace dom
}  // namespace mozilla
