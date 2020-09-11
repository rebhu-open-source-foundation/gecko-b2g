/* -*- Mode: c++; c-basic-offset: 2; tab-width: 4; indent-tabs-mode: nil; -*-
 * vim: set sw=2 ts=4 expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GeckoEditableSupport.h"

#include "PuppetWidget.h"
#include "nsIContent.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPrefs_intl.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "mozilla/TextEvents.h"
#include "mozilla/ToString.h"
#include "mozilla/dom/BrowserChild.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/IMELog.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/InputMethodServiceChild.h"

typedef mozilla::dom::ContentChild ContentChild;
typedef mozilla::dom::InputMethodService InputMethodService;
typedef mozilla::dom::InputMethodServiceChild InputMethodServiceChild;
typedef mozilla::dom::HandleFocusRequest HandleFocusRequest;
typedef mozilla::dom::HandleBlurRequest HandleBlurRequest;

namespace mozilla {

namespace widget {

NS_IMPL_ISUPPORTS(GeckoEditableSupport, TextEventDispatcherListener,
                  nsIEditableSupportListener, nsISupportsWeakReference)

GeckoEditableSupport::GeckoEditableSupport(nsIGlobalObject* aGlobal,
                                           nsWindow* aWindow)
    : mGlobal(aGlobal), mIsRemote(!aWindow), mWindow(aWindow) {}

nsresult GeckoEditableSupport::NotifyIME(
    TextEventDispatcher* aTextEventDispatcher,
    const IMENotification& aNotification) {
  IME_LOGD("NotifyIME, TextEventDispatcher:[%p]", aTextEventDispatcher);
  nsresult result;
  switch (aNotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      IME_LOGD("IME: REQUEST_TO_COMMIT_COMPOSITION");
      break;
    }

    case REQUEST_TO_CANCEL_COMPOSITION: {
      IME_LOGD("IME: REQUEST_TO_CANCEL_COMPOSITION");
      break;
    }

    case NOTIFY_IME_OF_FOCUS: {
      IME_LOGD("IME: NOTIFY_IME_OF_FOCUS");
      mDispatcher = aTextEventDispatcher;
      IME_LOGD("NotifyIME, mDispatcher:[%p]", mDispatcher.get());
      result = BeginInputTransaction(mDispatcher);
      NS_ENSURE_SUCCESS(result, result);
      {
        // oop
        ContentChild* contentChild = ContentChild::GetSingleton();
        if (contentChild) {
          InputMethodServiceChild* child = new InputMethodServiceChild();
          child->SetEditableSupportListener(this);
          contentChild->SendPInputMethodServiceConstructor(child);
          HandleFocusRequest request(contentChild->GetID());
          child->SendRequest(request);
        } else {
          // Call from parent process (or in-proces app).
          RefPtr<InputMethodService> service =
              dom::InputMethodService::GetInstance();
          service->HandleFocus(this);
        }
      }
      break;
    }

    case NOTIFY_IME_OF_BLUR: {
      IME_LOGD("IME: NOTIFY_IME_OF_BLUR");
      mDispatcher->EndInputTransaction(this);
      OnRemovedFrom(mDispatcher);
      {
        // oop
        ContentChild* contentChild = ContentChild::GetSingleton();
        if (contentChild) {
          InputMethodServiceChild* child = new InputMethodServiceChild();
          child->SetEditableSupportListener(this);
          contentChild->SendPInputMethodServiceConstructor(child);
          HandleBlurRequest request(contentChild->GetID());
          child->SendRequest(request);
        } else {
          // Call from parent process (or in-proces app).
          RefPtr<InputMethodService> service =
              dom::InputMethodService::GetInstance();
          service->HandleBlur(this);
        }
      }
      break;
    }

    case NOTIFY_IME_OF_SELECTION_CHANGE: {
      IME_LOGD("IME: NOTIFY_IME_OF_SELECTION_CHANGE: SelectionChangeData=%s",
               ToString(aNotification.mSelectionChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_TEXT_CHANGE: {
      // TODO
      IME_LOGD("IME: NOTIFY_IME_OF_TEXT_CHANGE: TextChangeData=%s",
               ToString(aNotification.mTextChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED: {
      IME_LOGD("IME: NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED");
      break;
    }

    default:
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP_(IMENotificationRequests)
GeckoEditableSupport::GetIMENotificationRequests() {
  // Assume we support all change just as TextInputProcessor
  return IMENotificationRequests(
      IMENotificationRequests::NOTIFY_TEXT_CHANGE |
      IMENotificationRequests::NOTIFY_POSITION_CHANGE);
}

void GeckoEditableSupport::OnRemovedFrom(
    TextEventDispatcher* aTextEventDispatcher) {
  mDispatcher = nullptr;
}

void GeckoEditableSupport::WillDispatchKeyboardEvent(
    TextEventDispatcher* aTextEventDispatcher,
    WidgetKeyboardEvent& aKeyboardEvent, uint32_t aIndexOfKeypress,
    void* aData) {}

/* static */
void GeckoEditableSupport::SetOnBrowserChild(dom::BrowserChild* aBrowserChild,
                                             nsIGlobalObject* aGlobal) {
  MOZ_ASSERT(!XRE_IsParentProcess());
  NS_ENSURE_TRUE_VOID(aBrowserChild);
  IME_LOGD("IME: SetOnBrowserChild");
  RefPtr<widget::PuppetWidget> widget(aBrowserChild->WebWidget());
  RefPtr<widget::TextEventDispatcherListener> listener =
      widget->GetNativeTextEventDispatcherListener();

  if (!listener ||
      listener.get() ==
          static_cast<widget::TextEventDispatcherListener*>(widget)) {
    // We need to set a new listener.
    RefPtr<widget::GeckoEditableSupport> editableSupport =
        new widget::GeckoEditableSupport(aGlobal, nullptr);

    // Tell PuppetWidget to use our listener for IME operations.
    widget->SetNativeTextEventDispatcherListener(editableSupport);

    return;
  }
}

