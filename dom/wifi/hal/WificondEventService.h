/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */
#ifndef WificondEventService_H
#define WificondEventService_H

#include <android/net/wifi/BnScanEvent.h>
#include <binder/BinderService.h>

class WificondEventService
    : virtual public android::BinderService<WificondEventService>,
      virtual public android::net::wifi::BnScanEvent {
 public:
  WificondEventService(const std::string& aInterfaceName)
      : android::net::wifi::BnScanEvent(), mStaInterfaceName(aInterfaceName) {}
  virtual ~WificondEventService() = default;

  static char const* GetServiceName() { return "wificond_event"; }
  static WificondEventService* CreateService(const std::string& aInterfaceName);

  void RegisterEventCallback(EventCallback aCallback);
  void UnregisterEventCallback();

  // IScanEvent
  android::binder::Status OnScanResultReady() override;
  android::binder::Status OnScanFailed() override;

 private:
  static WificondEventService* s_Instance;
  static mozilla::Mutex s_Lock;
  std::string mStaInterfaceName;

  EventCallback mEventCallback;
};

#endif  // WificondEventService_H
