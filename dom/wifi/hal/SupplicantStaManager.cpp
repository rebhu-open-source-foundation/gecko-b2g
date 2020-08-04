/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "SupplicantStaManager"

#include "SupplicantStaManager.h"
#include "SupplicantCallback.h"
#include <utils/Log.h>
#include <string.h>
#include <mozilla/ClearOnShutdown.h>

#define EVENT_SUPPLICANT_TERMINATING u"SUPPLICANT_TERMINATING"_ns

using namespace mozilla::dom::wifi;

static const char SUPPLICANT_INTERFACE_NAME_V1_0[] =
    "android.hardware.wifi.supplicant@1.0::ISupplicant";

static const char SUPPLICANT_INTERFACE_NAME_V1_1[] =
    "android.hardware.wifi.supplicant@1.1::ISupplicant";

static const char SUPPLICANT_INTERFACE_NAME_V1_2[] =
    "android.hardware.wifi.supplicant@1.2::ISupplicant";

static const char HAL_INSTANCE_NAME[] = "default";

mozilla::Mutex SupplicantStaManager::sLock("supplicant-sta");
static StaticAutoPtr<SupplicantStaManager> sSupplicantManager;

SupplicantStaManager::SupplicantStaManager()
    : mServiceManager(nullptr),
      mSupplicant(nullptr),
      mSupplicantStaIface(nullptr),
      mServiceManagerDeathRecipient(nullptr),
      mSupplicantDeathRecipient(nullptr),
      mDeathEventHandler(nullptr),
      mDeathRecipientCookie(0) {}

SupplicantStaManager* SupplicantStaManager::Get() {
  MOZ_ASSERT(NS_IsMainThread());

  if (!sSupplicantManager) {
    sSupplicantManager = new SupplicantStaManager();
    ClearOnShutdown(&sSupplicantManager);
  }
  return sSupplicantManager;
}

void SupplicantStaManager::CleanUp() {
  if (sSupplicantManager != nullptr) {
    SupplicantStaManager::Get()->DeinitInterface();
  }
}

void SupplicantStaManager::ServiceManagerDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "IServiceManager HAL died, cleanup instance.");
  MutexAutoLock lock(sLock);

  if (mOuter != nullptr) {
    mOuter->SupplicantServiceDiedHandler(mOuter->mDeathRecipientCookie);
    mOuter->mServiceManager = nullptr;
  }
}

void SupplicantStaManager::SupplicantDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "ISupplicant HAL died, cleanup instance.");
  MutexAutoLock lock(sLock);

  if (mOuter != nullptr) {
    mOuter->SupplicantServiceDiedHandler(mOuter->mDeathRecipientCookie);
    mOuter->mSupplicant = nullptr;
  }
}

void SupplicantStaManager::RegisterEventCallback(
    const android::sp<WifiEventCallback>& aCallback) {
  mCallback = aCallback;
}

void SupplicantStaManager::UnregisterEventCallback() { mCallback = nullptr; }

void SupplicantStaManager::RegisterPasspointCallback(
    PasspointEventCallback* aCallback) {
  mPasspointCallback = aCallback;
}

void SupplicantStaManager::UnregisterPasspointCallback() {
  mPasspointCallback = nullptr;
}

Result_t SupplicantStaManager::InitInterface() {
  if (mSupplicant != nullptr) {
    return nsIWifiResult::SUCCESS;
  }
  return InitServiceManager();
}

Result_t SupplicantStaManager::DeinitInterface() { return TearDownInterface(); }

