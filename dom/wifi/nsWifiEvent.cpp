/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "nsWifiEvent.h"

#define WIFIEVENT_CID                                \
  {                                                  \
    0x93c570c2, 0x1ece, 0x44f2, {                    \
      0x9a, 0xa5, 0x34, 0xc2, 0xcd, 0xca, 0xde, 0x40 \
    }                                                \
  }

/**
 * nsWifiEvent
 */
NS_IMPL_ISUPPORTS(nsWifiEvent, nsIWifiEvent)

nsWifiEvent::nsWifiEvent() {}

nsWifiEvent::nsWifiEvent(const nsAString& aName) { mName = aName; }

void nsWifiEvent::updateStateChanged(nsStateChanged* aStateChanged) {
  mStateChanged = aStateChanged;
}

NS_IMETHODIMP
nsWifiEvent::GetName(nsAString& aName) {
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiEvent::GetBssid(nsAString& aBssid) {
  aBssid = mBssid;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiEvent::GetLocallyGenerated(bool* aLocallyGenerated) {
  *aLocallyGenerated = mLocallyGenerated;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiEvent::GetReason(uint32_t* aReason) {
  *aReason = mReason;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiEvent::GetStateChanged(nsIStateChanged** aStateChanged) {
  RefPtr<nsIStateChanged> stateChanged(mStateChanged);
  stateChanged.forget(aStateChanged);
  return NS_OK;
}

NS_DEFINE_NAMED_CID(WIFIEVENT_CID);
