/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcGonkVP8VideoCodec.h"

#include <media/stagefright/MediaDefs.h>

#include "common/browser_logging/CSFLog.h"
#include "OMXCodecWrapper.h"
#include "webrtc/modules/video_coding/include/video_error_codes.h"
#include "WebrtcGonkVideoCodec.h"

#undef LOG_TAG
#undef LOGE
#undef LOGW
#undef LOGI
#undef LOGD
#undef LOGV

#define LOG_TAG "Gonk"
#define LOGE(...) CSFLogError(LOG_TAG, __VA_ARGS__)
#define LOGW(...) CSFLogWarn(LOG_TAG, __VA_ARGS__)
#define LOGI(...) CSFLogInfo(LOG_TAG, __VA_ARGS__)
#define LOGD(...) CSFLogDebug(LOG_TAG, __VA_ARGS__)
#define LOGV(...) CSFLogVerbose(LOG_TAG, __VA_ARGS__)

namespace mozilla {

// Decoder.
WebrtcGonkVP8VideoDecoder::WebrtcGonkVP8VideoDecoder() {
  LOGD("Decoder:%p constructor", this);
  mReservation = new android::OMXCodecReservation(false);
}

int32_t WebrtcGonkVP8VideoDecoder::InitDecode(
    const webrtc::VideoCodec* aCodecSettings, int32_t aNumOfCores) {
  LOGD("Decoder:%p initializing", this);

  if (!mReservation->ReserveOMXCodec()) {
    LOGE("Decoder:%p failed to reserve codec", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  mDecoder = new android::WebrtcGonkVideoDecoder();
  if (mDecoder->Init(this, android::MEDIA_MIMETYPE_VIDEO_VP8,
                     aCodecSettings->width,
                     aCodecSettings->height) != android::OK) {
    mDecoder = nullptr;
    LOGE("Decoder:%p failed to initialize", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkVP8VideoDecoder::Decode(
    const webrtc::EncodedImage& aInputImage, bool aMissingFrames,
    const webrtc::RTPFragmentationHeader* aFragmentation,
    const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
    int64_t aRenderTimeMs) {
  if (aInputImage._length == 0 || !aInputImage._buffer) {
    LOGW("Decoder:%p empty input data, dropping", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (mDecoder->Decode(aInputImage, false, aRenderTimeMs) != android::OK) {
    LOGE("Decoder:%p failed to decode", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkVP8VideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* aCallback) {
  LOGD("Decoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkVP8VideoDecoder::Release() {
  LOGD("Decoder:%p releasing", this);

  if (mDecoder) {
    mDecoder->Release();
    mDecoder = nullptr;
  }
  mReservation->ReleaseOMXCodec();
  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcGonkVP8VideoDecoder::~WebrtcGonkVP8VideoDecoder() {
  LOGD("Decoder:%p destructor", this);
  Release();
}

}  // namespace mozilla
