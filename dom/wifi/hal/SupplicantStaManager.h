/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SupplicantStaManager_H
#define SupplicantStaManager_H

#include "WifiCommon.h"
#include "WifiEventCallback.h"
#include "SupplicantStaNetwork.h"

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantCallback.h>
#include <android/hardware/wifi/supplicant/1.0/ISupplicantIface.h>
#include <android/hardware/wifi/supplicant/1.0/types.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicant.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicantStaIface.h>
#include <android/hardware/wifi/supplicant/1.2/ISupplicantP2pIface.h>

#include "mozilla/Mutex.h"

/**
 * Class for supplicant HIDL client implementation.
 */
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::wifi::supplicant::V1_0::Bssid;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_2::ISupplicant;
using ::android::hidl::base::V1_0::IBase;

namespace SupplicantNameSpace = ::android::hardware::wifi::supplicant::V1_0;

class SupplicantStaManager
    : virtual public android::hidl::manager::V1_0::IServiceNotification,
      virtual public android::hardware::wifi::supplicant::V1_0::ISupplicantCallback,
      virtual public android::hardware::wifi::supplicant::V1_0::ISupplicantStaIfaceCallback {
 public:
  static SupplicantStaManager* Get();
  static void CleanUp();
  void RegisterEventCallback(WifiEventCallback* aCallback);
  void UnregisterEventCallback();

  // HIDL initialization
  Result_t InitInterface();
  Result_t DeinitInterface();
  bool IsInterfaceInitializing();
  bool IsInterfaceReady();

  // functions to invoke supplicant APIs
  Result_t GetMacAddress(nsAString& aMacAddress);
  Result_t GetSupplicantDebugLevel(uint32_t& aLevel);
  Result_t SetSupplicantDebugLevel(SupplicantDebugLevelOptions* aLevel);
  Result_t SetConcurrencyPriority(bool aEnable);
  Result_t SetPowerSave(bool aEnable);
  Result_t SetSuspendMode(bool aEnable);
  Result_t SetExternalSim(bool aEnable);
  Result_t SetAutoReconnect(bool aEnable);
  Result_t SetCountryCode(const std::string& aCountryCode);
  Result_t SetBtCoexistenceMode(uint8_t aMode);
  Result_t SetBtCoexistenceScanMode(bool aEnable);
  Result_t ConnectToNetwork(ConfigurationOptions* aConfig);
  Result_t SetupStaInterface(const std::string& aInterfaceName);
  Result_t SetupP2pInterface();
  Result_t Reconnect();
  Result_t Reassociate();
  Result_t Disconnect();
  Result_t RemoveNetworks();
  android::sp<SupplicantStaNetwork> CreateStaNetwork();
  android::sp<SupplicantStaNetwork> GetNetwork(uint32_t aNetId);

  // death event handler
  void RegisterDeathHandler(SupplicantDeathEventHandler* aHandler);
  void UnregisterDeathHandler();

  // IServiceNotification::onRegistration
  virtual Return<void> onRegistration(const hidl_string& fqName,
                                      const hidl_string& name,
                                      bool preexisting) override;

 private:
  //...................... ISupplicantCallback ......................../
  /**
   * Used to indicate that a new interface has been created.
   *
   * @param ifName Name of the network interface, e.g., wlan0
   */
  Return<void> onInterfaceCreated(const hidl_string& ifName) override;
  /**
   * Used to indicate that an interface has been removed.
   *
   * @param ifName Name of the network interface, e.g., wlan0
   */
  Return<void> onInterfaceRemoved(const hidl_string& ifName) override;
  /**
   * Used to indicate that the supplicant daemon is terminating.
   */
  Return<void> onTerminating() override;

  //.................. ISupplicantStaIfaceCallback ..................../
  /**
   * Used to indicate that a new network has been added.
   *
   * @param id Network ID allocated to the corresponding network.
   */
  Return<void> onNetworkAdded(uint32_t id) override;
  /**
   * Used to indicate that a network has been removed.
   *
   * @param id Network ID allocated to the corresponding network.
   */
  Return<void> onNetworkRemoved(uint32_t id) override;
  /**
   * Used to indicate a state change event on this particular iface. If this
   * event is triggered by a particular network, the |SupplicantNetworkId|,
   * |ssid|, |bssid| parameters must indicate the parameters of the network/AP
   * which cased this state transition.
   *
   * @param newState New State of the interface. This must be one of the |State|
   *        values above.
   * @param bssid BSSID of the corresponding AP which caused this state
   *        change event. This must be zero'ed if this event is not
   *        specific to a particular network.
   * @param id ID of the corresponding network which caused this
   *        state change event. This must be invalid (UINT32_MAX) if this
   *        event is not specific to a particular network.
   * @param ssid SSID of the corresponding network which caused this state
   *        change event. This must be empty if this event is not specific
   *        to a particular network.
   */
  Return<void> onStateChanged(
      ISupplicantStaIfaceCallback::State newState,
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid, uint32_t id,
      const ::android::hardware::hidl_vec<uint8_t>& ssid) override;
  /**
   * Used to indicate the result of ANQP (either for IEEE 802.11u Interworking
   * or Hotspot 2.0) query.
   *
   * @param bssid BSSID of the access point.
   * @param data ANQP data fetched from the access point.
   *        All the fields in this struct must be empty if the query failed.
   * @param hs20Data ANQP data fetched from the Hotspot 2.0 access point.
   *        All the fields in this struct must be empty if the query failed.
   */
  Return<void> onAnqpQueryDone(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      const ISupplicantStaIfaceCallback::AnqpData& data,
      const ISupplicantStaIfaceCallback::Hs20AnqpData& hs20Data) override;
  /**
   * Used to indicate the result of Hotspot 2.0 Icon query.
   *
   * @param bssid BSSID of the access point.
   * @param fileName Name of the file that was requested.
   * @param data Icon data fetched from the access point.
   *        Must be empty if the query failed.
   */
  Return<void> onHs20IconQueryDone(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      const ::android::hardware::hidl_string& fileName,
      const ::android::hardware::hidl_vec<uint8_t>& data) override;
  /**
   * Used to indicate a Hotspot 2.0 subscription remediation event.
   *
   * @param bssid BSSID of the access point.
   * @param osuMethod OSU method.
   * @param url URL of the server.
   */
  Return<void> onHs20SubscriptionRemediation(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      ISupplicantStaIfaceCallback::OsuMethod osuMethod,
      const ::android::hardware::hidl_string& url) override;
  /**
   * Used to indicate a Hotspot 2.0 imminent deauth notice.
   *
   * @param bssid BSSID of the access point.
   * @param reasonCode Code to indicate the deauth reason.
   *        Refer to section 3.2.1.2 of the Hotspot 2.0 spec.
   * @param reAuthDelayInSec Delay before reauthenticating.
   * @param url URL of the server.
   */
  Return<void> onHs20DeauthImminentNotice(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      uint32_t reasonCode, uint32_t reAuthDelayInSec,
      const ::android::hardware::hidl_string& url) override;
  /**
   * Used to indicate the disconnection from the currently connected
   * network on this iface.
   *
   * @param bssid BSSID of the AP from which we disconnected.
   * @param locallyGenerated If the disconnect was triggered by
   *        wpa_supplicant.
   * @param reasonCode 802.11 code to indicate the disconnect reason
   *        from access point. Refer to section 8.4.1.7 of IEEE802.11 spec.
   */
  Return<void> onDisconnected(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      bool locallyGenerated,
      ISupplicantStaIfaceCallback::ReasonCode reasonCode) override;
  /**
   * Used to indicate an association rejection recieved from the AP
   * to which the connection is being attempted.
   *
   * @param bssid BSSID of the corresponding AP which sent this
   *        reject.
   * @param statusCode 802.11 code to indicate the reject reason.
   *        Refer to section 8.4.1.9 of IEEE 802.11 spec.
   * @param timedOut Whether failure is due to timeout rather
   *        than explicit rejection response from the AP.
   */
  Return<void> onAssociationRejected(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      ISupplicantStaIfaceCallback::StatusCode statusCode,
      bool timedOut) override;
  /**
   * Used to indicate the timeout of authentication to an AP.
   *
   * @param bssid BSSID of the corresponding AP.
   */
  Return<void> onAuthenticationTimeout(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid) override;
  /**
   * Used to indicate an EAP authentication failure.
   */
  Return<void> onEapFailure() override;
  /**
   * Used to indicate the change of active bssid.
   * This is useful to figure out when the driver/firmware roams to a bssid
   * on its own.
   *
   * @param reason Reason why the bssid changed.
   * @param bssid BSSID of the corresponding AP.
   */
  Return<void> onBssidChanged(
      ISupplicantStaIfaceCallback::BssidChangeReason reason,
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid) override;
  /**
   * Used to indicate the success of a WPS connection attempt.
   */
  Return<void> onWpsEventSuccess() override;
  /**
   * Used to indicate the failure of a WPS connection attempt.
   *
   * @param bssid BSSID of the AP to which we initiated WPS
   *        connection.
   * @param configError Configuration error code.
   * @param errorInd Error indication code.
   */
  Return<void> onWpsEventFail(
      const ::android::hardware::hidl_array<uint8_t, 6>& bssid,
      ISupplicantStaIfaceCallback::WpsConfigError configError,
      ISupplicantStaIfaceCallback::WpsErrorIndication errorInd) override;
  /**
   * Used to indicate the overlap of a WPS PBC connection attempt.
   */
  Return<void> onWpsEventPbcOverlap() override;
  /**
   * Used to indicate that the external radio work can start now.
   *
   * @return id Identifier generated for the radio work request.
   */
  Return<void> onExtRadioWorkStart(uint32_t id) override;
  /**
   * Used to indicate that the external radio work request has timed out.
   *
   * @return id Identifier generated for the radio work request.
   */
  Return<void> onExtRadioWorkTimeout(uint32_t id) override;

  struct ServiceManagerDeathRecipient : public hidl_death_recipient {
    ServiceManagerDeathRecipient(SupplicantStaManager* aOuter)
        : mOuter(aOuter) {}
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie,
                             const ::android::wp<IBase>& who) override;

   private:
    SupplicantStaManager* mOuter;
  };

  struct SupplicantDeathRecipient : public hidl_death_recipient {
    SupplicantDeathRecipient(SupplicantStaManager* aOuter) : mOuter(aOuter) {}
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie,
                             const ::android::wp<IBase>& who) override;

   private:
    SupplicantStaManager* mOuter;
  };

  SupplicantStaManager();
  virtual ~SupplicantStaManager();

  Result_t InitServiceManager();
  Result_t InitSupplicantInterface();
  Result_t TearDownInterface();

  android::sp<ISupplicantStaIface> GetSupplicantStaIface();
  android::sp<ISupplicantStaIface> AddSupplicantStaIface();
  android::sp<ISupplicantP2pIface> GetSupplicantP2pIface();
  Result_t FindIfaceOfType(SupplicantNameSpace::IfaceType aDesired,
                           ISupplicant::IfaceInfo* aInfo);

  void supplicantServiceDiedHandler(int32_t aCookie);

  static SupplicantStaManager* s_Instance;
  static mozilla::Mutex s_Lock;

  android::sp<::android::hidl::manager::V1_0::IServiceManager> mServiceManager;
  android::sp<ISupplicant> mSupplicant;
  android::sp<ISupplicantStaIface> mSupplicantStaIface;
  android::sp<ServiceManagerDeathRecipient> mServiceManagerDeathRecipient;
  android::sp<SupplicantDeathRecipient> mSupplicantDeathRecipient;
  android::sp<SupplicantDeathEventHandler> mDeathEventHandler;
  android::sp<WifiEventCallback> mCallback;

  int32_t mDeathRecipientCookie;
  std::string mStaInterfaceName;

  DISALLOW_COPY_AND_ASSIGN(SupplicantStaManager);
};

#endif /* SupplicantStaManager_H */
