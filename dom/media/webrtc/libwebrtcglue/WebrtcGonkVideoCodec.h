/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBRTC_GONK_VIDEO_CODEC_H_
#define WEBRTC_GONK_VIDEO_CODEC_H_

#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/MediaCodec.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>

#include "common/browser_logging/CSFLog.h"
#include "GonkNativeWindow.h"
#include "webrtc/api/video/video_frame.h"
#include "webrtc/common_video/include/video_frame.h"
#include "webrtc/modules/include/module_common_types_public.h"

#define CODEC_LOG_TAG "OMX"
#define CODEC_LOGV(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGD(...) CSFLogDebug(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGI(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGW(...) CSFLogWarn(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGE(...) CSFLogError(CODEC_LOG_TAG, __VA_ARGS__)

namespace android {

struct FrameInfo;

class FrameInfoQueue {
 public:
  void SetOwner(void* aOwner, const char* aTag);

  void Push(const sp<FrameInfo>& aFrameInfo);

  sp<FrameInfo> Pop(int64_t aTimestampUs);

 private:
  Mutex mMutex;
  void* mOwner;
  std::string mTag;
  std::deque<sp<FrameInfo>> mQueue;
};

class WebrtcGonkVideoEncoder final : public AHandler {
 public:
  class Callback {
   public:
    virtual void OnEncoded(webrtc::EncodedImage& aEncodedImage) = 0;
  };

  WebrtcGonkVideoEncoder();

  status_t Init(Callback* aCallback, const char* aMime);

  status_t Release();

  status_t Configure(const sp<AMessage>& aFormat);

  status_t Encode(const webrtc::VideoFrame& aInputImage);

  status_t RequestIDRFrame();

  status_t SetBitrate(int32_t aKbps);

 private:
  class FrameBufferGrip;

  enum {
    kWhatCodecNotify = 'codc',
    kWhatConfigure = 'conf',
    kWhatQueueInputData = 'qIDt',
  };

  ~WebrtcGonkVideoEncoder();

  virtual void onMessageReceived(const sp<AMessage>& aMsg) override;

  void OnConfigure(const sp<AMessage>& aFormat);

  void OnFillInputBuffers();

  void OnDrainOutputBuffer(size_t aIndex, size_t aOffset, size_t aSize,
                           int64_t aTimeUs, int32_t aFlags);

  Callback* mCallback = nullptr;
  int32_t mColorFormat = 0;
  bool mStarted = false;

  sp<ALooper> mEncoderLooper;
  sp<ALooper> mCodecLooper;
  sp<MediaCodec> mCodec;

  std::deque<std::pair<sp<FrameInfo>, sp<FrameBufferGrip>>> mInputFrames;
  std::deque<size_t> mInputBuffers;
  FrameInfoQueue mFrameInfoQueue;
};

// Generic decoder using stagefright.
// It implements gonk native window callback to receive buffers from
// MediaCodec::RenderOutputBufferAndRelease().
class WebrtcGonkVideoDecoder final : public AHandler,
                                     public GonkNativeWindowNewFrameCallback {
 public:
  class Callback {
   public:
    virtual void OnDecoded(webrtc::VideoFrame& aVideoFrame) = 0;
  };

  WebrtcGonkVideoDecoder();

  status_t Init(Callback* aCallback, const char* aMime, int32_t aWidth,
                int32_t aHeight);

  status_t Release();

  status_t Decode(const webrtc::EncodedImage& aEncoded, bool aIsCodecConfig,
                  int64_t aRenderTimeMs);

  // After MediaCodec::RenderOutputBufferAndRelease() returns a buffer back to
  // native window for rendering, this function will called directly from
  // GonkBufferQueueProducer::queueBuffer(), which is on ACodec looper thread.
  virtual void OnNewFrame() override;

 private:
  enum {
    kWhatCodecNotify = 'codc',
    kWhatQueueInputData = 'qIDt',
  };

  virtual ~WebrtcGonkVideoDecoder();

  virtual void onMessageReceived(const sp<AMessage>& aMsg) override;

  void OnFillInputBuffers();

  // Called on ACodec looper thread when MediaCodec renders a buffer into native
  // window.
  void OnOutputBufferQueued(ANativeWindowBuffer* aBuffer, int64_t aTimestampNs);

  sp<Surface> InitBufferQueue();

  Callback* mCallback = nullptr;
  webrtc::TimestampUnwrapper mUnwrapper;

  sp<ALooper> mDecoderLooper;
  sp<ALooper> mCodecLooper;
  sp<MediaCodec> mCodec;
  sp<GonkNativeWindow> mNativeWindow;

  std::deque<std::pair<sp<FrameInfo>, sp<ABuffer>>> mInputFrames;
  std::deque<size_t> mInputBuffers;
  FrameInfoQueue mFrameInfoQueue;
  sp<FrameInfo> mDecodedFrameInfo;  // accessed on ACodec looper thread
};

}  // namespace android

#endif  // WEBRTC_GONK_VIDEO_CODEC_H_
