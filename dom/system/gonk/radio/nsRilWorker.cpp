/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "nsRilWorker.h"

/* Logging related */
#if !defined(RILWORKER_LOG_TAG)
#  define RILWORKER_LOG_TAG "RilWorker"
#endif

#undef INFO
#undef ERROR
#undef DEBUG
#define INFO(args...) \
  __android_log_print(ANDROID_LOG_INFO, RILWORKER_LOG_TAG, ##args)
#define ERROR(args...) \
  __android_log_print(ANDROID_LOG_ERROR, RILWORKER_LOG_TAG, ##args)
#define DEBUG(args...) \
  __android_log_print(ANDROID_LOG_DEBUG, RILWORKER_LOG_TAG, ##args)


NS_IMPL_ISUPPORTS(nsRilWorker, nsIRilWorker)

static hidl_string HIDL_SERVICE_NAME[3] = {"slot1", "slot2", "slot3"};

/**
 *
 */
nsRilWorker::nsRilWorker(uint32_t aClientId)
{
  INFO("init nsRilWorker");
  mRadioProxy = nullptr;
  mDeathRecipient = nullptr;
  mRilCallback = nullptr;
  mClientId = aClientId;
  mRilResponse = new nsRilResponse(this);
  mRilIndication = new nsRilIndication(this);
}

/**
 * nsIRadioInterface implementation
 */
NS_IMETHODIMP nsRilWorker::SendRilRequest(JS::HandleValue message) {

  return NS_OK;
}


NS_IMETHODIMP nsRilWorker::InitRil(nsIRilCallback *callback) {
  mRilCallback = callback;
  GetRadioProxy();
  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetRadioPower(int32_t serial, bool enabled) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_RADIO_POWER on = %d", serial, enabled);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setRadioPower(serial, enabled);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetDeviceIdentity(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_DEVICE_IDENTITY", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getDeviceIdentity(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetVoiceRegistrationState(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_VOICE_REGISTRATION_STATE", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getVoiceRegistrationState(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetDataRegistrationState(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_DATA_REGISTRATION_STATE", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getDataRegistrationState(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetOperator(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_OPERATOR", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getOperator(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetNetworkSelectionMode(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getNetworkSelectionMode(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetSignalStrength(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SIGNAL_STRENGTH", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getSignalStrength(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetVoiceRadioTechnology(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_VOICE_RADIO_TECH", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getVoiceRadioTechnology(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetIccCardStatus(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_SIM_STATUS", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getIccCardStatus(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::ReportSmsMemoryStatus(int32_t serial, bool available) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_REPORT_SMS_MEMORY_STATUS available = %d", serial, available);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->reportSmsMemoryStatus(serial, available);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetCellInfoListRate(int32_t serial, int32_t rateInMillis) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE rateInMillis = %d", serial , rateInMillis);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  if (rateInMillis == 0) {
    rateInMillis = INT32_MAX;
  }

  mRadioProxy->setCellInfoListRate(serial, rateInMillis);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetDataAllowed(int32_t serial, bool allowed) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_ALLOW_DATA allowed = %d", serial, allowed);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setDataAllowed(serial, allowed);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetBasebandVersion(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_BASEBAND_VERSION", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getBasebandVersion(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetUiccSubscription(int32_t serial, int32_t slotId, int32_t appIndex, int32_t subId
                                               , int32_t subStatus) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_UICC_SUBSCRIPTION slotId = %d appIndex = %d subId = %d subStatus = %d"
       , serial, slotId, appIndex, subId, subStatus);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  SelectUiccSub info;
  info.slot = slotId;
  info.appIndex = appIndex;
  info.subType = SubscriptionType(subId);
  info.actStatus = UiccSubActStatus(subStatus);

  mRadioProxy->setUiccSubscription(serial, info);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetMute(int32_t serial, bool enableMute) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_MUTE enableMute = %d", serial, enableMute);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setMute(serial, enableMute);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetSmscAddress(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_SMSC_ADDRESS", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getSmscAddress(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::RequestDial(int32_t serial, const nsAString& address, int32_t clirMode, int32_t uusType, int32_t uusDcs, const nsAString& uusData) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_DIAL", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  INFO("nsRilWorker: Uusinfo");
  UusInfo info;
  info.uusType = UusType(uusType);
  info.uusDcs = UusDcs(uusDcs);
  if (uusData.Length() == 0) {
    info.uusData = NULL;
  } else {
    info.uusData = NS_ConvertUTF16toUTF8(uusData).get();
  }

  //hidl_vec<UusInfo> uusResult;
  //uusResult.setToExternal(const_cast<UusInfo*>(&info), sizeof(info));

  INFO("nsRilWorker: dial");
  Dial dialInfo;
  dialInfo.address = NS_ConvertUTF16toUTF8(address).get();
  __android_log_print(ANDROID_LOG_INFO, "nsRilWorker", "dialInfo.address= %s" , NS_ConvertUTF16toUTF8(address).get());
  dialInfo.clir = Clir(clirMode);
  // TODO it would cause crash...
  //dialInfo.uusInfo = uusResult;
  INFO("nsRilWorker: dial start.");
  mRadioProxy->dial(serial, dialInfo);
  INFO("nsRilWorker: dial done.");

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetCurrentCalls(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_CURRENT_CALLS", serial);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getCurrentCalls(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::HangupConnection(int32_t serial, int32_t callIndex) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_HANGUP callIndex = %d", serial, callIndex);

  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->hangup(serial, callIndex);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetLastCallFailCause(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_LAST_CALL_FAIL_CAUSE", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getLastCallFailCause(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::AcceptCall(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_ANSWER", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->acceptCall(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetPreferredNetworkType(int32_t serial, int32_t networkType) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE networkType=%d", serial, networkType);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setPreferredNetworkType(serial, PreferredNetworkType(networkType));

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetPreferredNetworkType(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getPreferredNetworkType(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetNetworkSelectionModeAutomatic(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setNetworkSelectionModeAutomatic(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetNetworkSelectionModeManual(int32_t serial, const nsAString& operatorNumeric) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL operatorNumeric = %s", serial, NS_ConvertUTF16toUTF8(operatorNumeric).get());
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->setNetworkSelectionModeManual(serial, NS_ConvertUTF16toUTF8(operatorNumeric).get());

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetAvailableNetworks(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_QUERY_AVAILABLE_NETWORKS", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getAvailableNetworks(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetInitialAttachApn(int32_t serial, nsIDataProfile *profile, bool isRoaming) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SET_INITIAL_ATTACH_APN", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  bool modemCognitive;
  profile->GetModemCognitive(&modemCognitive);

  mRadioProxy->setInitialAttachApn(serial, convertToHalDataProfile(profile), modemCognitive, isRoaming);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::SetupDataCall(int32_t serial, int32_t radioTechnology, nsIDataProfile *profile, bool isRoaming, bool allowRoaming) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SETUP_DATA_CALL", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  bool modemCognitive;
  profile->GetModemCognitive(&modemCognitive);

  mRadioProxy->setupDataCall(serial, RadioTechnology(radioTechnology), convertToHalDataProfile(profile), modemCognitive, allowRoaming, isRoaming);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::DeactivateDataCall(int32_t serial, int32_t cid, int32_t reason) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_DEACTIVATE_DATA_CALL", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->deactivateDataCall(serial, cid, (reason == 0 ? false: true));

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetDataCallList(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_DATA_CALL_LIST", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getDataCallList(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetCellInfoList(int32_t serial) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_CELL_INFO_LIST", serial);
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getCellInfoList(serial);

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::GetIMSI(int32_t serial, const nsAString& aid) {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_GET_IMSI aid = %s", serial, NS_ConvertUTF16toUTF8(aid).get());
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }
  mRadioProxy->getImsiForApp(serial, NS_ConvertUTF16toUTF8(aid).get());

  return NS_OK;
}

NS_IMETHODIMP nsRilWorker::IccIOForApp(int32_t serial, int32_t command, int32_t fileId
                                         , const nsAString& path, int32_t p1, int32_t p2, int32_t p3
                                         , const nsAString& data, const nsAString& pin2, const nsAString& aid)
 {
  INFO("nsRilWorker: [%d] > RIL_REQUEST_SIM_IO command = %d, fileId = %d, path = %s, p1 = %d, p2 = %d, p3 = %d, data = %s, pin2 = %s, aid = %s"
       , serial, command, fileId, NS_ConvertUTF16toUTF8(path).get(), p1, p2, p3, NS_ConvertUTF16toUTF8(data).get(), NS_ConvertUTF16toUTF8(pin2).get(), NS_ConvertUTF16toUTF8(aid).get());
  GetRadioProxy();
  if (mRadioProxy == nullptr) {
    ERROR("No Radio HAL exist");
  }

  IccIo iccIo;
  iccIo.command = command;
  iccIo.fileId = fileId;
  iccIo.path = NS_ConvertUTF16toUTF8(path).get();
  iccIo.p1 = p1;
  iccIo.p2 = p2;
  iccIo.p3 = p3;
  iccIo.data = NS_ConvertUTF16toUTF8(data).get();
  iccIo.pin2 = NS_ConvertUTF16toUTF8(pin2).get();
  iccIo.aid = NS_ConvertUTF16toUTF8(aid).get();

  mRadioProxy->iccIOForApp(serial, iccIo);

  return NS_OK;
}

nsRilWorker::~nsRilWorker() {
  INFO("Destructor nsRilWorker");
  mRilResponse = nullptr;
  MOZ_ASSERT(!mRilResponse);
  mRilIndication = nullptr;
  MOZ_ASSERT(!mRilIndication);
  mRadioProxy = nullptr;
  MOZ_ASSERT(!mRadioProxy);
  mDeathRecipient = nullptr;
  MOZ_ASSERT(!mDeathRecipient);
}

void nsRilWorker::RadioProxyDeathRecipient::serviceDied(uint64_t, const ::android::wp<IBase>&) {
  INFO("nsRilWorker HAL died, cleanup instance.");
}


void nsRilWorker::GetRadioProxy(){
  if (mRadioProxy != nullptr) {
    return;
  }
  INFO("GetRadioProxy");

  mRadioProxy = IRadio::getService(HIDL_SERVICE_NAME[mClientId]);

  if (mRadioProxy != nullptr) {
    if (mDeathRecipient == nullptr) {
      mDeathRecipient = new RadioProxyDeathRecipient();
    }
    if (mDeathRecipient != nullptr) {
      Return<bool> linked = mRadioProxy->linkToDeath(mDeathRecipient, 0 /*cookie*/);
      if (!linked || !linked.isOk()) {
        ERROR("Failed to link to radio hal death notifications");
      }
    }
    INFO("setResponseFunctions");
    mRadioProxy->setResponseFunctions(mRilResponse, mRilIndication);
  } else {
    ERROR("Get Radio hal failed");
  }
}

void nsRilWorker::processIndication(RadioIndicationType indicationType) {
  INFO("processIndication, type= %d", indicationType);
    if (indicationType == RadioIndicationType::UNSOLICITED_ACK_EXP) {
      sendAck();
      INFO("Unsol response received; Sending ack to ril.cpp");
    } else {
    // ack is not expected to be sent back. Nothing is required to be done here.
    }
}

void nsRilWorker::processResponse(RadioResponseType responseType) {
  INFO("processResponse, type= %d", responseType);
    if (responseType == RadioResponseType::SOLICITED_ACK_EXP) {
      sendAck();
      INFO("Solicited response received; Sending ack to ril.cpp");
    } else {
    // ack is not expected to be sent back. Nothing is required to be done here.
    }
}

void nsRilWorker::sendAck() {
  INFO("sendAck");
  GetRadioProxy();
  if (mRadioProxy != nullptr) {
    mRadioProxy->responseAcknowledgement();
  } else {
    ERROR("sendAck mRadioProxy == nullptr");
  }
}

//Helper function
MvnoType nsRilWorker::convertToHalMvnoType(const nsAString& mvnoType) {
  if (NS_LITERAL_STRING("imsi").Equals(mvnoType)) {
    return MvnoType::IMSI;
  } else if (NS_LITERAL_STRING("gid").Equals(mvnoType)) {
    return MvnoType::GID;
  } else if (NS_LITERAL_STRING("spn").Equals(mvnoType)) {
    return MvnoType::SPN;
  } else {
    return MvnoType::NONE;
  }
}

DataProfileInfo nsRilWorker::convertToHalDataProfile(nsIDataProfile *profile) {
  DataProfileInfo dataProfileInfo;

  int32_t profileId;
  profile->GetProfileId(&profileId);
  dataProfileInfo.profileId = DataProfileId(profileId);

  nsString apn;
  profile->GetApn(apn);
  dataProfileInfo.apn = NS_ConvertUTF16toUTF8(apn).get();

  nsString protocol;
  profile->GetProtocol(protocol);
  dataProfileInfo.protocol = NS_ConvertUTF16toUTF8(protocol).get();


  nsString roamingProtocol;
  profile->GetRoamingProtocol(roamingProtocol);
  dataProfileInfo.roamingProtocol = NS_ConvertUTF16toUTF8(roamingProtocol).get();

  int32_t authType;
  profile->GetAuthType(&authType);
  dataProfileInfo.authType = ApnAuthType(authType);

  nsString user;
  profile->GetUser(user);
  dataProfileInfo.user = NS_ConvertUTF16toUTF8(user).get();


  nsString password;
  profile->GetPassword(password);
  dataProfileInfo.password = NS_ConvertUTF16toUTF8(password).get();

  int32_t type;
  profile->GetType(&type);
  dataProfileInfo.type = DataProfileInfoType(type);

  int32_t maxConnsTime;
  profile->GetMaxConnsTime(&maxConnsTime);
  dataProfileInfo.maxConnsTime = maxConnsTime;

  int32_t maxConns;
  profile->GetMaxConns(&maxConns);
  dataProfileInfo.maxConns = maxConns;

  int32_t waitTime;
  profile->GetWaitTime(&waitTime);
  dataProfileInfo.waitTime = waitTime;

  bool enabled;
  profile->GetEnabled(&enabled);
  dataProfileInfo.enabled = enabled;

  int32_t supportedApnTypesBitmap;
  profile->GetSupportedApnTypesBitmap(&supportedApnTypesBitmap);
  dataProfileInfo.supportedApnTypesBitmap = (int32_t)ApnTypes(supportedApnTypesBitmap);


  int32_t bearerBitmap;
  profile->GetBearerBitmap(&bearerBitmap);
  dataProfileInfo.bearerBitmap = (int32_t)RadioAccessFamily(bearerBitmap);

  int32_t mtu;
  profile->GetMtu(&mtu);
  dataProfileInfo.mtu = mtu;

  nsString mvnoType;
  profile->GetMvnoType(mvnoType);
  dataProfileInfo.mvnoType = convertToHalMvnoType(mvnoType);

  nsString mvnoMatchData;
  profile->GetMvnoMatchData(mvnoMatchData);
  dataProfileInfo.mvnoMatchData = NS_ConvertUTF16toUTF8(mvnoMatchData).get();

  return dataProfileInfo;
}

// Runnable used dispatch command result on the main thread.
class RilResultDispatcher : public mozilla::Runnable {
 public:
  RilResultDispatcher(nsRilWorker* aRil, nsRilIndicationResult* aIndication)
    : mozilla::Runnable("RilIndicationResultDispatcher")
    , mRil(aRil)
    , mResponse(nullptr)
    , mIndication(aIndication)
  {
    INFO("RilResultDispatcher nsRilIndicationResult");
    MOZ_ASSERT(!NS_IsMainThread());
  }
  RilResultDispatcher(nsRilWorker* aRil, nsRilResponseResult* aResponse)
    : mozilla::Runnable("RilResponseResultDispatcher")
    , mRil(aRil)
    , mResponse(aResponse)
    , mIndication(nullptr)
  {
    INFO("RilResultDispatcher nsRilResponseResult");
    MOZ_ASSERT(!NS_IsMainThread());
  }
  NS_IMETHOD Run() override {
    MOZ_ASSERT(NS_IsMainThread());
    if (mRil && mRil->mRilCallback) {
      if (mResponse) {
        mRil->mRilCallback->HandleRilResponse(mResponse);
      } else if (mIndication) {
        mRil->mRilCallback->HandleRilIndication(mIndication);
      } else {
        INFO("RilIndicationResultDispatcher: no result");
      }
    } else {
      INFO("RilIndicationResultDispatcher: no mRIL or callback");
    }
    return NS_OK;
  }

  private:
  nsRilWorker* mRil;
  RefPtr<nsRilResponseResult> mResponse;
  RefPtr<nsRilIndicationResult> mIndication;
};

void nsRilWorker::sendRilIndicationResult(nsRilIndicationResult* aIndication) {
  INFO("nsRilWorker: [USOL]< %s", NS_LossyConvertUTF16toASCII(aIndication->mRilMessageType).get());
  nsCOMPtr<nsIRunnable> runnable = new RilResultDispatcher(this , aIndication);
  NS_DispatchToMainThread(runnable);
  INFO("IndicationResult.mRilMessageType done.");
}

void nsRilWorker::sendRilResponseResult(nsRilResponseResult* aResponse) {
  INFO("nsRilWorker: [%d] < %s", aResponse-> mRilMessageToken, NS_LossyConvertUTF16toASCII(aResponse->mRilMessageType).get());

  if (aResponse-> mRilMessageToken > 0) {
    nsCOMPtr<nsIRunnable> runnable = new RilResultDispatcher(this, aResponse);
    NS_DispatchToMainThread(runnable);
    INFO("ResponseResult.mRilMessageType done.");
  } else {
    INFO("ResponseResult internal reqeust.");
  }
}