Result_t SupplicantStaManager::InitServiceManager() {
  MutexAutoLock lock(sLock);
  if (mServiceManager != nullptr) {
    // service already existed.
    return nsIWifiResult::SUCCESS;
  }

  mServiceManager =
      ::android::hidl::manager::V1_0::IServiceManager::getService();
  if (mServiceManager == nullptr) {
    WIFI_LOGE(LOG_TAG, "Failed to get HIDL service manager");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  if (mServiceManagerDeathRecipient == nullptr) {
    mServiceManagerDeathRecipient =
        new ServiceManagerDeathRecipient(sSupplicantManager);
  }

  Return<bool> linked =
      mServiceManager->linkToDeath(mServiceManagerDeathRecipient, 0);
  if (!linked || !linked.isOk()) {
    WIFI_LOGE(LOG_TAG, "Error on linkToDeath to IServiceManager");
    SupplicantServiceDiedHandler(mDeathRecipientCookie);
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  if (!mServiceManager->registerForNotifications(SUPPLICANT_INTERFACE_NAME_V1_2,
                                                 "", this)) {
    WIFI_LOGE(LOG_TAG, "Failed to register for notifications to ISupplicant");
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::InitSupplicantInterface() {
  MutexAutoLock lock(sLock);
  mSupplicant = ISupplicant::getService();

  if (mSupplicant != nullptr) {
    if (mSupplicantDeathRecipient == nullptr) {
      mSupplicantDeathRecipient =
          new SupplicantDeathRecipient(sSupplicantManager);
    }

    if (mSupplicantDeathRecipient != nullptr) {
      mDeathRecipientCookie = mDeathRecipientCookie + 1;
      Return<bool> linked = mSupplicant->linkToDeath(mSupplicantDeathRecipient,
                                                     mDeathRecipientCookie);
      if (!linked || !linked.isOk()) {
        WIFI_LOGE(LOG_TAG,
                  "Failed to link to supplicant hal death notifications");
        SupplicantServiceDiedHandler(mDeathRecipientCookie);
        mSupplicant = nullptr;
        return nsIWifiResult::ERROR_COMMAND_FAILED;
      }
    }

    SupplicantStatus response;
    mSupplicant->registerCallback(
        this, [&](const SupplicantStatus& status) { response = status; });
    if (response.code != SupplicantStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "registerCallback failed: %d, reason: %s",
                response.code, response.debugMessage.c_str());
      mSupplicant = nullptr;
    }
  } else {
    WIFI_LOGE(LOG_TAG, "get supplicant hal failed");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

bool SupplicantStaManager::IsInterfaceInitializing() {
  MutexAutoLock lock(sLock);
  return mServiceManager != nullptr;
}

bool SupplicantStaManager::IsInterfaceReady() {
  MutexAutoLock lock(sLock);
  return mSupplicant != nullptr;
}

Result_t SupplicantStaManager::TearDownInterface() {
  MutexAutoLock lock(sLock);

  // Should remove wireless interface from supplicant since version 1.1.
  if (IsSupplicantV1_1()) {
    if (mSupplicantStaIface.get()) {
      SupplicantStatus response;
      ISupplicant::IfaceInfo info;
      info.name = mInterfaceName;
      info.type = SupplicantNameSpaceV1_0::IfaceType::STA;

      GetSupplicantV1_1()->removeInterface(
          info, [&](const SupplicantStatus& status) { response = status; });

      if (response.code != SupplicantStatusCode::SUCCESS) {
        WIFI_LOGE(LOG_TAG, "Failed to remove sta interface");
        return nsIWifiResult::ERROR_COMMAND_FAILED;
      }
    }
  }

  mCurrentConfiguration.clear();
  mCurrentNetwork.clear();
  mSupplicant = nullptr;
  mSupplicantStaIface = nullptr;
  mServiceManager = nullptr;

  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::GetMacAddress(nsAString& aMacAddress) {
  SupplicantStatus response;
  mSupplicantStaIface->getMacAddress(
      [&](const SupplicantStatus& status,
          const hidl_array<uint8_t, 6>& macAddr) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          std::string address_str = ConvertMacToString(macAddr);
          nsString address(NS_ConvertUTF8toUTF16(address_str.c_str()));
          aMacAddress.Assign(address);
        }
      });
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::GetSupportedFeatures(
    uint32_t& aSupportedFeatures) {
  uint32_t capabilities = 0;

  SupplicantStatus response;
  if (IsSupplicantV1_2()) {
    android::sp<ISupplicantStaIfaceV1_2> supplicantV1_2 =
        GetSupplicantStaIfaceV1_2();
    if (!supplicantV1_2.get()) {
      return nsIWifiResult::ERROR_INVALID_INTERFACE;
    }

    supplicantV1_2->getKeyMgmtCapabilities(
        [&](const SupplicantStatus& status, uint32_t keyMgmtMask) {
          capabilities = keyMgmtMask;
          response = status;
        });

    if (response.code != SupplicantStatusCode::SUCCESS) {
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }

    if (capabilities & ISupplicantStaNetworkV1_2::KeyMgmtMask::SAE) {
      // SAE supported
      aSupportedFeatures |= nsIWifiResult::FEATURE_WPA3_SAE;
    }
    if (capabilities & ISupplicantStaNetworkV1_2::KeyMgmtMask::SUITE_B_192) {
      // SUITE_B supported
      aSupportedFeatures |= nsIWifiResult::FEATURE_WPA3_SUITE_B;
    }
    if (capabilities & ISupplicantStaNetworkV1_2::KeyMgmtMask::OWE) {
      // OWE supported
      aSupportedFeatures |= nsIWifiResult::FEATURE_OWE;
    }
    if (capabilities & ISupplicantStaNetworkV1_2::KeyMgmtMask::DPP) {
      // DPP supported
      aSupportedFeatures |= nsIWifiResult::FEATURE_DPP;
    }
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::GetSupplicantDebugLevel(uint32_t& aLevel) {
  if (mSupplicant == nullptr) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }
  aLevel = (uint32_t)ISupplicant::DebugLevel(mSupplicant->getDebugLevel());
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::SetSupplicantDebugLevel(
    SupplicantDebugLevelOptions* aLevel) {
  mSupplicant->setDebugParams(
      static_cast<ISupplicant::DebugLevel>(aLevel->mLogLevel),
      aLevel->mShowTimeStamp, aLevel->mShowKeys,
      [](const SupplicantStatus& status) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
          WIFI_LOGE(LOG_TAG, "Failed to set suppliant debug level");
        }
      });
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::SetConcurrencyPriority(bool aEnable) {
  SupplicantNameSpaceV1_0::IfaceType type =
      (aEnable ? SupplicantNameSpaceV1_0::IfaceType::STA
               : SupplicantNameSpaceV1_0::IfaceType::P2P);

  SupplicantStatus response;
  HIDL_SET(mSupplicant, setConcurrencyPriority, SupplicantStatus, response,
           type);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetPowerSave(bool aEnable) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setPowerSave, SupplicantStatus, response,
           aEnable);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetSuspendMode(bool aEnable) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setSuspendModeEnabled, SupplicantStatus,
           response, aEnable);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetExternalSim(bool aEnable) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setExternalSim, SupplicantStatus, response,
           aEnable);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetAutoReconnect(bool aEnable) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, enableAutoReconnect, SupplicantStatus, response,
           aEnable);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetCountryCode(const std::string& aCountryCode) {
  if (aCountryCode.length() != 2) {
    WIFI_LOGE(LOG_TAG, "Invalid country code: %s", aCountryCode.c_str());
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }
  std::array<int8_t, 2> countryCode;
  countryCode[0] = aCountryCode.at(0);
  countryCode[1] = aCountryCode.at(1);

  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setCountryCode, SupplicantStatus, response,
           countryCode);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetBtCoexistenceMode(uint8_t aMode) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setBtCoexistenceMode, SupplicantStatus,
           response, (ISupplicantStaIface::BtCoexistenceMode)aMode);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::SetBtCoexistenceScanMode(bool aEnable) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, setBtCoexistenceScanModeEnabled,
           SupplicantStatus, response, aEnable);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

// Helper function to find any iface of the desired type exposed.
Result_t SupplicantStaManager::FindIfaceOfType(
    SupplicantNameSpaceV1_0::IfaceType aDesired,
    ISupplicant::IfaceInfo* aInfo) {
  if (mSupplicant == nullptr) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }

  SupplicantStatus response;
  std::vector<ISupplicant::IfaceInfo> iface_infos;
  mSupplicant->listInterfaces([&](const SupplicantStatus& status,
                                  hidl_vec<ISupplicant::IfaceInfo> infos) {
    response = status;
    iface_infos = infos;
  });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  for (const auto& info : iface_infos) {
    if (info.type == aDesired) {
      *aInfo = info;
      return nsIWifiResult::SUCCESS;
    }
  }
  return nsIWifiResult::ERROR_COMMAND_FAILED;
}

