/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SmsService.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace dom {
namespace mobilemessage {

NS_IMPL_ISUPPORTS(SmsService, nsISmsService)

NS_IMETHODIMP
SmsService::GetSmsDefaultServiceId(uint32_t* aServiceId) {
  // Android has no official DSDS support.
  *aServiceId = 0;
  return NS_OK;
}

NS_IMETHODIMP
SmsService::GetSegmentInfoForText(const nsAString& aText,
                                  nsIMobileMessageCallback* aRequest) {
  if (!AndroidBridge::Bridge()) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = AndroidBridge::Bridge()->GetSegmentInfoForText(aText, aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
SmsService::Send(uint32_t aServiceId, const nsAString& aNumber,
                 const nsAString& aMessage, bool aSilent,
                 nsIMobileMessageCallback* aRequest) {
  if (!AndroidBridge::Bridge()) {
    return NS_OK;
  }

  AndroidBridge::Bridge()->SendMessage(aNumber, aMessage, aRequest);
  return NS_OK;
}

NS_IMETHODIMP
SmsService::AddSilentNumber(const nsAString& aNumber) {
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
SmsService::RemoveSilentNumber(const nsAString& aNumber) {
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
SmsService::GetSmscAddress(uint32_t aServiceId,
                           nsIMobileMessageCallback* aRequest) {
  // TODO: bug 878016 - Android backend: implement getSMSCAddress/setSMSCAddress
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
SmsService::SetSmscAddress(uint32_t aServiceId, const nsAString& aNumber,
                           uint32_t aTypeOfNumber,
                           uint32_t aNumberPlanIdentification,
                           nsIMobileMessageCallback* aRequest) {
  // TODO: bug 878016 - Android backend: implement getSMSCAddress/setSMSCAddress
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

}  // namespace mobilemessage
}  // namespace dom
}  // namespace mozilla
