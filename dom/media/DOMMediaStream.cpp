/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMMediaStream.h"

#include "AudioCaptureTrack.h"
#include "AudioChannelAgent.h"
#include "AudioStreamTrack.h"
#include "Layers.h"
#include "MediaTrackGraph.h"
#include "MediaTrackGraphImpl.h"
#include "MediaTrackListener.h"
#include "VideoStreamTrack.h"
#include "mozilla/dom/AudioTrack.h"
#include "mozilla/dom/AudioTrackList.h"
#include "mozilla/dom/DocGroup.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/MediaStreamTrackEvent.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/VideoTrack.h"
#include "mozilla/dom/VideoTrackList.h"
#include "mozilla/media/MediaUtils.h"
#include "nsContentUtils.h"
#include "nsGlobalWindowInner.h"
#include "nsIScriptError.h"
#include "nsIUUIDGenerator.h"
#include "nsPIDOMWindow.h"
#include "nsProxyRelease.h"
#include "nsRFPService.h"
#include "nsServiceManagerUtils.h"

#ifdef LOG
#  undef LOG
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace mozilla::media;

static LazyLogModule gMediaStreamLog("MediaStream");
#define LOG(type, msg) MOZ_LOG(gMediaStreamLog, type, msg)

const TrackID TRACK_VIDEO_PRIMARY = 1;


DOMMediaStream::TrackPort::TrackPort(MediaInputPort* aInputPort,
                                     MediaStreamTrack* aTrack,
                                     const InputPortOwnership aOwnership)
  : mInputPort(aInputPort)
  , mTrack(aTrack)
  , mOwnership(aOwnership)
{
  // XXX Bug 1124630. nsDOMCameraControl requires adding a track without and
  // input port.
  // MOZ_ASSERT(mInputPort);
  MOZ_ASSERT(mTrack);

  MOZ_COUNT_CTOR(TrackPort);
}

DOMMediaStream::TrackPort::~TrackPort()
{
  MOZ_COUNT_DTOR(TrackPort);

  if (mOwnership == InputPortOwnership::OWNED) {
    DestroyInputPort();
  }
}

void
DOMMediaStream::TrackPort::DestroyInputPort()
{
  if (mInputPort) {
    mInputPort->Destroy();
    mInputPort = nullptr;
  }
}

MediaStream*
DOMMediaStream::TrackPort::GetSource() const
{
  return mInputPort ? mInputPort->GetSource() : nullptr;
}

TrackID
DOMMediaStream::TrackPort::GetSourceTrackId() const
{
  return mInputPort ? mInputPort->GetSourceTrackId() : TRACK_INVALID;
}

already_AddRefed<Pledge<bool>>
DOMMediaStream::TrackPort::BlockSourceTrackId(TrackID aTrackId, BlockingMode aBlockingMode)
{
  if (mInputPort) {
    return mInputPort->BlockSourceTrackId(aTrackId, aBlockingMode);
  }
  RefPtr<Pledge<bool>> rejected = new Pledge<bool>();
  rejected->Reject(NS_ERROR_FAILURE);
  return rejected.forget();
}

NS_IMPL_CYCLE_COLLECTION(DOMMediaStream::TrackPort, mTrack)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(DOMMediaStream::TrackPort, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(DOMMediaStream::TrackPort, Release)

NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaStreamTrackSourceGetter)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaStreamTrackSourceGetter)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MediaStreamTrackSourceGetter)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
NS_IMPL_CYCLE_COLLECTION_0(MediaStreamTrackSourceGetter)

/**
 * Listener registered on the Owned stream to detect added and ended owned
 * tracks for keeping the list of MediaStreamTracks in sync with the tracks
 * added and ended directly at the source.
 */
class DOMMediaStream::OwnedStreamListener : public MediaStreamListener {
public:
  explicit OwnedStreamListener(DOMMediaStream* aStream)
    : mStream(aStream)
  {}

  void Forget() { mStream = nullptr; }

  void DoNotifyTrackCreated(TrackID aTrackID, MediaSegment::Type aType,
                            MediaStream* aInputStream, TrackID aInputTrackID)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!mStream) {
      return;
    }
  }

  return false;
}

static bool ContainsLiveTracks(
    const nsTArray<RefPtr<MediaStreamTrack>>& aTracks) {
  for (const auto& track : aTracks) {
    if (track->ReadyState() == MediaStreamTrackState::Live) {
      return true;
    }
  }

  return false;
}


