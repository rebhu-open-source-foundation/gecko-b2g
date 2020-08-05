/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/InputMethod.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Document.h"
#include "mozilla/dom/Promise.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"

#ifdef MOZ_WIDGET_GONK
#undef LOG
#include <android/log.h>
#define LOG(msg, ...)  __android_log_print(ANDROID_LOG_WARN,     \
                                               "IME",  \
                                               msg,                  \
                                               ##__VA_ARGS__)
#else
#  define LOG(...)
#endif

using namespace mozilla::widget;

namespace mozilla {
namespace dom {

static nsCOMPtr<nsIInputMethodProxy> sInputMethodProxy;

already_AddRefed<nsIInputMethodProxy>
GetOrCreateInputMethodProxy(nsISupports* aSupports) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sInputMethodProxy) {
    sInputMethodProxy = do_CreateInstance("@mozilla.org/dom/inputmethod/proxy;1");
    LOG("sInputMethodProxy created:[%p]", sInputMethodProxy.get());
    MOZ_ASSERT(sInputMethodProxy);
    sInputMethodProxy->Init();
  }

  nsCOMPtr<nsIInputMethodProxy> proxy = sInputMethodProxy;
  proxy->Attach(aSupports);
  return proxy.forget();
}

NS_IMPL_ISUPPORTS(InputMethodSetCompositionCallback, nsIInputMethodSetCompositionCallback)

InputMethodSetCompositionCallback::InputMethodSetCompositionCallback(Promise* aPromise)
    : mPromise(aPromise) {}

InputMethodSetCompositionCallback::~InputMethodSetCompositionCallback() {}

NS_IMETHODIMP
InputMethodSetCompositionCallback::OnSetComposition(nsresult aStatus,
    JS::HandleValue aResult, JSContext* aCx) {
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      LOG("-- InputMethod OnSetComposition: Resolve");
      mPromise->MaybeResolve(aResult);
    } else {
      LOG("-- InputMethod OnSetComposition: Reject");
      mPromise->MaybeReject(aResult);
    }
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(InputMethodEndCompositionCallback, nsIInputMethodEndCompositionCallback)

InputMethodEndCompositionCallback::InputMethodEndCompositionCallback(Promise* aPromise)
    : mPromise(aPromise) {}

InputMethodEndCompositionCallback::~InputMethodEndCompositionCallback() {}

NS_IMETHODIMP
InputMethodEndCompositionCallback::OnEndComposition(nsresult aStatus,
    JS::HandleValue aResult, JSContext* aCx) {
  if (mPromise) {
    if (NS_SUCCEEDED(aStatus)) {
      LOG("-- InputMethod OnEndComposition: Resolve");
      mPromise->MaybeResolve(aResult);
    } else {
      LOG("-- InputMethod OnEndComposition: Reject");
      mPromise->MaybeReject(aResult);
    }
  }
  return NS_OK;
}

InputMethod::InputMethod(nsIGlobalObject* aGlobal)
 : mGlobal(aGlobal){
  MOZ_ASSERT(aGlobal);
}

InputMethod::~InputMethod() {}

nsresult InputMethod::PermissionCheck()
{
  // TODO file bug 103458 to track
  return NS_OK; 
}

already_AddRefed<Promise> InputMethod::SetComposition(
    const nsAString& aText) {

  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  LOG("-- InputMethod::SetComposition");
  RefPtr<nsIInputMethodSetCompositionCallback> callback = 
      new InputMethodSetCompositionCallback(promise);
  nsCOMPtr<nsIInputMethodProxy> proxy = GetOrCreateInputMethodProxy(mGlobal);
  proxy->SetComposition(mGlobal, aText, callback);

  return promise.forget();
}

already_AddRefed<Promise> InputMethod::EndComposition(
    const Optional<nsAString>& aText) {

  RefPtr<Promise> promise;
  ErrorResult rv;
  promise = Promise::Create(mGlobal, rv);
  ENSURE_SUCCESS(rv, nullptr);

  LOG("-- InputMethod::EndComposition");
  RefPtr<nsIInputMethodEndCompositionCallback> callback = 
      new InputMethodEndCompositionCallback(promise);

  nsCOMPtr<nsIInputMethodProxy> proxy = GetOrCreateInputMethodProxy(mGlobal);
  proxy->EndComposition(mGlobal,
                        aText.WasPassed() ? aText.Value() : VoidString(),
                        callback);

  return promise.forget();
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
