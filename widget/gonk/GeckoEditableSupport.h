/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_widget_GeckoEditableSupport_h
#define mozilla_widget_GeckoEditableSupport_h

#include "nsAppShell.h"
#include "nsIWidget.h"
#include "nsTArray.h"

#include "mozilla/TextEventDispatcher.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "mozilla/UniquePtr.h"
#include "nsIInputMethodListener.h"
#include "nsIDOMEventListener.h"
#include "nsIObserver.h"
class nsWindow;
class nsIGlobalObject;
class nsPIDOMWindowOuter;

namespace mozilla {

class TextComposition;

namespace dom {
class BrowserChild;
class Promise;
class nsInputContext;
class EventTarget;
}  // namespace dom

namespace widget {

class GeckoEditableSupport final : public TextEventDispatcherListener,
                                   public nsIEditableSupportListener,
                                   public nsIDOMEventListener,
                                   public nsIObserver {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEDITABLESUPPORTLISTENER
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSIOBSERVER

  static void SetOnBrowserChild(dom::BrowserChild* aBrowserChild,
                                nsPIDOMWindowOuter* aDOMWindow);

  // Constructor for main process
  GeckoEditableSupport(nsPIDOMWindowOuter* aDOMWindow, nsWindow* aWindow);

  // Constructor for content process GeckoEditableSupport.
  explicit GeckoEditableSupport(nsPIDOMWindowOuter* aDOMWindow)
      : GeckoEditableSupport(aDOMWindow, nullptr) {}

  // TextEventDispatcherListener methods
  NS_IMETHOD NotifyIME(TextEventDispatcher* aTextEventDispatcher,
                       const IMENotification& aNotification) override;

  NS_IMETHOD_(IMENotificationRequests) GetIMENotificationRequests() override;

  NS_IMETHOD_(void)
  OnRemovedFrom(TextEventDispatcher* aTextEventDispatcher) override;

  NS_IMETHOD_(void)
  WillDispatchKeyboardEvent(TextEventDispatcher* aTextEventDispatcher,
                            WidgetKeyboardEvent& aKeyboardEvent,
                            uint32_t aIndexOfKeypress, void* aData) override;

  nsresult BeginInputTransaction(TextEventDispatcher* aDispatcher) {
    if (mIsRemote) {
      return aDispatcher->BeginInputTransaction(this);
    } else {
      return aDispatcher->BeginNativeInputTransaction();
    }
  }

  nsresult GetInputContextBag(dom::nsInputContext* aInputContext);

  void HandleFocus();
  void HandleBlur();

 protected:
  virtual ~GeckoEditableSupport();

 private:
  nsWindow* mWindow;
  const bool mIsRemote;
  RefPtr<TextEventDispatcher> mDispatcher;
  nsCOMPtr<dom::EventTarget> mChromeEventHandler;
};

}  // namespace widget
}  // namespace mozilla

#endif  // mozilla_widget_GeckoEditableSupport_h