static bool ContainsLiveAudioTracks(
    const nsTArray<RefPtr<MediaStreamTrack>>& aTracks) {
  for (const auto& track : aTracks) {
    if (track->AsAudioStreamTrack() &&
        track->ReadyState() == MediaStreamTrackState::Live) {
      return true;
    }
  }

  return false;
}

class DOMMediaStream::PlaybackTrackListener : public MediaStreamTrackConsumer {
 public:
  explicit PlaybackTrackListener(DOMMediaStream* aStream) : mStream(aStream) {}

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PlaybackTrackListener)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(PlaybackTrackListener)

  void NotifyEnded(MediaStreamTrack* aTrack) override {
    if (!mStream) {
      MOZ_ASSERT(false);
      return;
    }

    if (!aTrack) {
      MOZ_ASSERT(false);
      return;
    }

    MOZ_ASSERT(mStream->HasTrack(*aTrack));
    mStream->NotifyTrackRemoved(aTrack);
  }

 protected:
  virtual ~PlaybackTrackListener() {}

  RefPtr<DOMMediaStream> mStream;
};

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(DOMMediaStream::PlaybackTrackListener,
                                     AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(DOMMediaStream::PlaybackTrackListener,
                                       Release)
NS_IMPL_CYCLE_COLLECTION(DOMMediaStream::PlaybackTrackListener, mStream)

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMMediaStream)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(DOMMediaStream,
                                                DOMEventTargetHelper)
  tmp->Destroy();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mTracks)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mConsumersToKeepAlive)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPlaybackTrackListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(DOMMediaStream,
                                                  DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTracks)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mConsumersToKeepAlive)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlaybackTrackListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(DOMMediaStream, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(DOMMediaStream, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMMediaStream)
  NS_INTERFACE_MAP_ENTRY(DOMMediaStream)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

DOMMediaStream::DOMMediaStream(nsPIDOMWindowInner* aWindow)
    : mWindow(aWindow),
      mPlaybackTrackListener(MakeAndAddRef<PlaybackTrackListener>(this)) {
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1", &rv);

  if (NS_SUCCEEDED(rv) && uuidgen) {
    nsID uuid;
    memset(&uuid, 0, sizeof(uuid));
    rv = uuidgen->GenerateUUIDInPlace(&uuid);
    if (NS_SUCCEEDED(rv)) {
      char buffer[NSID_LENGTH];
      uuid.ToProvidedString(buffer);
      mID = NS_ConvertASCIItoUTF16(buffer);
    }
  }
}

DOMMediaStream::~DOMMediaStream() { Destroy(); }

void DOMMediaStream::Destroy() {
  LOG(LogLevel::Debug, ("DOMMediaStream %p Being destroyed.", this));
  for (const auto& track : mTracks) {
    // We must remove ourselves from each track's principal change observer list
    // before we die.
    if (!track->Ended()) {
      track->RemoveConsumer(mPlaybackTrackListener);
    }
  }
  mTrackListeners.Clear();
}

JSObject* DOMMediaStream::WrapObject(JSContext* aCx,
                                     JS::Handle<JSObject*> aGivenProto) {
  return dom::MediaStream_Binding::Wrap(aCx, this, aGivenProto);
}

/* static */
already_AddRefed<DOMMediaStream> DOMMediaStream::Constructor(
    const GlobalObject& aGlobal, ErrorResult& aRv) {
  Sequence<OwningNonNull<MediaStreamTrack>> emptyTrackSeq;
  return Constructor(aGlobal, emptyTrackSeq, aRv);
}

/* static */
already_AddRefed<DOMMediaStream> DOMMediaStream::Constructor(
    const GlobalObject& aGlobal, const DOMMediaStream& aStream,
    ErrorResult& aRv) {
  nsTArray<RefPtr<MediaStreamTrack>> tracks;
  aStream.GetTracks(tracks);

  Sequence<OwningNonNull<MediaStreamTrack>> nonNullTrackSeq;
  if (!nonNullTrackSeq.SetLength(tracks.Length(), fallible)) {
    MOZ_ASSERT(false);
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  for (size_t i = 0; i < tracks.Length(); ++i) {
    nonNullTrackSeq[i] = tracks[i];
  }

  return Constructor(aGlobal, nonNullTrackSeq, aRv);
}

/* static */
already_AddRefed<DOMMediaStream> DOMMediaStream::Constructor(
    const GlobalObject& aGlobal,
    const Sequence<OwningNonNull<MediaStreamTrack>>& aTracks,
    ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindowInner> ownerWindow =
      do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  auto newStream = MakeRefPtr<DOMMediaStream>(ownerWindow);
  for (MediaStreamTrack& track : aTracks) {
    newStream->AddTrack(track);
  }
  return newStream.forget();
}

already_AddRefed<Promise> DOMMediaStream::CountUnderlyingStreams(
    const GlobalObject& aGlobal, ErrorResult& aRv) {
  nsCOMPtr<nsPIDOMWindowInner> window =
      do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(aGlobal.GetAsSupports());
  if (!go) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  RefPtr<Promise> p = Promise::Create(go, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  MediaTrackGraph* graph = MediaTrackGraph::GetInstanceIfExists(
      window, MediaTrackGraph::REQUEST_DEFAULT_SAMPLE_RATE);
  if (!graph) {
    p->MaybeResolve(0);
    return p.forget();
  }

  auto* graphImpl = static_cast<MediaTrackGraphImpl*>(graph);

  class Counter : public ControlMessage {
   public:
    Counter(MediaTrackGraphImpl* aGraph, const RefPtr<Promise>& aPromise)
        : ControlMessage(nullptr), mGraph(aGraph), mPromise(aPromise) {
      MOZ_ASSERT(NS_IsMainThread());
    }

    void Run() override {
      uint32_t streams =
          mGraph->mTracks.Length() + mGraph->mSuspendedTracks.Length();
      mGraph->DispatchToMainThreadStableState(NS_NewRunnableFunction(
          "DOMMediaStream::CountUnderlyingStreams (stable state)",
          [promise = std::move(mPromise), streams]() mutable {
            NS_DispatchToMainThread(NS_NewRunnableFunction(
                "DOMMediaStream::CountUnderlyingStreams",
                [promise = std::move(promise), streams]() {
                  promise->MaybeResolve(streams);
                }));
          }));
    }

    // mPromise can only be AddRefed/Released on main thread.
    // In case of shutdown, Run() does not run, so we dispatch mPromise to be
    // released on main thread here.
    void RunDuringShutdown() override {
      NS_ReleaseOnMainThreadSystemGroup(
          "DOMMediaStream::CountUnderlyingStreams::Counter::RunDuringShutdown",
          mPromise.forget());
    }

   private:
    // mGraph owns this Counter instance and decides its lifetime.
    MediaTrackGraphImpl* mGraph;
    RefPtr<Promise> mPromise;
  };
  graphImpl->AppendMessage(MakeUnique<Counter>(graphImpl, p));

  return p.forget();
}

void DOMMediaStream::GetId(nsAString& aID) const { aID = mID; }

void DOMMediaStream::GetAudioTracks(
    nsTArray<RefPtr<AudioStreamTrack>>& aTracks) const {
  for (const auto& track : mTracks) {
    if (AudioStreamTrack* t = track->AsAudioStreamTrack()) {
      aTracks.AppendElement(t);
    }
  }
}

void DOMMediaStream::GetAudioTracks(
    nsTArray<RefPtr<MediaStreamTrack>>& aTracks) const {
  for (const auto& track : mTracks) {
    if (track->AsAudioStreamTrack()) {
      aTracks.AppendElement(track);
    }
  }
}

void DOMMediaStream::GetVideoTracks(
    nsTArray<RefPtr<VideoStreamTrack>>& aTracks) const {
  for (const auto& track : mTracks) {
    if (VideoStreamTrack* t = track->AsVideoStreamTrack()) {
      aTracks.AppendElement(t);
    }
  }
}

void DOMMediaStream::GetVideoTracks(
    nsTArray<RefPtr<MediaStreamTrack>>& aTracks) const {
  for (const auto& track : mTracks) {
    if (track->AsVideoStreamTrack()) {
      aTracks.AppendElement(track);
    }
  }
}

void DOMMediaStream::GetTracks(
    nsTArray<RefPtr<MediaStreamTrack>>& aTracks) const {
  for (const auto& track : mTracks) {
    aTracks.AppendElement(track);
  }
}

void DOMMediaStream::AddTrack(MediaStreamTrack& aTrack) {
  LOG(LogLevel::Info, ("DOMMediaStream %p Adding track %p (from track %p)",
                       this, &aTrack, aTrack.GetTrack()));

  if (HasTrack(aTrack)) {
    LOG(LogLevel::Debug,
        ("DOMMediaStream %p already contains track %p", this, &aTrack));
    return;
  }

  mTracks.AppendElement(&aTrack);
  NotifyTrackAdded(&aTrack);
}

void DOMMediaStream::RemoveTrack(MediaStreamTrack& aTrack) {
  LOG(LogLevel::Info, ("DOMMediaStream %p Removing track %p (from track %p)",
                       this, &aTrack, aTrack.GetTrack()));

  if (!mTracks.RemoveElement(&aTrack)) {
    LOG(LogLevel::Debug,
        ("DOMMediaStream %p does not contain track %p", this, &aTrack));
    return;
  }

  if (!aTrack.Ended()) {
    NotifyTrackRemoved(&aTrack);
  }
}

already_AddRefed<DOMMediaStream> DOMMediaStream::Clone() {
  auto newStream = MakeRefPtr<DOMMediaStream>(GetParentObject());

  LOG(LogLevel::Info,
      ("DOMMediaStream %p created clone %p", this, newStream.get()));

  for (const auto& track : mTracks) {
    LOG(LogLevel::Debug,
        ("DOMMediaStream %p forwarding external track %p to clone %p", this,
         track.get(), newStream.get()));
    RefPtr<MediaStreamTrack> clone = track->Clone();
    newStream->AddTrack(*clone);
  }

  return newStream.forget();
}

bool DOMMediaStream::Active() const { return mActive; }

MediaStreamTrack* DOMMediaStream::GetTrackById(const nsAString& aId) const {
  for (const auto& track : mTracks) {
    nsString id;
    track->GetId(id);
    if (id == aId) {
      return track;
    }
  }
  return nullptr;
}

bool DOMMediaStream::HasTrack(const MediaStreamTrack& aTrack) const {
  return mTracks.Contains(&aTrack);
}

void DOMMediaStream::AddTrackInternal(MediaStreamTrack* aTrack) {
  LOG(LogLevel::Debug,
      ("DOMMediaStream %p Adding owned track %p", this, aTrack));
  AddTrack(*aTrack);
  DispatchTrackEvent(NS_LITERAL_STRING("addtrack"), aTrack);
}

already_AddRefed<nsIPrincipal> DOMMediaStream::GetPrincipal() {
  nsCOMPtr<nsIPrincipal> principal =
      nsGlobalWindowInner::Cast(mWindow)->GetPrincipal();
  for (const auto& t : mTracks) {
    if (t->Ended()) {
      continue;
    }
    nsContentUtils::CombineResourcePrincipals(&principal, t->GetPrincipal());
  }
  return principal.forget();
}

void DOMMediaStream::NotifyActive() {
  LOG(LogLevel::Info, ("DOMMediaStream %p NotifyActive(). ", this));

  MOZ_ASSERT(mActive);
  for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
    mTrackListeners[i]->NotifyActive();
  }
}

void DOMMediaStream::NotifyInactive() {
  LOG(LogLevel::Info, ("DOMMediaStream %p NotifyInactive(). ", this));

  MOZ_ASSERT(!mActive);
  for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
    mTrackListeners[i]->NotifyInactive();
  }
}

void DOMMediaStream::NotifyAudible() {
  LOG(LogLevel::Info, ("DOMMediaStream %p NotifyAudible(). ", this));

  MOZ_ASSERT(mAudible);
  for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
    mTrackListeners[i]->NotifyAudible();
  }
}

