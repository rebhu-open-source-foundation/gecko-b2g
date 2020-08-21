/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaOffloadPlayer.h"

#include "GonkOffloadPlayer.h"
#include "MediaTrackGraph.h"

namespace mozilla {

using namespace mozilla::media;
using namespace mozilla::layers;

/* static */
RefPtr<MediaOffloadPlayer> MediaOffloadPlayer::Create(
    MediaFormatReaderInit& aInit, nsIURI* aURI) {
  return new GonkOffloadPlayer(aInit, aURI);
}

#define INIT_MIRROR(name, val) \
  name(mTaskQueue, val, "MediaOffloadPlayer::" #name " (Mirror)")
#define INIT_CANONICAL(name, val) \
  name(mTaskQueue, val, "MediaOffloadPlayer::" #name " (Canonical)")

MediaOffloadPlayer::MediaOffloadPlayer(MediaFormatReaderInit& aInit)
    : mTaskQueue(new TaskQueue(GetMediaThreadPool(MediaThreadType::MDSM),
                               "MediaOffloadPlayer::mTaskQueue",
                               /* aSupportsTailDispatch = */ true)),
      mWatchManager(this, mTaskQueue),
      mCurrentPositionTimer(mTaskQueue, true /*aFuzzy*/),
      mDormantTimer(mTaskQueue, true /*aFuzzy*/),
      mVideoFrameContainer(aInit.mVideoFrameContainer),
      mResource(aInit.mResource),
      INIT_CANONICAL(mBuffered, TimeIntervals()),
      INIT_CANONICAL(mDuration, NullableTimeUnit()),
      INIT_CANONICAL(mCurrentPosition, TimeUnit::Zero()),
      INIT_CANONICAL(mIsAudioDataAudible, false),
      INIT_MIRROR(mPlayState, MediaDecoder::PLAY_STATE_LOADING),
      INIT_MIRROR(mVolume, 1.0),
      INIT_MIRROR(mPreservesPitch, true),
      INIT_MIRROR(mLooping, false),
      INIT_MIRROR(mSinkDevice, nullptr),
      INIT_MIRROR(mSecondaryVideoContainer, nullptr),
      INIT_MIRROR(mOutputCaptured, false),
      INIT_MIRROR(mOutputTracks, nsTArray<RefPtr<ProcessedMediaTrack>>()),
      INIT_MIRROR(mOutputPrincipal, PRINCIPAL_HANDLE_NONE) {}

#undef INIT_MIRROR
#undef INIT_CANONICAL

MediaOffloadPlayer::~MediaOffloadPlayer() {}

void MediaOffloadPlayer::InitializationTask(MediaDecoder* aDecoder) {
  AUTO_PROFILER_LABEL("MediaOffloadPlayer::InitializationTask", MEDIA_PLAYBACK);
  MOZ_ASSERT(OnTaskQueue());

  // Connect mirrors.
  mPlayState.Connect(aDecoder->CanonicalPlayState());
  mVolume.Connect(aDecoder->CanonicalVolume());
  mPreservesPitch.Connect(aDecoder->CanonicalPreservesPitch());
  mLooping.Connect(aDecoder->CanonicalLooping());
  mSinkDevice.Connect(aDecoder->CanonicalSinkDevice());
  mSecondaryVideoContainer.Connect(
      aDecoder->CanonicalSecondaryVideoContainer());
  mOutputCaptured.Connect(aDecoder->CanonicalOutputCaptured());
  mOutputTracks.Connect(aDecoder->CanonicalOutputTracks());
  mOutputPrincipal.Connect(aDecoder->CanonicalOutputPrincipal());

  // Initialize watchers.
  mWatchManager.Watch(mPlayState, &MediaOffloadPlayer::PlayStateChanged);
  mWatchManager.Watch(mVolume, &MediaOffloadPlayer::VolumeChanged);
  mWatchManager.Watch(mPreservesPitch,
                      &MediaOffloadPlayer::PreservesPitchChanged);
  mWatchManager.Watch(mLooping, &MediaOffloadPlayer::LoopingChanged);
  mWatchManager.Watch(mSecondaryVideoContainer,
                      &MediaOffloadPlayer::UpdateSecondaryVideoContainer);
  mWatchManager.Watch(mOutputCaptured,
                      &MediaOffloadPlayer::UpdateOutputCaptured);
  mWatchManager.Watch(mOutputTracks, &MediaOffloadPlayer::OutputTracksChanged);
  mWatchManager.Watch(mOutputPrincipal,
                      &MediaOffloadPlayer::OutputPrincipalChanged);

  mTransportSeekable = aDecoder->IsTransportSeekable();
  mAudioChannel = aDecoder->GetAudioChannel();
  mContainerType = aDecoder->ContainerType().Type().AsString();

  InitInternal();
}

nsresult MediaOffloadPlayer::Init(MediaDecoder* aDecoder) {
  MOZ_ASSERT(NS_IsMainThread());

  // Dispatch initialization that needs to happen on that task queue.
  OwnerThread()->DispatchStateChange(NewRunnableMethod<RefPtr<MediaDecoder>>(
      "MediaOffloadPlayer::InitializationTask", this,
      &MediaOffloadPlayer::InitializationTask, aDecoder));
  return NS_OK;
}

RefPtr<ShutdownPromise> MediaOffloadPlayer::Shutdown() {
  MOZ_ASSERT(OnTaskQueue());

  mCurrentPositionTimer.Reset();
  mDormantTimer.Reset();
  ResetInternal();

  // Disconnect canonicals and mirrors before shutting down our task queue.
  mPlayState.DisconnectIfConnected();
  mVolume.DisconnectIfConnected();
  mPreservesPitch.DisconnectIfConnected();
  mLooping.DisconnectIfConnected();
  mSinkDevice.DisconnectIfConnected();
  mSecondaryVideoContainer.DisconnectIfConnected();
  mOutputCaptured.DisconnectIfConnected();
  mOutputTracks.DisconnectIfConnected();
  mOutputPrincipal.DisconnectIfConnected();

  mBuffered.DisconnectAll();
  mDuration.DisconnectAll();
  mCurrentPosition.DisconnectAll();
  mIsAudioDataAudible.DisconnectAll();

  // Shut down the watch manager to stop further notifications.
  mWatchManager.Shutdown();
  return OwnerThread()->BeginShutdown();
}

RefPtr<ShutdownPromise> MediaOffloadPlayer::BeginShutdown() {
  MOZ_ASSERT(NS_IsMainThread());
  return InvokeAsync(OwnerThread(), this, __func__,
                     &MediaOffloadPlayer::Shutdown);
}

void MediaOffloadPlayer::UpdateCurrentPositionPeriodically() {
  bool scheduleNextUpdate = UpdateCurrentPosition();
  if (!scheduleNextUpdate) {
    return;
  }

  TimeStamp target = TimeStamp::Now() + TimeDuration::FromMilliseconds(250);
  mCurrentPositionTimer.Ensure(
      target,
      [this, self = RefPtr<MediaOffloadPlayer>(this)]() {
        mCurrentPositionTimer.CompleteRequest();
        UpdateCurrentPositionPeriodically();
      },
      []() { MOZ_DIAGNOSTIC_ASSERT(false); });
}

RefPtr<GenericPromise> MediaOffloadPlayer::RequestDebugInfo(
    dom::MediaDecoderStateMachineDebugInfo& aInfo) {
  return GenericPromise::CreateAndReject(NS_ERROR_ABORT, __func__);
}

void MediaOffloadPlayer::NotifySeeked(bool aSuccess) {
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(mCurrentSeek.Exists());

  if (aSuccess) {
    mCurrentSeek.mPromise.ResolveIfExists(true, __func__);
  } else {
    mCurrentSeek.mPromise.RejectIfExists(true, __func__);
  }

  // If there is a pending seek job, dispatch it to another runnable.
  if (mPendingSeek.Exists()) {
    mCurrentSeek = std::move(mPendingSeek);
    mPendingSeek = SeekObject();
    nsresult rv = OwnerThread()->Dispatch(NS_NewRunnableFunction(
        "MediaOffloadPlayer::SeekPendingJob",
        [this, self = RefPtr<MediaOffloadPlayer>(this)]() {
          SeekInternal(mCurrentSeek.mTarget.ref(), mCurrentSeek.mVisible);
        }));
    MOZ_DIAGNOSTIC_ASSERT(NS_SUCCEEDED(rv));
    Unused << rv;
  } else {
    mCurrentSeek = SeekObject();
    // We may just exit from dormant state, so manually call PlayStateChanged()
    // to enter playing/paused state correctly.
    PlayStateChanged();
  }
}

RefPtr<MediaDecoder::SeekPromise> MediaOffloadPlayer::HandleSeek(
    const SeekTarget& aTarget, bool aVisible) {
  MOZ_ASSERT(OnTaskQueue());

  // If user seeks to a new position, need to exit dormant and fire seek so the
  // displayed image will be updated.
  if (aVisible && mVideoFrameContainer) {
    ExitDormant();
  }

  // If we are currently seeking or we need to defer seeking, save the job in
  // mPendingSeek. If there is already a pending seek job, reject it and replace
  // it by the new one.
  if (mCurrentSeek.Exists() || NeedToDeferSeek()) {
    mPendingSeek.RejectIfExists(__func__);
    mPendingSeek.mTarget.emplace(aTarget);
    mPendingSeek.mVisible = aVisible;
    return mPendingSeek.mPromise.Ensure(__func__);
  }

  MOZ_ASSERT(!mPendingSeek.Exists());
  mPendingSeek = SeekObject();
  mCurrentSeek.mTarget.emplace(aTarget);
  mCurrentSeek.mVisible = aVisible;
  RefPtr<MediaDecoder::SeekPromise> p = mCurrentSeek.mPromise.Ensure(__func__);
  SeekInternal(aTarget, aVisible);
  return p;
}

RefPtr<MediaDecoder::SeekPromise> MediaOffloadPlayer::InvokeSeek(
    const SeekTarget& aTarget) {
  return InvokeAsync(OwnerThread(), this, __func__,
                     &MediaOffloadPlayer::HandleSeek, aTarget, true);
}

void MediaOffloadPlayer::FirePendingSeekIfExists() {
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(!mCurrentSeek.Exists());

  if (mPendingSeek.Exists()) {
    mCurrentSeek = std::move(mPendingSeek);
    mPendingSeek = SeekObject();
    SeekInternal(mCurrentSeek.mTarget.ref(), mCurrentSeek.mVisible);
  }
}

void MediaOffloadPlayer::DispatchSetPlaybackRate(double aPlaybackRate) {
  MOZ_ASSERT(NS_IsMainThread());
  OwnerThread()->DispatchStateChange(NS_NewRunnableFunction(
      "MediaOffloadPlayer::SetPlaybackRate",
      [this, self = RefPtr<MediaOffloadPlayer>(this), aPlaybackRate]() {
        mPlaybackRate = aPlaybackRate;
        PlaybackRateChanged();
      }));
}

RefPtr<GenericPromise> MediaOffloadPlayer::InvokeSetSink(
    RefPtr<AudioDeviceInfo> aSink) {
  return GenericPromise::CreateAndReject(NS_ERROR_ABORT, __func__);
}

RefPtr<SetCDMPromise> MediaOffloadPlayer::SetCDMProxy(CDMProxy* aProxy) {
  return SetCDMPromise::CreateAndReject(NS_ERROR_ABORT, __func__);
}

void MediaOffloadPlayer::UpdateCompositor(
    already_AddRefed<layers::KnowsCompositor> aCompositor) {
  RefPtr<layers::KnowsCompositor> compositor = aCompositor;
}

void MediaOffloadPlayer::StartDormantTimer() {
  if (mInDormant) {
    return;
  }

  if (!mTransportSeekable || !mInfo.mMediaSeekable) {
    return;
  }

  auto timeout = StaticPrefs::media_dormant_on_pause_timeout_ms();
  if (timeout < 0) {
    return;
  } else if (timeout == 0) {
    EnterDormant();
    return;
  }

  TimeStamp target = TimeStamp::Now() + TimeDuration::FromMilliseconds(timeout);
  mDormantTimer.Ensure(
      target,
      [this, self = RefPtr<MediaOffloadPlayer>(this)]() {
        mDormantTimer.CompleteRequest();
        EnterDormant();
      },
      []() { MOZ_DIAGNOSTIC_ASSERT(false); });
}

void MediaOffloadPlayer::EnterDormant() {
  mInDormant = true;
  UpdateCurrentPosition();
  ResetInternal();
  // This queues a seek job of the current position, and it will be fired when
  // leaving dormant state.
  HandleSeek(SeekTarget(mCurrentPosition, SeekTarget::Accurate), false);
}

void MediaOffloadPlayer::ExitDormant() {
  mDormantTimer.Reset();
  if (mInDormant) {
    mInDormant = false;
    InitInternal();
  }
}

void MediaOffloadPlayer::PlayStateChanged() {
  MOZ_ASSERT(OnTaskQueue());
  if (mPlayState == MediaDecoder::PLAY_STATE_PAUSED) {
    StartDormantTimer();
  } else if (mPlayState == MediaDecoder::PLAY_STATE_PLAYING) {
    ExitDormant();
  }
}

void MediaOffloadPlayer::NotifyMediaNotSeekable() {
  mOnMediaNotSeekable.Notify();
}

void MediaOffloadPlayer::NotifyMetadataLoaded(UniquePtr<MediaInfo> aInfo,
                                              UniquePtr<MetadataTags> aTags) {
  mMetadataLoadedEvent.Notify(std::move(aInfo), std::move(aTags),
                              MediaDecoderEventVisibility::Observable);
}

void MediaOffloadPlayer::NotifyFirstFrameLoaded(UniquePtr<MediaInfo> aInfo) {
  mFirstFrameLoadedEvent.Notify(std::move(aInfo),
                                MediaDecoderEventVisibility::Observable);
}

void MediaOffloadPlayer::NotifyPlaybackEvent(MediaPlaybackEvent aEvent) {
  mOnPlaybackEvent.Notify(aEvent);
}

void MediaOffloadPlayer::NotifyPlaybackError(MediaResult aError) {
  mOnPlaybackErrorEvent.Notify(aError);
}

void MediaOffloadPlayer::NotifyNextFrameStatus(NextFrameStatus aStatus) {
  mOnNextFrameStatus.Notify(aStatus);
}

}  // namespace mozilla
