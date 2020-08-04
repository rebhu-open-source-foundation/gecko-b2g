/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PasspointHandler_H
#define PasspointHandler_H

#include "AnqpElement.h"
#include "WifiCommon.h"
#include "PasspointEventCallback.h"
#include "nsDataHashtable.h"

BEGIN_WIFI_NAMESPACE

class SupplicantStaManager;

class PasspointHandler final : public nsISupports,
                               public PasspointEventCallback {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS

  static already_AddRefed<PasspointHandler> Get();

  void Cleanup();
  void SetSupplicantManager(const android::sp<SupplicantStaManager>& aManager);
  void RegisterEventCallback(const android::sp<WifiEventCallback>& aCallback);
  void UnregisterEventCallback();
  Result_t RequestAnqp(const nsAString& aAnqpKey, const nsAString& aBssid,
                       bool aRoamingConsortiumOIs, bool aSupportRelease2);

 private:
  explicit PasspointHandler();
  ~PasspointHandler() {}

  // PasspointEventCallback
  virtual void NotifyAnqpResponse(const nsACString& aIface,
                                  const nsAString& aBssid,
                                  AnqpResponseMap& aAnqpData) override;
  virtual void NotifyIconResponse(const nsACString& aIface,
                                  const nsAString& aBssid) override;
  virtual void NotifyWnmFrameReceived(const nsACString& aIface,
                                      const nsAString& aBssid) override;

  bool ReadyToRequest(const nsAString& aBssid);
  bool UpdateTimeStame(const nsAString& aBssid);

  Result_t StartAnqpQuery(const nsAString& aBssid, bool aRoamingConsortiumOIs,
                          bool aSupportRelease2);

  class AnqpRequestTime {
   public:
    explicit AnqpRequestTime(const mozilla::TimeStamp& aTimestamp)
        : mTimeIncrement(0), mTimeStamp(aTimestamp) {}

    // Hold the counter to indicate the increment times.
    uint32_t mTimeIncrement;
    // Time stamp in milliseconds while ANQP request sent.
    mozilla::TimeStamp mTimeStamp;
  };

  class AnqpIdentity {
   public:
    explicit AnqpIdentity(const nsAString& aAnqpKey, const nsAString& aBssid)
        : mAnqpKey(aAnqpKey), mBssid(aBssid) {}

    bool Compare(AnqpIdentity* aIdentity);

    nsString mAnqpKey;
    nsString mBssid;
  };

  static StaticRefPtr<PasspointHandler> sPasspointHandler;
  static StaticMutex sMutex;

  android::sp<SupplicantStaManager> mSupplicantManager;
  android::sp<WifiEventCallback> mCallback;

  // The map holds the ANQP time information per bssid
  nsClassHashtable<nsStringHashKey, AnqpRequestTime> mAnqpRequestTime;
  // The map holds the ANQP identity data per bssid
  nsClassHashtable<nsStringHashKey, AnqpIdentity> mAnqpPendingRequest;

  DISALLOW_COPY_AND_ASSIGN(PasspointHandler);
};

END_WIFI_NAMESPACE

#endif  // PasspointHandler_H
