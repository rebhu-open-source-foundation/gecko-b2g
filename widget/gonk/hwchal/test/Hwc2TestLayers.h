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

#ifndef _HWC2_TEST_LAYERS_H
#define _HWC2_TEST_LAYERS_H

#include <map>

#include <HWC2.h>
#include <ComposerHal.h>

#include "Hwc2TestProperties.h"
#include "Hwc2TestLayer.h"

class Hwc2TestLayers {
 public:
  Hwc2TestLayers(const std::vector<HWC2::Layer*>& layers,
                 Hwc2TestCoverage coverage, const Area& displayArea);

  Hwc2TestLayers(
      const std::vector<HWC2::Layer*>& layers, Hwc2TestCoverage coverage,
      const Area& displayArea,
      const std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage>&
          coverageExceptions);

  std::string Dump() const;

  void Reset();

  bool Advance();
  bool AdvanceVisibleRegions();

  /* Test cases with multiple layers and property values can take quite some
   * time to run. A significant amount of time can be spent on test cases
   * where one layer is changing property values but is not visible. To
   * decrease runtime, this function can be called. Removes layouts where a
   * layer is completely blocked from view. It also removes layouts that do
   * not cover the entire display.*/
  bool OptimizeLayouts();

  bool Contains(HWC2::Layer* aLayer) const;

  int GetBuffer(HWC2::Layer* aLayer, int& outSlot,
                android::sp<android::GraphicBuffer>& outBuffer,
                android::sp<android::Fence>& outAcquireFence);

  HWC2::BlendMode GetBlendMode(HWC2::Layer* aLayer) const;
  Area GetBufferArea(HWC2::Layer* aLayer) const;
  hwc_color_t GetColor(HWC2::Layer* aLayer) const;
  HWC2::Composition GetComposition(HWC2::Layer* aLayer) const;
  android::Rect GetCursorPosition(HWC2::Layer* aLayer) const;
  android::ui::Dataspace GetDataspace(HWC2::Layer* aLayer) const;
  android::Rect GetDisplayFrame(HWC2::Layer* aLayer) const;
  android_pixel_format_t GetFormat(HWC2::Layer* aLayer) const;
  float GetPlaneAlpha(HWC2::Layer* aLayer) const;
  android::FloatRect GetSourceCrop(HWC2::Layer* aLayer) const;
  android::Region GetSurfaceDamage(HWC2::Layer* aLayer) const;
  HWC2::Transform GetTransform(HWC2::Layer* aLayer) const;
  android::Region GetVisibleRegion(HWC2::Layer* aLayer) const;
  uint32_t GetZOrder(HWC2::Layer* aLayer) const;

 private:
  bool SetVisibleRegions();

  std::map<HWC2::Layer*, Hwc2TestLayer> mTestLayers;

  Area mDisplayArea;

  bool mOptimize = false;
};

#endif /* ifndef _HWC2_TEST_LAYERS_H */