Result_t SupplicantStaManager::SetupStaInterface(
    const std::string& aInterfaceName) {
  mInterfaceName = aInterfaceName;

  if (!mSupplicantStaIface.get()) {
    // Since hal version 1.1, hidl client can add or remove wireless interface
    // in supplicant dynamically.
    if (IsSupplicantV1_1()) {
      mSupplicantStaIface = AddSupplicantStaIface();
    } else {
      mSupplicantStaIface = GetSupplicantStaIface();
    }
  }

  if (!mSupplicantStaIface.get()) {
    WIFI_LOGE(LOG_TAG, "Failed to create STA interface in supplicant");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  SupplicantStatus response;
  // Instantiate supplicant callback
  android::sp<SupplicantStaIfaceCallback> supplicantCallback =
      new SupplicantStaIfaceCallback(mInterfaceName, mCallback,
                                     mPasspointCallback);
  if (IsSupplicantV1_2()) {
    android::sp<SupplicantStaIfaceCallbackV1_1> supplicantCallbackV1_1 =
        new SupplicantStaIfaceCallbackV1_1(mInterfaceName, mCallback,
                                           supplicantCallback);
    android::sp<SupplicantStaIfaceCallbackV1_2> supplicantCallbackV1_2 =
        new SupplicantStaIfaceCallbackV1_2(mInterfaceName, mCallback,
                                           supplicantCallbackV1_1);
    HIDL_SET(GetSupplicantStaIfaceV1_2(), registerCallback_1_2,
             SupplicantStatus, response, supplicantCallbackV1_2);
  } else if (IsSupplicantV1_1()) {
    android::sp<SupplicantStaIfaceCallbackV1_1> supplicantCallbackV1_1 =
        new SupplicantStaIfaceCallbackV1_1(mInterfaceName, mCallback,
                                           supplicantCallback);
    HIDL_SET(GetSupplicantStaIfaceV1_1(), registerCallback_1_1,
             SupplicantStatus, response, supplicantCallbackV1_1);
  } else {
    HIDL_SET(mSupplicantStaIface, registerCallback, SupplicantStatus, response,
             supplicantCallback);
  }

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "registerCallback failed: %d", response.code);
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  return CHECK_SUCCESS(mSupplicantStaIface != nullptr);
}