void DOMMediaStream::NotifyInaudible() {
  LOG(LogLevel::Info, ("DOMMediaStream %p NotifyInaudible(). ", this));

  MOZ_ASSERT(!mAudible);
  for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
    mTrackListeners[i]->NotifyInaudible();
  }
}

void DOMMediaStream::RegisterTrackListener(TrackListener* aListener) {
  MOZ_ASSERT(NS_IsMainThread());

  mTrackListeners.AppendElement(aListener);
}

void DOMMediaStream::UnregisterTrackListener(TrackListener* aListener) {
  MOZ_ASSERT(NS_IsMainThread());
  mTrackListeners.RemoveElement(aListener);
}

void DOMMediaStream::SetFinishedOnInactive(bool aFinishedOnInactive) {
  MOZ_ASSERT(NS_IsMainThread());

  if (mFinishedOnInactive == aFinishedOnInactive) {
    return;
  }

  mFinishedOnInactive = aFinishedOnInactive;

  if (mFinishedOnInactive && !ContainsLiveTracks(mTracks)) {
    NotifyTrackRemoved(nullptr);
  }
}

void DOMMediaStream::NotifyTrackAdded(const RefPtr<MediaStreamTrack>& aTrack) {
  MOZ_ASSERT(NS_IsMainThread());

  aTrack->AddConsumer(mPlaybackTrackListener);

  for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
<<<<<<< HEAD
    mTrackListeners[i]->NotifyTrackAdded(aTrack);
=======
    mTrackListeners[i]->NotifyTrackRemoved(aTrack);
  }

  // Don't call RecomputePrincipal here as the track may still exist in the
  // playback stream in the MediaStreamGraph. It will instead be called when the
  // track has been confirmed removed by the graph. See BlockPlaybackTrack().
}

