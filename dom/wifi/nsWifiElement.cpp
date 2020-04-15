/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsWifiElement.h"

/**
 * nsWifiConfiguration
 */
nsWifiConfiguration::nsWifiConfiguration(
    const nsAString& aSsid, const nsAString& aBssid,
    const nsAString& aKeyManagement, const nsAString& aPsk,
    const nsAString& aWepKey, int32_t aWepTxKeyIndex, bool aScanSsid, bool aPmf,
    int32_t aProto, int32_t aAuthAlg, int32_t aGroupCipher,
    int32_t aPairwiseCipher, int32_t aEap, int32_t aEapPhase2,
    const nsAString& aIdentity, const nsAString& aAnonymousId,
    const nsAString& aPassword, const nsAString& aClientCert,
    const nsAString& aCaCert, const nsAString& aCaPath,
    const nsAString& aSubjectMatch, const nsAString& aEngineId, bool aEngine,
    const nsAString& aPrivateKeyId, const nsAString& aAltSubjectMatch,
    const nsAString& aDomainSuffixMatch, bool aProactiveKeyCaching) {
  mSsid = aSsid;
  mBssid = aBssid;
  mKeyManagement = aKeyManagement;
  mPsk = aPsk;
  mWepKey = aWepKey;
  mWepTxKeyIndex = aWepTxKeyIndex;
  mScanSsid = aScanSsid;
  mPmf = aPmf;
  mProto = aProto;
  mAuthAlg = aAuthAlg;
  mGroupCipher = aGroupCipher;
  mEap = aEap;
  mEapPhase2 = aEapPhase2;
  mIdentity = aIdentity;
  mAnonymousId = aAnonymousId;
  mPassword = aPassword;
  mClientCert = aClientCert;
  mCaCert = aCaCert;
  mCaPath = aCaPath;
  mSubjectMatch = aSubjectMatch;
  mEngineId = aEngineId;
  mEngine = aEngine;
  mPrivateKeyId = aPrivateKeyId;
  mAltSubjectMatch = aAltSubjectMatch;
  mDomainSuffixMatch = aDomainSuffixMatch;
  mProactiveKeyCaching = aProactiveKeyCaching;
}

