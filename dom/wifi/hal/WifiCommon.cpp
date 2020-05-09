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

static int32_t HexToNum(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

template <typename T>
int32_t ConvertMacToByteArray(const std::string& mac, T& out) {
  size_t pos = 0;
  for (size_t i = 0; i < 6; i++) {
    int32_t a, b;

    while (mac.at(pos) == ':' || mac.at(pos) == '.' || mac.at(pos) == '-') {
      pos = pos + 1;
    }

    a = HexToNum(mac.at(pos++));
    if (a < 0) return -1;
    b = HexToNum(mac.at(pos++));
    if (b < 0) return -1;
    out[i] = (a << 4) | b;
  }
  return pos;
}

template <typename T>
int32_t ConvertHexStringToBytes(const std::string& in, T& out) {
  if (in.empty()) return -1;
  if (in.size() % 2 != 0) return -1;

  std::string::const_iterator it = in.cbegin();
  while (it != in.cend()) {
    int32_t number = (HexToNum(*it) << 4) | (HexToNum(*(it + 1)));
    if (number < 0) return -1;
    out.push_back(number);
    it += 2;
  }
  return out.size();
}

template <typename T>
int32_t ConvertHexStringToByteArray(const std::string& in, T& out) {
  if (in.empty()) return -1;
  if (in.size() % 2 != 0) return -1;

  uint32_t index = 0;
  std::string::const_iterator it = in.cbegin();
  while (it != in.cend()) {
    int32_t number = (HexToNum(*it) << 4) | (HexToNum(*(it + 1)));
    if (number < 0) return -1;
    out[index++] = number;
    it += 2;
  }
  return out.size();
}

void Dequote(std::string& s) {
  if (s.front() != '"' || s.back() != '"') {
    return;
  }
  s.erase(0, 1);
  s.erase(s.length() - 1);
}