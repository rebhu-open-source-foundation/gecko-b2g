/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_AudioChannelHandler_h
#define mozilla_dom_AudioChannelHandler_h

#include "mozilla/dom/AudioChannelBinding.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/ipc/MessageChannel.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIObserver.h"
#include "nsFrameLoader.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"

class nsIBrowserParent;
class nsPIDOMWindowOuter;

namespace mozilla {
namespace dom {

class Promise;

class AudioChannelHandler final : public DOMEventTargetHelper,
                                  public nsSupportsWeakReference,
                                  public nsIObserver {
 public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioChannelHandler,
                                           DOMEventTargetHelper)

  nsPIDOMWindowInner* GetParentObject() const { return GetOwner(); }

  // C++ helper
  static void GenerateAllowedChannels(
      nsPIDOMWindowInner* aParentWindow,
      nsTArray<RefPtr<AudioChannelHandler>>& aAudioChannels, ErrorResult& aRv) {
    GenerateAllowedChannelsInternal(aParentWindow, nullptr, aAudioChannels,
                                    aRv);
  }

  // WebIDL methods
  static void GenerateAllowedChannels(
      const GlobalObject& aGlobal, nsFrameLoader& aFrameLoader,
      nsTArray<RefPtr<AudioChannelHandler>>& aAudioChannels, ErrorResult& aRv) {
    GenerateAllowedChannelsInternal(nullptr, &aFrameLoader, aAudioChannels,
                                    aRv);
  };

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  AudioChannel Name() const;

  already_AddRefed<Promise> GetVolume(ErrorResult& aRv);
  already_AddRefed<Promise> SetVolume(float aVolume, ErrorResult& aRv);

  // In Gecko48, system app expects a muted channel to be paused, which now is
  // called "suspended" internally in the new Gecko. So just wrap GetSuspend/
  // SetSuspend into these two APIs.
  already_AddRefed<Promise> GetMuted(ErrorResult& aRv) {
    return GetSuspended(aRv);
  }
  already_AddRefed<Promise> SetMuted(bool aMuted, ErrorResult& aRv) {
    return SetSuspended(aMuted, aRv);
  }

  already_AddRefed<Promise> GetSuspended(ErrorResult& aRv);
  already_AddRefed<Promise> SetSuspended(bool aSuspended, ErrorResult& aRv);

  already_AddRefed<Promise> IsActive(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(activestatechanged);

 private:
  static already_AddRefed<AudioChannelHandler> Create(
      nsPIDOMWindowInner* aParentWindow, nsFrameLoader* aFrameLoader,
      AudioChannel aAudioChannel, ErrorResult& aRv);

  static void GenerateAllowedChannelsInternal(
      nsPIDOMWindowInner* aParentWindow, nsFrameLoader* aFrameLoader,
      nsTArray<RefPtr<AudioChannelHandler>>& aAudioChannels, ErrorResult& aRv);

  AudioChannelHandler(nsPIDOMWindowInner* aParentWindow,
                      nsFrameLoader* aFrameLoader, AudioChannel aAudioChannel);

  ~AudioChannelHandler();

  nsresult Initialize();

  void ProcessStateChanged(const char16_t* aData);

  bool IsRemoteFrame() { return !mFrameWindow; }

  RefPtr<nsFrameLoader> mFrameLoader;
  RefPtr<BrowserParent> mBrowserParent;
  nsCOMPtr<nsPIDOMWindowOuter> mFrameWindow;
  AudioChannel mAudioChannel;

  enum { eStateActive, eStateInactive, eStateUnknown } mState;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_AudioChannelHandler_h
