/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebrtcGonkVideoCodec.h"

// Android/Stagefright
#include <binder/ProcessState.h>
#include <gui/Surface.h>
#include <media/MediaCodecBuffer.h>
#include <media/stagefright/foundation/ABuffer.h>
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

namespace android {

#undef LOGE
#undef LOGW
#undef LOGI
#undef LOGD
#undef LOGV

static mozilla::LazyLogModule sCodecLog("WebrtcGonkVideoCodec");
#define LOGE(...) MOZ_LOG(sCodecLog, mozilla::LogLevel::Error, (__VA_ARGS__))
#define LOGW(...) MOZ_LOG(sCodecLog, mozilla::LogLevel::Warning, (__VA_ARGS__))
#define LOGI(...) MOZ_LOG(sCodecLog, mozilla::LogLevel::Info, (__VA_ARGS__))
#define LOGD(...) MOZ_LOG(sCodecLog, mozilla::LogLevel::Debug, (__VA_ARGS__))
#define LOGV(...) MOZ_LOG(sCodecLog, mozilla::LogLevel::Verbose, (__VA_ARGS__))

static inline bool IsVerboseLoggingEnabled() {
  const mozilla::LogModule* logModule = sCodecLog;
  return MOZ_LOG_TEST(logModule, mozilla::LogLevel::Verbose);
}

static std::vector<std::string> SplitMultilineString(const std::string& aStr) {
  std::vector<std::string> lines;
  std::string::size_type begin = 0, end;
  while ((end = aStr.find('\n', begin)) != std::string::npos) {
    lines.push_back(aStr.substr(begin, end - begin));
    begin = end + 1;
  }
  lines.push_back(aStr.substr(begin));
  return lines;
}

struct FrameInfo : public RefBase {
  bool mIsCodecConfig = false;
  uint32_t mWidth = 0;
  uint32_t mHeight = 0;
  uint32_t mTimestamp = 0;
  int64_t mTimestampUs = 0;
  int64_t mRenderTimeMs = 0;

