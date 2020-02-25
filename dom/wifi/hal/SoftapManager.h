/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef SOFTAPMANAGER_H
#define SOFTAPMANAGER_H

#include "WifiCommon.h"
#include "SoftapManager.h"

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <android/hardware/wifi/hostapd/1.0/types.h>
#include <android/hardware/wifi/hostapd/1.1/IHostapd.h>
#include <android/hardware/wifi/hostapd/1.1/IHostapdCallback.h>
#include "mozilla/Mutex.h"

/**
 * Class for softap management and hostpad HIDL client implementation.
 */
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_0::IHostapd;
using ::android::hidl::manager::V1_0::IServiceNotification;

using IHostapd_1_1 = ::android::hardware::wifi::hostapd::V1_1::IHostapd;

class SoftapManager
    : virtual public android::hidl::manager::V1_0::IServiceNotification,
      virtual public android::hardware::wifi::hostapd::V1_1::IHostapdCallback {
 public:
  static SoftapManager* Get();
  static void CleanUp();

  // HIDL initialization
  bool InitInterface();
  bool DeinitInterface();
  bool IsInterfaceInitializing();
  bool IsInterfaceReady();

  bool StartSoftap(const std::string& aInterfaceName,
                   SoftapConfigurationOptions* aSoftapConfig);
  bool StopSoftap(const std::string& aInterfaceName);
  void SetSoftapCountryCode(const std::string& aCountryCode);

  // IServiceNotification::onRegistration
  virtual Return<void> onRegistration(const hidl_string& fqName,
                                      const hidl_string& name,
                                      bool preexisting) override;

 private:
  //......................... IHostapdCallback .........................../
  /**
   * Invoked when an asynchronous failure is encountered in one of the access
   * points added via |IHostapd.addAccessPoint|.
   *
   * @param ifaceName Name of the interface.
   */
  Return<void> onFailure(const hidl_string& ifaceName) override;

  struct ServiceManagerDeathRecipient : public hidl_death_recipient {
    ServiceManagerDeathRecipient(SoftapManager* aOuter) : mOuter(aOuter) {}
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie,
                             const ::android::wp<IBase>& who) override;

   private:
    SoftapManager* mOuter;
  };

  struct HostapdDeathRecipient : public hidl_death_recipient {
    HostapdDeathRecipient(SoftapManager* aOuter) : mOuter(aOuter) {}
    // hidl_death_recipient interface
    virtual void serviceDied(uint64_t cookie,
                             const ::android::wp<IBase>& who) override;

   private:
    SoftapManager* mOuter;
  };

  SoftapManager();
  virtual ~SoftapManager();
  bool InitServiceManager();
  bool InitHostapdInterface();
  bool TearDownInterface();

  static SoftapManager* s_Instance;
  static mozilla::Mutex s_Lock;

  android::sp<android::hidl::manager::V1_0::IServiceManager> mServiceManager;
  android::sp<IHostapd_1_1> mHostapd;
  android::sp<ServiceManagerDeathRecipient> mServiceManagerDeathRecipient;
  android::sp<HostapdDeathRecipient> mHostapdDeathRecipient;
  std::string mCountryCode;

  DISALLOW_COPY_AND_ASSIGN(SoftapManager);
};

#endif /* SOFTAPMANAGER_H */
