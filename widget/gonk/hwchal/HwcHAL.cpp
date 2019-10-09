/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim:set ts=4 sw=4 sts=4 et: */
/*
 * Copyright (c) 2015 The Linux Foundation. All rights reserved.
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

#include "HwcHAL.h"
#include "libdisplay/GonkDisplay.h"
#include "mozilla/Assertions.h"
#include "nsIScreen.h"
#include <dlfcn.h>

#if ANDROID_VERSION >= 26
typedef android::GonkDisplay GonkDisplay;
extern GonkDisplay * GetGonkDisplay();

typedef HWC2::Display* (*fnGetDisplayById)(HWC2::Device *, hwc2_display_t);
HWC2::Display* hwc2_getDisplayById(HWC2::Device *p, hwc2_display_t id) {
  HWC2::Display *display = nullptr;
  void* lib = dlopen(SYSTEM_LIB_DIR "libcarthage.so", RTLD_NOW);
  if (lib == nullptr) {
    ALOGE("Could not dlopen(\"libcarthage.so\"):");
    return display;
  }

  fnGetDisplayById func = (fnGetDisplayById) dlsym(lib, "hwc2_getDisplayById") ;
  if (func == nullptr) {
    ALOGE("Symbol 'hwc2_getDisplayById' is missing from shared library!!\n");
    return display;
  }

  display = func(p, id);
  return display;
}

typedef HWC2::Error (*fnSetVsyncEnabled)(HWC2::Display *, HWC2::Vsync);
HWC2::Error hwc2_setVsyncEnabled(HWC2::Display *p, HWC2::Vsync enabled) {
  HWC2::Error err = HWC2::Error::None;
  void* lib = dlopen(SYSTEM_LIB_DIR "libcarthage.so", RTLD_NOW);
  if (lib == nullptr) {
    ALOGE("Could not dlopen(\"libcarthage.so\"):");
    return HWC2::Error::BadDisplay;
  }

  fnSetVsyncEnabled func = (fnSetVsyncEnabled) dlsym(lib, "hwc2_setVsyncEnabled") ;
  if (func == nullptr) {
    ALOGE("Symbol 'hwc2_setVsyncEnabled' is missing from shared library!!\n");
    return HWC2::Error::BadDisplay;
  }

  err = func(p, enabled);
  return err;
}
#endif

namespace mozilla {

HwcHAL::HwcHAL()
    : HwcHALBase()
{
    // Some HALs don't want to open hwc twice.
    // If GetDisplay already load hwc module, we don't need to load again
    mHwc = (HwcDevice*)GetGonkDisplay()->GetHWCDevice();

    if (!mHwc) {
        printf_stderr("HwcHAL Error: Cannot load hwcomposer");
        return;
    }
}

HwcHAL::~HwcHAL()
{
    mHwc = nullptr;
}

bool
HwcHAL::Query(QueryType aType)
{
#if ANDROID_VERSION >= 26
    return false;
#else
    if (!mHwc || !mHwc->query) {
        return false;
    }

    bool value = false;
    int supported = 0;
    if (mHwc->query(mHwc, static_cast<int>(aType), &supported) == 0/*android::NO_ERROR*/) {
        value = !!supported;
    }
    return value;
#endif
}

int
HwcHAL::Set(HwcList *aList,
            uint32_t aDisp)
{
#if ANDROID_VERSION >= 26
    return -1;
#else
    MOZ_ASSERT(mHwc);
    if (!mHwc) {
        return -1;
    }

    HwcList *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[aDisp] = aList;
    return mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
#endif
}

int
HwcHAL::ResetHwc()
{
    return Set(nullptr, HWC_DISPLAY_PRIMARY);
}

int
HwcHAL::Prepare(HwcList *aList,
                uint32_t aDisp,
                hwc_rect_t aDispRect,
                buffer_handle_t aHandle,
                int aFenceFd)
{
#if ANDROID_VERSION >= 26
    return -1;
#else
    MOZ_ASSERT(mHwc);
    if (!mHwc) {
        printf_stderr("HwcHAL Error: HwcDevice doesn't exist. A fence might be leaked.");
        return -1;
    }

    HwcList *displays[HWC_NUM_DISPLAY_TYPES] = { nullptr };
    displays[aDisp] = aList;
#if ANDROID_VERSION >= 18
    aList->outbufAcquireFenceFd = -1;
    aList->outbuf = nullptr;
#endif
    aList->retireFenceFd = -1;

    const auto idx = aList->numHwLayers - 1;
    aList->hwLayers[idx].hints = 0;
    aList->hwLayers[idx].flags = 0;
    aList->hwLayers[idx].transform = 0;
    aList->hwLayers[idx].handle = aHandle;
    aList->hwLayers[idx].blending = HWC_BLENDING_PREMULT;
    aList->hwLayers[idx].compositionType = HWC_FRAMEBUFFER_TARGET;
    SetCrop(aList->hwLayers[idx], aDispRect);
    aList->hwLayers[idx].displayFrame = aDispRect;
    aList->hwLayers[idx].visibleRegionScreen.numRects = 1;
    aList->hwLayers[idx].visibleRegionScreen.rects = &aList->hwLayers[idx].displayFrame;
    aList->hwLayers[idx].acquireFenceFd = aFenceFd;
    aList->hwLayers[idx].releaseFenceFd = -1;
#if ANDROID_VERSION >= 18
    aList->hwLayers[idx].planeAlpha = 0xFF;
#endif
    return mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
#endif
}

