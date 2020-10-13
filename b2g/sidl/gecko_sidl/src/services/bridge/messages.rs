/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is generated. Do not edit.
// @generated

use crate::common::traits::TrackerId;
#[allow(unused_imports)]
use crate::common::{JsonValue, SystemTime};
use serde::{Deserialize, Serialize};

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq, Serialize, Deserialize)]
pub struct ObjectRef(TrackerId);
impl From<TrackerId> for ObjectRef {
    fn from(val: TrackerId) -> Self {
        Self(val)
    }
}
impl From<ObjectRef> for TrackerId {
    fn from(val: ObjectRef) -> Self {
        val.0
    }
}

pub static SERVICE_FINGERPRINT: &str =
    "df567ff0a08482f3501f3597504b87a9144020e77f5be96059cc2531d17ec4e4";

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug)]
pub enum CardInfoType {
    Imei,   // #0
    Imsi,   // #1
    Msisdn, // #2
}
impl Copy for CardInfoType {}

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug)]
pub enum NetworkState {
    NetworkStateUnknown,       // #0
    NetworkStateConnecting,    // #1
    NetworkStateConnected,     // #2
    NetworkStateDisconnecting, // #3
    NetworkStateDisconnected,  // #4
    NetworkStateEnabled,       // #5
    NetworkStateDisabled,      // #6
    NetworkStateSuspended,     // #7
}
impl Copy for NetworkState {}

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug)]
pub enum NetworkType {
    NetworkTypeUnknown,     // #0
    NetworkTypeWifi,        // #1
    NetworkTypeMobile,      // #2
    NetworkTypeMobileMms,   // #3
    NetworkTypeMobileSupl,  // #4
    NetworkTypeWifiP2p,     // #5
    NetworkTypeMobileIms,   // #6
    NetworkTypeMobileDun,   // #7
    NetworkTypeMobileFota,  // #8
    NetworkTypeEthernet,    // #9
    NetworkTypeMobileHipri, // #10
    NetworkTypeMobileCbs,   // #11
    NetworkTypeMobileIa,    // #12
    NetworkTypeMobileEcc,   // #13
    NetworkTypeMobileXcap,  // #14
}
impl Copy for NetworkType {}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct NetworkInfo {
    pub network_state: NetworkState,
    pub network_type: NetworkType,
}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct NetworkOperator {
    pub mnc: String,
    pub mcc: String,
}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct SimContactInfo {
    pub id: String,
    pub tel: String,
    pub email: String,
    pub name: String,
}

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeFromClient {
    GeckoFeaturesBoolPrefChanged(String, bool),   // 0
    GeckoFeaturesCharPrefChanged(String, String), // 1
    GeckoFeaturesImportSimContacts(Option<Vec<SimContactInfo>>), // 2
    GeckoFeaturesIntPrefChanged(String, i64),     // 3
    GeckoFeaturesRegisterToken(String, String, Option<Vec<String>>), // 4
    GeckoFeaturesSetAppsServiceDelegate(ObjectRef), // 5
    GeckoFeaturesSetMobileManagerDelegate(ObjectRef), // 6
    GeckoFeaturesSetNetworkManagerDelegate(ObjectRef), // 7
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef), // 8
    AppsServiceDelegateOnBootSuccess,             // 9
    AppsServiceDelegateOnBootError,               // 10
    AppsServiceDelegateOnBootDoneSuccess,         // 11
    AppsServiceDelegateOnBootDoneError,           // 12
    AppsServiceDelegateOnClearSuccess,            // 13
    AppsServiceDelegateOnClearError,              // 14
    AppsServiceDelegateOnInstallSuccess,          // 15
    AppsServiceDelegateOnInstallError,            // 16
    AppsServiceDelegateOnUninstallSuccess,        // 17
    AppsServiceDelegateOnUninstallError,          // 18
    AppsServiceDelegateOnUpdateSuccess,           // 19
    AppsServiceDelegateOnUpdateError,             // 20
    MobileManagerDelegateGetCardInfoSuccess(String), // 21
    MobileManagerDelegateGetCardInfoError,        // 22
    MobileManagerDelegateGetMncMccSuccess(NetworkOperator), // 23
    MobileManagerDelegateGetMncMccError,          // 24
    NetworkManagerDelegateGetNetworkInfoSuccess(NetworkInfo), // 25
    NetworkManagerDelegateGetNetworkInfoError,    // 26
    PowerManagerDelegateSetScreenEnabledSuccess,  // 27
    PowerManagerDelegateSetScreenEnabledError,    // 28
}

#[derive(Debug, Deserialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess,                 // 0
    GeckoFeaturesBoolPrefChangedError,                   // 1
    GeckoFeaturesCharPrefChangedSuccess,                 // 2
    GeckoFeaturesCharPrefChangedError,                   // 3
    GeckoFeaturesImportSimContactsSuccess,               // 4
    GeckoFeaturesImportSimContactsError,                 // 5
    GeckoFeaturesIntPrefChangedSuccess,                  // 6
    GeckoFeaturesIntPrefChangedError,                    // 7
    GeckoFeaturesRegisterTokenSuccess,                   // 8
    GeckoFeaturesRegisterTokenError,                     // 9
    GeckoFeaturesSetAppsServiceDelegateSuccess,          // 10
    GeckoFeaturesSetAppsServiceDelegateError,            // 11
    GeckoFeaturesSetMobileManagerDelegateSuccess,        // 12
    GeckoFeaturesSetMobileManagerDelegateError,          // 13
    GeckoFeaturesSetNetworkManagerDelegateSuccess,       // 14
    GeckoFeaturesSetNetworkManagerDelegateError,         // 15
    GeckoFeaturesSetPowerManagerDelegateSuccess,         // 16
    GeckoFeaturesSetPowerManagerDelegateError,           // 17
    AppsServiceDelegateOnBoot(String, JsonValue),        // 18
    AppsServiceDelegateOnBootDone,                       // 19
    AppsServiceDelegateOnClear(String, String),          // 20
    AppsServiceDelegateOnInstall(String, JsonValue),     // 21
    AppsServiceDelegateOnUninstall(String),              // 22
    AppsServiceDelegateOnUpdate(String, JsonValue),      // 23
    MobileManagerDelegateGetCardInfo(i64, CardInfoType), // 24
    MobileManagerDelegateGetMncMcc(i64, bool),           // 25
    NetworkManagerDelegateGetNetworkInfo,                // 26
    PowerManagerDelegateSetScreenEnabled(bool, bool),    // 27
}
