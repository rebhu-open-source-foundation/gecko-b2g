/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/TestInterfaceAsyncIterableSingle.h"
#include "mozilla/dom/TestInterfaceJSMaplikeSetlikeIterableBinding.h"
#include "nsPIDOMWindow.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/IterableIterator.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(TestInterfaceAsyncIterableSingle, mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TestInterfaceAsyncIterableSingle)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TestInterfaceAsyncIterableSingle)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TestInterfaceAsyncIterableSingle)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

TestInterfaceAsyncIterableSingle::TestInterfaceAsyncIterableSingle(
    nsPIDOMWindowInner* aParent)
    : mParent(aParent) {}

// static
already_AddRefed<TestInterfaceAsyncIterableSingle>
TestInterfaceAsyncIterableSingle::Constructor(const GlobalObject& aGlobal,
                                              ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindowInner> window =
      do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<TestInterfaceAsyncIterableSingle> r =
      new TestInterfaceAsyncIterableSingle(window);
  return r.forget();
}

JSObject* TestInterfaceAsyncIterableSingle::WrapObject(
    JSContext* aCx, JS::Handle<JSObject*> aGivenProto) {
  return TestInterfaceAsyncIterableSingle_Binding::Wrap(aCx, this, aGivenProto);
}

nsPIDOMWindowInner* TestInterfaceAsyncIterableSingle::GetParentObject() const {
  return mParent;
}

void TestInterfaceAsyncIterableSingle::InitAsyncIterator(itrType* aIterator) {
  *mIterators.AppendElement() = aIterator;
  UniquePtr<IteratorData> data(new IteratorData(0));
  aIterator->SetData((void*)data.release());
}

void TestInterfaceAsyncIterableSingle::DestroyAsyncIterator(
    itrType* aIterator) {
  mIterators.RemoveElement(aIterator);
  auto data = reinterpret_cast<IteratorData*>(aIterator->GetData());
  delete data;
}

already_AddRefed<Promise> TestInterfaceAsyncIterableSingle::GetNextPromise(
    JSContext* aCx, itrType* aIterator, ErrorResult& aRv) {
  RefPtr<Promise> promise = Promise::Create(mParent->AsGlobal(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  auto data = reinterpret_cast<IteratorData*>(aIterator->GetData());
  data->mPromise = promise;

  // In real world, promise might resolved asynchronously, especially when IPC
  // is involved, we shall be able to resolve the promise later since it is
  // stored in mIterators.
  if (data->mIndex >= 10) {
    IteratorUtils::ResolvePromiseWithUndefined(aCx, data->mPromise, aRv);
  } else {
    JS::Rooted<JS::Value> value(aCx);
    Unused << ToJSValue(aCx, (int32_t)(data->mIndex * 9 % 7), &value);
    IteratorUtils::ResolvePromiseWithKeyOrValue(aCx, data->mPromise, value,
                                                aRv);

    data->mIndex++;
  }

  return promise.forget();
}

}  // namespace dom
}  // namespace mozilla
