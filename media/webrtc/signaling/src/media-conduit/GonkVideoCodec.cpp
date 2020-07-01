/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GonkVideoCodec.h"

#include "WebrtcGonkH264VideoCodec.h"
#include "WebrtcGonkVP8VideoCodec.h"

namespace mozilla {

WebrtcVideoEncoder* GonkVideoCodec::CreateEncoder(
    webrtc::VideoCodecType aCodecType) {
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    return new WebrtcGonkH264VideoEncoder();
  }
  return nullptr;
}

WebrtcVideoDecoder* GonkVideoCodec::CreateDecoder(
    webrtc::VideoCodecType aCodecType) {
  if (aCodecType == webrtc::VideoCodecType::kVideoCodecH264) {
    return new WebrtcGonkH264VideoDecoder();
  } else if (aCodecType == webrtc::VideoCodecType::kVideoCodecVP8) {
    return new WebrtcGonkVP8VideoDecoder();
  }
  return nullptr;
}

}  // namespace mozilla
