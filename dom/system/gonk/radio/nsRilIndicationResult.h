/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef nsRilIndicationResult_H
#define nsRilIndicationResult_H

#include "nsISupports.h"
#include <nsISupportsImpl.h>
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsRilResult.h"
#include "nsIRilIndicationResult.h"

class nsRilIndicationResult final : public nsRilResult,
                                    public nsIRilIndicationResult
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  //NS_DECL_ISUPPORTS
  NS_DECL_NSIRILINDICATIONRESULT
  // For those has no parameter notify.
  nsRilIndicationResult(const nsAString &aRilMessageType);
  // For radioStateChanged
  void updateRadioStateChanged(int32_t aRadioState);
  // For newSmsOnSim
  void updateNewSmsOnSim(int32_t aRecordNumber);
  // For onUssd
  void updateOnUssd(int32_t aTypeCode, const nsAString &aMessage);
  // For nitzTimeReceived
  void updateNitzTimeReceived(const nsAString &aDateString, int64_t aReceiveTimeInMS);
  // For currentSignalStrength
  void updateCurrentSignalStrength(nsSignalStrength* aSignalStrength);
  // For dataCallListChanged
  void updateDataCallListChanged(nsTArray<RefPtr<nsSetupDataCallResult>> & aDatacalls);
  // For suppSvcNotify
  void updateSuppSvcNotify(nsSuppSvcNotification* aSuppSvc);
  // For stkProactiveCommand & stkEventNotify
  void updateStkProactiveCommand(const nsAString &aCmd);
  // For stkEventNotify
  void updateStkEventNotify(const nsAString &aCmd);
  // For stkCallSetup
  void updateStkCallSetup(int32_t aTimeout);
  // For simRefresh
  void updateSimRefresh(nsSimRefreshResult* aRefreshResult);
  // For callRing
  void updateCallRing(bool aIsGsm);
  // For newBroadcastSms
  void updateNewBroadcastSms(nsTArray<int32_t>& aData);
  // For restrictedStateChanged
  void updateRestrictedStateChanged(int32_t aRestrictedState);
  // For indicateRingbackTone
  void updateIndicateRingbackTone(bool aPlayRingbackTone);
  // For voiceRadioTechChanged
  void updateVoiceRadioTechChanged(int32_t aRat);
  // For cellInfoList
  void updateCellInfoList(nsTArray<RefPtr<nsRilCellInfo>>& aRecords);
  // For subscriptionStatusChanged
  void updateSubscriptionStatusChanged(bool aActivate);
  // For srvccStateNotify
  void updateSrvccStateNotify(int32_t aSrvccState);
  // For hardwareConfigChanged
  void updateHardwareConfigChanged(nsTArray<RefPtr<nsHardwareConfig>>& aConfigs);
  // For radioCapabilityIndication
  void updateRadioCapabilityIndication(nsRadioCapability* aRc);
  // For stkCallControlAlphaNotify
  void updateStkCallControlAlphaNotify(const nsAString &aAlpha);
  // For lceData
  void updateLceData(nsILceDataInfo* aLce);
  // For pcoData
  void updatePcoData(nsIPcoDataInfo* aPco);
  // For modemReset
  void updateModemReset(const nsAString &aReason);

  int32_t mRadioState;
  int32_t mRecordNumber;
  int32_t mTypeCode;
  nsString mMessage;
  nsString mDateString;
  int64_t mReceiveTimeInMS;
  RefPtr<nsSignalStrength> mSignalStrength;
  nsTArray<RefPtr<nsSetupDataCallResult>> mDatacalls;
  RefPtr<nsSuppSvcNotification> mSuppSvc;
  nsString mCmd;
  int32_t mTimeout;
  RefPtr<nsSimRefreshResult> mRefreshResult;
  bool mIsGsm;
  nsTArray<int32_t> mData;
  int32_t mRestrictedState;
  bool mPlayRingbackTone;
  int32_t mRadioTech;
  nsTArray<RefPtr<nsRilCellInfo>> mRecords;
  bool mActivate;
  int32_t mSrvccState;
  nsTArray<RefPtr<nsHardwareConfig>> mConfigs;
  RefPtr<nsRadioCapability> mRc;
  nsString mAlpha;
  nsCOMPtr<nsILceDataInfo> mLce;
  nsCOMPtr<nsIPcoDataInfo> mPco;
  nsString mReason;

private:
  ~nsRilIndicationResult();
};


#endif //nsRilIndicationResult_H
