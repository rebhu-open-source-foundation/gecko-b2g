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

#ifndef _HWC2_TEST_BUFFER_H
#define _HWC2_TEST_BUFFER_H

#include <android-base/unique_fd.h>
#include <set>

#include "Hwc2TestProperties.h"

#include <ui/GraphicBuffer.h>

class Hwc2TestFenceGenerator;
class Hwc2TestLayers;

class Hwc2TestBuffer {
 public:
  Hwc2TestBuffer();
  ~Hwc2TestBuffer();

  void updateBufferArea(const Area& bufferArea);

  int get(int& outSlot, android::sp<android::GraphicBuffer>& outBuffer,
          android::sp<android::Fence>& outFence);

 protected:
  int generateBuffer();

  android::sp<android::GraphicBuffer> mGraphicBuffer;

  std::unique_ptr<Hwc2TestFenceGenerator> mFenceGenerator;

  Area mBufferArea = {-1, -1};
  const android_pixel_format_t mFormat = HAL_PIXEL_FORMAT_RGBA_8888;

  bool mValidBuffer = false;
  android::sp<android::GraphicBuffer> mBuffer;
};

class Hwc2TestClientTargetBuffer {
 public:
  Hwc2TestClientTargetBuffer();
  ~Hwc2TestClientTargetBuffer();

  int get(int& outSlot, android::sp<android::GraphicBuffer>& outBuffer,
          android::sp<android::Fence>& outFence, const Area& bufferArea,
          const Hwc2TestLayers* testLayers,
          const std::set<HWC2::Layer*>* clientLayers,
          const std::set<HWC2::Layer*>* clearLayers);

 protected:
  android::sp<android::GraphicBuffer> mGraphicBuffer;

  std::unique_ptr<Hwc2TestFenceGenerator> mFenceGenerator;

  const android_pixel_format_t mFormat = HAL_PIXEL_FORMAT_RGBA_8888;
};

class Hwc2TestVirtualBuffer {
 public:
  void updateBufferArea(const Area& bufferArea);

  bool writeBufferToFile(std::string path);

  android::sp<android::GraphicBuffer>& graphicBuffer() {
    return mGraphicBuffer;
  }

 protected:
  android::sp<android::GraphicBuffer> mGraphicBuffer;

  Area mBufferArea = {-1, -1};

  const android_pixel_format_t mFormat = HAL_PIXEL_FORMAT_RGBA_8888;
};

class Hwc2TestExpectedBuffer : public Hwc2TestVirtualBuffer {
 public:
  int generateExpectedBuffer(const Hwc2TestLayers* testLayers,
                             const std::vector<HWC2::Layer*>* allLayers,
                             const std::set<HWC2::Layer*>* clearLayers);
};

class Hwc2TestOutputBuffer : public Hwc2TestVirtualBuffer {
 public:
  int getOutputBuffer(int& outSlot,
                      android::sp<android::GraphicBuffer>& outBuffer,
                      android::sp<android::Fence>& outFence);
};

#endif /* ifndef _HWC2_TEST_BUFFER_H */
