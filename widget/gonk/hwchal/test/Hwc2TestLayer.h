/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _HWC2_TEST_LAYER_H
#define _HWC2_TEST_LAYER_H

#include <android-base/unique_fd.h>
#include <unordered_map>

#include "Hwc2TestBuffer.h"
#include "Hwc2TestProperties.h"

class Hwc2TestLayer {
 public:
  Hwc2TestLayer(Hwc2TestCoverage coverage, const Area& displayArea);

  Hwc2TestLayer(
      Hwc2TestCoverage coverage, const Area& displayArea,
      const std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage>&
          coverage_exceptions);

  std::string dump() const;

  int getBuffer(int& outSlot, android::sp<android::GraphicBuffer>& outBuffer,
                android::sp<android::Fence>& outAcquireFence);

  void setZOrder(uint32_t zOrder);
  void setVisibleRegion(const android::Region& region);

  void reset();
  bool advance();

  HWC2::BlendMode getBlendMode() const;
  Area getBufferArea() const;
  hwc_color_t getColor() const;
  HWC2::Composition getComposition() const;
  android::Rect getCursorPosition() const;
  android::ui::Dataspace getDataspace() const;
  android::Rect getDisplayFrame() const;
  float getPlaneAlpha() const;
  android::FloatRect getSourceCrop() const;
  android::Region getSurfaceDamage() const;
  HWC2::Transform getTransform() const;
  android::Region getVisibleRegion() const;
  uint32_t getZOrder() const;

  bool advanceBlendMode();
  bool advanceBufferArea();
  bool advanceColor();
  bool advanceComposition();
  bool advanceCursorPosition();
  bool advanceDataspace();
  bool advanceDisplayFrame();
  bool advancePlaneAlpha();
  bool advanceSourceCrop();
  bool advanceSurfaceDamage();
  bool advanceTransform();
  bool advanceVisibleRegion();

 private:
  std::array<Hwc2TestContainer*, 10> mProperties = {
      {&mTransform, &mColor, &mDataspace, &mPlaneAlpha, &mSourceCrop,
       &mSurfaceDamage, &mBlendMode, &mBufferArea, &mDisplayFrame,
       &mComposition}};

  Hwc2TestBuffer mBuffer;

  Hwc2TestBlendMode mBlendMode;
  Hwc2TestBufferArea mBufferArea;
  Hwc2TestColor mColor;
  Hwc2TestComposition mComposition;
  Hwc2TestDataspace mDataspace;
  Hwc2TestDisplayFrame mDisplayFrame;
  Hwc2TestPlaneAlpha mPlaneAlpha;
  Hwc2TestSourceCrop mSourceCrop;
  Hwc2TestSurfaceDamage mSurfaceDamage;
  Hwc2TestTransform mTransform;
  Hwc2TestVisibleRegion mVisibleRegion;

  uint32_t mZOrder = UINT32_MAX;
};

#endif /* ifndef _HWC2_TEST_LAYER_H */
