
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_PanSimulator_h
#define mozilla_dom_PanSimulator_h

#include "nsIDOMEventListener.h"
#include "nsISupportsImpl.h"

class nsITimer;
class nsIDOMWindowUtils;

namespace mozilla {
namespace dom {

class PanSimulator final : public nsIDOMEventListener,
                           public nsITimerCallback,
                           public nsINamed {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSINAMED

  enum {
    eTouchStart = 0,
    eTouchMove,
    eTouchEnd,

    eTouchMax
  };

  explicit PanSimulator(nsPIDOMWindowOuter* aWindow,
                        nsIDOMWindowUtils* aWinUtils);
  bool IsPanning() const { return mIsPanning; }
  void Start(nsIntPoint aPoint);
  void Finish();

 private:
  ~PanSimulator() = default;
  void FireTouchEvent(int aType, nsIntPoint aPoint);
  nsIntPoint CalculatePointPosition();

  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsPIDOMWindowOuter> mWindow;
  nsCOMPtr<nsIDOMWindowUtils> mWinUtils;

  // Simulating touch by keyboard events or not.
  bool mIsPanning;

  // In order to simulate panning, we need to increase each position of touch
  // move event. This variable will be increased after each move event fired, so
  // the position can be calculated by this factor. Note: It will be reset when
  // the new panning is simulating.
  int mRepeatTimes;
  // To record which direction key trigerred this simulation.
  uint32_t mKeyCode;
  // The start point of the panning.
  nsIntPoint mPoint;
  // Unit is ms.
  uint32_t mTouchInterval;
  // A static offset value when simulating touch-pan.
  float mPointOffset;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_PanSimulator_h