nsresult
DOMMediaStream::DispatchTrackEvent(const nsAString& aName,
                                   const RefPtr<MediaStreamTrack>& aTrack)
{
  MOZ_ASSERT(aName == NS_LITERAL_STRING("addtrack"),
             "Only 'addtrack' is supported at this time");

  MediaStreamTrackEventInit init;
  init.mTrack = aTrack;

  RefPtr<MediaStreamTrackEvent> event =
    MediaStreamTrackEvent::Constructor(this, aName, init);

  return DispatchTrustedEvent(event);
}

void
DOMMediaStream::CreateAndAddPlaybackStreamListener(MediaStream* aStream)
{
  MOZ_ASSERT(GetCameraStream(), "I'm a hack. Only DOMCameraControl may use me.");
  mPlaybackListener = new PlaybackStreamListener(this);
  aStream->AddListener(mPlaybackListener);
}

void
DOMMediaStream::BlockPlaybackTrack(TrackPort* aTrack)
{
  MOZ_ASSERT(aTrack);
  ++mTracksPendingRemoval;
  RefPtr<Pledge<bool>> p =
    aTrack->BlockSourceTrackId(aTrack->GetTrack()->mTrackID,
                               BlockingMode::CREATION);
  RefPtr<DOMMediaStream> self = this;
  p->Then([self] (const bool& aIgnore) { self->NotifyPlaybackTrackBlocked(); },
          [] (const nsresult& aIgnore) { NS_ERROR("Could not remove track from MSG"); }
  );
}

