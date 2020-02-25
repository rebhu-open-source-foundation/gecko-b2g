/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef SoftapEventCallback_H
#define SoftapEventCallback_H

#include <android/net/wifi/BnApInterfaceEventCallback.h>
#include <binder/BinderService.h>

class SoftapEventService
    : virtual public android::BinderService<SoftapEventService>,
      virtual public android::net::wifi::BnApInterfaceEventCallback {
 public:
  SoftapEventService(const std::string& aInterfaceName)
      : android::net::wifi::BnApInterfaceEventCallback(),
        mSoftapInterfaceName(aInterfaceName) {}
  virtual ~SoftapEventService() = default;

  static char const* GetServiceName() { return "softap_event"; }
  static SoftapEventService* CreateService(const std::string& aInterfaceName);

  void RegisterEventCallback(EventCallback aCallback);
  void UnregisterEventCallback();

  // IApInterfaceEventCallback
  android::binder::Status onNumAssociatedStationsChanged(
      int32_t numStations) override;
  android::binder::Status onSoftApChannelSwitched(
      int32_t frequency, int32_t bandwidth) override;

 private:
  static SoftapEventService* s_Instance;
  std::string mSoftapInterfaceName;

  EventCallback mEventCallback;
};

#endif  // SoftapEventCallback_H
