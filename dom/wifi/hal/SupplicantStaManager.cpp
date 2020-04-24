/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_TAG "SupplicantStaManager"

#include "SupplicantStaManager.h"
#include <utils/Log.h>
#include <string.h>
#include <mozilla/ClearOnShutdown.h>

#define EVENT_SUPPLICANT_TERMINATING "SUPPLICANT_TERMINATING"
#define EVENT_SUPPLICANT_STATE_CHANGED "SUPPLICANT_STATE_CHANGED"
#define EVENT_SUPPLICANT_NETWORK_CONNECTED "SUPPLICANT_NETWORK_CONNECTED"
#define EVENT_SUPPLICANT_NETWORK_DISCONNECTED "SUPPLICANT_NETWORK_DISCONNECTED"
#define EVENT_SUPPLICANT_AUTH_FAILURE "SUPPLICANT_AUTH_FAILURE"
#define EVENT_SUPPLICANT_ASSOC_REJECT "SUPPLICANT_ASSOC_REJECT"
#define EVENT_SUPPLICANT_TARGET_BSSID "SUPPLICANT_TARGET_BSSID"
#define EVENT_SUPPLICANT_ASSOCIATED_BSSID "SUPPLICANT_ASSOCIATED_BSSID"

static const char SUPPLICANT_INTERFACE_NAME[] =
    "android.hardware.wifi.supplicant@1.2::ISupplicant";

mozilla::Mutex SupplicantStaManager::s_Lock("supplicant-sta");
SupplicantStaManager* SupplicantStaManager::s_Instance = nullptr;

SupplicantStaManager::SupplicantStaManager()
    : mServiceManager(nullptr),
      mSupplicant(nullptr),
      mSupplicantStaIface(nullptr),
      mServiceManagerDeathRecipient(nullptr),
      mSupplicantDeathRecipient(nullptr),
      mDeathEventHandler(nullptr),
      mDeathRecipientCookie(0),
      mFourwayHandshake(false) {}

SupplicantStaManager::~SupplicantStaManager() {}

SupplicantStaManager* SupplicantStaManager::Get() {
  if (s_Instance == nullptr) {
    s_Instance = new SupplicantStaManager();
    mozilla::ClearOnShutdown(&s_Instance);
  }
  return s_Instance;
}

void SupplicantStaManager::CleanUp() {
  if (s_Instance != nullptr) {
    SupplicantStaManager::Get()->DeinitInterface();

    delete s_Instance;
    s_Instance = nullptr;
  }
}

void SupplicantStaManager::ServiceManagerDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "IServiceManager HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);

  if (mOuter != nullptr) {
    mOuter->SupplicantServiceDiedHandler(mOuter->mDeathRecipientCookie);
    mOuter->mServiceManager = nullptr;
  }
}

void SupplicantStaManager::SupplicantDeathRecipient::serviceDied(
    uint64_t, const android::wp<IBase>&) {
  WIFI_LOGE(LOG_TAG, "ISupplicant HAL died, cleanup instance.");
  MutexAutoLock lock(s_Lock);

  if (mOuter != nullptr) {
    mOuter->SupplicantServiceDiedHandler(mOuter->mDeathRecipientCookie);
    mOuter->mSupplicant = nullptr;
  }
}

void SupplicantStaManager::RegisterEventCallback(WifiEventCallback* aCallback) {
  mCallback = aCallback;
}

void SupplicantStaManager::UnregisterEventCallback() { mCallback = nullptr; }

Result_t SupplicantStaManager::InitInterface() {
  if (mSupplicant != nullptr) {
    return nsIWifiResult::SUCCESS;
  }
  return InitServiceManager();
}

Result_t SupplicantStaManager::DeinitInterface() { return TearDownInterface(); }

