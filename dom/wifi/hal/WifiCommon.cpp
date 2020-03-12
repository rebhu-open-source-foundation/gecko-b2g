/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "WifiCommon"

#include "WifiCommon.h"
#include <iomanip>

template <typename T>
std::string ConvertMacToString(const T& mac) {
  size_t len = mac.size();
  if (len != 6) {
    return std::string();
  }
  std::ostringstream stream;
  stream << std::hex << std::uppercase;
  for (size_t i = 0; i < len; i++) {
    stream << std::setw(2) << std::setfill('0') << unsigned(mac[i]);
    if (i != len - 1) stream << ":";
  }
  return stream.str();
}
