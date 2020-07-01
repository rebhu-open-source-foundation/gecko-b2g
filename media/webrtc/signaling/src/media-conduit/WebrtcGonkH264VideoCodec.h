/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBRTC_OMX_H264_VIDEO_CODEC_H_
#define WEBRTC_OMX_H264_VIDEO_CODEC_H_

#include <utils/RefBase.h>

#include "VideoConduit.h"
#include "webrtc/modules/include/module_common_types_public.h"

namespace android {
class OMXCodecReservation;
class OMXVideoEncoder;
}  // namespace android

namespace mozilla {

class WebrtcGonkVideoDecoder;
class CodecOutputDrain;

#define OMX_IDR_NEEDED_FOR_BITRATE 0

class WebrtcGonkH264VideoEncoder : public WebrtcVideoEncoder {
 public:
  WebrtcGonkH264VideoEncoder();

  virtual ~WebrtcGonkH264VideoEncoder();

  // Implement VideoEncoder interface.
  virtual uint64_t PluginID() const override { return 0; }

  virtual int32_t InitEncode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumOfCores,
                             size_t aMaxPayloadSize) override;

  virtual int32_t Encode(
      const webrtc::VideoFrame& aInputImage,
      const webrtc::CodecSpecificInfo* aCodecSpecificInfo,
      const std::vector<webrtc::FrameType>* aFrameTypes) override;

  virtual int32_t RegisterEncodeCompleteCallback(
      webrtc::EncodedImageCallback* aCallback) override;

  virtual int32_t Release() override;

  virtual int32_t SetChannelParameters(uint32_t aPacketLossRate,
                                       int64_t aRoundTripTimeMs) override;

  virtual int32_t SetRates(uint32_t aBitRate, uint32_t aFrameRate) override;

  virtual bool SupportsNativeHandle() const override { return true; }

 private:
  UniquePtr<android::OMXVideoEncoder> mOMX;
  android::sp<android::OMXCodecReservation> mReservation;

  webrtc::EncodedImageCallback* mCallback;
  RefPtr<CodecOutputDrain> mOutputDrain;
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFrameRate;
  uint32_t mBitRateKbps;
#ifdef OMX_IDR_NEEDED_FOR_BITRATE
  uint32_t mBitRateAtLastIDR;
  TimeStamp mLastIDRTime;
#endif
  bool mOMXConfigured;
  bool mOMXReconfigure;
  webrtc::EncodedImage mEncodedImage;
  webrtc::TimestampUnwrapper mUnwrapper;
};

class WebrtcGonkH264VideoDecoder : public WebrtcVideoDecoder {
 public:
  WebrtcGonkH264VideoDecoder();

  virtual ~WebrtcGonkH264VideoDecoder();

  // Implement VideoDecoder interface.
  virtual uint64_t PluginID() const override { return 0; }

  virtual int32_t InitDecode(const webrtc::VideoCodec* aCodecSettings,
                             int32_t aNumOfCores) override;
  virtual int32_t Decode(
      const webrtc::EncodedImage& aInputImage, bool aMissingFrames,
      const webrtc::RTPFragmentationHeader* aFragmentation,
      const webrtc::CodecSpecificInfo* aCodecSpecificInfo = nullptr,
      int64_t aRenderTimeMs = -1) override;
  virtual int32_t RegisterDecodeCompleteCallback(
      webrtc::DecodedImageCallback* callback) override;

  virtual int32_t Release() override;

 private:
  RefPtr<WebrtcGonkVideoDecoder> mDecoder;
  android::sp<android::OMXCodecReservation> mReservation;
  bool mCodecConfigSubmitted;
};

}  // namespace mozilla

#endif  // WEBRTC_OMX_H264_VIDEO_CODEC_H_
