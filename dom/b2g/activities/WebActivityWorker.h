/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_WebActivityWorker_h
#define mozilla_dom_WebActivityWorker_h

#include "mozilla/dom/WebActivity.h"

namespace mozilla {
namespace dom {

class Promise;
class PromiseWorkerProxy;

class ActivityStartWorkerCallback final : public nsIActivityStartCallback {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIACTIVITYSTARTCALLBACK
  explicit ActivityStartWorkerCallback(PromiseWorkerProxy* aPromiseWorkerProxy);

 protected:
  ~ActivityStartWorkerCallback();

 private:
  RefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
};

class WebActivityWorker final : public WebActivityImpl {
 public:
  NS_INLINE_DECL_REFCOUNTING(WebActivityWorker, override)

  explicit WebActivityWorker();
  virtual void Start(Promise* aPromise) override;
  virtual void Cancel() override;
  virtual nsresult PermissionCheck() override;
  virtual nsresult Initialize(const GlobalObject& aOwner,
                              const WebActivityOptions& aOptions) override;
  virtual void AddOuter(WebActivity* aOuter) override;
  virtual void RemoveOuter(WebActivity* aOuter) override;

 private:
  ~WebActivityWorker();
  WebActivity* mOuter;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_WebActivityWorker_h
