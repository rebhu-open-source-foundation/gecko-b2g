/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef nsWifiElement_H
#define nsWifiElement_H

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIWifiElement.h"

/**
 * nsIWifiConfiguration
 */
class nsWifiConfiguration final : public nsIWifiConfiguration {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFICONFIGURATION
  nsWifiConfiguration(
      const nsAString& aSsid, const nsAString& aBssid, const nsAString& aPsk,
      const nsAString& aWepKey, int32_t aWepTxKeyIndex, bool aScanSsid,
      bool aPmf, uint32_t aKeyManagement, int32_t aProto, int32_t aAuthAlg,
      int32_t aGroupCipher, int32_t aPairwiseCipher, int32_t aEap,
      int32_t aEapPhase2, const nsAString& aIdentity,
      const nsAString& aAnonymousId, const nsAString& aPassword,
      const nsAString& aClientCert, const nsAString& aCaCert,
      const nsAString& aCaPath, const nsAString& aSubjectMatch,
      const nsAString& aEngineId, bool aEngine, const nsAString& aPrivateKeyId,
      const nsAString& aAltSubjectMatch, const nsAString& aDomainSuffixMatch,
      bool aProactiveKeyCaching);

 private:
  ~nsWifiConfiguration(){};

  nsString mSsid;
  nsString mBssid;
  nsString mPsk;
  nsString mWepKey;
  int32_t mWepTxKeyIndex;
  bool mScanSsid;
  bool mPmf;
  uint32_t mKeyManagement;

  int32_t mProto;
  int32_t mAuthAlg;
  int32_t mGroupCipher;
  int32_t mPairwiseCipher;

  int32_t mEap;
  int32_t mEapPhase2;
  nsString mIdentity;
  nsString mAnonymousId;
  nsString mPassword;
  nsString mClientCert;
  nsString mCaCert;
  nsString mCaPath;
  nsString mSubjectMatch;
  nsString mEngineId;
  bool mEngine;
  nsString mPrivateKeyId;
  nsString mAltSubjectMatch;
  nsString mDomainSuffixMatch;
  bool mProactiveKeyCaching;
};

class nsScanResult final : public nsIScanResult {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISCANRESULT
  nsScanResult(const nsAString& aSsid, const nsAString& aBssid,
               const nsAString& aInfoElement, uint32_t aFrequency,
               uint32_t aTsf, uint32_t aCapability, int32_t aSignal,
               bool aAssociated);

 private:
  ~nsScanResult(){};

  nsString mSsid;
  nsString mBssid;
  nsString mInfoElement;
  uint32_t mFrequency;
  uint32_t mTsf;
  uint32_t mCapability;
  int32_t mSignal;
  bool mAssociated;
};

class nsStateChanged final : public nsIStateChanged {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTATECHANGED
  nsStateChanged(uint32_t aState, uint32_t aId, const nsAString& aBssid,
                 const nsAString& aSsid);

 private:
  ~nsStateChanged() {}

  uint32_t mState;
  uint32_t mId;
  nsString mBssid;
  nsString mSsid;
};

#endif  // nsWifiElement_H
