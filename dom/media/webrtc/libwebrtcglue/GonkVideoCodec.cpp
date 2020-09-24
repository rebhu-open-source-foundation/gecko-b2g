/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GonkVideoCodec.h"

#include "nsIGfxInfo.h"
#include "nsServiceManagerUtils.h"
#include "WebrtcGonkH264VideoCodec.h"
#include "WebrtcGonkVP8VideoCodec.h"

namespace mozilla {

WebrtcVideoEncoder* GonkVideoCodec::CreateEncoder(
    webrtc::VideoCodecType aCodecType) {
  if (!CodecEnabled(aCodecType, true) || !CodecSupported(aCodecType, true)) {
    return nullptr;
  }
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    return new WebrtcGonkH264VideoEncoder();
  }
  return nullptr;
}

WebrtcVideoDecoder* GonkVideoCodec::CreateDecoder(
    webrtc::VideoCodecType aCodecType) {
  if (!CodecEnabled(aCodecType, false) || !CodecSupported(aCodecType, false)) {
    return nullptr;
  }
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    return new WebrtcGonkH264VideoDecoder();
  } else if (aCodecType == webrtc::VideoCodecType::kVideoCodecVP8) {
    return new WebrtcGonkVP8VideoDecoder();
  }
  return nullptr;
}

bool GonkVideoCodec::CodecEnabled(webrtc::VideoCodecType aCodecType,
                                  bool aIsEncoder) {
  const char* prefName = nullptr;
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    prefName = "media.webrtc.hw.h264.enabled";
  } else if (aCodecType == webrtc::VideoCodecType::kVideoCodecVP8) {
    prefName = aIsEncoder
                   ? "media.navigator.hardware.vp8_encode.acceleration_enabled"
                   : "media.navigator.hardware.vp8_decode.acceleration_enabled";
  } else {
    return false;
  }
  return mozilla::Preferences::GetBool(prefName, false);
}

bool GonkVideoCodec::CodecSupported(webrtc::VideoCodecType aCodecType,
                                    bool aIsEncoder) {
  int32_t aFeature = 0;
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    aFeature = nsIGfxInfo::FEATURE_WEBRTC_HW_ACCELERATION_H264;
  } else if (aCodecType == webrtc::VideoCodecType::kVideoCodecVP8) {
    aFeature = aIsEncoder ? nsIGfxInfo::FEATURE_WEBRTC_HW_ACCELERATION_ENCODE
                          : nsIGfxInfo::FEATURE_WEBRTC_HW_ACCELERATION_DECODE;
  } else {
    return false;
  }

  nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
  if (!gfxInfo) {
    return false;
  }

  int32_t status;
  nsCString discardFailureId;
  nsresult err = gfxInfo->GetFeatureStatus(aFeature, discardFailureId, &status);
  return NS_SUCCEEDED(err) && status == nsIGfxInfo::FEATURE_STATUS_OK;
}

}  // namespace mozilla
