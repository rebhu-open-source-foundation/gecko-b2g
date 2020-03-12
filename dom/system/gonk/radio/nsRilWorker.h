/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsRilWorker_H
#define nsRilWorker_H

#include <nsISupportsImpl.h>
#include <nsIRilWorkerService.h>
#include <nsThreadUtils.h>
#include <nsTArray.h>
#include "nsRilIndication.h"
#include "nsRilIndicationResult.h"
#include "nsRilResponse.h"
#include "nsRilResponseResult.h"
#include "nsRilResult.h"

#include <android/hardware/radio/1.0/IRadio.h>
#include <android/hardware/radio/1.0/types.h>

using ::android::sp;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::radio::V1_0::ApnTypes;
using ::android::hardware::radio::V1_0::CallForwardInfo;
using ::android::hardware::radio::V1_0::CallForwardInfoStatus;
using ::android::hardware::radio::V1_0::ClipStatus;
using ::android::hardware::radio::V1_0::DataProfileId;
using ::android::hardware::radio::V1_0::DataProfileInfo;
using ::android::hardware::radio::V1_0::DataProfileInfoType;
using ::android::hardware::radio::V1_0::Dial;
using ::android::hardware::radio::V1_0::HardwareConfigModem;
using ::android::hardware::radio::V1_0::HardwareConfigSim;
using ::android::hardware::radio::V1_0::HardwareConfigState;
using ::android::hardware::radio::V1_0::HardwareConfigType;
using ::android::hardware::radio::V1_0::IccIo;
using ::android::hardware::radio::V1_0::IRadio;
using ::android::hardware::radio::V1_0::IRadioIndication;
using ::android::hardware::radio::V1_0::IRadioResponse;
using ::android::hardware::radio::V1_0::MvnoType;
using ::android::hardware::radio::V1_0::OperatorStatus;
using ::android::hardware::radio::V1_0::PhoneRestrictedState;
using ::android::hardware::radio::V1_0::PreferredNetworkType;
using ::android::hardware::radio::V1_0::RadioAccessFamily;
using ::android::hardware::radio::V1_0::RadioCapabilityPhase;
using ::android::hardware::radio::V1_0::RadioCapabilityStatus;
using ::android::hardware::radio::V1_0::RadioIndicationType;
using ::android::hardware::radio::V1_0::RadioResponseType;
using ::android::hardware::radio::V1_0::RadioTechnology;
using ::android::hardware::radio::V1_0::SelectUiccSub;
using ::android::hardware::radio::V1_0::SimRefreshType;
using ::android::hardware::radio::V1_0::SrvccState;
using ::android::hardware::radio::V1_0::SubscriptionType;
using ::android::hardware::radio::V1_0::TtyMode;
using ::android::hardware::radio::V1_0::UiccSubActStatus;
using ::android::hardware::radio::V1_0::UssdModeType;
using ::android::hardware::radio::V1_0::UusInfo;

using ::android::hidl::base::V1_0::IBase;

class nsRilWorker;

class nsRilWorker final : public nsIRilWorker {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRILWORKER

  nsRilWorker(uint32_t aClientId);

  struct RadioProxyDeathRecipient : public hidl_death_recipient {
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie,
                             const ::android::wp<IBase>& who) override;
  };

  void GetRadioProxy();
  void processIndication(RadioIndicationType indicationType);
  void processResponse(RadioResponseType responseType);
  void sendAck();
  void sendRilIndicationResult(nsRilIndicationResult* aIndication);
  void sendRilResponseResult(nsRilResponseResult* aResponse);
  nsCOMPtr<nsIRilCallback> mRilCallback;

 private:
  ~nsRilWorker();
  int32_t mClientId;
  sp<IRadio> mRadioProxy;
  sp<IRadioResponse> mRilResponse;
  sp<IRadioIndication> mRilIndication;
  sp<RadioProxyDeathRecipient> mDeathRecipient;
  MvnoType convertToHalMvnoType(const nsAString& mvnoType);
  DataProfileInfo convertToHalDataProfile(nsIDataProfile* profile);
};
#endif
