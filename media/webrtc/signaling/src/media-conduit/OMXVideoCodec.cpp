/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "OMXVideoCodec.h"

#include "WebrtcOMXH264VideoCodec.h"
#include "WebrtcOMXVP8VideoCodec.h"

namespace mozilla {

WebrtcVideoEncoder* OMXVideoCodec::CreateEncoder(CodecType aCodecType) {
  if (aCodecType == CODEC_H264) {
    return new WebrtcOMXH264VideoEncoder();
  }
  return nullptr;
}

WebrtcVideoDecoder* OMXVideoCodec::CreateDecoder(CodecType aCodecType) {
  if (aCodecType == CODEC_H264) {
    return new WebrtcOMXH264VideoDecoder();
  } else if (aCodecType == CODEC_VP8) {
    return new WebrtcOMXVP8VideoDecoder();
  }
  return nullptr;
}

}  // namespace mozilla