Result_t SupplicantStaManager::InitServiceManager() {
  MutexAutoLock lock(s_Lock);
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
        new ServiceManagerDeathRecipient(s_Instance);
  }
  Return<bool> linked =
      mServiceManager->linkToDeath(mServiceManagerDeathRecipient, 0);
  if (!linked || !linked.isOk()) {
    WIFI_LOGE(LOG_TAG, "Error on linkToDeath to IServiceManager");
    SupplicantServiceDiedHandler(mDeathRecipientCookie);
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  if (!mServiceManager->registerForNotifications(SUPPLICANT_INTERFACE_NAME, "",
                                                 this)) {
    WIFI_LOGE(LOG_TAG, "Failed to register for notifications to ISupplicant");
    mServiceManager = nullptr;
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::InitSupplicantInterface() {
  MutexAutoLock lock(s_Lock);
  mSupplicant = ISupplicant::getService();

  if (mSupplicant != nullptr) {
    if (mSupplicantDeathRecipient == nullptr) {
      mSupplicantDeathRecipient = new SupplicantDeathRecipient(s_Instance);
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
        this, [&response](const SupplicantStatus &status) { response = status; });
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
  MutexAutoLock lock(s_Lock);
  return mServiceManager != nullptr;
}

bool SupplicantStaManager::IsInterfaceReady() {
  MutexAutoLock lock(s_Lock);
  return mSupplicant != nullptr;
}

Result_t SupplicantStaManager::TearDownInterface() {
  MutexAutoLock lock(s_Lock);

  if (mSupplicantStaIface.get()) {
    SupplicantStatus response;
    ISupplicant::IfaceInfo info;
    info.name = mInterfaceName;
    info.type = SupplicantNameSpace::IfaceType::STA;

    mSupplicant->removeInterface(
        info, [&](const SupplicantStatus& status) { response = status; });

    if (response.code != SupplicantStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to remove sta interface");
      return nsIWifiResult::ERROR_COMMAND_FAILED;
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
      aLevel->mShowTimeStamp,
      aLevel->mShowKeys,
      [](const SupplicantStatus& status) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
          WIFI_LOGE(LOG_TAG, "Failed to set suppliant debug level");
        }
      });
  return nsIWifiResult::SUCCESS;
}

Result_t SupplicantStaManager::SetConcurrencyPriority(bool aEnable) {
  SupplicantNameSpace::IfaceType type =
      (aEnable ? SupplicantNameSpace::IfaceType::STA
               : SupplicantNameSpace::IfaceType::P2P);

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
    SupplicantNameSpace::IfaceType aDesired, ISupplicant::IfaceInfo* aInfo) {
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

  if (mSupplicantStaIface == nullptr) {
    mSupplicantStaIface = AddSupplicantStaIface();
  }
  return CHECK_SUCCESS(mSupplicantStaIface != nullptr);
}

android::sp<ISupplicantStaIface> SupplicantStaManager::GetSupplicantStaIface() {
  if (mSupplicant == nullptr) {
    return nullptr;
  }
  ISupplicant::IfaceInfo info;
  if (FindIfaceOfType(SupplicantNameSpace::IfaceType::STA, &info) !=
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

  staIface->registerCallback(
      this, [&response](const SupplicantStatus &status) { response = status; });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "registerCallback failed: %d", response.code);
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
  if (FindIfaceOfType(SupplicantNameSpace::IfaceType::STA, &info) ==
      nsIWifiResult::SUCCESS) {
    // STA interface already exist, remove it to add a new one
    mSupplicant->removeInterface(
        info, [&](const SupplicantStatus& status) { response = status; });

    if (response.code != SupplicantStatusCode::SUCCESS) {
      WIFI_LOGE(LOG_TAG, "Failed to remove exist STA interface");
    }
  }

  info.name = mInterfaceName;
  info.type = SupplicantNameSpace::IfaceType::STA;
  android::sp<ISupplicantStaIface> staIface;
  mSupplicant->addInterface(
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

  staIface->registerCallback(
      this, [&response](const SupplicantStatus &status) { response = status; });

  if (response.code != SupplicantStatusCode::SUCCESS) {
    WIFI_LOGE(LOG_TAG, "registerCallback failed: %d", response.code);
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
  return new SupplicantStaNetwork(staNetwork.get());
}

android::sp<SupplicantStaNetwork> SupplicantStaManager::GetNetwork(
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
  return new SupplicantStaNetwork(staNetwork.get());
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
    std::unordered_map<std::string,
                       android::sp<SupplicantStaNetwork>>::const_iterator
        existNetwork = mCurrentNetwork.find(mInterfaceName);

    if (existNetwork == mCurrentNetwork.end()) {
      WIFI_LOGE(LOG_TAG, "Network is not available");
      return nsIWifiResult::ERROR_COMMAND_FAILED;
    }
    staNetwork = mCurrentNetwork.at(mInterfaceName);
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
    // TODO: need to remove network here
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

  std::unordered_map<std::string,
                     android::sp<SupplicantStaNetwork>>::const_iterator found =
      mCurrentNetwork.find(mInterfaceName);
  if (found == mCurrentNetwork.end()) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  network = mCurrentNetwork.at(mInterfaceName);
  return network->EnableNetwork();
}

Result_t SupplicantStaManager::DisableNetwork() {
  android::sp<SupplicantStaNetwork> network;
  std::unordered_map<std::string,
                     android::sp<SupplicantStaNetwork>>::const_iterator found =
      mCurrentNetwork.find(mInterfaceName);
  if (found == mCurrentNetwork.end()) {
    return nsIWifiResult::ERROR_COMMAND_FAILED;
  }

  network = mCurrentNetwork.at(mInterfaceName);
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
    HIDL_SET(mSupplicantStaIface, removeNetwork, SupplicantStatus,
             response, id);
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

/**
 * P2P functions
 */
android::sp<ISupplicantP2pIface> SupplicantStaManager::GetSupplicantP2pIface() {
  if (mSupplicant == nullptr) {
    return nullptr;
  }
  ISupplicant::IfaceInfo info;
  if (FindIfaceOfType(SupplicantNameSpace::IfaceType::P2P, &info) !=
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

void SupplicantStaManager::NotifyTerminating() {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_TERMINATING));

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyStateChanged(uint32_t aState,
                                              const std::string& aSsid,
                                              const std::string& aBssid) {
  mFourwayHandshake = (aState ==
      (uint32_t)ISupplicantStaIfaceCallback::State::FOURWAY_HANDSHAKE);

  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_STATE_CHANGED));
  RefPtr<nsStateChanged> stateChanged =
      new nsStateChanged(aState, 0, NS_ConvertUTF8toUTF16(aBssid.c_str()),
                         NS_ConvertUTF8toUTF16(aSsid.c_str()));
  event->updateStateChanged(stateChanged);

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyConnected(const std::string& aSsid,
                                           const std::string& aBssid) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_NETWORK_CONNECTED));
  event->mBssid = NS_ConvertUTF8toUTF16(aBssid.c_str());

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyDisconnected(const std::string& aBssid,
                                              bool aLocallyGenerated,
                                              uint32_t aReason) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_NETWORK_DISCONNECTED));
  event->mBssid = NS_ConvertUTF8toUTF16(aBssid.c_str());
  event->mLocallyGenerated = aLocallyGenerated;
  event->mReason = aReason;

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyAuthenticationFailure(uint32_t aReason,
                                                       int32_t aErrorCode) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_AUTH_FAILURE));
  event->mReason = aReason;
  event->mErrorCode = aErrorCode;

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyAssociationReject(const std::string& aBssid,
                                                   uint32_t aStatusCode,
                                                   bool aTimeout) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_ASSOC_REJECT));
  event->mBssid = NS_ConvertUTF8toUTF16(aBssid.c_str());
  event->mStatusCode = aStatusCode;
  event->mTimeout = aTimeout;

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyTargetBssid(const std::string& aBssid) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_TARGET_BSSID));
  event->mBssid = NS_ConvertUTF8toUTF16(aBssid.c_str());

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
}

