/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_SystemMessageManagerWorker_h
#define mozilla_dom_SystemMessageManagerWorker_h

#include "mozilla/dom/SystemMessageManager.h"

namespace mozilla {
namespace dom {

class Promise;

class SystemMessageManagerWorker final : public SystemMessageManagerImpl {
 public:
  NS_INLINE_DECL_REFCOUNTING(SystemMessageManagerWorker, override)

  explicit SystemMessageManagerWorker();
  virtual already_AddRefed<Promise> Subscribe(const nsAString& aMessageName,
                                              ErrorResult& aRv) override;
  virtual void AddOuter(SystemMessageManager* aOuter) override;
  virtual void RemoveOuter(SystemMessageManager* aOuter) override;

 private:
  ~SystemMessageManagerWorker();
  SystemMessageManager* mOuter;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_SystemMessageManagerWorker_h