android::sp<ISupplicantStaIface> SupplicantStaManager::GetSupplicantStaIface() {
  if (mSupplicant == nullptr) {
    return nullptr;
  }

  ISupplicant::IfaceInfo info;
  if (FindIfaceOfType(SupplicantNameSpaceV1_0::IfaceType::STA, &info) !=
      nsIWifiResult::SUCCESS) {
    return nullptr;
  }

  SupplicantStatus response;
  android::sp<ISupplicantStaIface> staIface;
  mSupplicant->getInterface(
      info, [&](const SupplicantStatus& status,
                const android::sp<ISupplicantIface>& iface) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          staIface = ISupplicantStaIface::castFrom(iface);
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    return nullptr;
  }

  return staIface;
}

android::sp<ISupplicantStaIface> SupplicantStaManager::AddSupplicantStaIface() {
  if (mSupplicant == nullptr) {
    return nullptr;
  }

  SupplicantStatus response;
  ISupplicant::IfaceInfo info;
  if (FindIfaceOfType(SupplicantNameSpaceV1_0::IfaceType::STA, &info) ==
      nsIWifiResult::SUCCESS) {
    // STA interface already exist, remove it to add a new one
    GetSupplicantV1_1()->removeInterface(
        info, [&](const SupplicantStatus& status) { response = status; });

    if (response.code != SupplicantStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to remove exist STA interface");
    }
  }

  info.name = mInterfaceName;
  info.type = SupplicantNameSpaceV1_0::IfaceType::STA;
  android::sp<ISupplicantStaIface> staIface;
  // Here add a STA interface in supplicant.
  GetSupplicantV1_1()->addInterface(
      info, [&](const SupplicantStatus& status,
                const android::sp<ISupplicantIface>& iface) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          staIface = ISupplicantStaIface::castFrom(iface);
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    return nullptr;
  }

  return staIface;
}

android::sp<SupplicantStaNetwork> SupplicantStaManager::CreateStaNetwork() {
  if (mSupplicantStaIface == nullptr) {
    return nullptr;
  }

  SupplicantStatus response;
  android::sp<ISupplicantStaNetwork> staNetwork;
  mSupplicantStaIface->addNetwork(
      [&](const SupplicantStatus& status,
          const android::sp<ISupplicantNetwork>& network) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          staNetwork = ISupplicantStaNetwork::castFrom(network);
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to add network with status code: %d",
              response.code);
    return nullptr;
  }
  return new SupplicantStaNetwork(mInterfaceName, mCallback, staNetwork);
}

