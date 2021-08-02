/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/layers/CompositorVsyncScheduler.h"

#ifdef MOZ_WIDGET_GONK
#include "mozilla/widget/GonkCompositorWidget.h"
#endif

namespace mozilla {

namespace layers {

/* static */ already_AddRefed<CompositorScheduler> CompositorScheduler::Create(
    CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner,
    widget::CompositorWidget* aWidget) {
  RefPtr<CompositorScheduler> scheduler;
#ifdef MOZ_WIDGET_GONK
  if (aWidget->AsGonk()->GetVsyncSupport()) {
    scheduler = new CompositorVsyncScheduler(aVsyncSchedulerOwner, aWidget);
  } else {
    scheduler = new CompositorSoftwareTimerScheduler(aVsyncSchedulerOwner);
  }
#else
  scheduler = new CompositorVsyncScheduler(aVsyncSchedulerOwner, aWidget);
#endif
  return scheduler.forget();
}

CompositorScheduler::CompositorScheduler(
    CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner)
    : mVsyncSchedulerOwner(aVsyncSchedulerOwner),
      mLastComposeTime(SampleTime::FromNow()),
      mLastVsyncTime(TimeStamp::Now()),
      mLastVsyncOutputTime(TimeStamp::Now()) {}

CompositorScheduler::~CompositorScheduler() {
  MOZ_ASSERT(!mVsyncSchedulerOwner);
}

const SampleTime& CompositorScheduler::GetLastComposeTime() const {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  return mLastComposeTime;
}

const TimeStamp& CompositorScheduler::GetLastVsyncTime() const {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  return mLastVsyncTime;
}

const TimeStamp& CompositorScheduler::GetLastVsyncOutputTime() const {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  return mLastVsyncOutputTime;
}

const VsyncId& CompositorScheduler::GetLastVsyncId() const {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  return mLastVsyncId;
}

void CompositorScheduler::UpdateLastComposeTime() {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  mLastComposeTime = SampleTime::FromNow();
}

/* CompositorSoftwareTimerScheduler methods */
CompositorSoftwareTimerScheduler::CompositorSoftwareTimerScheduler(
    CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner)
    : CompositorScheduler(aVsyncSchedulerOwner),
      mCurrentCompositeTaskMonitor("CurrentCompositeTaskMonitor"),
      mCurrentCompositeTask(nullptr) {}

CompositorSoftwareTimerScheduler::~CompositorSoftwareTimerScheduler() {
  MOZ_ASSERT(!mVsyncSchedulerOwner);
}

void CompositorSoftwareTimerScheduler::ScheduleComposition() {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  if (mCurrentCompositeTask) {
    return;
  }

  TimeDuration delayForCompose;
  // First time composite, no delay for compose
  if (mLastComposeTime.IsNull()) {
    delayForCompose = TimeDuration::FromMilliseconds(0);
  } else {
    TimeDuration frameDuration = mVsyncSchedulerOwner->GetVsyncInterval();
    TimeDuration frameComposeDelay = TimeStamp::Now() - mLastComposeTime.Time();
    // frameComposeDelay > vsync period, no delay
    if (frameComposeDelay >= frameDuration) {
      delayForCompose = TimeDuration::FromMilliseconds(0);
    } else {
      delayForCompose = frameDuration - frameComposeDelay;
    }
  }

  {
    MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
    RefPtr<CancelableRunnable> task = NewCancelableRunnableMethod(
        "layers::CompositorSoftwareTimerScheduler::ScheduleComposition", this,
        &CompositorSoftwareTimerScheduler::CallComposite);
    mCurrentCompositeTask = task;
    if ((uint32_t)delayForCompose.ToMilliseconds() == 0) {
      CompositorThread()->Dispatch(task.forget());
    } else {
      CompositorThread()->DelayedDispatch(
          task.forget(), (uint32_t)delayForCompose.ToMilliseconds());
    }
  }
}

void CompositorSoftwareTimerScheduler::CancelCurrentCompositeTask() {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread() ||
             NS_IsMainThread());
  MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
  if (mCurrentCompositeTask) {
    mCurrentCompositeTask->Cancel();
    mCurrentCompositeTask = nullptr;
  }
}

bool CompositorSoftwareTimerScheduler::NeedsComposite() {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  return mCurrentCompositeTask ? true : false;
}

void CompositorSoftwareTimerScheduler::CallComposite() {
  TimeStamp vsyncTime = TimeStamp::Now();
  TimeStamp outputTime = vsyncTime + mVsyncSchedulerOwner->GetVsyncInterval();
  VsyncEvent vsyncEvent(VsyncId(), vsyncTime, outputTime);

  Composite(vsyncEvent);
}

void CompositorSoftwareTimerScheduler::Composite(
    const VsyncEvent& aVsyncEvent) {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  {  // scope lock
    MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
    mCurrentCompositeTask = nullptr;
  }
  mLastVsyncTime = aVsyncEvent.mTime;
  mLastVsyncOutputTime = aVsyncEvent.mOutputTime;
  mLastVsyncId = aVsyncEvent.mId;
  mLastComposeTime = SampleTime::FromVsync(aVsyncEvent.mTime);
  ;
  mVsyncSchedulerOwner->CompositeToTarget(aVsyncEvent.mId, nullptr, nullptr);
}

void CompositorSoftwareTimerScheduler::ForceComposeToTarget(
    gfx::DrawTarget* aTarget, const gfx::IntRect* aRect) {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());

  mLastComposeTime = SampleTime::FromNow();
  mVsyncSchedulerOwner->CompositeToTarget(VsyncId(), aTarget, aRect);
}

void CompositorSoftwareTimerScheduler::Destroy() {
  MOZ_ASSERT(CompositorThreadHolder::IsInCompositorThread());
  CancelCurrentCompositeTask();
  mVsyncSchedulerOwner = nullptr;
}

}  // namespace layers
}  // namespace mozilla
