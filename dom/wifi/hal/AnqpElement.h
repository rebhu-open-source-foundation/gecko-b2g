/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef AnqpElement_H
#define AnqpElement_H

#include "WifiCommon.h"

BEGIN_WIFI_NAMESPACE

enum class AnqpElementType : uint32_t {
  ANQPQueryList = 256,
  ANQPVenueName = 258,
  ANQPRoamingConsortium = 261,
  ANQPIPAddrAvailability = 262,
  ANQPNAIRealm = 263,
  ANQP3GPPNetwork = 264,
  ANQPDomName = 268,
  ANQPVendorSpec = 56797,
  HSQueryList = 1,
  HSFriendlyName = 3,
  HSWANMetrics = 4,
  HSConnCapability = 5,
  HSNAIHomeRealmQuery = 6,
  HSOSUProviders = 8,
  HSIconRequest = 10,
  HSIconFile = 11
};

constexpr static uint32_t R1_ANQP_SET[] = {
    (uint32_t)AnqpElementType::ANQPVenueName,
    (uint32_t)AnqpElementType::ANQPIPAddrAvailability,
    (uint32_t)AnqpElementType::ANQPNAIRealm,
    (uint32_t)AnqpElementType::ANQP3GPPNetwork,
    (uint32_t)AnqpElementType::ANQPDomName};

constexpr static uint32_t R2_ANQP_SET[] = {
    (uint32_t)AnqpElementType::HSFriendlyName,
    (uint32_t)AnqpElementType::HSWANMetrics,
    (uint32_t)AnqpElementType::HSConnCapability,
    (uint32_t)AnqpElementType::HSOSUProviders};

class AnqpElement {
  // TODO: parsing anqp elements.
};

END_WIFI_NAMESPACE

#endif  // AnqpElement_H
