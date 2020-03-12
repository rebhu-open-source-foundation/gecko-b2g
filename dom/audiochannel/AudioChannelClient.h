/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_AudioChannelClient_h
#define mozilla_dom_AudioChannelClient_h

#include "AudioChannelAgent.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/AudioChannelClientBinding.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace dom {

class AudioChannelClient final : public DOMEventTargetHelper,
                                 public nsSupportsWeakReference,
                                 public nsIAudioChannelAgentCallback {
 public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioChannelClient,
                                           DOMEventTargetHelper)
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  static already_AddRefed<AudioChannelClient> Constructor(
      const GlobalObject& aGlobal, AudioChannel aChannel, ErrorResult& aRv);

  nsPIDOMWindowInner* GetParentObject() const { return GetOwner(); }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  void RequestChannel(ErrorResult& aRv);

  void AbandonChannel(ErrorResult& aRv);

  // Due to history reasons, ChannelMuted actually means "channel suspended".
  bool ChannelMuted() const { return mSuspended; }

  IMPL_EVENT_HANDLER(statechange)

 private:
  AudioChannelClient(nsPIDOMWindowInner* aWindow, AudioChannel aChannel);
  ~AudioChannelClient();

  static bool CheckAudioChannelPermissions(nsPIDOMWindowInner* aWindow,
                                           AudioChannel aChannel);

  RefPtr<AudioChannelAgent> mAgent;
  AudioChannel mChannel;
  bool mSuspended;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_AudioChannelClient_h
