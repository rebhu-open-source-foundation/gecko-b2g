/* * Copyright (C) 2016 The Android Open Source Project
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

#include <sstream>
#include <gtest/gtest.h>

#include "Hwc2TestLayers.h"

Hwc2TestLayers::Hwc2TestLayers(const std::vector<HWC2::Layer*>& layers,
                               Hwc2TestCoverage coverage,
                               const Area& displayArea)
    : Hwc2TestLayers(
          layers, coverage, displayArea,
          std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage>()) {}

Hwc2TestLayers::Hwc2TestLayers(
    const std::vector<HWC2::Layer*>& layers, Hwc2TestCoverage coverage,
    const Area& displayArea,
    const std::unordered_map<Hwc2TestPropertyName, Hwc2TestCoverage>&
        coverageExceptions)
    : mDisplayArea(displayArea) {
  for (auto layer : layers) {
    mTestLayers.emplace(
        std::piecewise_construct, std::forward_as_tuple(layer),
        std::forward_as_tuple(coverage, displayArea, coverageExceptions));
  }

  /* Iterate over the layers in order and assign z orders in the same order.
   * This allows us to iterate over z orders in the same way when computing
   * visible regions */
  uint32_t nextZOrder = layers.size();

  for (auto& testLayer : mTestLayers) {
    testLayer.second.setZOrder(nextZOrder--);
  }

  SetVisibleRegions();
}

std::string Hwc2TestLayers::Dump() const {
  std::stringstream dmp;
  for (auto& testLayer : mTestLayers) {
    dmp << testLayer.second.dump();
  }
  return dmp.str();
}

void Hwc2TestLayers::Reset() {
  for (auto& testLayer : mTestLayers) {
    testLayer.second.reset();
  }

  SetVisibleRegions();
}

bool Hwc2TestLayers::Advance() {
  auto itr = mTestLayers.begin();
  bool optimized;

  while (itr != mTestLayers.end()) {
    if (itr->second.advance()) {
      optimized = SetVisibleRegions();
      if (!mOptimize || optimized) return true;
      itr = mTestLayers.begin();
    } else {
      itr->second.reset();
      ++itr;
    }
  }
  return false;
}

bool Hwc2TestLayers::AdvanceVisibleRegions() {
  auto itr = mTestLayers.begin();
  bool optimized;

  while (itr != mTestLayers.end()) {
    if (itr->second.advanceVisibleRegion()) {
      optimized = SetVisibleRegions();
      if (!mOptimize || optimized) return true;
      itr = mTestLayers.begin();
    } else {
      itr->second.reset();
      ++itr;
    }
  }
  return false;
}

/* Removes layouts that do not cover the entire display.
 * Also removes layouts where a layer is completely blocked from view.
 */
bool Hwc2TestLayers::OptimizeLayouts() {
  mOptimize = true;

  if (SetVisibleRegions()) return true;
  return Advance();
}

bool Hwc2TestLayers::Contains(HWC2::Layer* layer) const {
  return mTestLayers.count(layer) != 0;
}

int Hwc2TestLayers::GetBuffer(HWC2::Layer* layer, int& outSlot,
                              android::sp<android::GraphicBuffer>& outBuffer,
                              android::sp<android::Fence>& outAcquireFence) {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getBuffer(outSlot, outBuffer, outAcquireFence);
}

HWC2::BlendMode Hwc2TestLayers::GetBlendMode(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getBlendMode();
}

Area Hwc2TestLayers::GetBufferArea(HWC2::Layer* layer) const {
  auto testLayer = mTestLayers.find(layer);
  if (testLayer == mTestLayers.end()) []() { GTEST_FAIL(); }();
  return testLayer->second.getBufferArea();
}

hwc_color_t Hwc2TestLayers::GetColor(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getColor();
}

HWC2::Composition Hwc2TestLayers::GetComposition(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getComposition();
}

android::Rect Hwc2TestLayers::GetCursorPosition(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getCursorPosition();
}

android::ui::Dataspace Hwc2TestLayers::GetDataspace(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getDataspace();
}

android::Rect Hwc2TestLayers::GetDisplayFrame(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getDisplayFrame();
}

float Hwc2TestLayers::GetPlaneAlpha(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getPlaneAlpha();
}

android::FloatRect Hwc2TestLayers::GetSourceCrop(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getSourceCrop();
}

android::Region Hwc2TestLayers::GetSurfaceDamage(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getSurfaceDamage();
}

HWC2::Transform Hwc2TestLayers::GetTransform(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getTransform();
}

android::Region Hwc2TestLayers::GetVisibleRegion(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getVisibleRegion();
}

uint32_t Hwc2TestLayers::GetZOrder(HWC2::Layer* layer) const {
  if (mTestLayers.count(layer) == 0) {
    []() { GTEST_FAIL(); }();
  }
  return mTestLayers.at(layer).getZOrder();
}

/* Sets the visible regions for a display. Returns false if the layers do not
 * cover the entire display or if a layer is not visible */
bool Hwc2TestLayers::SetVisibleRegions() {
  /* The region of the display that is covered by layers above the current
   * layer */
  android::Region aboveOpaqueLayers;

  bool optimized = true;

  /* Iterate over test layers from max z order to min z order. */
  for (auto& testLayer : mTestLayers) {
    android::Region visibleRegion;

    /* Set the visible region of this layer */
    const android::Rect displayFrame = testLayer.second.getDisplayFrame();

    visibleRegion.set(android::Rect(displayFrame.left, displayFrame.top,
                                    displayFrame.right, displayFrame.bottom));

    /* Remove the area covered by opaque layers above this layer
     * from this layer's visible region */
    visibleRegion.subtractSelf(aboveOpaqueLayers);

    testLayer.second.setVisibleRegion(visibleRegion);

    /* If a layer is not visible, return false */
    if (visibleRegion.isEmpty()) optimized = false;

    /* If this layer is opaque, store the region it covers */
    if (testLayer.second.getPlaneAlpha() == 1.0f)
      aboveOpaqueLayers.orSelf(visibleRegion);
  }

  /* If the opaque region does not cover the entire display return false */
  if (!aboveOpaqueLayers.isRect()) return false;

  const auto rect = aboveOpaqueLayers.begin();
  if (rect->left != 0 || rect->top != 0 || rect->right != mDisplayArea.width ||
      rect->bottom != mDisplayArea.height)
    return false;

  return optimized;
}
