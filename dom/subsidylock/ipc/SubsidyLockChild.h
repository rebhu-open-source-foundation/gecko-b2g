/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_subsidylock_SubsidyLockChild_h
#define mozilla_dom_subsidylock_SubsidyLockChild_h

#include "mozilla/dom/subsidylock/PSubsidyLockChild.h"
#include "mozilla/dom/subsidylock/PSubsidyLockRequestChild.h"
#include "nsISubsidyLockService.h"

namespace mozilla {
namespace dom {
namespace subsidylock {

/**
 * Child actor of PSubsidyLock. The object is created by
 * MSubsidyLockIPCService and destroyed after SubsidyLockIPCService is
 * shutdown. For multi-sim device, more than one instance will
 * be created and each instance represents a sim slot.
 */
class SubsidyLockChild final : public PSubsidyLockChild
                             , public nsISubsidyLock
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISUBSIDYLOCK

  explicit SubsidyLockChild(uint32_t aServiceId);

  void
  Shutdown();

private:
  SubsidyLockChild() = delete;

  // final suppresses -Werror,-Wdelete-non-virtual-dtor
  ~SubsidyLockChild()
  {
    MOZ_COUNT_DTOR(SubsidyLockChild);
  }

protected:
  bool
  SendRequest(const SubsidyLockRequest& aRequest,
              nsISubsidyLockCallback* aCallback);

  virtual void
  ActorDestroy(ActorDestroyReason why) override;

  virtual PSubsidyLockRequestChild*
  AllocPSubsidyLockRequestChild(const SubsidyLockRequest& request) override;

  virtual bool
  DeallocPSubsidyLockRequestChild(PSubsidyLockRequestChild* aActor) override;

private:
  uint32_t mServiceId;
  bool mLive;
};

/******************************************************************************
 * PSubsidyLockRequestChild
 ******************************************************************************/

/**
 * Child actor of PSubsidyLockRequest. The object is created when an
 * asynchronous request is made and destroyed after receiving the response sent
 * by parent actor.
 */
class SubsidyLockRequestChild : public PSubsidyLockRequestChild
{
public:
  explicit SubsidyLockRequestChild(nsISubsidyLockCallback* aRequestCallback)
    : mRequestCallback(aRequestCallback)
  {
    MOZ_COUNT_CTOR(SubsidyLockRequestChild);
    MOZ_ASSERT(mRequestCallback);
  }

  bool
  DoReply(const SubsidyLockGetStatusSuccess& aReply);

  bool
  DoReply(const SubsidyLockReplySuccess& aReply);

  bool
  DoReply(const SubsidyLockReplyError& aReply);

  bool
  DoReply(const SubsidyLockUnlockError& aReply);

protected:
  virtual
  ~SubsidyLockRequestChild()
  {
    MOZ_COUNT_DTOR(SubsidyLockRequestChild);
  }

  virtual void
  ActorDestroy(ActorDestroyReason why) override;

  virtual bool
  Recv__delete__(const SubsidyLockReply& aReply) override;

private:
  nsCOMPtr<nsISubsidyLockCallback> mRequestCallback;
};

} // namespace subsidylock
} // namespace dom
} // namespace mozilla


#endif // mozilla_dom_subsidylock_SubsidyLockChild_h
