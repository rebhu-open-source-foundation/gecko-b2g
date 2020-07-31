/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GonkMediaHTTPService.h"

#include <binder/Parcel.h>
#include "GonkMediaHTTPConnection.h"
#include "MediaResource.h"

namespace android {

using mozilla::MediaResource;

/** MUST stay in sync with IMediaHTTPService.cpp */

enum {
  MAKE_HTTP = IBinder::FIRST_CALL_TRANSACTION,
};

status_t BnMediaHTTPService::onTransact(uint32_t code, const Parcel& data,
                                        Parcel* reply, uint32_t flags) {
  switch (code) {
    case MAKE_HTTP: {
      CHECK_INTERFACE(IMediaHTTPService, data, reply);
      sp<MediaHTTPConnection> conn = makeHTTPConnection();
      reply->writeInt32(OK);
      reply->writeStrongBinder(
          IInterface::asBinder(static_cast<IMediaHTTPConnection*>(conn.get())));
      return NO_ERROR;
    } break;
    default:
      return BBinder::onTransact(code, data, reply, flags);
  }
}

GonkMediaHTTPService::GonkMediaHTTPService(RefPtr<MediaResource> resource,
                                           const char* resourceURI,
                                           const char* resourceMIMEType)
    : mResource(resource),
      mResourceURI(resourceURI),
      mResourceMIMEType(resourceMIMEType) {}

sp<MediaHTTPConnection> GonkMediaHTTPService::makeHTTPConnection() {
  return new GonkMediaHTTPConnection(mResource, mResourceURI.c_str(),
                                     mResourceMIMEType.c_str());
}

}  // namespace android