bool
HwcHAL::SupportTransparency() const
{
#if ANDROID_VERSION >= 18
    return true;
#else
    return false;
#endif
}

uint32_t
HwcHAL::GetGeometryChangedFlag(bool aGeometryChanged) const
{
#if ANDROID_VERSION >= 19
    return aGeometryChanged ? HWC_GEOMETRY_CHANGED : 0;
#else
    return HWC_GEOMETRY_CHANGED;
#endif
}

void
HwcHAL::SetCrop(HwcLayer &aLayer,
                const hwc_rect_t &aSrcCrop) const
{
    if (GetAPIVersion() >= HwcAPIVersion(1, 3)) {
#if ANDROID_VERSION >= 19
        aLayer.sourceCropf.left = aSrcCrop.left;
        aLayer.sourceCropf.top = aSrcCrop.top;
        aLayer.sourceCropf.right = aSrcCrop.right;
        aLayer.sourceCropf.bottom = aSrcCrop.bottom;
#endif
    } else {
        aLayer.sourceCrop = aSrcCrop;
    }
}

bool
HwcHAL::EnableVsync(bool aEnable)
{
    if (!mHwc) {
        printf_stderr("Failed to get hwc\n");
        return false;
    }
#if ANDROID_VERSION >= 26
    HWC2::Display *hwcDisplay = hwc2_getDisplayById(mHwc, HWC_DISPLAY_PRIMARY);
    auto error = hwc2_setVsyncEnabled(hwcDisplay, aEnable? HWC2::Vsync::Enable : HWC2::Vsync::Disable);
    if (error != HWC2::Error::None) {
        printf_stderr("setVsyncEnabled: Failed to set vsync to %d on %d/%" PRIu64
                ": %s (%d)", aEnable, HWC_DISPLAY_PRIMARY,
                hwcDisplay->getId(), to_string(error).c_str(),
                static_cast<int32_t>(error));
        return false;
    }
    return true;
#elif (ANDROID_VERSION == 19 || ANDROID_VERSION >= 21)
    return !mHwc->eventControl(mHwc,
                               HWC_DISPLAY_PRIMARY,
                               HWC_EVENT_VSYNC,
                               aEnable);
#else
    // Only support hardware vsync on kitkat, L and up due to inaccurate timings
    // with JellyBean.
    return false;
#endif
}

bool
HwcHAL::RegisterHwcEventCallback(const HwcHALProcs_t &aProcs)
{
    if (!mHwc) {
        printf_stderr("Failed to get hwc\n");
        return false;
    }
#if ANDROID_VERSION >= 26
    EnableVsync(false);

    // Register Vsync and Invalidate Callback only
    GetGonkDisplay()->registerVsyncCallBack(aProcs.vsync);
    GetGonkDisplay()->registerInvalidateCallBack(aProcs.invalidate);

    return true;
#elif (ANDROID_VERSION == 19 || ANDROID_VERSION >= 21)
    // Disable Vsync first, and then register callback functions.
    mHwc->eventControl(mHwc,
                       HWC_DISPLAY_PRIMARY,
                       HWC_EVENT_VSYNC,
                       false);
    static const hwc_procs_t sHwcJBProcs = {aProcs.invalidate,
                                            aProcs.vsync,
                                            aProcs.hotplug};
    mHwc->registerProcs(mHwc, &sHwcJBProcs);

    return true;
#else
    // Only support hardware vsync on kitkat, L and up due to inaccurate timings
    // with JellyBean.
    return false;
#endif
}

uint32_t
HwcHAL::GetAPIVersion() const
{
#if ANDROID_VERSION >= 26
    return HWC_DEVICE_API_VERSION_2_0;
#else
    if (!mHwc) {
        // default value: HWC_MODULE_API_VERSION_0_1
        return 1;
    }
    return mHwc->common.version;
#endif
}

// Create HwcHAL
UniquePtr<HwcHALBase>
HwcHALBase::CreateHwcHAL()
{
    return MakeUnique<HwcHAL>();
}

} // namespace mozilla
