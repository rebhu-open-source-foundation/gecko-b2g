/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaResourceDataSource.h"

#include "BaseMediaResource.h"
#include "MediaResource.h"

namespace mozilla {

MediaResourceDataSource::MediaResourceDataSource(MediaResource* aResource)
    : mResource(aResource) {}

android::status_t MediaResourceDataSource::initCheck() const {
  return android::OK;
}

android::status_t MediaResourceDataSource::getSize(off64_t* size) {
  int64_t length = mResource->GetLength();
  if (length == -1) {
    *size = 0;
    return android::ERROR_UNSUPPORTED;
  }
  *size = static_cast<off64_t>(length);
  return android::OK;
}

ssize_t MediaResourceDataSource::readAt(off64_t offset, void* data,
                                        size_t size) {
  uint32_t readSize = 0;
  nsresult err =
      mResource->ReadAt(offset, static_cast<char*>(data), size, &readSize);
  if (NS_FAILED(err)) {
    return -1;
  }
  return readSize;
}

}  // namespace mozilla
