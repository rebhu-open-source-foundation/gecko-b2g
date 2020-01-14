/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef nsRilResponseResult_H
#define nsRilResponseResult_H

#include "nsISupports.h"
#include <nsISupportsImpl.h>
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"

#include "nsRilResult.h"
#include "nsIRilResponseResult.h"

class nsRilResponseResult final : public nsRilResult,
                                  public nsIRilResponseResult
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  //NS_DECL_ISUPPORTS
  NS_DECL_NSIRILRESPONSERESULT
  // For those has no parameter notify.
  nsRilResponseResult(const nsAString &aRilMessageType, int32_t aRilMessageToken, int32_t aErrorMsg);

  // For DeviceIdentity
  void updateDeviceIdentity(const nsAString &aImei, const nsAString &aImeisv, const nsAString &aEsn, const nsAString &aMeid);

  // For VoiceRadioTechnology
  void updateVoiceRadioTechnology(int32_t aRadioTech);

  // For updateBasebandVersion
  void updateBasebandVersion(const nsAString &aBasebandVersion);

  // For IccCardStatus
  void updateIccCardStatus(nsCardStatus* aCardStatus);

  // For VoiceRegStatus
  void updateVoiceRegStatus(nsVoiceRegState* aVoiceRegState);

  // For DataRegStatus
  void updateDataRegStatus(nsDataRegState* aDataRegState);

  // For Operator
  void updateOperator(nsOperatorInfo* aOperatorInfo);

  // For NetworkSelectionMode
  void updateNetworkSelectionMode(bool aNwModeManual);

  // For SignalStrength
  void updateSignalStrength(nsSignalStrength* aSignalStrength);

  // For GetSmscAddress
  void updateSmscAddress(const nsAString &aSmsc);

  // For getCurrentCalls
  void updateCurrentCalls(nsTArray<RefPtr<nsCall>> & aCalls);

  // For updateLastCallsFailCause
  void updateFailCause(int32_t aCauseCode, const nsAString &aVendorCause);

  // For getPreferredNetworkType
  void updatePreferredNetworkType(int32_t aType);

  // For getAvailableNetwork
  void updateAvailableNetworks(nsTArray<RefPtr<nsOperatorInfo>> & aAvailableNetworks);

  // For setupDataCall
  void updateDataCallResponse(nsSetupDataCallResult* aDcResponse);

  // For getDataCallList
  void updateDcList(nsTArray<RefPtr<nsSetupDataCallResult>> & aDcLists);

  // For getCellInfoList
  void updateCellInfoList(nsTArray<RefPtr<nsRilCellInfo>> & aCellInfoLists);

  // For getIMSI
  void updateIMSI(const nsAString &aIMSI);

  // For IccIOForApp
  void updateIccIoResult(nsIccIoResult* aIccIoResult);

  nsString mImei;
  nsString mImeisv;
  nsString mEsn;
  nsString mMeid;
  int32_t mRadioTech;
  nsString mBasebandVersion;
  RefPtr<nsCardStatus> mCardStatus;
  RefPtr<nsVoiceRegState> mVoiceRegState;
  RefPtr<nsDataRegState> mDataRegState;
  RefPtr<nsOperatorInfo> mOperatorInfo;
  bool mNwModeManual;
  RefPtr<nsSignalStrength> mSignalStrength;
  nsString mSmsc;
  nsTArray<RefPtr<nsCall>> mCalls;
  int32_t mCauseCode;
  nsString mVendorCause;
  int32_t mType;
  nsTArray<RefPtr<nsOperatorInfo>> mAvailableNetworks;
  RefPtr<nsSetupDataCallResult> mDcResponse;
  nsTArray<RefPtr<nsSetupDataCallResult>> mDcLists;
  nsTArray<RefPtr<nsRilCellInfo>> mCellInfoLists;
  nsString mIMSI;
  RefPtr<nsIIccIoResult> mIccIoResult;

private:
  ~nsRilResponseResult();

};


#endif //nsRilResponseResult_H
