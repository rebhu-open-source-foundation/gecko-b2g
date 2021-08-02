/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_layers_CompositorScheduler_h
#define mozilla_layers_CompositorScheduler_h

#include "mozilla/layers/SampleTime.h"

namespace mozilla {

class CancelableRunnable;
class Runnable;

namespace gfx {
class DrawTarget;
} // namespace gfx

namespace layers {

class CompositorVsyncSchedulerOwner;

class CompositorScheduler {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorScheduler)
  explicit CompositorScheduler(CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner);

  virtual void ScheduleComposition() = 0;
  virtual void CancelCurrentCompositeTask() = 0;
  virtual bool NeedsComposite() = 0;
  virtual void Composite(const VsyncEvent& aVsyncEvent) = 0;
  virtual void ForceComposeToTarget(gfx::DrawTarget* aTarget,
                                    const gfx::IntRect* aRect) = 0;
  virtual void Destroy() = 0;

  static already_AddRefed<CompositorScheduler>
    Create(CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner,
           widget::CompositorWidget* aWidget);

  /**
   * Return the vsync timestamp of the last or ongoing composite. Must be called
   * on the compositor thread.
   */
  const SampleTime& GetLastComposeTime() const;

  /**
   * Return the vsync timestamp and id of the most recently received
   * vsync event. Must be called on the compositor thread.
   */
  const TimeStamp& GetLastVsyncTime() const;
  const TimeStamp& GetLastVsyncOutputTime() const;
  const VsyncId& GetLastVsyncId() const;

  /**
   * Update LastCompose TimeStamp to current timestamp.
   * The function is typically used when composition is handled outside the
   * CompositorScheduler.
   */
  void UpdateLastComposeTime();

protected:
  virtual ~CompositorScheduler();

  CompositorVsyncSchedulerOwner* mVsyncSchedulerOwner;
  SampleTime mLastComposeTime;
  TimeStamp mLastVsyncTime;
  TimeStamp mLastVsyncOutputTime;
  VsyncId mLastVsyncId;
};

/**
 * Manages the vsync (de)registration and tracking on behalf of the
 * compositor when it need to paint.
 * Turns vsync notifications into scheduled composites.
 **/
class CompositorSoftwareTimerScheduler final : public CompositorScheduler {
public:
  explicit CompositorSoftwareTimerScheduler(
    CompositorVsyncSchedulerOwner* aVsyncSchedulerOwner);

  // from CompositorScheduler
  virtual void ScheduleComposition() override;
  virtual void CancelCurrentCompositeTask() override;
  virtual bool NeedsComposite() override;
  virtual void Composite(const VsyncEvent& aVsyncEvent) override;
  virtual void ForceComposeToTarget(gfx::DrawTarget* aTarget,
                                    const gfx::IntRect* aRect) override;
  virtual void Destroy() override;

  void CallComposite();
private:
  ~CompositorSoftwareTimerScheduler();

  mozilla::Monitor mCurrentCompositeTaskMonitor;
  RefPtr<CancelableRunnable> mCurrentCompositeTask;
};

} // namespace layers
} // namespace mozilla

#endif // mozilla_layers_CompositorSoftwareTimerScheduler_h
