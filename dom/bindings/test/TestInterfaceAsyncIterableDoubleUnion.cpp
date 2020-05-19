/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/TestInterfaceAsyncIterableDoubleUnion.h"
#include "mozilla/dom/TestInterfaceJSMaplikeSetlikeIterableBinding.h"
#include "nsPIDOMWindow.h"
#include "mozilla/dom/BindingUtils.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(TestInterfaceAsyncIterableDoubleUnion,
                                      mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TestInterfaceAsyncIterableDoubleUnion)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TestInterfaceAsyncIterableDoubleUnion)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TestInterfaceAsyncIterableDoubleUnion)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

TestInterfaceAsyncIterableDoubleUnion::TestInterfaceAsyncIterableDoubleUnion(
    nsPIDOMWindowInner* aParent)
    : mParent(aParent) {
  OwningStringOrLong a;
  a.SetAsLong() = 1;
  mValues.AppendElement(
      std::pair<nsString, OwningStringOrLong>(NS_LITERAL_STRING("long"), a));
  a.SetAsString() = NS_LITERAL_STRING("a");
  mValues.AppendElement(
      std::pair<nsString, OwningStringOrLong>(NS_LITERAL_STRING("string"), a));
}

// static
already_AddRefed<TestInterfaceAsyncIterableDoubleUnion>
TestInterfaceAsyncIterableDoubleUnion::Constructor(const GlobalObject& aGlobal,
                                                   ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindowInner> window =
      do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<TestInterfaceAsyncIterableDoubleUnion> r =
      new TestInterfaceAsyncIterableDoubleUnion(window);
  return r.forget();
}

JSObject* TestInterfaceAsyncIterableDoubleUnion::WrapObject(
    JSContext* aCx, JS::Handle<JSObject*> aGivenProto) {
  return TestInterfaceAsyncIterableDoubleUnion_Binding::Wrap(aCx, this,
                                                             aGivenProto);
}

nsPIDOMWindowInner* TestInterfaceAsyncIterableDoubleUnion::GetParentObject()
    const {
  return mParent;
}

void TestInterfaceAsyncIterableDoubleUnion::InitAsyncIterator(
    itrType* aIterator) {
  *mIterators.AppendElement() = aIterator;
  UniquePtr<IteratorData> data(new IteratorData(0));
  aIterator->SetData((void*)data.release());
}

void TestInterfaceAsyncIterableDoubleUnion::DestroyAsyncIterator(
    itrType* aIterator) {
  mIterators.RemoveElement(aIterator);
  auto data = reinterpret_cast<IteratorData*>(aIterator->GetData());
  delete data;
}

already_AddRefed<Promise> TestInterfaceAsyncIterableDoubleUnion::GetNextPromise(
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

  // Test data:
  // [long, 1], [string, "a"]
  uint32_t idx = data->mIndex;
  if (idx >= mValues.Length()) {
    IteratorUtils::ResolvePromiseWithUndefined(aCx, data->mPromise, aRv);
  } else {
    JS::Rooted<JS::Value> key(aCx);
    JS::Rooted<JS::Value> value(aCx);
    switch (aIterator->GetIteratorType()) {
      case IterableIteratorBase::IteratorType::Keys:
        Unused << ToJSValue(aCx, mValues[idx].first, &key);
        IteratorUtils::ResolvePromiseWithKeyOrValue(aCx, data->mPromise, key,
                                                    aRv);
        break;
      case IterableIteratorBase::IteratorType::Values:
        Unused << ToJSValue(aCx, mValues[idx].second, &value);
        IteratorUtils::ResolvePromiseWithKeyOrValue(aCx, data->mPromise, value,
                                                    aRv);
        break;
      case IterableIteratorBase::IteratorType::Entries:
        Unused << ToJSValue(aCx, mValues[idx].first, &key);
        Unused << ToJSValue(aCx, mValues[idx].second, &value);
        IteratorUtils::ResolvePromiseWithKeyAndValue(aCx, data->mPromise, key,
                                                     value, aRv);
        break;
    }

    data->mIndex++;
  }

  return promise.forget();
}

}  // namespace dom
}  // namespace mozilla
