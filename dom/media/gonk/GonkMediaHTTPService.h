/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GonkMediaHTTPService_h_
#define GonkMediaHTTPService_h_

#include <media/IMediaHTTPService.h>
#include "mozilla/RefPtr.h"

namespace mozilla {
class MediaResource;
}

namespace android {

class BnMediaHTTPService : public BnInterface<IMediaHTTPService> {
 public:
  virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                              uint32_t flags = 0) override;
};

class GonkMediaHTTPService : public BnMediaHTTPService {
 public:
  GonkMediaHTTPService(RefPtr<mozilla::MediaResource> resource,
                       const char* resourceURI, const char* resourceMIMEType);
  virtual sp<MediaHTTPConnection> makeHTTPConnection() override;

 private:
  virtual ~GonkMediaHTTPService() = default;

  RefPtr<mozilla::MediaResource> mResource;
  String8 mResourceURI;
  String8 mResourceMIMEType;
};

}  // namespace android

#endif
