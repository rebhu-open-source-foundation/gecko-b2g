/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef nsRilIndication_H
#define nsRilIndication_H
#include <nsISupportsImpl.h>
#include <nsTArray.h>
#include "nsRilWorker.h"

#include <android/hardware/radio/1.0/IRadioIndication.h>
#include <android/hardware/radio/1.0/types.h>

using namespace ::android::hardware::radio::V1_0;
using ::android::hardware::radio::V1_0::IRadioIndication;
using ::android::hardware::radio::V1_0::RadioState;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;


class nsRilWorker;

class nsRilIndication : public IRadioIndication {

public:

  RefPtr<nsRilWorker> mRIL;
  nsRilIndication(nsRilWorker * aRil);
  ~nsRilIndication();

  Return<void> radioStateChanged(RadioIndicationType type, RadioState radioState);

  Return<void> callStateChanged(RadioIndicationType type);

  Return<void> networkStateChanged(RadioIndicationType type);

  Return<void> newSms(RadioIndicationType type,
                      const ::android::hardware::hidl_vec<uint8_t>& pdu);

  Return<void> newSmsStatusReport(RadioIndicationType type,
                                  const ::android::hardware::hidl_vec<uint8_t>& pdu);

  Return<void> newSmsOnSim(RadioIndicationType type, int32_t recordNumber);

  Return<void> onUssd(RadioIndicationType type, UssdModeType modeType,
                      const ::android::hardware::hidl_string& msg);

  Return<void> nitzTimeReceived(RadioIndicationType type,
                                const ::android::hardware::hidl_string& nitzTime,
                                uint64_t receivedTime);

  Return<void> currentSignalStrength(RadioIndicationType type,
                                     const SignalStrength& sig_strength);

  Return<void> dataCallListChanged(
      RadioIndicationType type, const ::android::hardware::hidl_vec<SetupDataCallResult>& dcList);

  Return<void> suppSvcNotify(RadioIndicationType type, const SuppSvcNotification& suppSvc);

  Return<void> stkSessionEnd(RadioIndicationType type);

  Return<void> stkProactiveCommand(RadioIndicationType type,
                                   const ::android::hardware::hidl_string& cmd);

  Return<void> stkEventNotify(RadioIndicationType type,
                              const ::android::hardware::hidl_string& cmd);

  Return<void> stkCallSetup(RadioIndicationType type, int64_t timeout);

  Return<void> simSmsStorageFull(RadioIndicationType type);

  Return<void> simRefresh(RadioIndicationType type, const SimRefreshResult& refreshResult);

  Return<void> callRing(RadioIndicationType type, bool isGsm, const CdmaSignalInfoRecord& record);

  Return<void> simStatusChanged(RadioIndicationType type);

  Return<void> cdmaNewSms(RadioIndicationType type, const CdmaSmsMessage& msg);

  Return<void> newBroadcastSms(RadioIndicationType type,
                               const ::android::hardware::hidl_vec<uint8_t>& data);

  Return<void> cdmaRuimSmsStorageFull(RadioIndicationType type);

  Return<void> restrictedStateChanged(RadioIndicationType type, PhoneRestrictedState state);

  Return<void> enterEmergencyCallbackMode(RadioIndicationType type);

  Return<void> cdmaCallWaiting(RadioIndicationType type,
                               const CdmaCallWaiting& callWaitingRecord);

  Return<void> cdmaOtaProvisionStatus(RadioIndicationType type, CdmaOtaProvisionStatus status);

  Return<void> cdmaInfoRec(RadioIndicationType type, const CdmaInformationRecords& records);

  Return<void> indicateRingbackTone(RadioIndicationType type, bool start);

  Return<void> resendIncallMute(RadioIndicationType type);

  Return<void> cdmaSubscriptionSourceChanged(RadioIndicationType type,
                                             CdmaSubscriptionSource cdmaSource);

  Return<void> cdmaPrlChanged(RadioIndicationType type, int32_t version);

  Return<void> exitEmergencyCallbackMode(RadioIndicationType type);

  Return<void> rilConnected(RadioIndicationType type);

  Return<void> voiceRadioTechChanged(RadioIndicationType type, RadioTechnology rat);

  Return<void> cellInfoList(RadioIndicationType type,
                            const ::android::hardware::hidl_vec<CellInfo>& records);

  Return<void> imsNetworkStateChanged(RadioIndicationType type);

  Return<void> subscriptionStatusChanged(RadioIndicationType type, bool activate);

  Return<void> srvccStateNotify(RadioIndicationType type, SrvccState state);

  Return<void> hardwareConfigChanged(
      RadioIndicationType type, const ::android::hardware::hidl_vec<HardwareConfig>& configs);

  Return<void> radioCapabilityIndication(RadioIndicationType type, const RadioCapability& rc);

  Return<void> onSupplementaryServiceIndication(RadioIndicationType type,
                                                const StkCcUnsolSsResult& ss);

  Return<void> stkCallControlAlphaNotify(RadioIndicationType type,
                                         const ::android::hardware::hidl_string& alpha);

  Return<void> lceData(RadioIndicationType type, const LceDataInfo& lce);

  Return<void> pcoData(RadioIndicationType type, const PcoDataInfo& pco);

  Return<void> modemReset(RadioIndicationType type,
                          const ::android::hardware::hidl_string& reason);

private:
  int32_t convertRadioStateToNum(RadioState state);

};

#endif
