/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcGonkVideoCodec.h"

// Android/Stagefright
#include <binder/ProcessState.h>
#include <gui/Surface.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <mediadrm/ICrypto.h>
#include <utils/RefBase.h>

// Gecko
#include "GonkBufferQueueProducer.h"
#include "GrallocImages.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/Mutex.h"
#include "nsThreadUtils.h"
#include "webrtc/rtc_base/refcountedobject.h"
#include "WebrtcImageBuffer.h"

using namespace android;

namespace mozilla {

#if defined(PRODUCT_MANUFACTURER_MTK)
#  define WEBRTC_OMX_MIN_DECODE_BUFFERS 4
#else
#  define WEBRTC_OMX_MIN_DECODE_BUFFERS 10
#endif

#define DEQUEUE_BUFFER_TIMEOUT_US (100 * 1000ll)  // 100ms.
#define DRAIN_THREAD_TIMEOUT_US (1000 * 1000ll)   // 1s.

void CodecOutputDrain::Start() {
  CODEC_LOGD("CodecOutputDrain starting");
  MonitorAutoLock lock(mMonitor);
  if (mThread == nullptr) {
    NS_NewNamedThread("OutputDrain", getter_AddRefs(mThread));
  }
  CODEC_LOGD("CodecOutputDrain started");
  mEnding = false;
  mThread->Dispatch(this, NS_DISPATCH_NORMAL);
}

void CodecOutputDrain::Stop() {
  CODEC_LOGD("CodecOutputDrain stopping");
  MonitorAutoLock lock(mMonitor);
  mEnding = true;
  lock.NotifyAll();  // In case Run() is waiting.

  if (mThread != nullptr) {
    MonitorAutoUnlock unlock(mMonitor);
    NS_DispatchToMainThread(
        NS_NewRunnableFunction("CodecOutputDrain::ShutdownThread",
                               [thread = mThread]() { thread->Shutdown(); }));
    mThread = nullptr;
  }
  CODEC_LOGD("CodecOutputDrain stopped");
}

void CodecOutputDrain::QueueInput(const EncodedFrame& aFrame) {
  MonitorAutoLock lock(mMonitor);

  MOZ_ASSERT(mThread);

  mInputFrames.Push(aFrame);
  // Notify Run() about queued input and it can start working.
  lock.NotifyAll();
}

NS_IMETHODIMP
CodecOutputDrain::Run() {
  MonitorAutoLock lock(mMonitor);
  if (mEnding) {
    return NS_OK;
  }
  MOZ_ASSERT(mThread);

  while (true) {
    if (mInputFrames.Empty()) {
      // Wait for new input.
      lock.Wait();
    }

    if (mEnding) {
      CODEC_LOGD("CodecOutputDrain Run() ending");
      // Stop draining.
      break;
    }

    MOZ_ASSERT(!mInputFrames.Empty());
    {
      // Release monitor while draining because it's blocking.
      MonitorAutoUnlock unlock(mMonitor);
      DrainOutput();
    }
  }

  CODEC_LOGD("CodecOutputDrain Ended");
  return NS_OK;
}

WebrtcGonkVideoDecoder::~WebrtcGonkVideoDecoder() {
  CODEC_LOGD("WebrtcGonkVideoDecoder:%p destructor", this);
  if (mStarted) {
    Stop();
  }
  if (mCodec != nullptr) {
    mCodec->release();
    mCodec.clear();
  }
  mLooper.clear();
}

WebrtcGonkVideoDecoder::WebrtcGonkVideoDecoder(const char* aMimeType)
    : mStarted(false),
      mEnding(false),
      mMimeType(aMimeType),
      mCallback(nullptr),
      mDecodedFrameLock("WebRTC decoded frame lock") {
  // Create binder thread pool required by stagefright.
  android::ProcessState::self()->startThreadPool();

  mLooper = new ALooper;
  mLooper->start();
  mCodec = MediaCodec::CreateByType(mLooper, aMimeType, false /* encoder */);
  CODEC_LOGD("WebrtcGonkVideoDecoder:%p created", this);
}

status_t WebrtcGonkVideoDecoder::ConfigureWithPicDimensions(int32_t aWidth,
                                                            int32_t aHeight) {
  MOZ_ASSERT(mCodec != nullptr);
  if (mCodec == nullptr) {
    return INVALID_OPERATION;
  }

  CODEC_LOGD("WebrtcGonkVideoDecoder:%p configuring with width:%d height:%d",
             this, aWidth, aHeight);

  sp<AMessage> config = new AMessage();
  config->setString("mime", mMimeType);
  config->setInt32("width", aWidth);
  config->setInt32("height", aHeight);

  sp<IGraphicBufferProducer> producer;
  sp<IGonkGraphicBufferConsumer> consumer;
  GonkBufferQueue::createBufferQueue(&producer, &consumer);
  mNativeWindow = new GonkNativeWindow(consumer);
  // listen to buffers queued by MediaCodec::RenderOutputBufferAndRelease().
  mNativeWindow->setNewFrameCallback(this);
  // XXX remove buffer changes after a better solution lands - bug 1009420

  static_cast<GonkBufferQueueProducer*>(producer.get())
      ->setSynchronousMode(false);
  // More spare buffers to avoid MediaCodec waiting for native window
  consumer->setMaxAcquiredBufferCount(WEBRTC_OMX_MIN_DECODE_BUFFERS);
  sp<Surface> surface = new Surface(producer);
  status_t err = mCodec->configure(config, surface, nullptr, 0);
  if (err != OK) {
    return err;
  }
  CODEC_LOGD("WebrtcGonkVideoDecoder:%p decoder configured", this);
  return Start();
}

status_t WebrtcGonkVideoDecoder::SetDecodedCallback(
    webrtc::DecodedImageCallback* aCallback) {
  mCallback = aCallback;
  return OK;
}

status_t WebrtcGonkVideoDecoder::FillInput(const webrtc::EncodedImage& aEncoded,
                                           bool aIsCodecConfig,
                                           int64_t aRenderTimeMs) {
  if (mCodec == nullptr || !aEncoded._buffer || aEncoded._length == 0) {
    return INVALID_OPERATION;
  }

  size_t index;
  status_t err = mCodec->dequeueInputBuffer(&index, DEQUEUE_BUFFER_TIMEOUT_US);
  if (err != OK) {
    if (err != -EAGAIN) {
      CODEC_LOGE("WebrtcGonkVideoDecoder:%p dequeue input buffer error:%d",
                 this, err);
    } else {
      CODEC_LOGE("WebrtcGonkVideoDecoder:%p dequeue input buffer timed out",
                 this);
    }
    return err;
  }

  const sp<MediaCodecBuffer>& omxIn = mInputBuffers.itemAt(index);
  MOZ_ASSERT(omxIn->capacity() >= aEncoded._length);
  if (omxIn->capacity() < aEncoded._length) {
    CODEC_LOGE("WebrtcGonkVideoDecoder:%p insufficient input buffer capacity",
               this);
    return UNKNOWN_ERROR;
  }
  omxIn->setRange(0, aEncoded._length);
  // Copying is needed because MediaCodec API doesn't support externally
  // allocated buffer as input.
  memcpy(omxIn->data(), aEncoded._buffer, aEncoded._length);
  // Unwrap RTP timestamp and convert it from 90kHz clock to usec.
  int64_t inputTimeUs = mUnwrapper.Unwrap(aEncoded._timeStamp) * 1000 / 90;
  uint32_t flags = aIsCodecConfig ? MediaCodec::BUFFER_FLAG_CODECCONFIG : 0;

  err =
      mCodec->queueInputBuffer(index, 0, aEncoded._length, inputTimeUs, flags);
  if (err == OK && !aIsCodecConfig) {
    if (mOutputDrain == nullptr) {
      mOutputDrain = new OutputDrain(this);
      mOutputDrain->Start();
    }
    EncodedFrame frame;
    frame.mWidth = aEncoded._encodedWidth;
    frame.mHeight = aEncoded._encodedHeight;
    frame.mTimestamp = aEncoded._timeStamp;
    frame.mTimestampUs = inputTimeUs;
    frame.mRenderTimeMs = aRenderTimeMs;
    mOutputDrain->QueueInput(frame);
  }

  return err;
}

status_t WebrtcGonkVideoDecoder::DrainOutput(FrameList& aInputFrames,
                                             Monitor& aMonitor) {
  MOZ_ASSERT(mCodec != nullptr);
  if (mCodec == nullptr) {
    return INVALID_OPERATION;
  }

  size_t index = 0;
  size_t outOffset = 0;
  size_t outSize = 0;
  int64_t outTime = -1ll;
  uint32_t outFlags = 0;
  status_t err =
      mCodec->dequeueOutputBuffer(&index, &outOffset, &outSize, &outTime,
                                  &outFlags, DRAIN_THREAD_TIMEOUT_US);
  switch (err) {
    case OK:
      break;
    case -EAGAIN:
      // Not an error: output not available yet. Try later.
      CODEC_LOGI("WebrtcGonkVideoDecoder:%p dequeue output buffer timed out",
                 this);
      return err;
    case INFO_FORMAT_CHANGED:
      // Not an error: will get this value when MediaCodec output buffer is
      // enabled, or when input size changed.
      CODEC_LOGD("WebrtcGonkVideoDecoder:%p output buffer format change", this);
      return err;
    case INFO_OUTPUT_BUFFERS_CHANGED:
      // Not an error: will get this value when MediaCodec output buffer changed
      // (probably because of input size change).
      CODEC_LOGD("WebrtcGonkVideoDecoder:%p output buffer change", this);
      err = mCodec->getOutputBuffers(&mOutputBuffers);
      MOZ_ASSERT(err == OK);
      return INFO_OUTPUT_BUFFERS_CHANGED;
    default:
      CODEC_LOGE("WebrtcGonkVideoDecoder:%p dequeue output buffer error:%d",
                 this, err);
      return OK;
  }

  if (mCallback) {
    EncodedFrame frame;
    {
      MonitorAutoLock lock(aMonitor);
      frame = aInputFrames.Pop(outTime);
    }
    {
      // Store info of this frame. OnNewFrame() will need the timestamp later.
      MutexAutoLock lock(mDecodedFrameLock);
      if (mEnding) {
        mCodec->releaseOutputBuffer(index);
        return err;
      }
      mDecodedFrames.push(frame);
    }
    // Ask codec to queue buffer back to native window. OnNewFrame() will be
    // called.
    mCodec->renderOutputBufferAndRelease(index);
    // Once consumed, buffer will be queued back to GonkNativeWindow for codec
    // to dequeue/use.
  } else {
    mCodec->releaseOutputBuffer(index);
  }

  return err;
}

void WebrtcGonkVideoDecoder::OnNewFrame() {
  RefPtr<layers::TextureClient> buffer = mNativeWindow->getCurrentBuffer();
  if (!buffer) {
    CODEC_LOGE("WebrtcGonkVideoDecoder:%p OnNewFrame got null buffer", this);
    return;
  }

  gfx::IntSize picSize(buffer->GetSize());
  RefPtr<layers::GrallocImage> grallocImage(new layers::GrallocImage());
  grallocImage->AdoptData(buffer, picSize);

  // Get timestamp of the frame about to render.
  uint32_t timestamp = 0;
  int64_t renderTimeMs = -1;
  {
    MutexAutoLock lock(mDecodedFrameLock);
    if (mDecodedFrames.empty()) {
      return;
    }
    EncodedFrame decoded = mDecodedFrames.front();
    timestamp = decoded.mTimestamp;
    renderTimeMs = decoded.mRenderTimeMs;
    mDecodedFrames.pop();
  }
  MOZ_ASSERT(renderTimeMs >= 0);

  rtc::scoped_refptr<ImageBuffer> imageBuffer(
      new rtc::RefCountedObject<ImageBuffer>(std::move(grallocImage)));
  webrtc::VideoFrame videoFrame(imageBuffer, timestamp, renderTimeMs,
                                webrtc::kVideoRotation_0);
  mCallback->Decoded(videoFrame);
}

status_t WebrtcGonkVideoDecoder::Start() {
  MOZ_ASSERT(!mStarted);
  if (mStarted) {
    return OK;
  }

  {
    MutexAutoLock lock(mDecodedFrameLock);
    mEnding = false;
  }
  status_t err = mCodec->start();
  if (err == OK) {
    mStarted = true;
    mCodec->getInputBuffers(&mInputBuffers);
    mCodec->getOutputBuffers(&mOutputBuffers);
  }

  return err;
}

status_t WebrtcGonkVideoDecoder::Stop() {
  MOZ_ASSERT(mStarted);
  if (!mStarted) {
    return OK;
  }

  CODEC_LOGD("WebrtcGonkVideoDecoder:%p decoder stopping", this);
  // Drop all 'pending to render' frames.
  {
    MutexAutoLock lock(mDecodedFrameLock);
    mEnding = true;
    while (!mDecodedFrames.empty()) {
      mDecodedFrames.pop();
    }
  }

  if (mOutputDrain != nullptr) {
    CODEC_LOGD("WebrtcGonkVideoDecoder:%p OutputDrain stopping", this);
    mOutputDrain->Stop();
    mOutputDrain = nullptr;
  }

  status_t err = mCodec->stop();
  if (err == OK) {
    mInputBuffers.clear();
    mOutputBuffers.clear();
    mStarted = false;
  } else {
    MOZ_ASSERT(false);
  }
  CODEC_LOGD("WebrtcGonkVideoDecoder:%p decoder stopped", this);
  return err;
}

}  // namespace mozilla
