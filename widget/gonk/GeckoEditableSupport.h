/* -*- Mode: c++; c-basic-offset: 2; tab-width: 20; indent-tabs-mode: nil; -*- */
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
class nsWindow;
class nsIGlobalObject;

namespace mozilla {

class TextComposition;

namespace dom {
class BrowserChild;
class Promise;
}  // namespace dom

namespace widget {

class GeckoEditableSupport final : public TextEventDispatcherListener,
                                   public nsIEditableSupportListener {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEDITABLESUPPORTLISTENER

  static void SetOnBrowserChild(dom::BrowserChild* aBrowserChild,
                                nsIGlobalObject* aGlobal);

  // Constructor for main process
  GeckoEditableSupport(nsIGlobalObject* aGlobal, nsWindow* aWindow);

  // Constructor for content process GeckoEditableSupport.
  explicit GeckoEditableSupport(nsIGlobalObject* aGlobal)
      : GeckoEditableSupport(aGlobal, nullptr) {}

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

 protected:
  virtual ~GeckoEditableSupport() {}

 private:
  nsWindow* mWindow;
  const bool mIsRemote;
  RefPtr<TextEventDispatcher> mDispatcher;
  nsCOMPtr<nsIGlobalObject> mGlobal;
};

}  // namespace widget
}  // namespace mozilla

#endif  // mozilla_widget_GeckoEditableSupport_h