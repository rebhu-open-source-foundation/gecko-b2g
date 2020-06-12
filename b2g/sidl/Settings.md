# Settings API

The Settings API make applications and services able to access, change, and monitor changes to a centralized settings system together.

Settings is a collection of name-value pairs, a setting, or a setting info, consists of **name** and **value**.

Settings API service runs in api daemon, implemented by sqlite, with schema
```sql
CREATE TABLE settings ( name TEXT UNIQUE, value TEXT);
```
The stored **value** in db is a **json-stringified string**.

Gecko communicates with api daemon by **Unix Domain Socket**, so every call is **async**, callers should not expect their calls will return in sequence.

Settings **get** function **rejects** on **non-existing** settings, see sample codes for error code usage.

:warning: The sample codes are for reference only, please note that the actual usage should still be subject to **nsISettings.idl**.

:warning: The following codes omit null check for simplicity and readability, please remember to **add null check** before **every use**.

## Sample Codes of Settings API Callers from Gecko in C++
### nsISettingInfo used and shared by the following sample c++ codes
#### header declaration
```c++
class SettingInfo final : public nsISettingInfo {
  public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISETTINGINFO
  explicit SettingInfo(nsAString& aName, nsAString& aValue);

  protected:
    ~SettingInfo() = default;
  private:
    nsString mName;
    nsString mValue;
};
```
#### cpp definition
```c++
NS_IMPL_ISUPPORTS(SettingInfo, nsISettingInfo)
SettingInfo::SettingInfo(nsAString& aName, nsAString& aValue) :
  mName(aName),
  mValue(aValue) {}

NS_IMETHODIMP
SettingInfo::GetName(nsAString& aName) {
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::SetName(const nsAString& aName) {
  mName = aName;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::GetValue(nsAString& aValue) {
  aValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP
SettingInfo::SetValue(const nsAString& aValue) {
  mValue = aValue;
  return NS_OK;
}
```
### Set Function
#### header declaration
```c++
class TestSetCallback final : public nsISidlDefaultResponse {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIDLDEFAULTRESPONSE
    explicit TestSetCallback(const nsTArray<RefPtr<nsISettingInfo>>& aSettings);

  protected:
    ~TestSetCallback() = default;
  private:
    nsTArray<RefPtr<nsISettingInfo>> mSettings;
};
```
#### cpp definition
```c++
NS_IMPL_ISUPPORTS(TestSetCallback, nsISidlDefaultResponse)
TestSetCallback::TestSetCallback(const nsTArray<RefPtr<nsISettingInfo>>& aSettings) :
  mSettings(aSettings.Clone()) {
}

NS_IMETHODIMP
TestSetCallback::Resolve() {
  return NS_OK;
}

NS_IMETHODIMP
TestSetCallback::Reject() {
  return NS_OK;
}
```
#### cpp caller
```c++
nsCOMPtr<nsISettingsManager> settingsManager = do_GetService("@mozilla.org/sidl-native/settings;1");

nsTArray<RefPtr<nsISettingInfo>> settings;
// set testing.setting1 to number 42
RefPtr<nsISettingInfo> settingInfo1 = new SettingInfo(NS_LITERAL_STRING("testing.setting1"), NS_LITERAL_STRING("42"));
// set setting:json to object {"foo": bar, "success": true, "amount": 42}
RefPtr<nsISettingInfo> settingInfo2 = new SettingInfo(NS_LITERAL_STRING("setting:json"), NS_LITERAL_STRING("{\"foo\":\"bar\",\"success\":true,\"amount\":42}"));
settings.AppendElement(settingInfo1);
settings.AppendElement(settingInfo2);

nsCOMPtr<nsISidlDefaultResponse> setCallback = new TestSetCallback(settings);

settingsManager->Set(settings, setCallback);
```
### Get Function
#### header declaration
```c++
class TestGetCallback final : public nsISettingsGetResponse {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISETTINGSGETRESPONSE
    explicit TestGetCallback(nsString aName);

  protected:
    ~TestGetCallback() = default;
  private:
    nsString mName;
};
```
#### cpp definition
```c++
NS_IMPL_ISUPPORTS(TestGetCallback, nsISettingsGetResponse)
TestGetCallback::TestGetCallback(nsString aName) : mName(aName) {}

NS_IMETHODIMP
TestGetCallback::Resolve(nsISettingInfo *aSettingInfo) {
  nsString name;
  nsString value;
  aSettingInfo->GetName(name);
  aSettingInfo->GetValue(value);

  return NS_OK;
}

NS_IMETHODIMP
TestGetCallback::Reject(nsISettingError* aError) {
  nsString name;
  uint16_t reason;

  aError->GetName(name);
  aError->GetReason(&reason);

  if (reason == aError->NON_EXISTING_SETTING) {
    // Setting of such name does not exist.
  } else {
    // Something bad actually happened!
  }

  return NS_OK;
}
```
#### cpp caller
```c++
nsCOMPtr<nsISettingsManager> settingsManager = do_GetService("@mozilla.org/sidl-native/settings;1");
nsCOMPtr<nsISettingsGetResponse> getCallback = new TestGetCallback(aName);
settingsManager->Get(aName, getCallback);
```
### AddEventListener and RemoveEventListener Functions
#### header declaration
```c++
class TestEventHandler final : public nsISidlEventListener {
  public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIDLEVENTLISTENER

  protected:
    ~TestEventHandler() = default;
};
```
#### cpp definition
```c++
NS_IMPL_ISUPPORTS(TestEventHandler, nsISidlEventListener)
NS_IMETHODIMP
TestEventHandler::HandleEvent(nsISupports* aData) {
  nsCOMPtr<nsISettingInfo> settingInfo = do_QueryInterface(aData);
  if (settingInfo) {
    nsString name;
    nsString value;
    settingInfo->GetName(name);
    settingInfo->GetValue(value);
  }
  return NS_OK;
}
```
#### cpp caller
``` c++
nsCOMPtr<nsISettingsManager> settingsManager = do_GetService("@mozilla.org/sidl-native/settings;1");
nsCOMPtr<nsISidlEventListener> eventHandler = new TestEventHandler();
settingsManager->AddEventListener(settingsManager->CHANGE_EVENT, eventHandler);
settingsManager->RemoveEventListener(settingsManager->CHANGE_EVENT, eventHandler);
```
### AddObserver and RemoveObserver Functions
#### header declaration
```c++
class TestAddObserverCallback final : public nsISidlDefaultResponse {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIDLDEFAULTRESPONSE

  protected:
    ~TestAddObserverCallback() = default;
};

class TestRemoveObserverCallback final : public nsISidlDefaultResponse {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIDLDEFAULTRESPONSE

  protected:
    ~TestRemoveObserverCallback() = default;
};

class TestSettingsObserver final: public nsISettingsObserver {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISETTINGSOBSERVER

  protected:
    ~TestSettingsObserver() = default;
};
```
#### cpp definition
```c++
NS_IMPL_ISUPPORTS(TestAddObserverCallback, nsISidlDefaultResponse)
NS_IMETHODIMP
TestAddObserverCallback::Resolve() {
  return NS_OK;
}

NS_IMETHODIMP
TestAddObserverCallback::Reject() {
  return NS_OK;
}

NS_IMPL_ISUPPORTS(TestRemoveObserverCallback, nsISidlDefaultResponse)
NS_IMETHODIMP
TestRemoveObserverCallback::Resolve() {
  return NS_OK;
}

NS_IMETHODIMP
TestRemoveObserverCallback::Reject() {
  return NS_OK;
}

NS_IMPL_ISUPPORTS(TestSettingsObserver, nsISettingsObserver)
NS_IMETHODIMP
TestSettingsObserver::ObserveSetting(nsISettingInfo *aSettingInfo) {
  nsString name;
  nsString value;
  aSettingInfo->GetName(name);
  aSettingInfo->GetValue(value);
  return NS_OK;
}
```
#### cpp caller
```c++
nsCOMPtr<nsISettingsManager> settingsManager = do_GetService("@mozilla.org/sidl-native/settings;1");
nsCOMPtr<nsISettingsObserver> observer = new TestSettingsObserver();

nsCOMPtr<nsISidlDefaultResponse> addObsCallback = new TestAddObserverCallback();
settingsManager->AddObserver(NS_LITERAL_STRING("testing.observed"), observer, addObsCallback);

nsCOMPtr<nsISidlDefaultResponse> removeObsCallback = new TestRemoveObserverCallback();
settingsManager->RemoveObserver(NS_LITERAL_STRING("testing.observed"), observer, removeObsCallback);
```

