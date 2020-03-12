/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/**
 * Abstraction on top of the network support from libnetutils that we
 * use to set up network connections.
 */

#ifndef DhcpUtils_h
#define DhcpUtils_h

class DhcpUtils {
 public:
  DhcpUtils();

  int GetDhcpResults(const char* interface, char* ipAddr, char* gateway,
                     uint32_t* prefixLength, char* dns[], char* server,
                     uint32_t* lease, char* vendorInfo, char* domain,
                     char* mtu);
  int DhcpStart(const char* interface);
  int DhcpStop(const char* interface);
  int DhcpRelease(const char* interface);
  int DhcpRenew(const char* interface);
  char* GetErrMsg();
};
#endif  // DhcpUtils_h
