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
#include "transport/runnable_utils.h"
#include "webrtc/api/video_codecs/video_decoder.h"
#include "webrtc/modules/include/module_common_types_public.h"

#define CODEC_LOG_TAG "OMX"
#define CODEC_LOGV(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGD(...) CSFLogDebug(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGI(...) CSFLogInfo(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGW(...) CSFLogWarn(CODEC_LOG_TAG, __VA_ARGS__)
#define CODEC_LOGE(...) CSFLogError(CODEC_LOG_TAG, __VA_ARGS__)

namespace mozilla {

struct EncodedFrame {
  uint32_t mWidth = 0;
  uint32_t mHeight = 0;
  uint32_t mTimestamp = 0;
  int64_t mTimestampUs = 0;
  int64_t mRenderTimeMs = 0;
};

class FrameList {
 public:
  bool Empty() { return mQueue.empty(); }

  void Push(const EncodedFrame& aFrame) {
    mQueue.push_back(aFrame);
    if (mQueue.size() > 32) {
      CODEC_LOGE("FrameList exceeds max capacity, dropping one frame");
      mQueue.pop_front();
    }
  }

  EncodedFrame Pop(int64_t aTimestampUs) {
    EncodedFrame frame;
    if (mQueue.empty()) {
      CODEC_LOGE("FrameList has no frame, return an empty one");
      return frame;
    }

    // Find the target frame and clear the frames before it.
    do {
      frame = mQueue.front();
      mQueue.pop_front();
    } while (!mQueue.empty() && mQueue.front().mTimestampUs <= aTimestampUs);

    if (frame.mTimestampUs != aTimestampUs) {
      CODEC_LOGE("FrameList has no matched frame, return the nearby one");
    }

    // Keep the found frame in the list, in case the next Pop() maps to the same
    // frame.
    mQueue.push_front(frame);
    return frame;
  }

 private:
  std::deque<EncodedFrame> mQueue;
};

// Base runnable class to repeatly pull MediaCodec output buffers in seperate
// thread. How to use:
// - implementing DrainOutput() to get output. Remember to return false to tell
//   drain not to pop input queue.
// - call QueueInput() to schedule a run to drain output. The input, aFrame,
//   should contains corresponding info such as image size and timestamps for
//   DrainOutput() implementation to construct data needed by encoded/decoded
//   callbacks.
// TODO: Bug 997110 - Revisit queue/drain logic. Current design assumes that
//       encoder only generate one output buffer per input frame and won't work
//       if encoder drops frames or generates multiple output per input.
class CodecOutputDrain : public Runnable {
 public:
  void Start();

  void Stop();

  void QueueInput(const EncodedFrame& aFrame);

  NS_IMETHODIMP Run() override;

 protected:
  CodecOutputDrain()
      : Runnable("CodecOutputDrain"),
        mMonitor("CodecOutputDrain monitor"),
        mEnding(false) {}

  // Drain output buffer for input frame queue mInputFrames.
  // mInputFrames contains info such as size and time of the input frames.
  // We have to give a queue to handle encoder frame skips - we can input 10
  // frames and get one back.  NOTE: any access of aInputFrames MUST be preceded
  // locking mMonitor!

  // Blocks waiting for decoded buffers, but for a limited period because
  // we need to check for shutdown.
  virtual bool DrainOutput() = 0;

 protected:
  // This monitor protects all things below it, and is also used to
  // wait/notify queued input.
  Monitor mMonitor;
  FrameList mInputFrames;

 private:
  // also protected by mMonitor
  nsCOMPtr<nsIThread> mThread;
  bool mEnding;
};

}  // namespace mozilla

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
