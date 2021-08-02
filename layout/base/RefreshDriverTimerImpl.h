/* Copyright (C) 2015 Acadine Technologies. All rights reserved. */
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef RefreshDriverTimerImpl_h_
#define RefreshDriverTimerImpl_h_

#ifdef XP_WIN
#include <windows.h>
// mmsystem isn't part of WIN32_LEAN_AND_MEAN, so we have
// to manually include it
#include <mmsystem.h>
#include "WinUtils.h"
#endif

#include "mozilla/dom/Document.h"
#include "mozilla/InputTaskManager.h"
#include "mozilla/dom/VsyncChild.h"
#include "mozilla/StaticPrefs_page_load.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/VsyncDispatcher.h"
#include "nsITimer.h"
#include "nsIXULRuntime.h"
#include "nsRefreshDriver.h"
#include "nsTArray.h"
#include "nsPresContext.h"
#include "VsyncSource.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layout;

static mozilla::LazyLogModule sRefreshDriverLog("nsRefreshDriver");
#define LOG(...) \
  MOZ_LOG(sRefreshDriverLog, mozilla::LogLevel::Debug, (__VA_ARGS__))

namespace mozilla {

/*
 * The base class for all global refresh driver timers.  It takes care
 * of managing the list of refresh drivers attached to them and
 * provides interfaces for querying/setting the rate and actually
 * running a timer 'Tick'.  Subclasses must implement StartTimer(),
 * StopTimer(), and ScheduleNextTick() -- the first two just
 * start/stop whatever timer mechanism is in use, and ScheduleNextTick
 * is called at the start of the Tick() implementation to set a time
 * for the next tick.
 */
class RefreshDriverTimer {
 public:
  RefreshDriverTimer() = default;

  NS_INLINE_DECL_REFCOUNTING(RefreshDriverTimer)

  virtual void AddRefreshDriver(nsRefreshDriver* aDriver) {
    LOG("[%p] AddRefreshDriver %p", this, aDriver);

    bool startTimer =
        mContentRefreshDrivers.IsEmpty() && mRootRefreshDrivers.IsEmpty();
    if (IsRootRefreshDriver(aDriver)) {
      NS_ASSERTION(!mRootRefreshDrivers.Contains(aDriver),
                   "Adding a duplicate root refresh driver!");
      mRootRefreshDrivers.AppendElement(aDriver);
    } else {
      NS_ASSERTION(!mContentRefreshDrivers.Contains(aDriver),
                   "Adding a duplicate content refresh driver!");
      mContentRefreshDrivers.AppendElement(aDriver);
    }

    if (startTimer) {
      StartTimer();
    }
  }

  void RemoveRefreshDriver(nsRefreshDriver* aDriver) {
    LOG("[%p] RemoveRefreshDriver %p", this, aDriver);

    if (IsRootRefreshDriver(aDriver)) {
      NS_ASSERTION(mRootRefreshDrivers.Contains(aDriver),
                   "RemoveRefreshDriver for a refresh driver that's not in the "
                   "root refresh list!");
      mRootRefreshDrivers.RemoveElement(aDriver);
    } else {
      nsPresContext* pc = aDriver->GetPresContext();
      nsPresContext* rootContext = pc ? pc->GetRootPresContext() : nullptr;
      // During PresContext shutdown, we can't accurately detect
      // if a root refresh driver exists or not. Therefore, we have to
      // search and find out which list this driver exists in.
      if (!rootContext) {
        if (mRootRefreshDrivers.Contains(aDriver)) {
          mRootRefreshDrivers.RemoveElement(aDriver);
        } else {
          NS_ASSERTION(mContentRefreshDrivers.Contains(aDriver),
                       "RemoveRefreshDriver without a display root for a "
                       "driver that is not in the content refresh list");
          mContentRefreshDrivers.RemoveElement(aDriver);
        }
      } else {
        NS_ASSERTION(mContentRefreshDrivers.Contains(aDriver),
                     "RemoveRefreshDriver for a driver that is not in the "
                     "content refresh list");
        mContentRefreshDrivers.RemoveElement(aDriver);
      }
    }

    bool stopTimer =
        mContentRefreshDrivers.IsEmpty() && mRootRefreshDrivers.IsEmpty();
    if (stopTimer) {
      StopTimer();
    }
  }

