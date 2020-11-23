/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AMR_DEMUXER_H_
#define AMR_DEMUXER_H_

#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "MediaDataDemuxer.h"
#include "MediaResource.h"

namespace mozilla {

#define OFFSET_TABLE_LEN 300

class AMRTrackDemuxer;

DDLoggedTypeDeclNameAndBase(AMRDemuxer, MediaDataDemuxer);

class AMRDemuxer : public MediaDataDemuxer,
                   public DecoderDoctorLifeLogger<AMRDemuxer> {
 public:
  // MediaDataDemuxer interface.
  explicit AMRDemuxer(MediaResource* aSource);
  RefPtr<InitPromise> Init() override;
  uint32_t GetNumberTracks(TrackInfo::TrackType aType) const override;
  already_AddRefed<MediaTrackDemuxer> GetTrackDemuxer(
      TrackInfo::TrackType aType, uint32_t aTrackNumber) override;
  bool IsSeekable() const override;

  // Return true if a valid AMR frame header could be found.
  static bool AMRSniffer(const uint8_t* aData, const uint32_t aLength);

 private:
  bool InitInternal();

  RefPtr<MediaResource> mSource;
  RefPtr<AMRTrackDemuxer> mTrackDemuxer;
};

DDLoggedTypeNameAndBase(AMRTrackDemuxer, MediaTrackDemuxer);

class AMRTrackDemuxer : public MediaTrackDemuxer,
                        public DecoderDoctorLifeLogger<AMRTrackDemuxer> {
 public:
  explicit AMRTrackDemuxer(MediaResource* aSource);

  // Initializes the track demuxer by reading the first frame for meta data.
  // Returns initialization success state.
  bool Init();

  // Return true if a valid AMR frame header could be found.
  static bool SniffAMR(const uint8_t* aSource, const uint32_t aLength,
                       bool* aIsWide = nullptr);

  // MediaTrackDemuxer interface.
  UniquePtr<TrackInfo> GetInfo() const override;
  RefPtr<SeekPromise> Seek(const media::TimeUnit& aTime) override;
  RefPtr<SamplesPromise> GetSamples(int32_t aNumSamples = 1) override;
  void Reset() override;
  RefPtr<SkipAccessPointPromise> SkipToNextRandomAccessPoint(
      const media::TimeUnit& aTimeThreshold) override;
  int64_t GetResourceOffset() const override;
  media::TimeIntervals GetBuffered() override;

 private:
  // Destructor.
  ~AMRTrackDemuxer();

  // Reads aSize bytes into aBuffer from the source starting at aOffset.
  // Returns the actual size read.
  int32_t Read(uint8_t* aBuffer, int64_t aOffset, int32_t aSize);
  int32_t GetFrameSize(bool aIsWide, unsigned aFT);
  nsresult GetFrameSizeByOffset(off64_t aOffset, bool aIsWide,
                                size_t* aFrameSize);

  // The (hopefully) AMR resource.
  MediaResourceIndex mSource;

  // Current byte offset in the source stream.
  int64_t mOffset;

  // Audio track config info.
  UniquePtr<AudioInfo> mInfo;

  // Flag this file is wide-band or narrow-band.
  bool mIsWide;

  // Current playback time.
  int64_t mCurrentTimeUs;

  // Store seek index to speed seek speed.
  uint64_t mOffsetTable[OFFSET_TABLE_LEN];  // 5 min
  uint32_t mOffsetTableLength;
};

}  // namespace mozilla

#endif  // !AMR_DEMUXER_H_