void SupplicantStaManager::NotifyAssociatedBssid(const std::string& aBssid) {
  nsCString iface(mInterfaceName);
  RefPtr<nsWifiEvent> event =
      new nsWifiEvent(NS_LITERAL_STRING(EVENT_SUPPLICANT_ASSOCIATED_BSSID));
  event->mBssid = NS_ConvertUTF8toUTF16(aBssid.c_str());

  if (mCallback) {
    mCallback->Notify(event, iface);
  }
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
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "SupplicantCallback.onTerminating()");

  NotifyTerminating();
  return android::hardware::Void();
}

/**
 * ISupplicantStaIfaceCallback implementation
 */
Return<void> SupplicantStaManager::onNetworkAdded(uint32_t id) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onNetworkAdded()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onNetworkRemoved(uint32_t id) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onNetworkRemoved()");
  mFourwayHandshake = false;
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onStateChanged(
    ISupplicantStaIfaceCallback::State newState,
    const android::hardware::hidl_array<uint8_t, 6>& bssid, uint32_t id,
    const android::hardware::hidl_vec<uint8_t>& ssid) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onStateChanged()");

  std::string bssid_str = ConvertMacToString(bssid);
  std::string ssid_str(ssid.begin(), ssid.end());

  if (newState == ISupplicantStaIfaceCallback::State::COMPLETED) {
    NotifyConnected(ssid_str, bssid_str);
  }
  NotifyStateChanged((uint32_t)newState, ssid_str, bssid_str);

  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onAnqpQueryDone(
    const android::hardware::hidl_array<uint8_t, 6>& bssid,
    const ISupplicantStaIfaceCallback::AnqpData& data,
    const ISupplicantStaIfaceCallback::Hs20AnqpData& hs20Data) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onAnqpQueryDone()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onHs20IconQueryDone(
    const android::hardware::hidl_array<uint8_t, 6>& bssid,
    const ::android::hardware::hidl_string& fileName,
    const ::android::hardware::hidl_vec<uint8_t>& data) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onHs20IconQueryDone()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onHs20SubscriptionRemediation(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
    ISupplicantStaIfaceCallback::OsuMethod osuMethod,
    const ::android::hardware::hidl_string& url) {
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaIfaceCallback.onHs20SubscriptionRemediation()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onHs20DeauthImminentNotice(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
    uint32_t reasonCode, uint32_t reAuthDelayInSec,
    const ::android::hardware::hidl_string& url) {
  WIFI_LOGD(LOG_TAG,
            "ISupplicantStaIfaceCallback.onHs20DeauthImminentNotice()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onDisconnected(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
    bool locallyGenerated, ISupplicantStaIfaceCallback::ReasonCode reasonCode) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onDisconnected()");

  if (mFourwayHandshake &&
      (!locallyGenerated || reasonCode !=
          ISupplicantStaIfaceCallback::ReasonCode::IE_IN_4WAY_DIFFERS)) {

    NotifyAuthenticationFailure(nsIWifiEvent::AUTH_FAILURE_WRONG_KEY,
                                nsIWifiEvent::ERROR_CODE_NONE);
  }
  std::string bssid_str = ConvertMacToString(bssid);
  NotifyDisconnected(bssid_str, locallyGenerated, (uint32_t)reasonCode);

  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onAssociationRejected(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
    ISupplicantStaIfaceCallback::StatusCode statusCode, bool timedOut) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onAssociationRejected()");

  if (statusCode ==
      ISupplicantStaIfaceCallback::StatusCode::UNSPECIFIED_FAILURE) {
    // Special handling for WPA3-Personal networks. If the password is
    // incorrect, the AP will send association rejection, with status code 1
    // (unspecified failure). In SAE networks, the password authentication
    // is not related to the 4-way handshake. In this case, we will send an
    // authentication failure event up.

    // TODO: check if current connecting network is WPA3-Personal
    // NotifyAuthenticationFailure(nsIWifiEvent::AUTH_FAILURE_WRONG_KEY,
    //                             nsIWifiEvent::ERROR_CODE_NONE);
  }
  std::string bssid_str = ConvertMacToString(bssid);
  NotifyAssociationReject(bssid_str, (uint32_t)statusCode, timedOut);

  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onAuthenticationTimeout(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onAuthenticationTimeout()");
  NotifyAuthenticationFailure(nsIWifiEvent::AUTH_FAILURE_TIMEOUT,
                              nsIWifiEvent::ERROR_CODE_NONE);

  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onEapFailure() {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onEapFailure()");
  NotifyAuthenticationFailure(nsIWifiEvent::AUTH_FAILURE_EAP_FAILURE,
                              nsIWifiEvent::ERROR_CODE_NONE);

  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onBssidChanged(
    ISupplicantStaIfaceCallback::BssidChangeReason reason,
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid) {
  MutexAutoLock lock(s_Lock);
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onBssidChanged()");

  std::string bssid_str = ConvertMacToString(bssid);
  if (reason == ISupplicantStaIfaceCallback::BssidChangeReason::ASSOC_START) {
    NotifyTargetBssid(bssid_str);
  } else if (reason ==
             ISupplicantStaIfaceCallback::BssidChangeReason::ASSOC_COMPLETE) {
    NotifyAssociatedBssid(bssid_str);
  }
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onWpsEventSuccess() {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onWpsEventSuccess()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onWpsEventFail(
    const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
    ISupplicantStaIfaceCallback::WpsConfigError configError,
    ISupplicantStaIfaceCallback::WpsErrorIndication errorInd) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onWpsEventFail()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onWpsEventPbcOverlap() {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onWpsEventPbcOverlap()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onExtRadioWorkStart(uint32_t id) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onExtRadioWorkStart()");
  return android::hardware::Void();
}

Return<void> SupplicantStaManager::onExtRadioWorkTimeout(uint32_t id) {
  WIFI_LOGD(LOG_TAG, "ISupplicantStaIfaceCallback.onExtRadioWorkTimeout()");
  return android::hardware::Void();
}
