/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

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

using ::android::hardware::radio::V1_0::IRadio;
using ::android::hardware::radio::V1_0::IRadioResponse;
using ::android::hardware::radio::V1_0::IRadioIndication;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::radio::V1_0::RadioIndicationType;
using ::android::hardware::radio::V1_0::RadioResponseType;
using ::android::hardware::radio::V1_0::SelectUiccSub;
using ::android::hardware::radio::V1_0::SubscriptionType;
using ::android::hardware::radio::V1_0::UiccSubActStatus;
using ::android::hardware::radio::V1_0::Dial;
using ::android::hardware::radio::V1_0::UusInfo;
using ::android::hardware::radio::V1_0::PreferredNetworkType;
using ::android::hardware::radio::V1_0::DataProfileInfo;
using ::android::hardware::radio::V1_0::DataProfileId;
using ::android::hardware::radio::V1_0::DataProfileInfoType;
using ::android::hardware::radio::V1_0::MvnoType;
using ::android::hardware::radio::V1_0::ApnTypes;
using ::android::hardware::radio::V1_0::RadioAccessFamily;
using ::android::hardware::radio::V1_0::IccIo;

class nsRilWorker;


class nsRilWorker final : public nsIRilWorker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRILWORKER

  nsRilWorker(uint32_t aClientId);

  struct RadioProxyDeathRecipient : public hidl_death_recipient
  {
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie, const ::android::wp<IBase>& who) override;
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
  sp<IRadioResponse>  mRilResponse;
  sp<IRadioIndication>  mRilIndication;
  sp<RadioProxyDeathRecipient> mDeathRecipient;
  MvnoType convertToHalMvnoType(const nsAString& mvnoType);
  DataProfileInfo convertToHalDataProfile(nsIDataProfile *profile);
  RadioTechnology convertToHalRadioTechnology(uint32_t tech);

};
#endif
