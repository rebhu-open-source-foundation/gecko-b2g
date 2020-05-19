/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_TestInterfaceAsyncIterableDouble_h
#define mozilla_dom_TestInterfaceAsyncIterableDouble_h

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

class nsPIDOMWindowInner;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;

// Implementation of test binding for webidl iterable interfaces, using
// primitives for value type
class TestInterfaceAsyncIterableDouble final : public nsISupports,
                                               public nsWrapperCache {
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestInterfaceAsyncIterableDouble)

  explicit TestInterfaceAsyncIterableDouble(nsPIDOMWindowInner* aParent);
  nsPIDOMWindowInner* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  static already_AddRefed<TestInterfaceAsyncIterableDouble> Constructor(
      const GlobalObject& aGlobal, ErrorResult& rv);

  using itrType = AsyncIterableIterator<TestInterfaceAsyncIterableDouble>;
  void InitAsyncIterator(itrType* aIterator);
  void DestroyAsyncIterator(itrType* aIterator);
  already_AddRefed<Promise> GetNextPromise(JSContext* aCx, itrType* aIterator,
                                           ErrorResult& aRv);

 private:
  virtual ~TestInterfaceAsyncIterableDouble() = default;
  nsCOMPtr<nsPIDOMWindowInner> mParent;
  nsTArray<std::pair<nsString, nsString>> mValues;
  struct IteratorData {
    IteratorData(int32_t aIndex) : mIndex(aIndex) {}
    WeakPtr<Promise> mPromise;
    uint32_t mIndex;
  };
  nsTArray<WeakPtr<itrType>> mIterators;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_TestInterfaceAsyncIterableDouble_h
