/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcOMXVP8VideoCodec.h"

#include <media/stagefright/MediaDefs.h>

#include "OMXCodecWrapper.h"
#include "webrtc/modules/video_coding/include/video_error_codes.h"
#include "WebrtcMediaCodecWrapper.h"

using android::OMXCodecReservation;

namespace mozilla {

// Decoder.
WebrtcOMXVP8VideoDecoder::WebrtcOMXVP8VideoDecoder() {
  mReservation = new OMXCodecReservation(false);
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be constructed", this);
}

int32_t WebrtcOMXVP8VideoDecoder::InitDecode(
    const webrtc::VideoCodec* aCodecSettings, int32_t aNumOfCores) {
  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGE("WebrtcOMXVP8VideoDecoder:%p decoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  mOMX = new WebrtcOMXDecoder(android::MEDIA_MIMETYPE_VIDEO_VP8);
  if (mOMX->ConfigureWithPicDimensions(aCodecSettings->width,
                                       aCodecSettings->height) != android::OK) {
    mOMX = nullptr;
    CODEC_LOGE("WebrtcOMXVP8VideoDecoder:%p decoder not started", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p decoder started", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcOMXVP8VideoDecoder::Decode(
    const webrtc::EncodedImage& aInputImage, bool aMissingFrames,
    const webrtc::RTPFragmentationHeader* aFragmentation,
    const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
    int64_t aRenderTimeMs) {
  if (aInputImage._length == 0 || !aInputImage._buffer) {
    CODEC_LOGE("WebrtcOMXVP8VideoDecoder:%p empty input data", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (mOMX->FillInput(aInputImage, false, aRenderTimeMs) != android::OK) {
    CODEC_LOGE("WebrtcOMXVP8VideoDecoder:%p error sending input data", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcOMXVP8VideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* aCallback) {
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(mOMX);
  mOMX->SetDecodedCallback(aCallback);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcOMXVP8VideoDecoder::Release() {
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be released", this);

  mOMX = nullptr;  // calls Stop()
  mReservation->ReleaseOMXCodec();
  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcOMXVP8VideoDecoder::~WebrtcOMXVP8VideoDecoder() {
  CODEC_LOGD("WebrtcOMXVP8VideoDecoder:%p will be destructed", this);
  Release();
}

}  // namespace mozilla
