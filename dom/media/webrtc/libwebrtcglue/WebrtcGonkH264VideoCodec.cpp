/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcGonkH264VideoCodec.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/avc_utils.h>
#include <media/stagefright/MediaDefs.h>
#include <OMX_Component.h>

#include "OMXCodecWrapper.h"
#include "webrtc/modules/video_coding/include/video_error_codes.h"
#include "WebrtcGonkVideoCodec.h"

using android::ABuffer;
using android::OMXCodecReservation;
using android::sp;

namespace mozilla {

// Encoder.
WebrtcGonkH264VideoEncoder::WebrtcGonkH264VideoEncoder() {
  mReservation = new OMXCodecReservation(true);
  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p constructed", this);
}

int32_t WebrtcGonkH264VideoEncoder::InitEncode(
    const webrtc::VideoCodec* aCodecSettings, int32_t aNumOfCores,
    size_t aMaxPayloadSize) {
  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p init", this);

  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p encoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  mEncoder = new android::WebrtcGonkVideoEncoder();
  if (mEncoder->Init(this, android::MEDIA_MIMETYPE_VIDEO_AVC) != android::OK) {
    mEncoder = nullptr;
    CODEC_LOGE("WebrtcGonkH264VideoEncoder:%p encoder not initialized", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  // Defer configuration until 1st frame is received because this function will
  // be called more than once, and unfortunately with incorrect setting values
  // at first.
  mWidth = aCodecSettings->width;
  mHeight = aCodecSettings->height;
  mFrameRate = aCodecSettings->maxFramerate;
  mBitRateKbps = aCodecSettings->startBitrate;
  // XXX handle maxpayloadsize (aka mode 0/1)

  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p encoder initialized", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkH264VideoEncoder::Encode(
    const webrtc::VideoFrame& aInputImage,
    const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
    const std::vector<webrtc::FrameType>* aFrameTypes) {
  MOZ_ASSERT(mEncoder);

  // Have to reconfigure for resolution or framerate changes :-(
  // ~220ms initial configure on 8x10, 50-100ms for re-configure it appears
  // XXX drop frames while this is happening?
  if (aInputImage.width() < 0 || (uint32_t)aInputImage.width() != mWidth ||
      aInputImage.height() < 0 || (uint32_t)aInputImage.height() != mHeight) {
    mWidth = aInputImage.width();
    mHeight = aInputImage.height();
    mOMXReconfigure = true;
  }

  if (!mOMXConfigured || mOMXReconfigure) {
    if (mOMXConfigured) {
      CODEC_LOGD(
          "WebrtcGonkH264VideoEncoder:%p reconfiguring encoder %dx%d @ %u fps",
          this, mWidth, mHeight, mFrameRate);
      mOMXConfigured = false;
    }
    mOMXReconfigure = false;
    // XXX This can take time.  Encode() likely assumes encodes are queued
    // "quickly" and don't block the input too long.  Frames may build up.

    // XXX take from negotiated SDP in codecSpecific data
    OMX_VIDEO_AVCLEVELTYPE level = OMX_VIDEO_AVCLevel3;
    OMX_VIDEO_CONTROLRATETYPE bitrateMode = OMX_Video_ControlRateConstant;

    // Set up configuration parameters for AVC/H.264 encoder.
    sp<android::AMessage> format = new android::AMessage;
    // Fixed values
    format->setString("mime", android::MEDIA_MIMETYPE_VIDEO_AVC);
    // XXX We should only set to < infinity if we're not using any recovery RTCP
    // options However, we MUST set it to a lower value because the 8x10 rate
    // controller only changes rate at GOP boundaries.... but it also changes
    // rate on requested GOPs

    // Too long and we have very low bitrates for the first second or two...
    // plus bug 1014921 means we have to force them every ~3 seconds or less.
    format->setInt32("i-frame-interval", 4 /* seconds */);
    // See mozilla::layers::GrallocImage, supports YUV 4:2:0, CbCr width and
    // height is half that of Y
    format->setInt32("color-format", OMX_COLOR_FormatYUV420SemiPlanar);
    format->setInt32("profile", OMX_VIDEO_AVCProfileBaseline);
    format->setInt32("level", level);
    format->setInt32("bitrate-mode", bitrateMode);
    format->setInt32("store-metadata-in-buffers", 0);
    // XXX Unfortunately, 8x10 doesn't support this, but ask anyways
    format->setInt32("prepend-sps-pps-to-idr-frames", 1);
    // Input values.
    format->setInt32("width", mWidth);
    format->setInt32("height", mHeight);
    format->setInt32("stride", mWidth);
    format->setInt32("slice-height", mHeight);
    format->setInt32("frame-rate", mFrameRate);
    format->setInt32("bitrate", mBitRateKbps * 1000);

    CODEC_LOGD(
        "WebrtcGonkH264VideoEncoder:%p configuring encoder %dx%d @ %d fps, "
        "rate %d kbps",
        this, mWidth, mHeight, mFrameRate, mBitRateKbps);
    if (mEncoder->Configure(format) != android::OK) {
      CODEC_LOGE("WebrtcGonkH264VideoEncoder:%p FAILED configuring encoder",
                 this);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mOMXConfigured = true;
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
#endif
  }

  if (aFrameTypes && aFrameTypes->size() &&
      ((*aFrameTypes)[0] == webrtc::kVideoFrameKey)) {
    mEncoder->RequestIDRFrame();
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
    mLastIDRTime = TimeStamp::Now();
    mBitRateAtLastIDR = mBitRateKbps;
  } else if (mBitRateKbps != mBitRateAtLastIDR) {
    // 8x10 OMX codec requires a keyframe to shift bitrates!
    TimeStamp now = TimeStamp::Now();
    if (mLastIDRTime.IsNull()) {
      // paranoia
      mLastIDRTime = now;
    }
    int32_t timeSinceLastIDR = (now - mLastIDRTime).ToMilliseconds();

    // Balance asking for IDRs too often against direction and amount of bitrate
    // change.

    // HACK for bug 1014921: 8x10 has encode/decode mismatches that build up
    // errors if you go too long without an IDR.  In normal use, bitrate will
    // change often enough to never hit this time limit.
    if ((timeSinceLastIDR > 3000) ||
        (mBitRateKbps < (mBitRateAtLastIDR * 8) / 10) ||
        (timeSinceLastIDR < 300 &&
         mBitRateKbps < (mBitRateAtLastIDR * 9) / 10) ||
        (timeSinceLastIDR < 1000 &&
         mBitRateKbps < (mBitRateAtLastIDR * 97) / 100) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps < mBitRateAtLastIDR) ||
        (mBitRateKbps > (mBitRateAtLastIDR * 15) / 10) ||
        (timeSinceLastIDR < 500 &&
         mBitRateKbps > (mBitRateAtLastIDR * 13) / 10) ||
        (timeSinceLastIDR < 1000 &&
         mBitRateKbps > (mBitRateAtLastIDR * 11) / 10) ||
        (timeSinceLastIDR >= 1000 && mBitRateKbps > mBitRateAtLastIDR)) {
      CODEC_LOGD(
          "Requesting IDR for bitrate change from %u to %u (time since last "
          "idr %dms)",
          mBitRateAtLastIDR, mBitRateKbps, timeSinceLastIDR);

      mEncoder->RequestIDRFrame();
      mLastIDRTime = now;
      mBitRateAtLastIDR = mBitRateKbps;
    }
#endif
  }

  if (mEncoder->Encode(aInputImage) != android::OK) {
    CODEC_LOGE("WebrtcGonkH264VideoEncoder:%p error sending input data", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkH264VideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* aCallback) {
  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;
  return WEBRTC_VIDEO_CODEC_OK;
}

void WebrtcGonkH264VideoEncoder::OnEncoded(
    webrtc::EncodedImage& aEncodedImage) {
  struct nal_entry {
    uint32_t offset;
    uint32_t size;
  };
  AutoTArray<nal_entry, 1> nals;

  // Break input encoded data into NALUs and send each one to callback.
  const uint8_t* data = aEncodedImage._buffer;
  size_t size = aEncodedImage._length;
  const uint8_t* nalStart = nullptr;
  size_t nalSize = 0;
  while (android::getNextNALUnit(&data, &size, &nalStart, &nalSize, true) ==
         android::OK) {
    // XXX optimize by making buffer an offset
    nal_entry nal = {((uint32_t)(nalStart - aEncodedImage._buffer)),
                     (uint32_t)nalSize};
    nals.AppendElement(nal);
  }

  size_t num_nals = nals.Length();
  if (num_nals > 0) {
    webrtc::RTPFragmentationHeader fragmentation;
    fragmentation.VerifyAndAllocateFragmentationHeader(num_nals);
    for (size_t i = 0; i < num_nals; i++) {
      fragmentation.fragmentationOffset[i] = nals[i].offset;
      fragmentation.fragmentationLength[i] = nals[i].size;
    }
    webrtc::EncodedImage unit(aEncodedImage);
    unit._completeFrame = true;

    webrtc::CodecSpecificInfo info;
    info.codecType = webrtc::kVideoCodecH264;
    info.codecSpecific.H264.packetization_mode =
        webrtc::H264PacketizationMode::NonInterleaved;

    mCallback->OnEncodedImage(unit, &info, &fragmentation);
  }
}

int32_t WebrtcGonkH264VideoEncoder::Release() {
  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p will be released", this);

  if (mEncoder) {
    mEncoder->Release();
    mEncoder = nullptr;
    mReservation->ReleaseOMXCodec();
  }
  mOMXConfigured = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcGonkH264VideoEncoder::~WebrtcGonkH264VideoEncoder() {
  CODEC_LOGD("WebrtcGonkH264VideoEncoder:%p will be destructed", this);

  Release();
}

// Inform the encoder of the new packet loss rate and the round-trip time of
// the network. aPacketLossRate is fraction lost and can be 0~255
// (255 means 100% lost).
// Note: stagefright doesn't handle these parameters.
int32_t WebrtcGonkH264VideoEncoder::SetChannelParameters(
    uint32_t aPacketLossRate, int64_t aRoundTripTimeMs) {
  CODEC_LOGD(
      "WebrtcGonkH264VideoEncoder:%p set channel packet loss:%u, rtt:%" PRIi64,
      this, aPacketLossRate, aRoundTripTimeMs);

  return WEBRTC_VIDEO_CODEC_OK;
}

// TODO: Bug 997567. Find the way to support frame rate change.
int32_t WebrtcGonkH264VideoEncoder::SetRates(uint32_t aBitRateKbps,
                                             uint32_t aFrameRate) {
  CODEC_LOGE(
      "WebrtcGonkH264VideoEncoder:%p set bitrate:%u, frame rate:%u (%u))", this,
      aBitRateKbps, aFrameRate, mFrameRate);
  MOZ_ASSERT(mEncoder);

  // XXX Should use StageFright framerate change, perhaps only on major changes
  // of framerate.

  // Without Stagefright support, Algorithm should be:
  // if (frameRate < 50% of configured) {
  //   drop framerate to next step down that includes current framerate within
  //   50%
  // } else if (frameRate > configured) {
  //   change config to next step up that includes current framerate
  // }
#if !defined(TEST_OMX_FRAMERATE_CHANGES)
  if (aFrameRate > mFrameRate || aFrameRate < mFrameRate / 2) {
    uint32_t old_rate = mFrameRate;
    if (aFrameRate >= 15) {
      mFrameRate = 30;
    } else if (aFrameRate >= 10) {
      mFrameRate = 20;
    } else if (aFrameRate >= 8) {
      mFrameRate = 15;
    } else /* if (aFrameRate >= 5)*/ {
      // don't go lower; encoder may not be stable
      mFrameRate = 10;
    }
    if (mFrameRate < aFrameRate) {  // safety
      mFrameRate = aFrameRate;
    }
    if (old_rate != mFrameRate) {
      mOMXReconfigure = true;  // force re-configure on next frame
    }
  }
#else
  // XXX for testing, be wild!
  if (aFrameRate != mFrameRate) {
    mFrameRate = aFrameRate;
    mOMXReconfigure = true;  // force re-configure on next frame
  }
#endif

  mBitRateKbps = aBitRateKbps;
  if (mEncoder->SetBitrate(mBitRateKbps) != android::OK) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

// Decoder.
WebrtcGonkH264VideoDecoder::WebrtcGonkH264VideoDecoder() {
  mReservation = new OMXCodecReservation(false);
  CODEC_LOGD("WebrtcGonkH264VideoDecoder:%p will be constructed", this);
}

int32_t WebrtcGonkH264VideoDecoder::InitDecode(
    const webrtc::VideoCodec* aCodecSettings, int32_t aNumOfCores) {
  if (!mReservation->ReserveOMXCodec()) {
    CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p decoder in use", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  mDecoder = new android::WebrtcGonkVideoDecoder();
  if (mDecoder->Init(this, android::MEDIA_MIMETYPE_VIDEO_AVC,
                     aCodecSettings->width,
                     aCodecSettings->height) != android::OK) {
    mDecoder = nullptr;
    CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p decoder not started", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  CODEC_LOGD("WebrtcGonkH264VideoDecoder:%p decoder started", this);
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkH264VideoDecoder::Decode(
    const webrtc::EncodedImage& aInputImage, bool aMissingFrames,
    const webrtc::RTPFragmentationHeader* aFragmentation,
    const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
    int64_t aRenderTimeMs) {
  if (aInputImage._length == 0 || !aInputImage._buffer) {
    CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p empty input data", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }

  if (!mCodecConfigSubmitted) {
    int32_t width, height;
    sp<ABuffer> au = new ABuffer(aInputImage._buffer, aInputImage._length);
    sp<ABuffer> csd = android::MakeAVCCodecSpecificData(au, &width, &height);
    if (!csd) {
      CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p missing codec config", this);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }

    // Inherits metadata from input image.
    webrtc::EncodedImage codecConfig(aInputImage);
    codecConfig._buffer = csd->data();
    codecConfig._length = csd->size();
    codecConfig._size = csd->size();
    if (mDecoder->Decode(codecConfig, true, aRenderTimeMs) != android::OK) {
      CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p error sending codec config",
                 this);
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
    mCodecConfigSubmitted = true;
  }

  if (mDecoder->Decode(aInputImage, false, aRenderTimeMs) != android::OK) {
    CODEC_LOGE("WebrtcGonkH264VideoDecoder:%p error sending input data", this);
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkH264VideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* aCallback) {
  CODEC_LOGD("WebrtcGonkH264VideoDecoder:%p set callback:%p", this, aCallback);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcGonkH264VideoDecoder::Release() {
  CODEC_LOGD("WebrtcGonkH264VideoDecoder:%p will be released", this);

  if (mDecoder) {
    mDecoder->Release();
    mDecoder = nullptr;
  }
  mReservation->ReleaseOMXCodec();
  mCodecConfigSubmitted = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

WebrtcGonkH264VideoDecoder::~WebrtcGonkH264VideoDecoder() {
  CODEC_LOGD("WebrtcGonkH264VideoDecoder:%p will be destructed", this);
  Release();
}

}  // namespace mozilla
