/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include <utils/String16.h>
#include <binder/IServiceManager.h>
#include "mozilla/ClearOnShutdown.h"
#include "NetdUnsolService.h"
#include <android/log.h>

using android::binder::Status;

static bool ENABLE_NUS_DEBUG = false;

static void NUS_DBG(const char* format, ...) {
  // TODO: always true for now.
#if 0
  if (!ENABLE_DEBUG) {
    return;
  }
#endif

  va_list args;
  va_start(args, format);
  __android_log_vprint(ANDROID_LOG_INFO, "NetdUnsolService", format, args);
  va_end(args);
}

NetdUnsolService::NetdUnsolService(NetdEventCallback aCallback) :
mNetdEventCallback(aCallback)
{
  android::defaultServiceManager()->addService(
      android::String16(NetdUnsolService::getServiceName()), this, false,
      android::IServiceManager::DUMP_FLAG_PRIORITY_DEFAULT);
  android::sp<android::ProcessState> ps(android::ProcessState::self());
  ps->startThreadPool();
}

void NetdUnsolService::updateDebug(bool aEnable) {
  ENABLE_NUS_DEBUG = aEnable;
}

void NetdUnsolService::sendBroadcast(UnsolEvent evt, char* reason) {
  mozilla::dom::NetworkResultOptions result;

  switch (evt) {
    case InterfaceDnsServersAdded:
      result.mTopic = NS_ConvertUTF8toUTF16("interface-dns-info");
      break;
    case InterfaceAddressUpdated:
    case InterfaceAddressRemoved:
      result.mTopic = NS_ConvertUTF8toUTF16("interface-address-change");
      break;
    case InterfaceChanged:
      result.mTopic = NS_ConvertUTF8toUTF16("netd-interface-change");
      break;
    case RouteChanged:
      result.mTopic = NS_ConvertUTF8toUTF16("route-change");
      break;
    default:
      return;
  }

  result.mBroadcast = true;
  result.mReason = NS_ConvertUTF8toUTF16(reason);
  if (mNetdEventCallback) {
    mNetdEventCallback(result);
  }
}

Status NetdUnsolService::onInterfaceDnsServerInfo(
    const std::string& ifName, int64_t lifetime,
    const std::vector<std::string>& servers) {
  char message[BUF_SIZE];
  char dnses[BUF_SIZE];
  char buffer[BUF_SIZE];
  for (uint32_t i = 0; i < servers.size(); i++) {
    memset(&buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), " %s", servers[i].c_str());
    strcat(dnses, buffer);
  }

  snprintf(message, sizeof(message), "DnsInfo servers %s %ld%s", ifName.c_str(),
           lifetime, dnses);
  NUS_DBG("%s", message);
  sendBroadcast(InterfaceDnsServersAdded, message);
  return Status::ok();
}

Status NetdUnsolService::onInterfaceAddressUpdated(const std::string& addr,
                                                   const std::string& ifName,
                                                   int flags, int scope) {
  char message[BUF_SIZE];

  snprintf(message, sizeof(message), "Address updated %s %s %d %d",
           addr.c_str(), ifName.c_str(), flags, scope);
  NUS_DBG("%s", message);
  sendBroadcast(InterfaceAddressUpdated, message);
  return Status::ok();
}

Status NetdUnsolService::onInterfaceAddressRemoved(const std::string& addr,
                                                   const std::string& ifName,
                                                   int flags, int scope) {
  char message[BUF_SIZE];
  snprintf(message, sizeof(message), "Address updated %s %s %d %d",
           addr.c_str(), ifName.c_str(), flags, scope);
  NUS_DBG("%s", message);
  sendBroadcast(InterfaceAddressRemoved, message);
  return Status::ok();
}

Status NetdUnsolService::onInterfaceLinkStateChanged(const std::string& ifName,
                                                     bool status) {
  char message[BUF_SIZE];
  snprintf(message, sizeof(message), "Iface linkstate %s %s", ifName.c_str(),
           status ? "up" : "down");
  NUS_DBG("%s", message);
  sendBroadcast(InterfaceLinkStatusChanged, message);
  return Status::ok();
}

Status NetdUnsolService::onRouteChanged(bool updated, const std::string& route,
                                        const std::string& gateway,
                                        const std::string& ifName) {
  char message[BUF_SIZE];
  snprintf(message, sizeof(message), "Route %s %s via %s dev %s",
           updated ? "updated" : "removed", route.c_str(), gateway.c_str(),
           ifName.c_str());
  NUS_DBG("%s", message);
  sendBroadcast(RouteChanged, message);
  return Status::ok();
}

// TODO: Implement notify if we need these.
Status NetdUnsolService::onInterfaceAdded(const std::string& ifName) {
  return Status::ok();
}

Status NetdUnsolService::onInterfaceRemoved(const std::string& ifName) {
  return Status::ok();
}

Status NetdUnsolService::onInterfaceClassActivityChanged(bool isActive,
                                                         int label,
                                                         int64_t timestamp,
                                                         int uid) {
  return Status::ok();
}

Status NetdUnsolService::onQuotaLimitReached(const std::string& alertName,
                                             const std::string& ifName) {
  return Status::ok();
}

Status NetdUnsolService::onInterfaceChanged(const std::string& ifName,
                                            bool status) {
  return Status::ok();
}

Status NetdUnsolService::onStrictCleartextDetected(int uid,
                                                   const std::string& hex) {
  return Status::ok();
}
