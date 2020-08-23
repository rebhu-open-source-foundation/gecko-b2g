/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_InputMethod_h
#define mozilla_dom_InputMethod_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/InputMethodBinding.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "nsIInputMethodProxy.h"


class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;

class InputMethodSetCompositionCallback final : public nsIInputMethodSetCompositionCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODSETCOMPOSITIONCALLBACK
  explicit InputMethodSetCompositionCallback(Promise* aPromise);

 protected:
  ~InputMethodSetCompositionCallback();

 private:
  RefPtr<Promise> mPromise;
};

class InputMethodEndCompositionCallback final : public nsIInputMethodEndCompositionCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODENDCOMPOSITIONCALLBACK
  explicit InputMethodEndCompositionCallback(Promise* aPromise);

 protected:
  ~InputMethodEndCompositionCallback();

 private:
  RefPtr<Promise> mPromise;
};

class InputMethodSendKeyCallback final : public nsIInputMethodSendKeyCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODSENDKEYCALLBACK
  explicit InputMethodSendKeyCallback(Promise* aPromise);

 protected:
  ~InputMethodSendKeyCallback();

 private:
  RefPtr<Promise> mPromise;
};

class InputMethodKeydownCallback final : public nsIInputMethodKeydownCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODKEYDOWNCALLBACK
  explicit InputMethodKeydownCallback(Promise* aPromise);

 protected:
  ~InputMethodKeydownCallback();

 private:
  RefPtr<Promise> mPromise;
};

class InputMethodKeyupCallback final : public nsIInputMethodKeyupCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINPUTMETHODKEYUPCALLBACK
  explicit InputMethodKeyupCallback(Promise* aPromise);

 protected:
  ~InputMethodKeyupCallback();

 private:
  RefPtr<Promise> mPromise;
};

class InputMethod final : public nsISupports,
                          public nsWrapperCache
{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(InputMethod)

  nsIGlobalObject* GetParentObject() const { return mGlobal; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  explicit InputMethod(nsIGlobalObject* aGlobal);

  // WebIDL methods
  already_AddRefed<Promise> SetComposition(const nsAString& aText);
  already_AddRefed<Promise> EndComposition(const Optional<nsAString>& aText);

  already_AddRefed<Promise> SendKey(const nsAString& aKey);
  already_AddRefed<Promise> Keydown(const nsAString& aKey);
  already_AddRefed<Promise> Keyup(const nsAString& aKey);

 protected:
  ~InputMethod();

  nsresult PermissionCheck();

  nsCOMPtr<nsIGlobalObject> mGlobal;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_InputMethod_h
