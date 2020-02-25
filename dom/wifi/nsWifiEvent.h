/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef nsWifiEvent_H
#define nsWifiEvent_H

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIWifiEvent.h"

class nsStateChanged;

class nsWifiEvent final : public nsIWifiEvent {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFIEVENT
  nsWifiEvent();
  nsWifiEvent(const nsAString& aName);

  void updateStateChanged(nsStateChanged* aStateChanged);

  nsString mName;
  nsString mBssid;
  bool mLocallyGenerated;
  uint32_t mReason;
  RefPtr<nsIStateChanged> mStateChanged;

 private:
  ~nsWifiEvent() {}
};

#endif  // nsWifiEvent_H
