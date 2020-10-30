/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_AlarmManager_h
#define mozilla_dom_AlarmManager_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AlarmManagerBinding.h"
#include "mozilla/dom/Promise.h"
#include "nsIAlarmProxy.h"
#include "nsWrapperCache.h"

/*
  - AlarmManager:
    The AlarmManager webidl implementation.
  - AlarmManagerImpl:
    The abstract class for AlarmManager to handle requests from both main
    threads and worker threads.
  - AlarmManagerMain:
    The AlarmManagerImpl implementation on main threads, creates an AlarmProxy
    instance by itself, and uses the AlarmProxy to send requests to
    AlarmService.
  - AlarmManagerWorker:
    The AlarmManagerImpl implementation on worker threads, cannot create
    AlarmProxy instances by itself, so dispatches all requests to main thread
    by runnables.
*/

namespace mozilla {
namespace dom {

class AlarmManagerImpl {
 public:
  explicit AlarmManagerImpl(nsCString aUrl, nsIGlobalObject* aOuterGlobal)
      : mUrl(aUrl), mOuterGlobal(aOuterGlobal) {}
  virtual already_AddRefed<Promise> GetAll() = 0;
  virtual already_AddRefed<Promise> Add(JSContext* aCx,
                                        const AlarmOptions& aOptions) = 0;
  virtual void Remove(long aId) = 0;

  NS_INLINE_DECL_PURE_VIRTUAL_REFCOUNTING

 protected:
  nsCString mUrl;
  nsCOMPtr<nsIGlobalObject> mOuterGlobal;
};

class AlarmManagerMain final : public AlarmManagerImpl {
 public:
  NS_INLINE_DECL_REFCOUNTING(AlarmManagerMain, override)
  explicit AlarmManagerMain(nsCString aUrl, nsIGlobalObject* aOuterGlobal);
  virtual already_AddRefed<Promise> GetAll() override;
  virtual already_AddRefed<Promise> Add(JSContext* aCx,
                                        const AlarmOptions& aOptions) override;
  virtual void Remove(long aId) override;

 private:
  ~AlarmManagerMain() = default;
};

class AlarmManager final : public nsISupports,
                           public nsWrapperCache

{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AlarmManager)

  static already_AddRefed<AlarmManager> Create(nsIGlobalObject* aGlobal,
                                               nsresult& aRv);

  nsIGlobalObject* GetParentObject() const { return mGlobal; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  explicit AlarmManager(nsIGlobalObject* aGlobal);
  NS_IMETHODIMP Init();

  already_AddRefed<Promise> GetAll();
  already_AddRefed<Promise> Add(JSContext* aCx, const AlarmOptions& options);
  void Remove(long aId);

 protected:
  ~AlarmManager() = default;
  nsresult PermissionCheck();
  nsCOMPtr<nsIGlobalObject> mGlobal;
  RefPtr<AlarmManagerImpl> mImpl;
  nsCString mUrl;
};

namespace alarm {
already_AddRefed<nsIAlarmProxy> CreateAlarmProxy();
}  // namespace alarm
}  // namespace dom
}  // namespace mozilla

#endif
