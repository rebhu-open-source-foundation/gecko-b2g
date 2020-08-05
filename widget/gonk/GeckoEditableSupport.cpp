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
#include "nsIGeckoEditableSupportProxy.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/Promise.h"
#ifdef MOZ_WIDGET_GONK
#undef LOG
#include <android/log.h>
#define LOG(msg, ...)  __android_log_print(ANDROID_LOG_INFO,     \
                                               "IME",  \
                                               msg,                  \
                                               ##__VA_ARGS__)
#else
#  define LOG(...)
#endif

namespace mozilla {
namespace widget {

static nsCOMPtr<nsIGeckoEditableSupportProxy> sGeckoEditableSupportProxy;

already_AddRefed<nsIGeckoEditableSupportProxy>
GetOrCreateGeckoEditableSupportProxy(nsISupports* aSupports) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sGeckoEditableSupportProxy) {
    sGeckoEditableSupportProxy = do_CreateInstance(
      "@mozilla.org/dom/inputmethod/supportproxy;1");
    LOG("sGeckoEditableSupportProxy:[%p]", sGeckoEditableSupportProxy.get());
    MOZ_ASSERT(sGeckoEditableSupportProxy);
    sGeckoEditableSupportProxy->Init();
  }

  nsCOMPtr<nsIGeckoEditableSupportProxy> proxy = sGeckoEditableSupportProxy;
  proxy->Attach(aSupports);
  return proxy.forget();
}

class EditableSupportSetCompositionCallback final
    : public nsIEditableSupportSetCompositionCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEDITABLESUPPORTSETCOMPOSITIONCALLBACK
  explicit EditableSupportSetCompositionCallback(TextEventDispatcher* aDispatcher);

 protected:
  ~EditableSupportSetCompositionCallback();

 private:
  RefPtr<TextEventDispatcher> mDispatcher;
};

NS_IMPL_ISUPPORTS(EditableSupportSetCompositionCallback,
                  nsIEditableSupportSetCompositionCallback)

EditableSupportSetCompositionCallback::EditableSupportSetCompositionCallback(
    TextEventDispatcher* aDispatcher)
        : mDispatcher(aDispatcher) {
  LOG("EditableSupportSetCompositionCallback: mDispatcher:[%p]", mDispatcher.get());
}

EditableSupportSetCompositionCallback::~EditableSupportSetCompositionCallback() {}

NS_IMETHODIMP
EditableSupportSetCompositionCallback::OnSetComposition(const nsAString& aText) {
  LOG("-- EditableSupport OnSetComposition: OK");
  LOG("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  LOG("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
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

class EditableSupportEndCompositionCallback final
    : public nsIEditableSupportEndCompositionCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEDITABLESUPPORTENDCOMPOSITIONCALLBACK
  explicit EditableSupportEndCompositionCallback(TextEventDispatcher* aDispatcher);

 protected:
  ~EditableSupportEndCompositionCallback();

 private:
  RefPtr<TextEventDispatcher> mDispatcher;
};

NS_IMPL_ISUPPORTS(EditableSupportEndCompositionCallback,
                  nsIEditableSupportEndCompositionCallback)

EditableSupportEndCompositionCallback::EditableSupportEndCompositionCallback(
    TextEventDispatcher* aDispatcher)
        : mDispatcher(aDispatcher) {
  LOG("EditableSupportEndCompositionCallback: mDispatcher:[%p]", mDispatcher.get());
}

EditableSupportEndCompositionCallback::~EditableSupportEndCompositionCallback() {}

NS_IMETHODIMP
EditableSupportEndCompositionCallback::OnEndComposition(const nsAString& aText) {
  LOG("-- EditableSupport OnEndComposition: OK");
  LOG("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  LOG("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
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


NS_IMPL_ISUPPORTS(GeckoEditableSupport, TextEventDispatcherListener,
                  nsISupportsWeakReference)

GeckoEditableSupport::GeckoEditableSupport(nsIGlobalObject* aGlobal,
                                           nsWindow* aWindow)
      : mGlobal(aGlobal),
        mIsRemote(!aWindow),
        mWindow(aWindow) {

}

nsresult GeckoEditableSupport::NotifyIME(
    TextEventDispatcher* aTextEventDispatcher,
    const IMENotification& aNotification) {
  LOG("NotifyIME, TextEventDispatcher:[%p]", aTextEventDispatcher);
  nsresult result;
  switch (aNotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      LOG("IME: REQUEST_TO_COMMIT_COMPOSITION");
      break;
    }

    case REQUEST_TO_CANCEL_COMPOSITION: {
      LOG("IME: REQUEST_TO_CANCEL_COMPOSITION");
      break;
    }

    case NOTIFY_IME_OF_FOCUS: {
      LOG("IME: NOTIFY_IME_OF_FOCUS");
      mDispatcher = aTextEventDispatcher;
      LOG("NotifyIME, mDispatcher:[%p]", mDispatcher.get());
      nsCOMPtr<nsIGeckoEditableSupportProxy> proxy = 
        GetOrCreateGeckoEditableSupportProxy(mGlobal);
      proxy->HandleFocus(/* maybe input context */);
      {
        // Set callback
        mSetCompositionCallback = new EditableSupportSetCompositionCallback(mDispatcher);
        LOG("mSetCompositionCallback:[%p]", mSetCompositionCallback.get());
        mEndCompositionCallback = new EditableSupportEndCompositionCallback(mDispatcher);
        LOG("mEndCompositionCallback:[%p]", mEndCompositionCallback.get());
        proxy->SetComposition(mGlobal, mSetCompositionCallback);
        proxy->EndComposition(mGlobal, mEndCompositionCallback);
        result = BeginInputTransaction(mDispatcher);
        NS_ENSURE_SUCCESS(result, result);
      }
      break;
    }

    case NOTIFY_IME_OF_BLUR: {
      LOG("IME: NOTIFY_IME_OF_BLUR");
      mDispatcher->EndInputTransaction(this);
      OnRemovedFrom(mDispatcher);
      nsCOMPtr<nsIGeckoEditableSupportProxy> proxy = 
        GetOrCreateGeckoEditableSupportProxy(mGlobal);
      proxy->HandleBlur();
      break;
    }

    case NOTIFY_IME_OF_SELECTION_CHANGE: {
      LOG("IME: NOTIFY_IME_OF_SELECTION_CHANGE: SelectionChangeData=%s",
             ToString(aNotification.mSelectionChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_TEXT_CHANGE: {
      // TODO
      LOG("IME: NOTIFY_IME_OF_TEXT_CHANGE: TextChangeData=%s",
            ToString(aNotification.mTextChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED: {
      LOG("IME: NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED");
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

void GeckoEditableSupport::SetOnBrowserChild(dom::BrowserChild* aBrowserChild,
                                             nsIGlobalObject* aGlobal) {
  MOZ_ASSERT(!XRE_IsParentProcess());
  NS_ENSURE_TRUE_VOID(aBrowserChild);
  LOG("IME: SetOnBrowserChild");
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

}  // namespace widget
}  // namespace mozilla
