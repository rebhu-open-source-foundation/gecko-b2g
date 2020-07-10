/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "CardInfoManagerDelegate.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsXULAppAPI.h"
#include "nsQueryObject.h"
#include "mozilla/Hal.h"
#include "nsIIccService.h"
#include "nsIIccInfo.h"
#include "nsIMobileConnectionService.h"
#include "nsIMobileConnectionInfo.h"
#include "nsIMobileNetworkInfo.h"
#include "nsIMobileDeviceIdentities.h"
#include "nsINetworkInterface.h"
#include "nsINetworkManager.h"


#define RETURN_IF_NULL_MSG(varname, msg) if (NULL == varname) {   \
          return NS_ERROR_FAILURE;                                \
        }

#define RETURN_IF_ERROR_MSG(varname, msg) if (NS_OK != varname) { \
          return NS_ERROR_FAILURE;                                \
        }

using namespace mozilla;

namespace mozilla {

// The singleton CardInfoManagerDelegate service, to be used on the main thread.
static StaticRefPtr<CardInfoManagerDelegateService> gCardInfoManagerDelegateService;

NS_IMPL_ISUPPORTS(CardInfoManagerDelegateService, nsICardInfoManagerDelegate)

CardInfoManagerDelegateService::CardInfoManagerDelegateService() { }

CardInfoManagerDelegateService::~CardInfoManagerDelegateService() { }

/* static */
already_AddRefed<CardInfoManagerDelegateService>
CardInfoManagerDelegateService::ConstructCardInfoManagerDelegate() {
  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gCardInfoManagerDelegateService) {
    gCardInfoManagerDelegateService = new CardInfoManagerDelegateService();
  }

  RefPtr<CardInfoManagerDelegateService> service = gCardInfoManagerDelegateService.get();
  return service.forget();
}

NS_IMETHODIMP
CardInfoManagerDelegateService::GetCardInfo(int type, int cardId, nsAString& result) {
  printf_stderr("GetCardInfo %d %d\n", type, cardId);
  if (CIT_IMEI == type) { //IMEI
    nsCOMPtr<nsIMobileConnectionService> service = do_GetService(NS_MOBILE_CONNECTION_SERVICE_CONTRACTID);
    RETURN_IF_NULL_MSG(service, "Invalid mobile connection service")

    nsCOMPtr<nsIMobileConnection> connection;
    service->GetItemByServiceId(cardId, getter_AddRefs(connection));
    RETURN_IF_NULL_MSG(connection, "Invalid mobile connection")

    nsCOMPtr<nsIMobileDeviceIdentities> deviceIdentities;
    connection->GetDeviceIdentities(getter_AddRefs(deviceIdentities));
    RETURN_IF_NULL_MSG(deviceIdentities, "Invalid mobile device identities")

    RETURN_IF_ERROR_MSG(deviceIdentities->GetImei(result), "Failed to get IMEI")
  } else { //IMSI and MSISDN both used icc
    nsCOMPtr<nsIIccService> iccService = do_GetService(ICC_SERVICE_CONTRACTID);

    nsCOMPtr<nsIIcc> icc;
    iccService->GetIccByServiceId(cardId, getter_AddRefs(icc));
    RETURN_IF_NULL_MSG(icc, "Invalid icc service")

    if (CIT_IMSI == type) { //IMSI ends here
      RETURN_IF_ERROR_MSG(icc->GetImsi(result), "Failed to get IMSI")
    } else { //MSISDN should do something more
      nsCOMPtr<nsIIccInfo> iccInfo;
      RETURN_IF_ERROR_MSG(icc->GetIccInfo(getter_AddRefs(iccInfo)), "Invalid icc info")

      nsCOMPtr<nsIGsmIccInfo> gsmIccInfo = do_QueryInterface(iccInfo);
      if (gsmIccInfo) {
        RETURN_IF_ERROR_MSG(gsmIccInfo->GetMsisdn(result), "Failed to get MSISDN")
      } else {
        nsCOMPtr<nsICdmaIccInfo> cdmaIccInfo = do_QueryInterface(iccInfo);
        RETURN_IF_NULL_MSG(cdmaIccInfo, "Invalid cdma icc info")
        RETURN_IF_ERROR_MSG(cdmaIccInfo->GetMdn(result), "Failed to get MDN")
      }
    } //end of IMSI and MSISDN
  } // end of IMEI and IMSI and MSISDN

  return NS_OK;
}

}  // namespace mozilla