// nsIEditableSupportListener methods.
NS_IMETHODIMP
GeckoEditableSupport::DoSetComposition(const nsAString& aText) {
  IME_LOGD("-- GeckoEditableSupport::DoSetComposition");
  IME_LOGD("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (aText.Length() == 0) {
    mDispatcher->SetPendingCompositionString(EmptyString());
  } else {
    mDispatcher->SetPendingCompositionString(aText);
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->FlushPendingComposition(status);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoEndComposition(const nsAString& aText) {
  IME_LOGD("-- GeckoEditableSupport::DoEndComposition");
  IME_LOGD("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (!mDispatcher->IsComposing()) {
    return NS_ERROR_FAILURE;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->CommitComposition(status, &aText);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoKeydown(const nsAString& aKey) {
  IME_LOGD("-- GeckoEditableSupport::DoKeydown");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  uint32_t flags = nsITextInputProcessor::KEY_NON_PRINTABLE_KEY |
                   nsITextInputProcessor::KEY_DONT_DISPATCH_MODIFIER_KEY_EVENT;
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eKeyDown, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport OnKeydown: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  if (mDispatcher->DispatchKeyboardEvent(eKeyDown, event, status)) {
    mDispatcher->MaybeDispatchKeypressEvents(event, status);
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoKeyup(const nsAString& aKey) {
  IME_LOGD("-- GeckoEditableSupport::DoKeyup");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  uint32_t flags = nsITextInputProcessor::KEY_NON_PRINTABLE_KEY |
                   nsITextInputProcessor::KEY_DONT_DISPATCH_MODIFIER_KEY_EVENT;
  // EventMessage msg = eKeyDown;
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eKeyUp, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport OnKeyup: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  mDispatcher->DispatchKeyboardEvent(eKeyUp, event, status);
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoSendKey(const nsAString& aKey) {
  IME_LOGD("-- EditableSupport DoSendKey");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  uint32_t flags = nsITextInputProcessor::KEY_NON_PRINTABLE_KEY |
                   nsITextInputProcessor::KEY_DONT_DISPATCH_MODIFIER_KEY_EVENT;
  // EventMessage msg = eKeyDown;
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eVoidEvent, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport DoSendKey: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  if (mDispatcher->DispatchKeyboardEvent(eKeyDown, event, status)) {
    mDispatcher->MaybeDispatchKeypressEvents(event, status);
  }
  mDispatcher->DispatchKeyboardEvent(eKeyUp, event, status);
  return NS_OK;
}

}  // namespace widget
}  // namespace mozilla
