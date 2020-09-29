/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is generated. Do not edit.
// @generated

use crate::common::traits::TrackerId;
#[allow(unused_imports)]
use crate::common::JsonValue;
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
    "1c20d952702efd5720abaaa75fa9915dc6e71f17c9d059262dc530cd8ed8ee";

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

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeFromClient {
    GeckoFeaturesBoolPrefChanged(String, bool),   // 0
    GeckoFeaturesCharPrefChanged(String, String), // 1
    GeckoFeaturesIntPrefChanged(String, i64),     // 2
    GeckoFeaturesRegisterToken(String, String, Option<Vec<String>>), // 3
    GeckoFeaturesSetAppsServiceDelegate(ObjectRef), // 4
    GeckoFeaturesSetMobileManagerDelegate(ObjectRef), // 5
    GeckoFeaturesSetNetworkManagerDelegate(ObjectRef), // 6
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef), // 7
    AppsServiceDelegateOnBootSuccess,             // 8
    AppsServiceDelegateOnBootError,               // 9
    AppsServiceDelegateOnInstallSuccess,          // 10
    AppsServiceDelegateOnInstallError,            // 11
    AppsServiceDelegateOnUninstallSuccess,        // 12
    AppsServiceDelegateOnUninstallError,          // 13
    AppsServiceDelegateOnUpdateSuccess,           // 14
    AppsServiceDelegateOnUpdateError,             // 15
    MobileManagerDelegateGetCardInfoSuccess(String), // 16
    MobileManagerDelegateGetCardInfoError,        // 17
    MobileManagerDelegateGetMncMccSuccess(NetworkOperator), // 18
    MobileManagerDelegateGetMncMccError,          // 19
    NetworkManagerDelegateGetNetworkInfoSuccess(NetworkInfo), // 20
    NetworkManagerDelegateGetNetworkInfoError,    // 21
    PowerManagerDelegateSetScreenEnabledSuccess,  // 22
    PowerManagerDelegateSetScreenEnabledError,    // 23
}

#[derive(Debug, Deserialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess,                 // 0
    GeckoFeaturesBoolPrefChangedError,                   // 1
    GeckoFeaturesCharPrefChangedSuccess,                 // 2
    GeckoFeaturesCharPrefChangedError,                   // 3
    GeckoFeaturesIntPrefChangedSuccess,                  // 4
    GeckoFeaturesIntPrefChangedError,                    // 5
    GeckoFeaturesRegisterTokenSuccess,                   // 6
    GeckoFeaturesRegisterTokenError,                     // 7
    GeckoFeaturesSetAppsServiceDelegateSuccess,          // 8
    GeckoFeaturesSetAppsServiceDelegateError,            // 9
    GeckoFeaturesSetMobileManagerDelegateSuccess,        // 10
    GeckoFeaturesSetMobileManagerDelegateError,          // 11
    GeckoFeaturesSetNetworkManagerDelegateSuccess,       // 12
    GeckoFeaturesSetNetworkManagerDelegateError,         // 13
    GeckoFeaturesSetPowerManagerDelegateSuccess,         // 14
    GeckoFeaturesSetPowerManagerDelegateError,           // 15
    AppsServiceDelegateOnBoot(String, JsonValue),        // 16
    AppsServiceDelegateOnInstall(String, JsonValue),     // 17
    AppsServiceDelegateOnUninstall(String),              // 18
    AppsServiceDelegateOnUpdate(String, JsonValue),      // 19
    MobileManagerDelegateGetCardInfo(i64, CardInfoType), // 20
    MobileManagerDelegateGetMncMcc(i64, bool),           // 21
    NetworkManagerDelegateGetNetworkInfo,                // 22
    PowerManagerDelegateSetScreenEnabled(bool, bool),    // 23
}
