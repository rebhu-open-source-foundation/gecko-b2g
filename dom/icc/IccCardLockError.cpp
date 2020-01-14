/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/IccCardLockError.h"
#include "mozilla/dom/IccCardLockErrorBinding.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

//NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(IccCardLockError, mWindow)
//NS_IMPL_CYCLE_COLLECTING_ADDREF(IccCardLockError)
//NS_IMPL_CYCLE_COLLECTING_RELEASE(IccCardLockError)

//NS_IMPL_ISUPPORTS_INHERITED0(IccCardLockError, DOMException)

/* static */ already_AddRefed<IccCardLockError>
IccCardLockError::Constructor(const GlobalObject& aGlobal,
                              const nsAString& aName,
                              int16_t aRetryCount)
{
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
   // aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<IccCardLockError> result =
    new IccCardLockError(window, aName, aRetryCount);
  return result.forget();
}

IccCardLockError::IccCardLockError(nsPIDOMWindowInner* aWindow,
                                   const nsAString& aName,
                                   int16_t aRetryCount)
  : mWindow(aWindow)
  , mName(aName)
  , mRetryCount(aRetryCount)
{

}

JSObject*
IccCardLockError::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return IccCardLockError_Binding::Wrap(aCx, this, aGivenProto);
}

IccCardLockError::~IccCardLockError()
{
}

} // namespace dom
} // namespace mozilla
