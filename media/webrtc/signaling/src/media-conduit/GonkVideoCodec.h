/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OMX_VIDEO_CODEC_H_
#define OMX_VIDEO_CODEC_H_

#include "VideoConduit.h"

namespace mozilla {

class GonkVideoCodec {
 public:
  /**
   * Create encoder object for codec type |aCodecType|. Return |nullptr| when
   * failed.
   */
  static WebrtcVideoEncoder* CreateEncoder(webrtc::VideoCodecType aCodecType);

  /**
   * Create decoder object for codec type |aCodecType|. Return |nullptr| when
   * failed.
   */
  static WebrtcVideoDecoder* CreateDecoder(webrtc::VideoCodecType aCodecType);

 private:
  static bool CodecEnabled(webrtc::VideoCodecType aCodecType, bool aIsEncoder);
  static bool CodecSupported(webrtc::VideoCodecType aCodecType,
                             bool aIsEncoder);
};

}  // namespace mozilla

#endif  // OMX_VIDEO_CODEC_H_