  // Debug info
  int64_t mInputTimeNs = 0;
  ANativeWindowBuffer* mNativeBuffer = 0;
};

void FrameInfoQueue::SetOwner(void* aOwner, const char* aTag) {
  mOwner = aOwner;
  mTag = aTag;
}

void FrameInfoQueue::Push(const sp<FrameInfo>& aFrameInfo) {
  Mutex::Autolock lock(mMutex);
  mQueue.push_back(aFrameInfo);
  if (mQueue.size() > 32) {
    LOGE("%s:%p FrameInfoQueue has too many frames, dropping an old one",
         mTag.c_str(), mOwner);
    mQueue.pop_front();
  }
}

sp<FrameInfo> FrameInfoQueue::Pop(int64_t aTimestampUs) {
  Mutex::Autolock lock(mMutex);

  if (mQueue.empty()) {
    LOGE("%s:%p FrameInfoQueue is empty, return null", mTag.c_str(), mOwner);
    return nullptr;
  }

  // Find the target frame and clear the frames before it.
  sp<FrameInfo> frameInfo;
  do {
    frameInfo = mQueue.front();
    mQueue.pop_front();
  } while (!mQueue.empty() && mQueue.front()->mTimestampUs <= aTimestampUs);

  if (frameInfo->mTimestampUs != aTimestampUs) {
    LOGE("%s:%p FrameInfoQueue has no matched frame, return a nearby one",
         mTag.c_str(), mOwner);
  }

  // Keep the found frame in the queue, in case the next Pop() maps to the same
  // frame.
  mQueue.push_front(frameInfo);
  return frameInfo;
}

WebrtcGonkVideoDecoder::WebrtcGonkVideoDecoder() {
  LOGD("Decoder:%p constructor", this);

  ProcessState::self()->startThreadPool();
  mDecoderLooper = new ALooper;
  mDecoderLooper->setName("WebrtcGonkVideoDecoder");
  mCodecLooper = new ALooper;
  mCodecLooper->setName("WebrtcGonkVideoDecoder/Codec");

  mFrameInfoQueue.SetOwner(this, "Decoder");
}

WebrtcGonkVideoDecoder::~WebrtcGonkVideoDecoder() {
  LOGD("Decoder:%p destructor", this);
  Release();
}

status_t WebrtcGonkVideoDecoder::Init(Callback* aCallback, const char* aMime,
                                      int32_t aWidth, int32_t aHeight) {
  LOGD("Decoder:%p initializing, mime:%s, width:%d, height:%d", this, aMime,
       aWidth, aHeight);

  mCallback = aCallback;

  mDecoderLooper->registerHandler(this);
  mDecoderLooper->start();
  mCodecLooper->start();

  mCodec = MediaCodec::CreateByType(mCodecLooper, aMime, false);
  if (!mCodec) {
    return UNKNOWN_ERROR;
  }

  sp<Surface> surface = InitBufferQueue();
  sp<AMessage> config = new AMessage();
  config->setString("mime", aMime);
  config->setInt32("width", aWidth);
  config->setInt32("height", aHeight);

  status_t err = mCodec->configure(config, surface, nullptr, 0);
  if (err != OK) {
    LOGE("Decoder:%p failed to configure codec", this);
    return err;
  }

  // Run MediaCodec in async mode.
  sp<AMessage> reply = new AMessage(kWhatCodecNotify, this);
  err = mCodec->setCallback(reply);
  if (err != OK) {
    LOGE("Decoder:%p failed to set codec callback", this);
    return err;
  }

  err = mCodec->start();
  if (err != OK) {
    LOGE("Decoder:%p failed to start codec", this);
    return err;
  }
  return OK;
}

status_t WebrtcGonkVideoDecoder::Release() {
  LOGD("Decoder:%p releasing", this);

  mDecoderLooper->stop();
  mDecoderLooper->unregisterHandler(id());

  if (mCodec) {
    mCodec->release();
    mCodec = nullptr;
  }
  if (mNativeWindow) {
    mNativeWindow->abandon();
    mNativeWindow = nullptr;
  }
  mInputFrames.clear();
  mInputBuffers.clear();
  mDecodedFrameInfo = nullptr;
  mCallback = nullptr;
  return OK;
}

sp<Surface> WebrtcGonkVideoDecoder::InitBufferQueue() {
  // Extend Surface class and override its queueBuffer() in order to let the
  // decoder know which timestamp is bound to the graphic buffer we just queued.
  class SurfaceExt : public Surface {
   public:
    explicit SurfaceExt(const wp<WebrtcGonkVideoDecoder>& aDecoder,
                        const sp<IGraphicBufferProducer>& aBufferProducer)
        : Surface(aBufferProducer, false), mDecoder(aDecoder) {}

   private:
    virtual int queueBuffer(ANativeWindowBuffer* aBuffer,
                            int aFenceFd) override {
      if (auto decoder = mDecoder.promote()) {
        decoder->OnOutputBufferQueued(aBuffer, mTimestamp);
      }
      return Surface::queueBuffer(aBuffer, aFenceFd);
    }

    wp<WebrtcGonkVideoDecoder> mDecoder;
  };

  sp<IGraphicBufferProducer> producer;
  sp<IGonkGraphicBufferConsumer> consumer;
  GonkBufferQueue::createBufferQueue(&producer, &consumer);

  mNativeWindow = new GonkNativeWindow(consumer);
  mNativeWindow->setNewFrameCallback(this);

  static_cast<GonkBufferQueueProducer*>(producer.get())
      ->setSynchronousMode(false);
  // More spare buffers to avoid MediaCodec waiting for native window
  consumer->setMaxAcquiredBufferCount(WEBRTC_OMX_MIN_DECODE_BUFFERS);
  return new SurfaceExt(this, producer);
}

status_t WebrtcGonkVideoDecoder::Decode(const webrtc::EncodedImage& aEncoded,
                                        bool aIsCodecConfig,
                                        int64_t aRenderTimeMs) {
  if (!mCodec || !aEncoded._buffer || !aEncoded._length) {
    return INVALID_OPERATION;
  }

  sp<FrameInfo> frameInfo = new FrameInfo();
  frameInfo->mWidth = aEncoded._encodedWidth;
  frameInfo->mHeight = aEncoded._encodedHeight;
  frameInfo->mTimestamp = aEncoded._timeStamp;
  // Unwrap RTP timestamp and convert it from 90kHz clock to micro seconds.
  frameInfo->mTimestampUs = mUnwrapper.Unwrap(aEncoded._timeStamp) * 1000 / 90;
  frameInfo->mRenderTimeMs = aRenderTimeMs;
  frameInfo->mIsCodecConfig = aIsCodecConfig;
  frameInfo->mInputTimeNs = systemTime();

  // We cannot take ownership of |aEncoded|, so make a copy of it.
  sp<ABuffer> frameData =
      ABuffer::CreateAsCopy(aEncoded._buffer, aEncoded._length);

  sp<AMessage> msg = new AMessage(kWhatQueueInputData, this);
  msg->setObject("frame-info", frameInfo.get());
  msg->setObject("frame-data", frameData.get());
  msg->post();
  return OK;
}

void WebrtcGonkVideoDecoder::onMessageReceived(const sp<AMessage>& aMsg) {
  if (IsVerboseLoggingEnabled()) {
    auto lines = SplitMultilineString(aMsg->debugString().c_str());
    for (auto& line : lines) {
      LOGV("Decoder:%p onMessage: %s", this, line.c_str());
    }
  }

  switch (aMsg->what()) {
    case kWhatCodecNotify: {
      int32_t cbID;
      CHECK(aMsg->findInt32("callbackID", &cbID));

      switch (cbID) {
        case MediaCodec::CB_INPUT_AVAILABLE: {
          int32_t index;
          CHECK(aMsg->findInt32("index", &index));
          mInputBuffers.push_back(index);
          OnFillInputBuffers();
          break;
        }

        case MediaCodec::CB_OUTPUT_AVAILABLE: {
          int32_t index;
          int64_t timeUs;
          CHECK(aMsg->findInt32("index", &index));
          CHECK(aMsg->findInt64("timeUs", &timeUs));
          mCodec->renderOutputBufferAndRelease(index);
          break;
        }

        case MediaCodec::CB_OUTPUT_FORMAT_CHANGED: {
          sp<AMessage> format;
          CHECK(aMsg->findMessage("format", &format));
          auto lines = SplitMultilineString(format->debugString().c_str());
          for (auto& line : lines) {
            LOGI("Decoder:%p format changed: %s", this, line.c_str());
          }
          break;
        }

        case MediaCodec::CB_ERROR: {
          status_t err;
          CHECK(aMsg->findInt32("err", &err));
          LOGE("Decoder:%p reported error: 0x%x", this, err);
          break;
        }

        default: {
          TRESPASS();
          break;
        }
      }

      break;
    }

    case kWhatQueueInputData: {
      sp<RefBase> obj;
      CHECK(aMsg->findObject("frame-info", &obj));
      sp<FrameInfo> frameInfo = static_cast<FrameInfo*>(obj.get());
      CHECK(aMsg->findObject("frame-data", &obj));
      sp<ABuffer> frameData = static_cast<ABuffer*>(obj.get());

      mInputFrames.push_back(std::make_pair(frameInfo, frameData));
      OnFillInputBuffers();
      break;
    }
  }
}

void WebrtcGonkVideoDecoder::OnFillInputBuffers() {
  while (mInputFrames.size() && mInputBuffers.size()) {
    auto index = mInputBuffers.front();
    mInputBuffers.pop_front();

    sp<MediaCodecBuffer> buffer;
    mCodec->getInputBuffer(index, &buffer);
    if (!buffer) {
      LOGE("Decoder:%p failed to get input buffer", this);
      continue;
    }

    auto [frameInfo, frameData] = mInputFrames.front();
    mInputFrames.pop_front();

    if (buffer->capacity() < frameData->size()) {
      LOGE("Decoder:%p exceed input buffer size, dropping one frame", this);
      mInputBuffers.push_front(index);  // keep the buffer
      continue;
    }

    buffer->setRange(0, frameData->size());
    memcpy(buffer->data(), frameData->data(), frameData->size());

    uint32_t flags =
        frameInfo->mIsCodecConfig ? MediaCodec::BUFFER_FLAG_CODECCONFIG : 0;

    status_t err = mCodec->queueInputBuffer(index, 0, frameData->size(),
                                            frameInfo->mTimestampUs, flags);
    if (err != OK) {
      LOGE("Decoder:%p failed to queue input buffer", this);
      continue;
    }

    if (!frameInfo->mIsCodecConfig) {
      mFrameInfoQueue.Push(frameInfo);
    }
  }
}

void WebrtcGonkVideoDecoder::OnOutputBufferQueued(ANativeWindowBuffer* aBuffer,
                                                  int64_t aTimestampNs) {
  mDecodedFrameInfo = mFrameInfoQueue.Pop(aTimestampNs / 1000);
  mDecodedFrameInfo->mNativeBuffer = aBuffer;
}

void WebrtcGonkVideoDecoder::OnNewFrame() {
  using mozilla::ImageBuffer;
  using mozilla::MakeRefPtr;
  using mozilla::layers::GrallocImage;
  using mozilla::layers::TextureClient;

  RefPtr<TextureClient> buffer = mNativeWindow->getCurrentBuffer();
  if (!buffer) {
    LOGE("Decoder:%p failed to get texture client", this);
    return;
  }

  if (!mDecodedFrameInfo) {
    LOGE("Decoder:%p no decoded frame info", this);
    return;
  }

  auto image = MakeRefPtr<GrallocImage>();
  image->AdoptData(buffer, buffer->GetSize());

  if (image->GetGraphicBuffer().get() != mDecodedFrameInfo->mNativeBuffer) {
    LOGE("Decoder:%p graphic buffer mismatch", this);
  }

  int64_t latency = (systemTime() - mDecodedFrameInfo->mInputTimeNs) / 1000000;
  LOGD("Decoder:%p frame decoded, latency %" PRId64 " ms", this, latency);

  rtc::scoped_refptr<ImageBuffer> imageBuffer =
      new rtc::RefCountedObject<ImageBuffer>(std::move(image));
  webrtc::VideoFrame videoFrame(imageBuffer, mDecodedFrameInfo->mTimestamp,
                                mDecodedFrameInfo->mRenderTimeMs,
                                webrtc::kVideoRotation_0);

  CHECK(mCallback);
  mCallback->OnDecoded(videoFrame);
}

}  // namespace android
