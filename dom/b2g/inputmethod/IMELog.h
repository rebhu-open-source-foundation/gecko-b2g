/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_IMELog_h
#define mozilla_IMELog_h

#include "mozilla/Logging.h"

namespace mozilla {
namespace dom {

extern mozilla::LogModule* GetIMELog();
#define IME_LOG(type, ...) \
  MOZ_LOG(mozilla::dom::GetIMELog(), (mozilla::LogLevel)type, (__VA_ARGS__))
#define IME_LOGD(...) \
  MOZ_LOG(mozilla::dom::GetIMELog(), LogLevel::Debug, (__VA_ARGS__))
#define IME_ERR(...) \
  MOZ_LOG(mozilla::dom::GetIMELog(), LogLevel::Error, (__VA_ARGS__))

}  // namespace dom

}  // namespace mozilla

#endif  // mozilla_IMELog_h
