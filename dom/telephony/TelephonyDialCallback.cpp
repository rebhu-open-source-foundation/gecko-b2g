/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TelephonyDialCallback.h"
#include "nsIMobileCallForwardingOptions.h"
#include "nsIMobileConnectionService.h"

using namespace mozilla::dom;
using namespace mozilla::dom::telephony;

NS_IMPL_ISUPPORTS_INHERITED(TelephonyDialCallback, TelephonyCallback,
                            nsITelephonyDialCallback)

TelephonyDialCallback::TelephonyDialCallback(nsPIDOMWindowInner* aWindow,
                                             Telephony* aTelephony,
                                             Promise* aPromise)
    : TelephonyCallback(aPromise), mWindow(aWindow), mTelephony(aTelephony) {
  MOZ_ASSERT(mTelephony);
}

// TODO build pass workaround
// nsresult
// TelephonyDialCallback::NotifyDialMMISuccess(JSContext* aCx,
//                                             const MozMMIResult& aResult)
// {
//   TODO build pass workaround
//   JS::Rooted<JS::Value> jsResult(aCx);

//   if (!ToJSValue(aCx, aResult, &jsResult)) {
//     JS_ClearPendingException(aCx);
//     return NS_ERROR_DOM_TYPE_ERR;
//   }

//   mMMICall->NotifyResult(jsResult);
// }

// nsITelephonyDialCallback

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMI(const nsAString& aServiceCode) {
  mServiceCode.Assign(aServiceCode);

  mMMICall = new MMICall(mWindow, aServiceCode);
  mPromise->MaybeResolve(mMMICall);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialCallSuccess(
    uint32_t aClientId, uint32_t aCallIndex, const nsAString& aNumber,
    bool aIsEmergency, uint16_t aRttMode, uint16_t aVoiceQuality,
    uint16_t aVideoCallState, uint32_t aCapabilities, uint16_t aRadioTech) {
  RefPtr<TelephonyCallId> id = mTelephony->CreateCallId(aNumber);

  RefPtr<TelephonyCall> call = mTelephony->CreateCall(
      id, aClientId, aCallIndex, TelephonyCallState::Dialing,
      TelephonyCall::ConvertToTelephonyCallVoiceQuality(aVoiceQuality),
      TelephonyCall::ConvertToTelephonyVideoCallState(aVideoCallState),
      aCapabilities, TelephonyCall::ConvertToTelephonyCallRadioTech(aRadioTech),
      aIsEmergency, false, true, true, false,
      TelephonyCall::ConvertToTelephonyRttMode(aRttMode));

  mPromise->MaybeResolve(call);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMISuccess(const nsAString& aStatusMessage) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = true;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aStatusMessage);

  // return NotifyDialMMISuccess(cx, result);

  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMISuccessWithInteger(
    const nsAString& aStatusMessage, uint16_t aAdditionalInformation) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = true;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aStatusMessage);
  // result.mAdditionalInformation.SetAsUnsignedShort() =
  // aAdditionalInformation;

  // return NotifyDialMMISuccess(cx, result);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMISuccessWithStrings(
    const nsAString& aStatusMessage, uint32_t aCount,
    const char16_t** aAdditionalInformation) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = true;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aStatusMessage);

  // nsTArray<nsString> additionalInformation;
  // nsString* infos = additionalInformation.AppendElements(aCount);
  // for (uint32_t i = 0; i < aCount; i++) {
  //   infos[i].Rebind(aAdditionalInformation[i],
  //                   nsCharTraits<char16_t>::length(aAdditionalInformation[i]));
  // }

  // JS::Rooted<JS::Value> jsAdditionalInformation(cx);
  // if (!ToJSValue(cx, additionalInformation, &jsAdditionalInformation)) {
  //   JS_ClearPendingException(cx);
  //   return NS_ERROR_DOM_TYPE_ERR;
  // }

  // result.mAdditionalInformation.SetAsObject() =
  //   &jsAdditionalInformation.toObject();

  // return NotifyDialMMISuccess(cx, result);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMISuccessWithCallForwardingOptions(
    const nsAString& aStatusMessage, uint32_t aCount,
    nsIMobileCallForwardingOptions** aResults) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = true;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aStatusMessage);

  // nsTArray<MozCallForwardingOptions> additionalInformation;
  // for (uint32_t i = 0; i < aCount; i++) {
  //   MozCallForwardingOptions options;

  //   bool active = false;
  //   aResults[i]->GetActive(&active);
  //   //options.mActive.Construct(active);

  //   int16_t action = nsIMobileConnection::CALL_FORWARD_ACTION_UNKNOWN;
  //   aResults[i]->GetAction(&action);
  //   if (action != nsIMobileConnection::CALL_FORWARD_ACTION_UNKNOWN) {
  //     options.mAction= action;
  //   }

  //   int16_t reason = nsIMobileConnection::CALL_FORWARD_REASON_UNKNOWN;
  //   aResults[i]->GetReason(&reason);
  //   if (reason != nsIMobileConnection::CALL_FORWARD_REASON_UNKNOWN) {
  //     options.mReason = reason;
  //   }

  //   nsAutoString number;
  //   aResults[i]->GetNumber(number);
  //   options.mNumber = number.get();

  //   int16_t timeSeconds = -1;
  //   aResults[i]->GetTimeSeconds(&timeSeconds);
  //   if (timeSeconds >= 0) {
  //     options.mTimeSeconds = timeSeconds;
  //   }

  //   int16_t serviceClass = nsIMobileConnection::ICC_SERVICE_CLASS_NONE;
  //   aResults[i]->GetServiceClass(&serviceClass);
  //   if (serviceClass != nsIMobileConnection::ICC_SERVICE_CLASS_NONE) {
  //     options.mServiceClass = serviceClass;
  //   }

  //   additionalInformation.AppendElement(options);
  // }

  // JS::Rooted<JS::Value> jsAdditionalInformation(cx);
  // if (!ToJSValue(cx, additionalInformation, &jsAdditionalInformation)) {
  //   JS_ClearPendingException(cx);
  //   return NS_ERROR_DOM_TYPE_ERR;
  // }

  // result.mAdditionalInformation.SetAsObject() =
  //   &jsAdditionalInformation.toObject();

  // return NotifyDialMMISuccess(cx, result);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMIError(const nsAString& aError) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = false;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aError);

  // return NotifyDialMMISuccess(cx, result);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyDialCallback::NotifyDialMMIErrorWithInfo(const nsAString& aError,
                                                  uint16_t aInfo) {
  // TODO build pass workaround
  // AutoJSAPI jsapi;
  // if (NS_WARN_IF(!jsapi.Init(mWindow))) {
  //   return NS_ERROR_FAILURE;
  // }

  // JSContext* cx = jsapi.cx();

  // RootedDictionary<MozMMIResult> result(cx);
  // result.mSuccess = false;
  // result.mServiceCode.Assign(mServiceCode);
  // result.mStatusMessage.Assign(aError);
  // result.mAdditionalInformation.SetAsUnsignedShort() = aInfo;

  // return NotifyDialMMISuccess(cx, result);
  return NS_OK;
}
