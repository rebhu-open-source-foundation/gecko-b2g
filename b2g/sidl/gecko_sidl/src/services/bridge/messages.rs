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
    "2acb3816f658c1a2cb18c2cab713b253de46d993db2bd5d53c1191a8eac831f5";

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
    AppsServiceDelegateOnInstallSuccess,          // 13
    AppsServiceDelegateOnInstallError,            // 14
    AppsServiceDelegateOnUninstallSuccess,        // 15
    AppsServiceDelegateOnUninstallError,          // 16
    AppsServiceDelegateOnUpdateSuccess,           // 17
    AppsServiceDelegateOnUpdateError,             // 18
    MobileManagerDelegateGetCardInfoSuccess(String), // 19
    MobileManagerDelegateGetCardInfoError,        // 20
    MobileManagerDelegateGetMncMccSuccess(NetworkOperator), // 21
    MobileManagerDelegateGetMncMccError,          // 22
    NetworkManagerDelegateGetNetworkInfoSuccess(NetworkInfo), // 23
    NetworkManagerDelegateGetNetworkInfoError,    // 24
    PowerManagerDelegateSetScreenEnabledSuccess,  // 25
    PowerManagerDelegateSetScreenEnabledError,    // 26
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
    AppsServiceDelegateOnInstall(String, JsonValue),     // 20
    AppsServiceDelegateOnUninstall(String),              // 21
    AppsServiceDelegateOnUpdate(String, JsonValue),      // 22
    MobileManagerDelegateGetCardInfo(i64, CardInfoType), // 23
    MobileManagerDelegateGetMncMcc(i64, bool),           // 24
    NetworkManagerDelegateGetNetworkInfo,                // 25
    PowerManagerDelegateSetScreenEnabled(bool, bool),    // 26
}
