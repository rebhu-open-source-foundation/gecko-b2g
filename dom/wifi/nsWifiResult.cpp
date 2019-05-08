/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "nsWifiResult.h"

#define WIFIRESULT_CID                               \
  {                                                  \
    0x5c610641, 0x1da9, 0x4741, {                    \
      0xa2, 0x6a, 0x8d, 0xff, 0x3c, 0x5e, 0x55, 0xf5 \
    }                                                \
  }

/**
 * nsWifiResult
 */
NS_IMPL_ISUPPORTS(nsWifiResult, nsIWifiResult)

nsWifiResult::nsWifiResult() : mId(0) {}

nsWifiResult::nsWifiResult(uint32_t aId, bool aStatus)
    : mId(aId), mStatus(aStatus) {}

void nsWifiResult::updateScanResults(
    nsTArray<RefPtr<nsScanResult>>& aScanResults) {
  mScanResults = aScanResults;
}

NS_IMETHODIMP
nsWifiResult::GetId(uint32_t* aId) {
  *aId = mId;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiResult::GetStatus(bool* aStatus) {
  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiResult::GetCapabilities(uint32_t* aCapabilities) {
  *aCapabilities = mCapabilities;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiResult::GetDebugLevel(uint32_t* aDebugLevel) {
  *aDebugLevel = mDebugLevel;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiResult::GetScanResults(uint32_t* count, nsIScanResult*** aScanResults) {
  *count = mScanResults.Length();
  nsIScanResult** scanResult =
      (nsIScanResult**)moz_xmalloc(*count * sizeof(nsIScanResult*));

  for (uint32_t i = 0; i < *count; i++) {
    NS_ADDREF(scanResult[i] = mScanResults[i]);
  }

  *aScanResults = scanResult;
  return NS_OK;
}

NS_DEFINE_NAMED_CID(WIFIRESULT_CID);
