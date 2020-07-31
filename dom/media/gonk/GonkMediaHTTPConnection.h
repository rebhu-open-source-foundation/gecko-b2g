/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GonkMediaHTTPConnection_h_
#define GonkMediaHTTPConnection_h_

#include <media/IMediaHTTPConnection.h>
#include <binder/IMemory.h>
#include "mozilla/RefPtr.h"

namespace mozilla {
class MediaResource;
}

namespace android {

class BnMediaHTTPConnection : public BnInterface<IMediaHTTPConnection> {
 public:
  virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
                              uint32_t flags = 0) override;

 protected:
  virtual sp<IMemory> getIMemory() = 0;
};

class GonkMediaHTTPConnection : public BnMediaHTTPConnection {
 public:
  GonkMediaHTTPConnection(RefPtr<mozilla::MediaResource> resource,
                          const char* resourceURI,
                          const char* resourceMIMEType);

  virtual bool connect(const char* uri,
                       const KeyedVector<String8, String8>* headers) override;
  virtual void disconnect() override;
  virtual ssize_t readAt(off64_t offset, void* data, size_t size) override;
  virtual off64_t getSize() override;
  virtual status_t getMIMEType(String8* mimeType) override;
  virtual status_t getUri(String8* uri) override;

 private:
  enum {
    kBufferSize = 65536,
  };

  virtual sp<IMemory> getIMemory() override { return mMemory; }

  RefPtr<mozilla::MediaResource> mResource;
  String8 mResourceURI;
  String8 mResourceMIMEType;
  sp<IMemory> mMemory;
};

}  // namespace android

#endif
