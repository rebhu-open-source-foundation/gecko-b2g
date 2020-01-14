/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "nsRilIndication.h"

/* Logging related */
#undef LOG_TAG
#define LOG_TAG "RilIndication"

#undef INFO
#undef ERROR
#undef DEBUG
#define INFO(args...) \
  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##args)
#define ERROR(args...) \
  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##args)
#define DEBUG(args...) \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, ##args)

/**
 *
 */
nsRilIndication::nsRilIndication(nsRilWorker* aRil) {
  INFO("init nsRilIndication");
  mRIL = aRil;
  INFO("init nsRilIndication done");
}

nsRilIndication::~nsRilIndication() {
  INFO("Destructor nsRilIndication");
  mRIL = nullptr;
  MOZ_ASSERT(!mRIL);
}

Return<void> nsRilIndication::radioStateChanged(RadioIndicationType type,
                                                RadioState radioState) {
    INFO("radioStateChanged");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("radiostatechange"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    result->updateRadioStateChanged(convertRadioStateToNum(radioState));
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::callStateChanged(RadioIndicationType type) {
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("callStateChanged"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::networkStateChanged(RadioIndicationType type) {
    INFO("networkStateChanged");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("networkStateChanged"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);

    return Void();
}

Return<void> nsRilIndication::newSms(RadioIndicationType /*type*/,
                                     const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    INFO("Not implement newSms");
    return Void();
}

Return<void> nsRilIndication::newSmsStatusReport(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    INFO("Not implement newSmsStatusReport");
    return Void();
}

Return<void> nsRilIndication::newSmsOnSim(RadioIndicationType /*type*/, int32_t /*recordNumber*/) {
    INFO("Not implement newSmsOnSim");
    return Void();
}

Return<void> nsRilIndication::onUssd(RadioIndicationType /*type*/, UssdModeType /*modeType*/,
                                     const ::android::hardware::hidl_string& /*msg*/) {
    INFO("Not implement onUssd");
    return Void();
}

Return<void> nsRilIndication::nitzTimeReceived(RadioIndicationType type,
                                               const ::android::hardware::hidl_string& nitzTime,
                                               uint64_t receivedTime) {
    INFO("nitzTimeReceived");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("nitzTimeReceived"));

    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    result->updateNitzTimeReceived(NS_ConvertUTF8toUTF16(nitzTime.c_str()), receivedTime);
    mRIL->sendRilIndicationResult(result);

    return Void();
}

Return<void> nsRilIndication::currentSignalStrength(RadioIndicationType type,
                                                    const SignalStrength& sig_strength) {
    INFO("currentSignalStrength");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("signalstrengthchange"));

    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    RefPtr<nsSignalStrength> signalStrength = result->convertSignalStrength(sig_strength);
    result->updateCurrentSignalStrength(signalStrength);
    mRIL->sendRilIndicationResult(result);

    return Void();
}

Return<void> nsRilIndication::dataCallListChanged(
    RadioIndicationType type,
    const ::android::hardware::hidl_vec<SetupDataCallResult>& dcList) {

  INFO("dataCallListChanged");
  mRIL->processIndication(type);

  RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(NS_LITERAL_STRING("datacalllistchanged"));
  uint32_t numDataCall = dcList.size();
  INFO("getDataCallListResponse numDataCall= %d" , numDataCall);
  nsTArray<RefPtr<nsSetupDataCallResult>> aDcLists(numDataCall);

  for (uint32_t i = 0; i < numDataCall; i++) {
    RefPtr<nsSetupDataCallResult> datcall = result->convertDcResponse(dcList[i]);
    aDcLists.AppendElement(datcall);
  }
  result->updateDataCallListChanged(aDcLists);


  mRIL->sendRilIndicationResult(result);
  return Void();
}

Return<void> nsRilIndication::suppSvcNotify(RadioIndicationType /*type*/,
                                            const SuppSvcNotification& /*suppSvc*/) {
    INFO("Not implement suppSvcNotify");
    return Void();
}

Return<void> nsRilIndication::stkSessionEnd(RadioIndicationType type) {

     mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("stkSessionEnd"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::stkProactiveCommand(RadioIndicationType /*type*/,
                                                  const ::android::hardware::hidl_string& /*cmd*/) {
    INFO("Not implement stkProactiveCommand");
    return Void();
}

Return<void> nsRilIndication::stkEventNotify(RadioIndicationType /*type*/,
                                             const ::android::hardware::hidl_string& /*cmd*/) {
    INFO("Not implement stkEventNotify");
    return Void();
}

Return<void> nsRilIndication::stkCallSetup(RadioIndicationType /*type*/, int64_t /*timeout*/) {
    INFO("Not implement stkCallSetup");
    return Void();
}

Return<void> nsRilIndication::simSmsStorageFull(RadioIndicationType type) {
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("simSmsStorageFull"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::simRefresh(RadioIndicationType /*type*/,
                                         const SimRefreshResult& /*refreshResult*/) {
    INFO("Not implement simRefresh");
    return Void();
}

Return<void> nsRilIndication::callRing(RadioIndicationType type, bool isGsm,
                                       const CdmaSignalInfoRecord& record) {
    mRIL->processIndication(type);

    //TODO impelment CDMA later.

    nsString rilmessageType (NS_LITERAL_STRING("callRing"));

    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::simStatusChanged(RadioIndicationType type) {
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("simStatusChanged"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::cdmaNewSms(RadioIndicationType /*type*/,
                                         const CdmaSmsMessage& /*msg*/) {
    INFO("Not implement cdmaNewSms");
    return Void();
}

Return<void> nsRilIndication::newBroadcastSms(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    INFO("Not implement newBroadcastSms");
    return Void();
}

Return<void> nsRilIndication::cdmaRuimSmsStorageFull(RadioIndicationType /*type*/) {
    INFO("Not implement cdmaRuimSmsStorageFull");
    return Void();
}

Return<void> nsRilIndication::restrictedStateChanged(RadioIndicationType /*type*/,
                                                     PhoneRestrictedState /*state*/) {
    INFO("Not implement restrictedStateChanged");
    return Void();
}

Return<void> nsRilIndication::enterEmergencyCallbackMode(RadioIndicationType /*type*/) {
    INFO("Not implement enterEmergencyCallbackMode");
    return Void();
}

Return<void> nsRilIndication::cdmaCallWaiting(RadioIndicationType /*type*/,
                                              const CdmaCallWaiting& /*callWaitingRecord*/) {
    INFO("Not implement cdmaCallWaiting");
    return Void();
}

Return<void> nsRilIndication::cdmaOtaProvisionStatus(RadioIndicationType /*type*/,
                                                     CdmaOtaProvisionStatus /*status*/) {
    INFO("Not implement cdmaOtaProvisionStatus");
    return Void();
}

Return<void> nsRilIndication::cdmaInfoRec(RadioIndicationType /*type*/,
                                          const CdmaInformationRecords& /*records*/) {
    INFO("Not implement cdmaInfoRec");
    return Void();
}

Return<void> nsRilIndication::indicateRingbackTone(RadioIndicationType /*type*/, bool /*start*/) {
    INFO("Not implement indicateRingbackTone");
    return Void();
}

Return<void> nsRilIndication::resendIncallMute(RadioIndicationType type) {
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("resendIncallMute"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::cdmaSubscriptionSourceChanged(RadioIndicationType /*type*/,
                                                            CdmaSubscriptionSource /*cdmaSource*/) {
    INFO("Not implement cdmaSubscriptionSourceChanged");
    return Void();
}

Return<void> nsRilIndication::cdmaPrlChanged(RadioIndicationType /*type*/, int32_t /*version*/) {
    INFO("Not implement cdmaPrlChanged");
    return Void();
}

Return<void> nsRilIndication::exitEmergencyCallbackMode(RadioIndicationType /*type*/) {
    INFO("Not implement exitEmergencyCallbackMode");
    return Void();
}

Return<void> nsRilIndication::rilConnected(RadioIndicationType type) {
    INFO("rilConnected");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("rilconnected"));
    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    mRIL->sendRilIndicationResult(result);
    return Void();
}

Return<void> nsRilIndication::voiceRadioTechChanged(RadioIndicationType type,
                                                    RadioTechnology rat) {
    INFO("voiceRadioTechChanged");
    mRIL->processIndication(type);

    nsString rilmessageType (NS_LITERAL_STRING("voiceRadioTechChanged"));

    RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(rilmessageType);
    result->updateVoiceRadioTechChanged(nsRilResult::convertRadioTechnology(rat));
    mRIL->sendRilIndicationResult(result);

    return Void();
}

Return<void> nsRilIndication::cellInfoList(
    RadioIndicationType type, const ::android::hardware::hidl_vec<CellInfo>& records) {

  INFO("cellInfoList");
  mRIL->processIndication(type);

  RefPtr<nsRilIndicationResult> result = new nsRilIndicationResult(NS_LITERAL_STRING("cellInfoList"));
  uint32_t numCellInfo = records.size();
  INFO("cellInfoList numCellInfo= %d" , numCellInfo);
  nsTArray<RefPtr<nsRilCellInfo>> aCellInfoLists(numCellInfo);

  for (uint32_t i = 0; i < numCellInfo; i++) {
    RefPtr<nsRilCellInfo> cellInfo = result->convertRilCellInfo(records[i]);
    aCellInfoLists.AppendElement(cellInfo);
  }
  result->updateCellInfoList(aCellInfoLists);


  mRIL->sendRilIndicationResult(result);
  return Void();
}

Return<void> nsRilIndication::imsNetworkStateChanged(RadioIndicationType /*type*/) {
    INFO("Not implement imsNetworkStateChanged");
    return Void();
}

Return<void> nsRilIndication::subscriptionStatusChanged(RadioIndicationType /*type*/,
                                                        bool /*activate*/) {
    INFO("Not implement subscriptionStatusChanged");
    return Void();
}

Return<void> nsRilIndication::srvccStateNotify(RadioIndicationType /*type*/, SrvccState /*state*/) {
    INFO("Not implement srvccStateNotify");
    return Void();
}

Return<void> nsRilIndication::hardwareConfigChanged(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<HardwareConfig>& /*configs*/) {
    INFO("Not implement hardwareConfigChanged");
    return Void();
}

Return<void> nsRilIndication::radioCapabilityIndication(RadioIndicationType /*type*/,
                                                        const RadioCapability& /*rc*/) {
    INFO("Not implement radioCapabilityIndication");
    return Void();
}

Return<void> nsRilIndication::onSupplementaryServiceIndication(RadioIndicationType /*type*/,
                                                               const StkCcUnsolSsResult& /*ss*/) {
    INFO("Not implement onSupplementaryServiceIndication");
    return Void();
}

Return<void> nsRilIndication::stkCallControlAlphaNotify(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*alpha*/) {
    INFO("Not implement stkCallControlAlphaNotify");
    return Void();
}

Return<void> nsRilIndication::lceData(RadioIndicationType /*type*/, const LceDataInfo& /*lce*/) {
    INFO("Not implement lceData");
    return Void();
}

Return<void> nsRilIndication::pcoData(RadioIndicationType /*type*/, const PcoDataInfo& /*pco*/) {
    INFO("Not implement pcoData");
    return Void();
}

Return<void> nsRilIndication::modemReset(RadioIndicationType /*type*/,
                                         const ::android::hardware::hidl_string& /*reason*/) {
    INFO("Not implement modemReset");
    return Void();
}

// Helper function
int32_t nsRilIndication::convertRadioStateToNum(RadioState state) {
  switch(state) {
    case RadioState::OFF:
      return nsIRilIndicationResult::RADIOSTATE_DISABLED;
    case RadioState::UNAVAILABLE:
      return nsIRilIndicationResult::RADIOSTATE_UNKNOWN;
    case RadioState::ON:
      return nsIRilIndicationResult::RADIOSTATE_ENABLED;
    default:
      return nsIRilIndicationResult::RADIOSTATE_UNKNOWN;
  }
}
