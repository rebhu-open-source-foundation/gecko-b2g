/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef GONKACTIVITYMANAGERSERVICE_H
#define GONKACTIVITYMANAGERSERVICE_H

#include <binder/BinderService.h>
#include <binder/IActivityManager.h>

namespace android {
class BnActivityManager : public BnInterface<IActivityManager>
{
public:
    // NOLINTNEXTLINE(google-default-arguments)
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

} // namespace android

namespace mozilla {

/*
 * Replacement of AOSP Java ActivityManagerService.
 * From Android Pie (API 28), Google introduced some permission check
 * for some resouces, such as microphone in AudioPolicyService.
 * In AudioPolicyService, it gets the APP foreground information from Java
 * ActivityManagerService. Since we don't have Java, we need to provide our
 * C++ version service to let AOSP AudioPolicyService to use.
 */
class GonkActivityManagerService :
  public android::BinderService<GonkActivityManagerService>,
  public android::BnActivityManager
{
public:
  virtual ~GonkActivityManagerService() {}
  // From android::BinderService
  static GonkActivityManagerService* GetInstance();
  static const char *getServiceName() {
    return "activity";
  }
  static void instantiate();

  // protected:
  // IBinder* onAsBinder() override;

  // From IActivityManager
  virtual int openContentUri(const android::String16& stringUri) override;
  virtual void registerUidObserver(const android::sp<android::IUidObserver>& observer,
                                    const int32_t event,
                                    const int32_t cutpoint,
                                    const android::String16& callingPackage) override;
  virtual void unregisterUidObserver(const android::sp<android::IUidObserver>& observer) override;
  virtual bool isUidActive(const uid_t uid, const android::String16& callingPackage) override;
  virtual int32_t getUidProcessState(const uid_t uid, const android::String16& callingPackage) override;
private:
  GonkActivityManagerService(): android::BnActivityManager() {}

};

} // namespace mozilla

#endif // GONKACTIVITYMANAGERSERVICE_H