NS_IMETHODIMP
nsWifiConfiguration::GetSsid(nsAString& aSsid) {
  aSsid = mSsid;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetBssid(nsAString& aBssid) {
  aBssid = mBssid;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetPsk(nsAString& aPsk) {
  aPsk = mPsk;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetWepKey(nsAString& aWepKey) {
  aWepKey = mWepKey;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetWepTxKeyIndex(int32_t* aWepTxKeyIndex) {
  *aWepTxKeyIndex = mWepTxKeyIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetScanSsid(bool* aScanSsid) {
  *aScanSsid = mScanSsid;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetPmf(bool* aPmf) {
  *aPmf = mPmf;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetKeyManagement(nsAString& aKeyManagement) {
  aKeyManagement = mKeyManagement;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetProto(int32_t* aProto) {
  *aProto = mProto;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetAuthAlg(int32_t* aAuthAlg) {
  *aAuthAlg = mAuthAlg;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetGroupCipher(int32_t* aGroupCipher) {
  *aGroupCipher = mGroupCipher;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetPairwiseCipher(int32_t* aPairwiseCipher) {
  *aPairwiseCipher = mPairwiseCipher;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetEap(int32_t* aEap) {
  *aEap = mEap;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetEapPhase2(int32_t* aEapPhase2) {
  *aEapPhase2 = mEapPhase2;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetIdentity(nsAString& aIdentity) {
  aIdentity = mIdentity;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetAnonymousId(nsAString& aAnonymousId) {
  aAnonymousId = mAnonymousId;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetPassword(nsAString& aPassword) {
  aPassword = mPassword;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetClientCert(nsAString& aClientCert) {
  aClientCert = mClientCert;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetCaCert(nsAString& aCaCert) {
  aCaCert = mCaCert;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetCaPath(nsAString& aCaPath) {
  aCaPath = mCaPath;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetSubjectMatch(nsAString& aSubjectMatch) {
  aSubjectMatch = mSubjectMatch;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetEngineId(nsAString& aEngineId) {
  aEngineId = mEngineId;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetEngine(bool* aEngine) {
  *aEngine = mEngine;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetPrivateKeyId(nsAString& aPrivateKeyId) {
  aPrivateKeyId = mPrivateKeyId;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetAltSubjectMatch(nsAString& aAltSubjectMatch) {
  aAltSubjectMatch = mAltSubjectMatch;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetDomainSuffixMatch(nsAString& aDomainSuffixMatch) {
  aDomainSuffixMatch = mDomainSuffixMatch;
  return NS_OK;
}

NS_IMETHODIMP
nsWifiConfiguration::GetProactiveKeyCaching(bool* aProactiveKeyCaching) {
  *aProactiveKeyCaching = mProactiveKeyCaching;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsWifiConfiguration, nsIWifiConfiguration);

/**
 * nsScanResult
 */
nsScanResult::nsScanResult(const nsAString& aSsid, const nsAString& aBssid,
                           const nsTArray<uint8_t>& aInfoElement,
                           uint32_t aFrequency, uint32_t aTsf,
                           uint32_t aCapability, int32_t aSignal,
                           bool aAssociated) {
  mSsid = aSsid;
  mBssid = aBssid;
  mInfoElement = aInfoElement;
  mFrequency = aFrequency;
  mTsf = aTsf;
  mCapability = aCapability;
  mSignal = aSignal;
  mAssociated = aAssociated;
}

NS_IMETHODIMP
nsScanResult::GetSsid(nsAString& aSsid) {
  aSsid = mSsid;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetBssid(nsAString& aBssid) {
  aBssid = mBssid;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetFrequency(uint32_t* aFrequency) {
  *aFrequency = mFrequency;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetTsf(uint32_t* aTsf) {
  *aTsf = mTsf;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetCapability(uint32_t* aCapability) {
  *aCapability = mCapability;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetSignal(int32_t* aSignal) {
  *aSignal = mSignal;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetAssociated(bool* aAssociated) {
  *aAssociated = mAssociated;
  return NS_OK;
}

NS_IMETHODIMP
nsScanResult::GetInfoElement(nsTArray<uint8_t>& aInfoElement) {
  aInfoElement = mInfoElement;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsScanResult, nsIScanResult);

/**
 * nsStateChanged
 */
nsStateChanged::nsStateChanged(uint32_t aState, uint32_t aId,
                               const nsAString& aBssid,
                               const nsAString& aSsid) {
  mState = aState;
  mId = aId;
  mBssid = aBssid;
  mSsid = aSsid;
}

NS_IMETHODIMP
nsStateChanged::GetState(uint32_t* aState) {
  *aState = mState;
  return NS_OK;
}

NS_IMETHODIMP
nsStateChanged::GetId(uint32_t* aId) {
  *aId = mId;
  return NS_OK;
}

NS_IMETHODIMP
nsStateChanged::GetSsid(nsAString& aSsid) {
  aSsid = mSsid;
  return NS_OK;
}

NS_IMETHODIMP
nsStateChanged::GetBssid(nsAString& aBssid) {
  aBssid = mBssid;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsStateChanged, nsIStateChanged);

/**
 * nsLinkLayerPacketStats
 */
nsLinkLayerPacketStats::nsLinkLayerPacketStats(uint64_t aRxMpdu,
                                               uint64_t aTxMpdu,
                                               uint64_t aLostMpdu,
                                               uint64_t aRetries) {
  mRxMpdu = aRxMpdu;
  mTxMpdu = aTxMpdu;
  mLostMpdu = aLostMpdu;
  mRetries = aRetries;
}

NS_IMETHODIMP
nsLinkLayerPacketStats::GetRxMpdu(uint64_t* aRxMpdu) {
  *aRxMpdu = mRxMpdu;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerPacketStats::GetTxMpdu(uint64_t* aTxMpdu) {
  *aTxMpdu = mTxMpdu;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerPacketStats::GetLostMpdu(uint64_t* aLostMpdu) {
  *aLostMpdu = mLostMpdu;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerPacketStats::GetRetries(uint64_t* aRetries) {
  *aRetries = mRetries;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsLinkLayerPacketStats, nsILinkLayerPacketStats);

/**
 * nsLinkLayerRadioStats
 */
nsLinkLayerRadioStats::nsLinkLayerRadioStats(
    uint32_t aOnTimeInMs, uint32_t aTxTimeInMs, uint32_t aRxTimeInMs,
    uint32_t aOnTimeInMsForScan,
    const nsTArray<uint32_t>& aTxTimeInMsPerLevel) {
  mOnTimeInMs = aOnTimeInMs;
  mTxTimeInMs = aTxTimeInMs;
  mRxTimeInMs = aRxTimeInMs;
  mOnTimeInMsForScan = aOnTimeInMsForScan;
  mTxTimeInMsPerLevel = aTxTimeInMsPerLevel;
}

NS_IMETHODIMP
nsLinkLayerRadioStats::GetOnTimeInMs(uint32_t* aOnTimeInMs) {
  *aOnTimeInMs = mOnTimeInMs;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerRadioStats::GetTxTimeInMs(uint32_t* aTxTimeInMs) {
  *aTxTimeInMs = mTxTimeInMs;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerRadioStats::GetRxTimeInMs(uint32_t* aRxTimeInMs) {
  *aRxTimeInMs = mRxTimeInMs;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerRadioStats::GetOnTimeInMsForScan(uint32_t* aOnTimeInMsForScan) {
  *aOnTimeInMsForScan = mOnTimeInMsForScan;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerRadioStats::GetTxTimeInMsPerLevel(
    nsTArray<uint32_t>& aTxTimeInMsPerLevel) {
  aTxTimeInMsPerLevel = mTxTimeInMsPerLevel;
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsLinkLayerRadioStats, nsILinkLayerRadioStats);

/**
 * nsLinkLayerStats
 */
nsLinkLayerStats::nsLinkLayerStats(uint32_t aBeaconRx, int32_t aAvgRssiMgmt,
                                   uint64_t aTimeStampMs) {
  mBeaconRx = aBeaconRx;
  mAvgRssiMgmt = aAvgRssiMgmt;
  mTimeStampMs = aTimeStampMs;
}

void nsLinkLayerStats::updatePacketStats(
    nsLinkLayerPacketStats* aWmeBePktStats,
    nsLinkLayerPacketStats* aWmeBkPktStats,
    nsLinkLayerPacketStats* aWmeViPktStats,
    nsLinkLayerPacketStats* aWmeVoPktStats) {
  mWmeBePktStats = aWmeBePktStats;
  mWmeBkPktStats = aWmeBkPktStats;
  mWmeViPktStats = aWmeViPktStats;
  mWmeVoPktStats = aWmeVoPktStats;
}

void nsLinkLayerStats::updateRadioStats(
    nsTArray<RefPtr<nsLinkLayerRadioStats>>& aLinkLayerRadioStats) {
  mLinkLayerRadioStats = aLinkLayerRadioStats;
}

NS_IMETHODIMP
nsLinkLayerStats::GetBeaconRx(uint32_t* aBeaconRx) {
  *aBeaconRx = mBeaconRx;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetAvgRssiMgmt(int32_t* aAvgRssiMgmt) {
  *aAvgRssiMgmt = mAvgRssiMgmt;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetTimeStampMs(uint64_t* aTimeStampMs) {
  *aTimeStampMs = mTimeStampMs;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetWmeBePktStats(nsILinkLayerPacketStats** aWmeBePktStats) {
  RefPtr<nsILinkLayerPacketStats> stats(mWmeBePktStats);
  stats.forget(aWmeBePktStats);
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetWmeBkPktStats(nsILinkLayerPacketStats** aWmeBkPktStats) {
  RefPtr<nsILinkLayerPacketStats> stats(mWmeBkPktStats);
  stats.forget(aWmeBkPktStats);
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetWmeViPktStats(nsILinkLayerPacketStats** aWmeViPktStats) {
  RefPtr<nsILinkLayerPacketStats> stats(mWmeViPktStats);
  stats.forget(aWmeViPktStats);
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetWmeVoPktStats(nsILinkLayerPacketStats** aWmeVoPktStats) {
  RefPtr<nsILinkLayerPacketStats> stats(mWmeVoPktStats);
  stats.forget(aWmeVoPktStats);
  return NS_OK;
}

NS_IMETHODIMP
nsLinkLayerStats::GetLinkLayerRadioStats(
    nsTArray<RefPtr<nsILinkLayerRadioStats>>& aLinkLayerRadioStats) {
  for (size_t i = 0; i < mLinkLayerRadioStats.Length(); i++) {
    aLinkLayerRadioStats.AppendElement(mLinkLayerRadioStats[i]);
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsLinkLayerStats, nsILinkLayerStats);
