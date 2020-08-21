/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MediaFormatReaderProxy.h"

namespace mozilla {

MediaFormatReaderProxy::MediaFormatReaderProxy(MediaFormatReader* aReader)
    : mTarget(RefPtr<MediaFormatReader>(aReader)) {
  DDLINKCHILD("media format reader", aReader);
}

MediaFormatReaderProxy::MediaFormatReaderProxy(MediaOffloadPlayer* aPlayer)
    : mTarget(RefPtr<MediaOffloadPlayer>(aPlayer)) {
  DDLINKCHILD("offload player", aPlayer);
}

MediaFormatReaderProxy::~MediaFormatReaderProxy() {}

void MediaFormatReaderProxy::NotifyDataArrived() {
  return mTarget.match([&](auto& aTarget) -> decltype(auto) {
    return aTarget->NotifyDataArrived();
  });
}

RefPtr<SetCDMPromise> MediaFormatReaderProxy::SetCDMProxy(CDMProxy* aProxy) {
  return mTarget.match([&](auto& aTarget) -> decltype(auto) {
    return aTarget->SetCDMProxy(aProxy);
  });
}

void MediaFormatReaderProxy::UpdateCompositor(
    already_AddRefed<layers::KnowsCompositor> aCompositor) {
  RefPtr<layers::KnowsCompositor> compositor(aCompositor);
  return mTarget.match([&](auto& aTarget) -> decltype(auto) {
    return aTarget->UpdateCompositor(compositor.forget());
  });
}

}  // namespace mozilla