android::sp<SupplicantStaNetwork> SupplicantStaManager::GetStaNetwork(
    uint32_t aNetId) {
  if (mSupplicantStaIface == nullptr) {
    return nullptr;
  }

  SupplicantStatus response;
  android::sp<ISupplicantStaNetwork> staNetwork;
  mSupplicantStaIface->getNetwork(
      aNetId, [&](const SupplicantStatus& status,
                  const ::android::sp<ISupplicantNetwork>& network) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          staNetwork = ISupplicantStaNetwork::castFrom(network);
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to get network with status code: %d",
              response.code);
    return nullptr;
  }
  return new SupplicantStaNetwork(mInterfaceName, mCallback, staNetwork);
}

android::sp<SupplicantStaNetwork> SupplicantStaManager::GetCurrentNetwork() {
  std::unordered_map<std::string,
                     android::sp<SupplicantStaNetwork>>::const_iterator found =
      mCurrentNetwork.find(mInterfaceName);
  if (found == mCurrentNetwork.end()) {
    return nullptr;
  }
  return mCurrentNetwork.at(mInterfaceName);
}

Result_t SupplicantStaManager::ConnectToNetwork(ConfigurationOptions* aConfig) {
  Result_t result = nsIWifiResult::ERROR_UNKNOWN;
  android::sp<SupplicantStaNetwork> staNetwork;
  NetworkConfiguration config(aConfig);

  std::unordered_map<std::string, NetworkConfiguration>::const_iterator
      existConfig = mCurrentConfiguration.find(mInterfaceName);

  if (existConfig != mCurrentConfiguration.end() &&
      CompareConfiguration(mCurrentConfiguration.at(mInterfaceName), config)) {
    WIFI_LOGD(LOG_TAG, "Same network, do not need to create a new one");

    staNetwork = GetCurrentNetwork();
    if (staNetwork == nullptr) {
      WIFI_LOGE(LOG_TAG, "Network is not available");
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  } else {
    mCurrentConfiguration.erase(mInterfaceName);
    mCurrentNetwork.erase(mInterfaceName);

    // remove all supplicant networks
    result = RemoveNetworks();
    if (result != nsIWifiResult::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to remove supplicant networks");
      return result;
    }

    // create network in supplicant and set configuration
    staNetwork = CreateStaNetwork();
    if (!staNetwork.get()) {
      WIFI_LOGE(LOG_TAG, "Failed to create STA network");
      return nsIWifiResult::ERROR_INVALID_INTERFACE;
    }
  }

  // set network configuration into supplicant
  result = staNetwork->SetConfiguration(config);
  if (result != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to set wifi configuration");
    mCurrentConfiguration.clear();
    mCurrentNetwork.clear();
    return result;
  }
  mCurrentConfiguration.insert(std::make_pair(mInterfaceName, config));
  mCurrentNetwork.insert(std::make_pair(mInterfaceName, staNetwork));

  // success, start to make connection
  staNetwork->SelectNetwork();
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::Reconnect() {
  SupplicantStatus response;
  HIDL_CALL(mSupplicantStaIface, reconnect, SupplicantStatus, response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::Reassociate() {
  SupplicantStatus response;
  HIDL_CALL(mSupplicantStaIface, reassociate, SupplicantStatus, response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::Disconnect() {
  SupplicantStatus response;
  HIDL_CALL(mSupplicantStaIface, disconnect, SupplicantStatus, response);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

Result_t SupplicantStaManager::EnableNetwork() {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->EnableNetwork();
}

Result_t SupplicantStaManager::DisableNetwork() {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->DisableNetwork();
}

Result_t SupplicantStaManager::RemoveNetworks() {
  SupplicantStatus response;
  // first, get network id list from supplicant
  std::vector<uint32_t> netIds;
  mSupplicantStaIface->listNetworks(
      [&](const SupplicantStatus& status,
          const ::android::hardware::hidl_vec<uint32_t>& networkIds) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          netIds = networkIds;
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "Failed to query saved networks in supplicant");
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  // remove network
  for (uint32_t id : netIds) {
    HIDL_SET(mSupplicantStaIface, removeNetwork, SupplicantStatus, response,
             id);
    if (response.code != SupplicantStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to remove network %d", id);
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::RoamToNetwork(ConfigurationOptions* aConfig) {
  NetworkConfiguration config(aConfig);
  NetworkConfiguration current = mCurrentConfiguration.at(mInterfaceName);

  if (config.mNetworkId == INVALID_NETWORK_ID) {
    return nsIWifiResult::ERROR_INVALID_ARGS;
  }

  if (config.mNetworkId != current.mNetworkId ||
      config.GetNetworkKey().compare(current.GetNetworkKey())) {
    return ConnectToNetwork(aConfig);
  }

  // now we are ready to roam to the target network.
  android::sp<SupplicantStaNetwork> network =
      mCurrentNetwork.at(mInterfaceName);

  Result_t result = network->SetConfiguration(config);

  WIFI_LOGD(LOG_TAG, "Trying to roam to network %s[%s]", config.mSsid.c_str(),
            config.mBssid.c_str());
  return (result == nsIWifiResult::SUCCESS) ? Reassociate() : result;
}

Result_t SupplicantStaManager::SendEapSimIdentityResponse(
    SimIdentityRespDataOptions* aIdentity) {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimIdentityResponse(aIdentity);
}

Result_t SupplicantStaManager::SendEapSimGsmAuthResponse(
    const nsTArray<SimGsmAuthRespDataOptions>& aGsmAuthResp) {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimGsmAuthResponse(aGsmAuthResp);
}

Result_t SupplicantStaManager::SendEapSimGsmAuthFailure() {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimGsmAuthFailure();
}

Result_t SupplicantStaManager::SendEapSimUmtsAuthResponse(
    SimUmtsAuthRespDataOptions* aUmtsAuthResp) {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimUmtsAuthResponse(aUmtsAuthResp);
}

Result_t SupplicantStaManager::SendEapSimUmtsAutsResponse(
    SimUmtsAutsRespDataOptions* aUmtsAutsResp) {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimUmtsAutsResponse(aUmtsAutsResp);
}

Result_t SupplicantStaManager::SendEapSimUmtsAuthFailure() {
  android::sp<SupplicantStaNetwork> network;

  network = GetCurrentNetwork();
  if (network == nullptr) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return network->SendEapSimUmtsAuthFailure();
}

Result_t SupplicantStaManager::SendAnqpRequest(
    const std::array<uint8_t, 6>& aBssid,
    const std::vector<uint32_t>& aInfoElements,
    const std::vector<uint32_t>& aHs20SubTypes) {
  SupplicantStatus response;
  HIDL_SET(mSupplicantStaIface, initiateAnqpQuery, SupplicantStatus, response,
           aBssid, (std::vector<AnqpInfoId>&)aInfoElements,
           (std::vector<Hs20AnqpSubtypes>&)aHs20SubTypes);
  return CHECK_SUCCESS(response.code == SupplicantStatusCode::SUCCESS);
}

/**
 * P2P functions
 */
android::sp<ISupplicantP2pIface> SupplicantStaManager::GetSupplicantP2pIface() {
  if (mSupplicant == nullptr) {
    return nullptr;
  }

  ISupplicant::IfaceInfo info;
  if (FindIfaceOfType(SupplicantNameSpaceV1_0::IfaceType::P2P, &info) !=
      nsIWifiResult::SUCCESS) {
    return nullptr;
  }
  SupplicantStatus response;
  android::sp<ISupplicantP2pIface> p2p_iface;
  mSupplicant->getInterface(
      info, [&](const SupplicantStatus& status,
                const android::sp<ISupplicantIface>& iface) {
        response = status;
        if (response.code == SupplicantStatusCode::SUCCESS) {
          p2p_iface = ISupplicantP2pIface::castFrom(iface);
        }
      });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    return nullptr;
  }
  return p2p_iface;
}

Result_t SupplicantStaManager::SetupP2pInterface() {
  android::sp<ISupplicantP2pIface> p2p_iface = GetSupplicantP2pIface();
  if (!p2p_iface.get()) {
    return nsIWifiResult::ERROR_INVALID_INTERFACE;
  }

  p2p_iface->saveConfig([](const SupplicantStatus& status) {
    WIFI_LOGD(LOG_TAG, "[P2P] save config: %d", status.code);
  });
  return nsIWifiResult::SUCCESS;
}

void SupplicantStaManager::RegisterDeathHandler(
    SupplicantDeathEventHandler* aHandler) {
  mDeathEventHandler = aHandler;
}

void SupplicantStaManager::UnregisterDeathHandler() {
  mDeathEventHandler = nullptr;
}

void SupplicantStaManager::SupplicantServiceDiedHandler(int32_t aCookie) {
  if (mDeathRecipientCookie != aCookie) {
    WIFI_LOGD(LOG_TAG, "Ignoring stale death recipient notification");
    return;
  }

  // TODO: notify supplicant has died.
  if (mDeathEventHandler != nullptr) {
    mDeathEventHandler->OnDeath();
  }
}

bool SupplicantStaManager::CompareConfiguration(
    const NetworkConfiguration& aOld, const NetworkConfiguration& aNew) {
  if (aOld.mNetworkId != aNew.mNetworkId) {
    return false;
  }
  if (aOld.mSsid.compare(aNew.mSsid)) {
    return false;
  }
  if (aOld.mKeyMgmt.compare(aNew.mKeyMgmt)) {
    return false;
  }
  return true;
}

/**
 * Hal wrapper functions
 */
android::sp<IServiceManager> SupplicantStaManager::GetServiceManager() {
  return mServiceManager.get() ? mServiceManager
                               : IServiceManager::getService();
}

android::sp<ISupplicant> SupplicantStaManager::GetSupplicant() {
  return mSupplicant.get() ? mSupplicant : ISupplicant::getService();
}

android::sp<ISupplicantV1_1> SupplicantStaManager::GetSupplicantV1_1() {
  return ISupplicantV1_1::castFrom(GetSupplicant());
}

android::sp<ISupplicantV1_2> SupplicantStaManager::GetSupplicantV1_2() {
  return ISupplicantV1_2::castFrom(GetSupplicant());
}

android::sp<ISupplicantStaIfaceV1_1>
SupplicantStaManager::GetSupplicantStaIfaceV1_1() {
  return ISupplicantStaIfaceV1_1::castFrom(mSupplicantStaIface);
}

android::sp<ISupplicantStaIfaceV1_2>
SupplicantStaManager::GetSupplicantStaIfaceV1_2() {
  return ISupplicantStaIfaceV1_2::castFrom(mSupplicantStaIface);
}

bool SupplicantStaManager::IsSupplicantV1_1() {
  return SupplicantVersionSupported(SUPPLICANT_INTERFACE_NAME_V1_1);
}

bool SupplicantStaManager::IsSupplicantV1_2() {
  return SupplicantVersionSupported(SUPPLICANT_INTERFACE_NAME_V1_2);
}

bool SupplicantStaManager::SupplicantVersionSupported(const std::string& name) {
  if (!mServiceManager.get()) {
    return false;
  }

  return mServiceManager->getTransport(name, HAL_INSTANCE_NAME) !=
         IServiceManager::Transport::EMPTY;
}

/**
 * IServiceNotification
 */
Return<void> SupplicantStaManager::onRegistration(const hidl_string& fqName,
                                                  const hidl_string& name,
                                                  bool preexisting) {
  // start to initialize supplicant hidl interface.
  if (InitSupplicantInterface() != nsIWifiResult::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "initialize ISupplicant failed");
    SupplicantServiceDiedHandler(mDeathRecipientCookie);
  }
  return Return<void>();
}

/**
 * Helper functions to notify event callback for ISupplicantCallback.
 */
void SupplicantStaManager::NotifyTerminating() {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event = new nsWifiEvent(EVENT_SUPPLICANT_TERMINATING);

  INVOKE_CALLBACK(mCallback, event, iface);
}

/**
 * ISupplicantCallback implementation
 */
Return<void> SupplicantStaManager::onInterfaceCreated(
    const hidl_string& ifName) {
  WIFI_LOGD(LOG_TAG, "SupplicantCallback.onInterfaceCreated(): %s",
            ifName.c_str());
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onInterfaceRemoved(
    const hidl_string& ifName) {
  WIFI_LOGD(LOG_TAG, "SupplicantCallback.onInterfaceRemoved(): %s",
            ifName.c_str());
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onTerminating() {
  MutexAutoLock lock(sLock);
  WIFI_LOGD(LOG_TAG, "SupplicantCallback.onTerminating()");

  NotifyTerminating();
  return android::hardware::Void();
}
