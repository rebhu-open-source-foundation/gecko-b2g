/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is generated. Do not edit.
// @generated

use crate::common::traits::TrackerId;
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
    "1d5dd53a8be53a95bc5e96321050d0aa5864d1ee6bd6d4587327c84630525";

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
    GeckoFeaturesBoolPrefChanged(String, bool),        // 0
    GeckoFeaturesCharPrefChanged(String, String),      // 1
    GeckoFeaturesIntPrefChanged(String, i64),          // 2
    GeckoFeaturesSetAppsServiceDelegate(ObjectRef),    // 3
    GeckoFeaturesSetMobileManagerDelegate(ObjectRef),  // 4
    GeckoFeaturesSetNetworkManagerDelegate(ObjectRef), // 5
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef),   // 6
    AppsServiceDelegateOnBootSuccess,                  // 7
    AppsServiceDelegateOnBootError,                    // 8
    AppsServiceDelegateOnInstallSuccess,               // 9
    AppsServiceDelegateOnInstallError,                 // 10
    AppsServiceDelegateOnUninstallSuccess,             // 11
    AppsServiceDelegateOnUninstallError,               // 12
    AppsServiceDelegateOnUpdateSuccess,                // 13
    AppsServiceDelegateOnUpdateError,                  // 14
    MobileManagerDelegateGetCardInfoSuccess(String),   // 15
    MobileManagerDelegateGetCardInfoError,             // 16
    MobileManagerDelegateGetMncMccSuccess(NetworkOperator), // 17
    MobileManagerDelegateGetMncMccError,               // 18
    NetworkManagerDelegateGetNetworkInfoSuccess(NetworkInfo), // 19
    NetworkManagerDelegateGetNetworkInfoError,         // 20
    PowerManagerDelegateSetScreenEnabledSuccess,       // 21
    PowerManagerDelegateSetScreenEnabledError,         // 22
}

#[derive(Debug, Deserialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess,                 // 0
    GeckoFeaturesBoolPrefChangedError,                   // 1
    GeckoFeaturesCharPrefChangedSuccess,                 // 2
    GeckoFeaturesCharPrefChangedError,                   // 3
    GeckoFeaturesIntPrefChangedSuccess,                  // 4
    GeckoFeaturesIntPrefChangedError,                    // 5
    GeckoFeaturesSetAppsServiceDelegateSuccess,          // 6
    GeckoFeaturesSetAppsServiceDelegateError,            // 7
    GeckoFeaturesSetMobileManagerDelegateSuccess,        // 8
    GeckoFeaturesSetMobileManagerDelegateError,          // 9
    GeckoFeaturesSetNetworkManagerDelegateSuccess,       // 10
    GeckoFeaturesSetNetworkManagerDelegateError,         // 11
    GeckoFeaturesSetPowerManagerDelegateSuccess,         // 12
    GeckoFeaturesSetPowerManagerDelegateError,           // 13
    AppsServiceDelegateOnBoot(String, JsonValue),        // 14
    AppsServiceDelegateOnInstall(String, JsonValue),     // 15
    AppsServiceDelegateOnUninstall(String),              // 16
    AppsServiceDelegateOnUpdate(String, JsonValue),      // 17
    MobileManagerDelegateGetCardInfo(i64, CardInfoType), // 18
    MobileManagerDelegateGetMncMcc(i64, bool),           // 19
    NetworkManagerDelegateGetNetworkInfo,                // 20
    PowerManagerDelegateSetScreenEnabled(bool),          // 21
}
