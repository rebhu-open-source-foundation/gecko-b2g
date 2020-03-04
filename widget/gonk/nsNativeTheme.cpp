/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/ClearOnShutdown.h"
#include "nsNativeBasicTheme.h"

already_AddRefed<nsITheme> do_GetNativeThemeDoNotUseDirectly() {
  static nsCOMPtr<nsITheme> inst;

  if (!inst) {
    // We don't have a native Gonk theme, so we just use the basic one.
    inst = new nsNativeBasicTheme();

    mozilla::ClearOnShutdown(&inst);
  }

  return do_AddRef(inst);
}
