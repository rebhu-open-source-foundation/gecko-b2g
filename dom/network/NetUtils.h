/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Abstraction on top of the network support from libnetutils that we
 * use to set up network connections.
 */

#ifndef NetUtils_h
#define NetUtils_h

#include <mozilla/UniquePtr.h>
#include "DhcpUtils.h"

// Implements netutils functions. No need for an abstract class here since we
// only have a one sdk specific method (dhcp_do_request)
class NetUtils {
 public:
  NetUtils();

  int32_t do_dhcp_stop(const char* ifname);
  int32_t do_dhcp_do_request(const char* ifname, char* ipaddr, char* gateway,
                             uint32_t* prefixLength, char* dns1, char* dns2,
                             char* server, uint32_t* lease, char* vendorinfo);

  static int32_t SdkVersion();

  mozilla::UniquePtr<DhcpUtils> mDhcpUtils;
};

#endif  // NetUtils_h
