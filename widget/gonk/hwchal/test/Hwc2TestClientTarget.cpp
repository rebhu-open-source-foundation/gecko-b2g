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

#include <sstream>

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>

#include "Hwc2TestClientTarget.h"

int Hwc2TestClientTarget::GetBuffer(
    const Hwc2TestLayers& testLayers,
    const std::set<HWC2::Layer*>& clientLayers,
    const std::set<HWC2::Layer*>& clearLayers, bool flipClientTarget,
    const Area& displayArea, int& outSlot,
    android::sp<android::GraphicBuffer>& outBuffer,
    android::sp<android::Fence>& outAcquireFence) {
  if (!flipClientTarget) {
    bool needsClientTarget = false;

    for (auto clientLayer : clientLayers) {
      size_t size;
      auto rects = testLayers.GetVisibleRegion(clientLayer).getArray(&size);
      if (rects && size > 0) {
        needsClientTarget = true;
        break;
      }
    }

    if (!needsClientTarget) {
      outAcquireFence = android::Fence::NO_FENCE;
      outBuffer = nullptr;
      outSlot = -1;
      return 0;
    }
  }

  return mBuffer.get(outSlot, outBuffer, outAcquireFence, displayArea,
                     &testLayers, &clientLayers, &clearLayers);
}

Hwc2TestClientTargetSupport::Hwc2TestClientTargetSupport(
    Hwc2TestCoverage coverage, const Area& displayArea)
    : mBufferArea(coverage, displayArea),
      mDataspace(coverage),
      mSurfaceDamage(coverage) {
  mBufferArea.setDependent(&mSurfaceDamage);
}

std::string Hwc2TestClientTargetSupport::dump() const {
  std::stringstream dmp;

  dmp << "client target: \n";

  for (auto property : properties) {
    dmp << property->dump();
  }

  return dmp.str();
}

void Hwc2TestClientTargetSupport::reset() {
  for (auto property : properties) {
    property->reset();
  }
}

bool Hwc2TestClientTargetSupport::advance() {
  for (auto property : properties) {
    if (property->advance()) return true;
  }
  return false;
}

Area Hwc2TestClientTargetSupport::getBufferArea() const {
  return mBufferArea.get();
}

android::ui::Dataspace Hwc2TestClientTargetSupport::getDataspace() const {
  return mDataspace.get();
}

const android::Region Hwc2TestClientTargetSupport::getSurfaceDamage() const {
  return mSurfaceDamage.get();
}
