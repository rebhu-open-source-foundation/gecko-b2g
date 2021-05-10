/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_AlarmManagerWorker_h
#define mozilla_dom_AlarmManagerWorker_h

#include "mozilla/dom/AlarmManager.h"

namespace mozilla {
namespace dom {

class AlarmManagerWorker final : public AlarmManagerImpl {
 public:
  NS_INLINE_DECL_REFCOUNTING(AlarmManagerWorker, override)
  AlarmManagerWorker() = default;
  virtual nsresult Init(nsIGlobalObject* aGlobal) override;

  virtual void GetAll(Promise* aPromise) override;
  virtual void Add(Promise* aPromise, JSContext* aCx,
                   const AlarmOptions& aOptions) override;
  virtual void Remove(long aId) override;

 private:
  ~AlarmManagerWorker() = default;
};

}  // namespace dom
}  // namespace mozilla

#endif
