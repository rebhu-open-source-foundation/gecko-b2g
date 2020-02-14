/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef nsWifiResult_H
#define nsWifiResult_H

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIWifiResult.h"

class nsScanResult;

class nsWifiResult final : public nsIWifiResult {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFIRESULT

  nsWifiResult();

  void updateScanResults(nsTArray<RefPtr<nsScanResult>>& aScanResults);
  void updateChannels(nsTArray<int32_t>& aChannels);

  uint32_t mId;
  bool mStatus;

  nsString mDriverVersion;
  nsString mFirmwareVersion;
  nsString mMacAddress;
  nsString mStaInterface;
  nsString mApInterface;
  uint32_t mCapabilities;
  uint32_t mStaCapabilities;
  uint32_t mDebugLevel;
  nsTArray<int32_t> mChannels;
  nsTArray<RefPtr<nsScanResult>> mScanResults;

 private:
  ~nsWifiResult(){};
};

#endif  // nsWifiResult_H