## Sample Codes of Settings API Callers from Gecko in JS
### Set Function
```js
let settingsManager = Cc["@mozilla.org/sidl-native/settings;1"].getService(Ci.nsISettingsManager);
settingsManager.set(
  [
    { name: "testing.setting1", value: JSON.stringify(42) }
    { name: "setting:json", value: JSON.stringify({"foo": bar, "success": true, "amount": 42}) }
  ],
  {
    "resolve": v => console.log("resolve " + v.name + " " + v.value),
    "reject": v => console.log("reject " + v)
  }
);
```
### Get Function
```js
let settingsManager = Cc["@mozilla.org/sidl-native/settings;1"].getService(Ci.nsISettingsManager);
settingsManager.get(
  "testing.setting1",
  {
    "resolve": v => console.log("resolve " + v.name + " " + v.value),
    "reject": v => {
      console.log("reject " + v.name + " " + v.reason);
      if (v.reason != v.NON_EXISTING_SETTING) {
        // Setting of such name does not exist.
      } else {
        // Something bad actually happened!
      }
    }
  }
);
```
### AddEventListener and RemoveEventListener Functions
```js
let settingsManager = Cc["@mozilla.org/sidl-native/settings;1"].getService(Ci.nsISettingsManager);
let handler = {
  "handleEvent": v => {
    let settingInfo = v.QueryInterface(Ci.nsISettingInfo);
    console.log("handleEvent " + settingInfo.name + " " + settingInfo.value);
  }
};
settingsManager.addEventListener(settingsManager.CHANGE_EVENT, handler);
settingsManager.removeEventListener(settingsManager.CHANGE_EVENT, handler);
```
### AddObserver and RemoveObserver Functions
```js
let settingsManager = Cc["@mozilla.org/sidl-native/settings;1"].getService(Ci.nsISettingsManager);
let observer = {
  "observeSetting": v => {
    console.log("observeSetting " + v.name + " " + v.value);
  }
};
settingsManager.addObserver("testing.setting1", observer);
settingsManager.removeObserver("testing.setting1", observer);
```
## Sample Codes of Settings API Callers from Apps
Please refer to `services/settings/client/test` in `git.kaiostech.com/KaiOS/sidl`