  TimeStamp MostRecentRefresh() const { return mLastFireTime; }
  VsyncId MostRecentRefreshVsyncId() const { return mLastFireId; }

  virtual TimeDuration GetTimerRate() = 0;

  TimeStamp GetIdleDeadlineHint(TimeStamp aDefault) {
    MOZ_ASSERT(NS_IsMainThread());

    TimeStamp mostRecentRefresh = MostRecentRefresh();
    TimeDuration refreshPeriod = GetTimerRate();
    TimeStamp idleEnd = mostRecentRefresh + refreshPeriod;

    // If we haven't painted for some time, then guess that we won't paint
    // again for a while, so the refresh driver is not a good way to predict
    // idle time.
    if (idleEnd +
            refreshPeriod *
                StaticPrefs::layout_idle_period_required_quiescent_frames() <
        TimeStamp::Now()) {
      return aDefault;
    }

    // End the predicted idle time a little early, the amount controlled by a
    // pref, to prevent overrunning the idle time and delaying a frame.
    idleEnd = idleEnd - TimeDuration::FromMilliseconds(
                            StaticPrefs::layout_idle_period_time_limit());
    return idleEnd < aDefault ? idleEnd : aDefault;
  }

  Maybe<TimeStamp> GetNextTickHint() {
    MOZ_ASSERT(NS_IsMainThread());
    TimeStamp nextTick = MostRecentRefresh() + GetTimerRate();
    return nextTick < TimeStamp::Now() ? Nothing() : Some(nextTick);
  }

  // Returns null if the RefreshDriverTimer is attached to several
  // RefreshDrivers. That may happen for example when there are
  // several windows open.
  nsPresContext* GetPresContextForOnlyRefreshDriver() {
    if (mRootRefreshDrivers.Length() == 1 && mContentRefreshDrivers.IsEmpty()) {
      return mRootRefreshDrivers[0]->GetPresContext();
    }
    if (mContentRefreshDrivers.Length() == 1 && mRootRefreshDrivers.IsEmpty()) {
      return mContentRefreshDrivers[0]->GetPresContext();
    }
    return nullptr;
  }

 protected:
  virtual ~RefreshDriverTimer() {
    MOZ_ASSERT(
        mContentRefreshDrivers.Length() == 0,
        "Should have removed all content refresh drivers from here by now!");
    MOZ_ASSERT(
        mRootRefreshDrivers.Length() == 0,
        "Should have removed all root refresh drivers from here by now!");
  }

  virtual void StartTimer() = 0;
  virtual void StopTimer() = 0;
  virtual void ScheduleNextTick(TimeStamp aNowTime) = 0;

  bool IsRootRefreshDriver(nsRefreshDriver* aDriver) {
    nsPresContext* pc = aDriver->GetPresContext();
    nsPresContext* rootContext = pc ? pc->GetRootPresContext() : nullptr;
    if (!rootContext) {
      return false;
    }

    return aDriver == rootContext->RefreshDriver();
  }

  /*
   * Actually runs a tick, poking all the attached RefreshDrivers.
   * Grabs the "now" time via TimeStamp::Now().
   */
  void Tick() {
    TimeStamp now = TimeStamp::Now();
    Tick(VsyncId(), now);
  }

  void TickRefreshDrivers(VsyncId aId, TimeStamp aNow,
                          nsTArray<RefPtr<nsRefreshDriver>>& aDrivers) {
    if (aDrivers.IsEmpty()) {
      return;
    }

    for (nsRefreshDriver* driver : aDrivers.Clone()) {
      // don't poke this driver if it's in test mode
      if (driver->IsTestControllingRefreshesEnabled()) {
        continue;
      }

      TickDriver(driver, aId, aNow);
    }
  }

  /*
   * Tick the refresh drivers based on the given timestamp.
   */
  void Tick(VsyncId aId, TimeStamp now) {
    ScheduleNextTick(now);

    mLastFireTime = now;
    mLastFireId = aId;

    LOG("[%p] ticking drivers...", this);

    TickRefreshDrivers(aId, now, mContentRefreshDrivers);
    TickRefreshDrivers(aId, now, mRootRefreshDrivers);

    LOG("[%p] done.", this);
  }

