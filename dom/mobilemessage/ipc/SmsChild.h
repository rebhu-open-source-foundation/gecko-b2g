/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_mobilemessage_SmsChild_h
#define mozilla_dom_mobilemessage_SmsChild_h

#include "mozilla/dom/mobilemessage/PSmsChild.h"
#include "mozilla/dom/mobilemessage/PSmsRequestChild.h"
#include "mozilla/dom/mobilemessage/PMobileMessageCursorChild.h"
#include "nsIMobileMessageCallback.h"
#include "nsIMobileMessageCursorCallback.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

class SmsChild : public PSmsChild {
  friend class PSmsChild;

 public:
  SmsChild() { MOZ_COUNT_CTOR(SmsChild); }

 protected:
  virtual ~SmsChild() { MOZ_COUNT_DTOR(SmsChild); }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  bool RecvNotifyReceivedMessage(const MobileMessageData& aMessage);

  bool RecvNotifyRetrievingMessage(const MobileMessageData& aMessage);

  bool RecvNotifySendingMessage(const MobileMessageData& aMessage);

  bool RecvNotifySentMessage(const MobileMessageData& aMessage);

  bool RecvNotifyFailedMessage(const MobileMessageData& aMessage);

  bool RecvNotifyDeliverySuccessMessage(const MobileMessageData& aMessage);

  bool RecvNotifyDeliveryErrorMessage(const MobileMessageData& aMessage);

  bool RecvNotifyReceivedSilentMessage(const MobileMessageData& aMessage);

  bool RecvNotifyReadSuccessMessage(const MobileMessageData& aMessage);

  bool RecvNotifyReadErrorMessage(const MobileMessageData& aMessage);

  bool RecvNotifyDeletedMessageInfo(const DeletedMessageInfoData& aDeletedInfo);

  PSmsRequestChild* AllocPSmsRequestChild(const IPCSmsRequest& aRequest);

  bool DeallocPSmsRequestChild(PSmsRequestChild* aActor);

  PMobileMessageCursorChild* AllocPMobileMessageCursorChild(
      const IPCMobileMessageCursor& aCursor);

  bool DeallocPMobileMessageCursorChild(
      PMobileMessageCursorChild* aActor);
};

class SmsRequestChild : public PSmsRequestChild {
  friend class SmsChild;
  friend class PSmsRequestChild;

  nsCOMPtr<nsIMobileMessageCallback> mReplyRequest;

 public:
  explicit SmsRequestChild(nsIMobileMessageCallback* aReplyRequest);

 protected:
  ~SmsRequestChild() { MOZ_COUNT_DTOR(SmsRequestChild); }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  mozilla::ipc::IPCResult Recv__delete__(const MessageReply& aReply);
};

class MobileMessageCursorChild : public PMobileMessageCursorChild,
                                 public nsICursorContinueCallback {
  friend class SmsChild;
  friend class PMobileMessageCursorChild;

  nsCOMPtr<nsIMobileMessageCursorCallback> mCursorCallback;

 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICURSORCONTINUECALLBACK

  explicit MobileMessageCursorChild(nsIMobileMessageCursorCallback* aCallback);

 protected:
  ~MobileMessageCursorChild() {}

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  bool RecvNotifyResult(const MobileMessageCursorData& aData);

  mozilla::ipc::IPCResult Recv__delete__(const int32_t& aError);

 private:
  void DoNotifyResult(const nsTArray<MobileMessageData>& aData);

  void DoNotifyResult(const nsTArray<ThreadData>& aData);
};

}  // namespace mobilemessage
}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_mobilemessage_SmsChild_h
