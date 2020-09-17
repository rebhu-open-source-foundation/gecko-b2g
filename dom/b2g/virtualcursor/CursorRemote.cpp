/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CursorRemote.h"
#include "mozilla/dom/BrowserChild.h"

extern mozilla::LazyLogModule gVirtualCursorLog;

namespace mozilla {
namespace dom {

typedef dom::BrowserChild BrowserChild;

NS_IMPL_ISUPPORTS0(CursorRemote)

CursorRemote::~CursorRemote() {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug, ("CursorRemote destruct"));
}

void CursorRemote::UpdatePos(const LayoutDeviceIntPoint& aPos) {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendUpdateCursorPos(aPos);
  }
}

void CursorRemote::CursorClick() {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorClick();
  }
}

void CursorRemote::CursorDown() {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorDown();
  }
}

void CursorRemote::CursorUp() {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorUp();
  }
}

void CursorRemote::CursorMove() {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorMove();
  }
}

void CursorRemote::CursorOut(bool aCheckActive) {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorOut();
  }
}

void CursorRemote::ShowContextMenu() {
  RefPtr<dom::BrowserChild> browserChild = BrowserChild::GetFrom(mOuterWindow);
  if (browserChild) {
    browserChild->SendCursorShowContextMenu();
  }
}

}  // namespace dom
}  // namespace mozilla
