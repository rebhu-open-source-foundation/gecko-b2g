/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AMR_DECODER_H_
#define AMR_DECODER_H_

#include "mozilla/UniquePtr.h"
#include "nsTArray.h"

namespace mozilla {

class MediaContainerType;
class TrackInfo;

class AMRDecoder {
 public:
  static bool IsEnabled();
  static bool IsSupportedType(const MediaContainerType& aContainerType);
  static nsTArray<UniquePtr<TrackInfo>> GetTracksInfo(
      const MediaContainerType& aType);
};

}  // namespace mozilla

#endif  // !AMR_DECODER_H_