void
DOMMediaStream::NotifyPlaybackTrackBlocked()
{
  MOZ_ASSERT(mTracksPendingRemoval > 0,
             "A track reported finished blocking more times than we asked for");
  if (--mTracksPendingRemoval == 0) {
    // The MediaStreamGraph has reported a track was blocked and we are not
    // waiting for any further tracks to get blocked. It is now safe to
    // recompute the principal based on our main thread track set state.
    LOG(LogLevel::Debug, ("DOMMediaStream %p saw all tracks pending removal "
                          "finish. Recomputing principal.", this));
    RecomputePrincipal();
  }
}

DOMLocalMediaStream::~DOMLocalMediaStream()
{
  if (mInputStream) {
    // Make sure Listeners of this stream know it's going away
    StopImpl();
>>>>>>> parent of 3c1524e5e00c... Bug 1306137 - remove b2g camera code: Remove dom/camera/ and code which depends on it. r=aosmond,bkelly
  }

  if (!mActive) {
    // Check if we became active.
    if (ContainsLiveTracks(mTracks)) {
      mActive = true;
      NotifyActive();
    }
  }

  if (!mAudible) {
    // Check if we became audible.
    if (ContainsLiveAudioTracks(mTracks)) {
      mAudible = true;
      NotifyAudible();
    }
  }
}

void DOMMediaStream::NotifyTrackRemoved(
    const RefPtr<MediaStreamTrack>& aTrack) {
  MOZ_ASSERT(NS_IsMainThread());

  if (aTrack) {
    // aTrack may be null to allow HTMLMediaElement::MozCaptureStream streams
    // to be played until the source media element has ended. The source media
    // element will then call NotifyTrackRemoved(nullptr) to signal that we can
    // go inactive, regardless of the timing of the last track ending.

    aTrack->RemoveConsumer(mPlaybackTrackListener);

    for (int32_t i = mTrackListeners.Length() - 1; i >= 0; --i) {
      mTrackListeners[i]->NotifyTrackRemoved(aTrack);
    }

    if (!mActive) {
      NS_ASSERTION(false, "Shouldn't remove a live track if already inactive");
      return;
    }
  }

  if (!mFinishedOnInactive) {
    return;
  }

  if (mAudible) {
    // Check if we became inaudible.
    if (!ContainsLiveAudioTracks(mTracks)) {
      mAudible = false;
      NotifyInaudible();
    }
  }

  // Check if we became inactive.
  if (!ContainsLiveTracks(mTracks)) {
    mActive = false;
    NotifyInactive();
  }
}

nsresult DOMMediaStream::DispatchTrackEvent(
    const nsAString& aName, const RefPtr<MediaStreamTrack>& aTrack) {
  MediaStreamTrackEventInit init;
  init.mTrack = aTrack;

  RefPtr<MediaStreamTrackEvent> event =
      MediaStreamTrackEvent::Constructor(this, aName, init);

  return DispatchTrustedEvent(event);
}