  static void TickDriver(nsRefreshDriver* driver, VsyncId aId, TimeStamp now) {
    driver->Tick(aId, now);
  }

  TimeStamp mLastFireTime;
  VsyncId mLastFireId;
  TimeStamp mTargetTime;

  nsTArray<RefPtr<nsRefreshDriver>> mContentRefreshDrivers;
  nsTArray<RefPtr<nsRefreshDriver>> mRootRefreshDrivers;

  // useful callback for nsITimer-based derived classes, here
  // because of c++ protected shenanigans
  static void TimerTick(nsITimer* aTimer, void* aClosure) {
    RefPtr<RefreshDriverTimer> timer =
        static_cast<RefreshDriverTimer*>(aClosure);
    timer->Tick();
  }
};

/*
 * A RefreshDriverTimer that uses a nsITimer as the underlying timer.  Note that
 * this is a ONE_SHOT timer, not a repeating one!  Subclasses are expected to
 * implement ScheduleNextTick and intelligently calculate the next time to tick,
 * and to reset mTimer.  Using a repeating nsITimer gets us into a lot of pain
 * with its attempt at intelligent slack removal and such, so we don't do it.
 */
class SimpleTimerBasedRefreshDriverTimer : public RefreshDriverTimer {
 public:
  /*
   * aRate -- the delay, in milliseconds, requested between timer firings
   */
  explicit SimpleTimerBasedRefreshDriverTimer(double aRate) {
    SetRate(aRate);
    mTimer = NS_NewTimer();
  }

  virtual ~SimpleTimerBasedRefreshDriverTimer() override { StopTimer(); }

  // will take effect at next timer tick
  virtual void SetRate(double aNewRate) {
    mRateMilliseconds = aNewRate;
    mRateDuration = TimeDuration::FromMilliseconds(mRateMilliseconds);
  }

  double GetRate() const { return mRateMilliseconds; }

  TimeDuration GetTimerRate() override { return mRateDuration; }

 protected:
  void StartTimer() override {
    // pretend we just fired, and we schedule the next tick normally
    mLastFireTime = TimeStamp::Now();
    mLastFireId = VsyncId();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithNamedFuncCallback(
        TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT,
        "SimpleTimerBasedRefreshDriverTimer::StartTimer");
  }

  void StopTimer() override { mTimer->Cancel(); }

