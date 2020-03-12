/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsRilWorkerService.h"
#include "mozilla/Preferences.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsAppRunner.h"

#if !defined(RILWORKERSERVICE_LOG_TAG)
#  define RILWORKERSERVICE_LOG_TAG "RilWorkerService"
#endif

#undef INFO
#undef ERROR
#undef DEBUG
#define INFO(args...) \
  __android_log_print(ANDROID_LOG_INFO, RILWORKERSERVICE_LOG_TAG, ##args)
#define ERROR(args...) \
  __android_log_print(ANDROID_LOG_ERROR, RILWORKERSERVICE_LOG_TAG, ##args)
#define DEBUG(args...) \
  __android_log_print(ANDROID_LOG_DEBUG, RILWORKERSERVICE_LOG_TAG, ##args)

/*================ Implementation of Class nsCellBroadcastService=============*/

// The singleton network worker, to be used on the main thread.
mozilla::StaticRefPtr<nsRilWorkerService> gRilWorkerService;

NS_IMPL_ISUPPORTS(nsRilWorkerService, nsIRilWorkerService)

nsRilWorkerService::nsRilWorkerService() : mNumRilWorkers(1) {
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!gRilWorkerService);

  INFO("init nsRilWorkerService");

  // TODO get the mNumRilWorkers value for the muti sim.
  if (mRilWorkers.Length() == 0) {
    for (int clientId = 0; clientId < mNumRilWorkers; clientId++) {
      INFO("Creating new ril_worker for clientId = %d", clientId);
      RefPtr<nsRilWorker> ri = new nsRilWorker(clientId);
      mRilWorkers.AppendElement(ri);
    }
  }
}

NS_IMETHODIMP nsRilWorkerService::GetRilWorker(uint32_t clientId,
                                               nsIRilWorker** aRilWorker) {
  INFO("GetRilWorker = %d", clientId);
  nsCOMPtr<nsIRilWorker> worker;

  if (clientId >= mRilWorkers.Length()) {
    *aRilWorker = nullptr;
    return NS_ERROR_FAILURE;
  } else {
    return mRilWorkers[clientId]->QueryInterface(NS_GET_IID(nsIRilWorker),
                                                 (void**)aRilWorker);
  }
}

nsRilWorkerService::~nsRilWorkerService() {
  /* destructor code */
  MOZ_ASSERT(!gRilWorkerService);
}

already_AddRefed<nsRilWorkerService>
nsRilWorkerService::CreateRilWorkerService() {
  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gRilWorkerService) {
    gRilWorkerService = new nsRilWorkerService();
    ClearOnShutdown(&gRilWorkerService);
  }

  RefPtr<nsRilWorkerService> service = gRilWorkerService.get();
  return service.forget();
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(
    nsRilWorkerService, nsRilWorkerService::CreateRilWorkerService);

NS_DEFINE_NAMED_CID(RILWORKERSERVICE_CID);

static const mozilla::Module::CIDEntry kRilWorkerServiceCIDs[] = {
    {&kRILWORKERSERVICE_CID, false, nullptr, nsRilWorkerServiceConstructor},
    {nullptr}};

static const mozilla::Module::ContractIDEntry kRilWorkerServiceContracts[] = {
    {"@mozilla.org/rilworkerservice;1", &kRILWORKERSERVICE_CID}, {nullptr}};

extern const mozilla::Module kRilWorkerServiceModule = {
    mozilla::Module::kVersion, kRilWorkerServiceCIDs,
    kRilWorkerServiceContracts, nullptr};
