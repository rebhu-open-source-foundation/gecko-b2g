/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "NetUtils.h"
#include "prinit.h"
#include "mozilla/Assertions.h"
#include "nsDebug.h"
#include "SystemProperty.h"

using mozilla::system::Property;

NetUtils::NetUtils() { mDhcpUtils.reset(new DhcpUtils()); }

int32_t NetUtils::do_dhcp_stop(const char* ifname) {
  return mDhcpUtils->DhcpStop(ifname);
}

int32_t NetUtils::do_dhcp_do_request(const char* ifname, char* ipaddr,
                                     char* gateway, uint32_t* prefixLength,
                                     char* dns1, char* dns2, char* server,
                                     uint32_t* lease, char* vendorinfo) {
  int32_t ret = -1;

  ret = mDhcpUtils->DhcpStart(ifname);
  if (ret != 0) return ret;

  char* dns[3] = {dns1, dns2, nullptr};
  char domains[Property::VALUE_MAX_LENGTH];
  char mtu[Property::VALUE_MAX_LENGTH];
  ret = mDhcpUtils->GetDhcpResults(ifname, ipaddr, gateway, prefixLength, dns,
                                   server, lease, vendorinfo, domains, mtu);
  return ret;
}