  double mRateMilliseconds;
  TimeDuration mRateDuration;
  RefPtr<nsITimer> mTimer;
};

/*
 * A refresh driver that listens to vsync events and ticks the refresh driver
 * on vsync intervals. We throttle the refresh driver if we get too many
 * vsync events and wait to catch up again.
 */
class VsyncRefreshDriverTimer : public RefreshDriverTimer {
 public:
  VsyncRefreshDriverTimer()
      : mVsyncDispatcher(nullptr),
        mVsyncChild(nullptr),
        mVsyncRate(TimeDuration::Forever()) {
    MOZ_ASSERT(XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncSource = gfxPlatform::GetPlatform()->GetHardwareVsync();
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    MOZ_ALWAYS_TRUE(mVsyncDispatcher =
                        mVsyncSource->GetRefreshTimerVsyncDispatcher());
  }

  VsyncRefreshDriverTimer(RefreshTimerVsyncDispatcher* aVsyncDispatcher)
  : mVsyncDispatcher(aVsyncDispatcher),
    mVsyncChild(nullptr) {
    MOZ_ASSERT(XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    RefPtr<mozilla::gfx::VsyncSource> vsyncSource =
        gfxPlatform::GetPlatform()->GetHardwareVsync();
    mVsyncDispatcher->AddChildRefreshTimer(mVsyncObserver);
    mVsyncRate = vsyncSource->GetGlobalDisplay().GetVsyncRate();
  }

  // Constructor for when we have a local vsync source. As it is local, we do
  // not have to worry about it being re-inited by gfxPlatform on frame rate
  // change on the global source.
  explicit VsyncRefreshDriverTimer(const RefPtr<gfx::VsyncSource>& aVsyncSource)
      : mVsyncSource(aVsyncSource),
        mVsyncDispatcher(nullptr),
        mVsyncChild(nullptr),
        mVsyncRate(TimeDuration::Forever()) {
    MOZ_ASSERT(XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    MOZ_ALWAYS_TRUE(mVsyncDispatcher =
                        aVsyncSource->GetRefreshTimerVsyncDispatcher());
  }

  explicit VsyncRefreshDriverTimer(const RefPtr<VsyncChild>& aVsyncChild)
      : mVsyncSource(nullptr),
        mVsyncDispatcher(nullptr),
        mVsyncChild(aVsyncChild),
        mVsyncRate(TimeDuration::Forever()) {
    MOZ_ASSERT(XRE_IsContentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
  }

  TimeDuration GetTimerRate() override {
    if (mVsyncSource) {
      mVsyncRate = mVsyncSource->GetGlobalDisplay().GetVsyncRate();
    } else if (mVsyncChild) {
      mVsyncRate = mVsyncChild->GetVsyncRate();
    }

    // If hardware queries fail / are unsupported, we have to just guess.
    return mVsyncRate != TimeDuration::Forever()
               ? mVsyncRate
               : TimeDuration::FromMilliseconds(1000.0 / 60.0);
  }

 private:
  // Since VsyncObservers are refCounted, but the RefreshDriverTimer are
  // explicitly shutdown. We create an inner class that has the VsyncObserver
  // and is shutdown when the RefreshDriverTimer is deleted.
  class RefreshDriverVsyncObserver final : public VsyncObserver {
   public:
    explicit RefreshDriverVsyncObserver(
        VsyncRefreshDriverTimer* aVsyncRefreshDriverTimer)
        : mVsyncRefreshDriverTimer(aVsyncRefreshDriverTimer),
          mParentProcessRefreshTickLock("RefreshTickLock"),
          mPendingParentProcessVsync(false),
          mRecentVsync(TimeStamp::Now()),
          mLastTick(TimeStamp::Now()),
          mVsyncRate(TimeDuration::Forever()),
          mProcessedVsync(true) {
      MOZ_ASSERT(NS_IsMainThread());
    }

    class ParentProcessVsyncNotifier final : public Runnable,
                                             public nsIRunnablePriority {
     public:
      explicit ParentProcessVsyncNotifier(RefreshDriverVsyncObserver* aObserver)
          : Runnable(
                "VsyncRefreshDriverTimer::RefreshDriverVsyncObserver::"
                "ParentProcessVsyncNotifier"),
            mObserver(aObserver) {}

      NS_DECL_ISUPPORTS_INHERITED

      NS_IMETHOD Run() override {
        MOZ_ASSERT(NS_IsMainThread());
        sVsyncPriorityEnabled = mozilla::BrowserTabsRemoteAutostart();

        mObserver->NotifyParentProcessVsync();
        return NS_OK;
      }

      NS_IMETHOD GetPriority(uint32_t* aPriority) override {
        *aPriority = sVsyncPriorityEnabled
                         ? nsIRunnablePriority::PRIORITY_VSYNC
                         : nsIRunnablePriority::PRIORITY_NORMAL;
        return NS_OK;
      }

     private:
      ~ParentProcessVsyncNotifier() = default;
      RefPtr<RefreshDriverVsyncObserver> mObserver;
      static mozilla::Atomic<bool> sVsyncPriorityEnabled;
    };

    bool NotifyVsync(const VsyncEvent& aVsync) override {
      // Compress vsync notifications such that only 1 may run at a time
      // This is so that we don't flood the refresh driver with vsync messages
      // if the main thread is blocked for long periods of time
      {  // scope lock
        MonitorAutoLock lock(mParentProcessRefreshTickLock);
        mRecentParentProcessVsync = aVsync;
        if (mPendingParentProcessVsync) {
          return true;
        }
        mPendingParentProcessVsync = true;
      }

      if (XRE_IsContentProcess()) {
        NotifyParentProcessVsync();
        return true;
      }

      nsCOMPtr<nsIRunnable> vsyncEvent = new ParentProcessVsyncNotifier(this);
      NS_DispatchToMainThread(vsyncEvent);
      return true;
    }

    void NotifyParentProcessVsync() {
      // IMPORTANT: All paths through this method MUST hold a strong ref on
      // |this| for the duration of the TickRefreshDriver callback.
      MOZ_ASSERT(NS_IsMainThread());

      // This clears the input handling start time.
      InputTaskManager::Get()->SetInputHandlingStartTime(TimeStamp());

      VsyncEvent aVsync;
      {
        MonitorAutoLock lock(mParentProcessRefreshTickLock);
        aVsync = mRecentParentProcessVsync;
        mPendingParentProcessVsync = false;
      }

      mRecentVsync = aVsync.mTime;
      mRecentVsyncId = aVsync.mId;
      if (!mBlockUntil.IsNull() && mBlockUntil > aVsync.mTime) {
        if (mProcessedVsync) {
          // Re-post vsync update as a normal priority runnable. This way
          // runnables already in normal priority queue get processed.
          mProcessedVsync = false;
          nsCOMPtr<nsIRunnable> vsyncEvent = NewRunnableMethod<>(
              "RefreshDriverVsyncObserver::NormalPriorityNotify", this,
              &RefreshDriverVsyncObserver::NormalPriorityNotify);
          NS_DispatchToMainThread(vsyncEvent);
        }

        return;
      }

      if (StaticPrefs::layout_lower_priority_refresh_driver_during_load() &&
          mVsyncRefreshDriverTimer) {
        nsPresContext* pctx =
            mVsyncRefreshDriverTimer->GetPresContextForOnlyRefreshDriver();
        if (pctx && pctx->HadContentfulPaint() && pctx->Document() &&
            pctx->Document()->GetReadyStateEnum() <
                Document::READYSTATE_COMPLETE) {
          nsPIDOMWindowInner* win = pctx->Document()->GetInnerWindow();
          uint32_t frameRateMultiplier = pctx->GetNextFrameRateMultiplier();
          if (!frameRateMultiplier) {
            pctx->DidUseFrameRateMultiplier();
          }
          if (win && frameRateMultiplier) {
            dom::Performance* perf = win->GetPerformance();
            // Limit slower refresh rate to 5 seconds between the
            // first contentful paint and page load.
            if (perf && perf->Now() <
                            StaticPrefs::page_load_deprioritization_period()) {
              if (mProcessedVsync) {
                mProcessedVsync = false;
                // Handle this case similarly to the code above, but just
                // use idle queue.
                TimeDuration rate = mVsyncRefreshDriverTimer->GetTimerRate();
                uint32_t slowRate = static_cast<uint32_t>(
                    rate.ToMilliseconds() * frameRateMultiplier);
                pctx->DidUseFrameRateMultiplier();
                nsCOMPtr<nsIRunnable> vsyncEvent = NewRunnableMethod<>(
                    "RefreshDriverVsyncObserver::NormalPriorityNotify[IDLE]",
                    this, &RefreshDriverVsyncObserver::NormalPriorityNotify);
                NS_DispatchToCurrentThreadQueue(vsyncEvent.forget(), slowRate,
                                                EventQueuePriority::Idle);
              }
              return;
            }
          }
        }
      }

      RefPtr<RefreshDriverVsyncObserver> kungFuDeathGrip(this);
      TickRefreshDriver(aVsync.mId, aVsync.mTime);
    }

    void Shutdown() {
      MOZ_ASSERT(NS_IsMainThread());
      mVsyncRefreshDriverTimer = nullptr;
    }

    void OnTimerStart() { mLastTick = TimeStamp::Now(); }

    void NormalPriorityNotify() {
      if (mLastProcessedTick.IsNull() || mRecentVsync > mLastProcessedTick) {
        // mBlockUntil is for high priority vsync notifications only.
        mBlockUntil = TimeStamp();
        TickRefreshDriver(mRecentVsyncId, mRecentVsync);
      }

      mProcessedVsync = true;
    }

   private:
    ~RefreshDriverVsyncObserver() = default;

    void RecordTelemetryProbes(TimeStamp aVsyncTimestamp) {
      MOZ_ASSERT(NS_IsMainThread());
#ifndef ANDROID /* bug 1142079 */
      if (XRE_IsParentProcess()) {
        TimeDuration vsyncLatency = TimeStamp::Now() - aVsyncTimestamp;
        uint32_t sample = (uint32_t)vsyncLatency.ToMilliseconds();
        Telemetry::Accumulate(
            Telemetry::FX_REFRESH_DRIVER_CHROME_FRAME_DELAY_MS, sample);
        Telemetry::Accumulate(
            Telemetry::FX_REFRESH_DRIVER_SYNC_SCROLL_FRAME_DELAY_MS, sample);
      } else if (mVsyncRate != TimeDuration::Forever()) {
        TimeDuration contentDelay = (TimeStamp::Now() - mLastTick) - mVsyncRate;
        if (contentDelay.ToMilliseconds() < 0) {
          // Vsyncs are noisy and some can come at a rate quicker than
          // the reported hardware rate. In those cases, consider that we have 0
          // delay.
          contentDelay = TimeDuration::FromMilliseconds(0);
        }
        uint32_t sample = (uint32_t)contentDelay.ToMilliseconds();
        Telemetry::Accumulate(
            Telemetry::FX_REFRESH_DRIVER_CONTENT_FRAME_DELAY_MS, sample);
        Telemetry::Accumulate(
            Telemetry::FX_REFRESH_DRIVER_SYNC_SCROLL_FRAME_DELAY_MS, sample);
      } else {
        // Request the vsync rate from the parent process. Might be a few vsyncs
        // until the parent responds.
        if (mVsyncRefreshDriverTimer) {
          mVsyncRate = mVsyncRefreshDriverTimer->mVsyncChild->GetVsyncRate();
        }
      }
#endif
    }

    void TickRefreshDriver(VsyncId aId, TimeStamp aVsyncTimestamp) {
      MOZ_ASSERT(NS_IsMainThread());

      RecordTelemetryProbes(aVsyncTimestamp);
      mLastTick = TimeStamp::Now();
      mLastProcessedTick = aVsyncTimestamp;

      // On 32-bit Windows we sometimes get times where TimeStamp::Now() is not
      // monotonic because the underlying system apis produce non-monontonic
      // results. (bug 1306896)
#if !defined(_WIN32)
      MOZ_ASSERT(aVsyncTimestamp <= TimeStamp::Now());
#endif

      // Let also non-RefreshDriver code to run at least for awhile if we have
      // a mVsyncRefreshDriverTimer. Note, if nothing else is running,
      // RefreshDriver will still run as fast as possible, some ticks will
      // just be triggered from a normal priority runnable.
      TimeDuration timeForOutsideTick = TimeDuration::FromMilliseconds(0.0f);

      // We might have a problem that we call ~VsyncRefreshDriverTimer() before
      // the scheduled TickRefreshDriver() runs. Check mVsyncRefreshDriverTimer
      // before use.
      if (mVsyncRefreshDriverTimer) {
        timeForOutsideTick = TimeDuration::FromMilliseconds(
            mVsyncRefreshDriverTimer->GetTimerRate().ToMilliseconds() / 100.0f);
        RefPtr<VsyncRefreshDriverTimer> timer = mVsyncRefreshDriverTimer;
        timer->RunRefreshDrivers(aId, aVsyncTimestamp);
        // Note: mVsyncRefreshDriverTimer might be null now.
      }

      TimeDuration tickDuration = TimeStamp::Now() - mLastTick;
      mBlockUntil = aVsyncTimestamp + tickDuration + timeForOutsideTick;
    }

    // VsyncRefreshDriverTimer holds this RefreshDriverVsyncObserver and it will
    // be always available before Shutdown(). We can just use the raw pointer
    // here.
    VsyncRefreshDriverTimer* mVsyncRefreshDriverTimer;

    Monitor mParentProcessRefreshTickLock;
    VsyncEvent mRecentParentProcessVsync;
    bool mPendingParentProcessVsync;

    TimeStamp mRecentVsync;
    VsyncId mRecentVsyncId;
    TimeStamp mLastTick;
    TimeStamp mLastProcessedTick;
    TimeStamp mBlockUntil;
    TimeDuration mVsyncRate;
    bool mProcessedVsync;
  };  // RefreshDriverVsyncObserver

  ~VsyncRefreshDriverTimer() override {
    if (mVsyncDispatcher) {
      mVsyncDispatcher->RemoveChildRefreshTimer(mVsyncObserver);
      mVsyncDispatcher = nullptr;
    } else if (mVsyncChild) {
      mVsyncChild->RemoveChildRefreshTimer(mVsyncObserver);
      mVsyncChild = nullptr;
    }

    // Detach current vsync timer from this VsyncObserver. The observer will no
    // longer tick this timer.
    mVsyncObserver->Shutdown();
    mVsyncObserver = nullptr;
  }

  void StartTimer() override {
    MOZ_ASSERT(NS_IsMainThread());

    mLastFireTime = TimeStamp::Now();
    mLastFireId = VsyncId();

    if (mVsyncDispatcher) {
      mVsyncDispatcher->AddChildRefreshTimer(mVsyncObserver);
    } else if (mVsyncChild) {
      mVsyncChild->AddChildRefreshTimer(mVsyncObserver);
      mVsyncObserver->OnTimerStart();
    }
  }

  void StopTimer() override {
    MOZ_ASSERT(NS_IsMainThread());

    if (mVsyncDispatcher) {
      mVsyncDispatcher->RemoveChildRefreshTimer(mVsyncObserver);
    } else if (mVsyncChild) {
      mVsyncChild->RemoveChildRefreshTimer(mVsyncObserver);
    }
  }

  void ScheduleNextTick(TimeStamp aNowTime) override {
    // Do nothing since we just wait for the next vsync from
    // RefreshDriverVsyncObserver.
  }

  void RunRefreshDrivers(VsyncId aId, TimeStamp aTimeStamp) {
    Tick(aId, aTimeStamp);
    for (auto& driver : mContentRefreshDrivers) {
      driver->FinishedVsyncTick();
    }
    for (auto& driver : mRootRefreshDrivers) {
      driver->FinishedVsyncTick();
    }
  }

  // When using local vsync source, we keep a strong ref to it here to ensure
  // that the weak ref in the vsync dispatcher does not end up dangling.
  // As this is a local vsync source, it is not affected by gfxPlatform vsync
  // source reinit.
  RefPtr<gfx::VsyncSource> mVsyncSource;
  RefPtr<RefreshDriverVsyncObserver> mVsyncObserver;
  // Used for parent process.
  RefPtr<RefreshTimerVsyncDispatcher> mVsyncDispatcher;
  // Used for child process.
  // The mVsyncChild will be always available before VsncChild::ActorDestroy().
  // After ActorDestroy(), StartTimer() and StopTimer() calls will be non-op.
  RefPtr<VsyncChild> mVsyncChild;
  TimeDuration mVsyncRate;
};  // VsyncRefreshDriverTimer

/**
 * Since the content process takes some time to setup
 * the vsync IPC connection, this timer is used
 * during the intial startup process.
 * During initial startup, the refresh drivers
 * are ticked off this timer, and are swapped out once content
 * vsync IPC connection is established.
 */
class StartupRefreshDriverTimer : public SimpleTimerBasedRefreshDriverTimer {
 public:
  explicit StartupRefreshDriverTimer(double aRate)
      : SimpleTimerBasedRefreshDriverTimer(aRate) {}

 protected:
  void ScheduleNextTick(TimeStamp aNowTime) override {
    // Since this is only used for startup, it isn't super critical
    // that we tick at consistent intervals.
    TimeStamp newTarget = aNowTime + mRateDuration;
    uint32_t delay =
        static_cast<uint32_t>((newTarget - aNowTime).ToMilliseconds());
    mTimer->InitWithNamedFuncCallback(
        TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT,
        "StartupRefreshDriverTimer::ScheduleNextTick");
    mTargetTime = newTarget;
  }
};

/*
 * A RefreshDriverTimer for inactive documents.  When a new refresh driver is
 * added, the rate is reset to the base (normally 1s/1fps).  Every time
 * it ticks, a single refresh driver is poked.  Once they have all been poked,
 * the duration between ticks doubles, up to mDisableAfterMilliseconds.  At that
 * point, the timer is quiet and doesn't tick (until something is added to it
 * again).
 *
 * When a timer is removed, there is a possibility of another timer
 * being skipped for one cycle.  We could avoid this by adjusting
 * mNextDriverIndex in RemoveRefreshDriver, but there's little need to
 * add that complexity.  All we want is for inactive drivers to tick
 * at some point, but we don't care too much about how often.
 */
class InactiveRefreshDriverTimer final
    : public SimpleTimerBasedRefreshDriverTimer {
 public:
  explicit InactiveRefreshDriverTimer(double aRate)
      : SimpleTimerBasedRefreshDriverTimer(aRate),
        mNextTickDuration(aRate),
        mDisableAfterMilliseconds(-1.0),
        mNextDriverIndex(0) {}

  InactiveRefreshDriverTimer(double aRate, double aDisableAfterMilliseconds)
      : SimpleTimerBasedRefreshDriverTimer(aRate),
        mNextTickDuration(aRate),
        mDisableAfterMilliseconds(aDisableAfterMilliseconds),
        mNextDriverIndex(0) {}

  void AddRefreshDriver(nsRefreshDriver* aDriver) override {
    RefreshDriverTimer::AddRefreshDriver(aDriver);

    LOG("[%p] inactive timer got new refresh driver %p, resetting rate", this,
        aDriver);

    // reset the timer, and start with the newly added one next time.
    mNextTickDuration = mRateMilliseconds;

    // we don't really have to start with the newly added one, but we may as
    // well not tick the old ones at the fastest rate any more than we need to.
    mNextDriverIndex = GetRefreshDriverCount() - 1;

    StopTimer();
    StartTimer();
  }

  TimeDuration GetTimerRate() override {
    return TimeDuration::FromMilliseconds(mNextTickDuration);
  }

 protected:
  uint32_t GetRefreshDriverCount() {
    return mContentRefreshDrivers.Length() + mRootRefreshDrivers.Length();
  }

  void StartTimer() override {
    mLastFireTime = TimeStamp::Now();
    mLastFireId = VsyncId();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithNamedFuncCallback(TimerTickOne, this, delay,
                                      nsITimer::TYPE_ONE_SHOT,
                                      "InactiveRefreshDriverTimer::StartTimer");
  }

  void StopTimer() override { mTimer->Cancel(); }

  void ScheduleNextTick(TimeStamp aNowTime) override {
    if (mDisableAfterMilliseconds > 0.0 &&
        mNextTickDuration > mDisableAfterMilliseconds) {
      // We hit the time after which we should disable
      // inactive window refreshes; don't schedule anything
      // until we get kicked by an AddRefreshDriver call.
      return;
    }

    // double the next tick time if we've already gone through all of them once
    if (mNextDriverIndex >= GetRefreshDriverCount()) {
      mNextTickDuration *= 2.0;
      mNextDriverIndex = 0;
    }

    // this doesn't need to be precise; do a simple schedule
    uint32_t delay = static_cast<uint32_t>(mNextTickDuration);
    mTimer->InitWithNamedFuncCallback(
        TimerTickOne, this, delay, nsITimer::TYPE_ONE_SHOT,
        "InactiveRefreshDriverTimer::ScheduleNextTick");

    LOG("[%p] inactive timer next tick in %f ms [index %d/%d]", this,
        mNextTickDuration, mNextDriverIndex, GetRefreshDriverCount());
  }

  /* Runs just one driver's tick. */
  void TickOne() {
    TimeStamp now = TimeStamp::Now();

    ScheduleNextTick(now);

    mLastFireTime = now;
    mLastFireId = VsyncId();

    nsTArray<RefPtr<nsRefreshDriver>> drivers(mContentRefreshDrivers.Clone());
    drivers.AppendElements(mRootRefreshDrivers);
    size_t index = mNextDriverIndex;

    if (index < drivers.Length() &&
        !drivers[index]->IsTestControllingRefreshesEnabled()) {
      TickDriver(drivers[index], VsyncId(), now);
    }

    mNextDriverIndex++;
  }

  static void TimerTickOne(nsITimer* aTimer, void* aClosure) {
    RefPtr<InactiveRefreshDriverTimer> timer =
        static_cast<InactiveRefreshDriverTimer*>(aClosure);
    timer->TickOne();
  }

  double mNextTickDuration;
  double mDisableAfterMilliseconds;
  uint32_t mNextDriverIndex;
};

} // namespace mozilla

#undef LOG

#endif /* !defined(RefreshDriverTimerImpl_h_) */
