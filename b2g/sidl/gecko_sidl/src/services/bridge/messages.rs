/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This file is generated. Do not edit.
// @generated

use crate::common::traits::TrackerId;
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
    "eda137bd6f65d0e6311395be6bac59f38fc3a4c375d54cfc4ce9b28a87f0d1";

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
    GeckoFeaturesBoolPrefChanged(String, bool),         // 0
    GeckoFeaturesCharPrefChanged(String, String),       // 1
    GeckoFeaturesIntPrefChanged(String, i64),           // 2
    GeckoFeaturesSetCardInfoManagerDelegate(ObjectRef), // 3
    GeckoFeaturesSetNetworkManagerDelegate(ObjectRef),  // 4
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef),    // 5
    CardInfoManagerDelegateGetCardInfoSuccess(String),  // 6
    CardInfoManagerDelegateGetCardInfoError,            // 7
    CardInfoManagerDelegateGetMncMccSuccess(NetworkOperator), // 8
    CardInfoManagerDelegateGetMncMccError,              // 9
    NetworkManagerDelegateGetNetworkInfoSuccess(NetworkInfo), // 10
    NetworkManagerDelegateGetNetworkInfoError,          // 11
    PowerManagerDelegateSetScreenEnabledSuccess,        // 12
    PowerManagerDelegateSetScreenEnabledError,          // 13
}

#[derive(Debug, Deserialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess,                   // 0
    GeckoFeaturesBoolPrefChangedError,                     // 1
    GeckoFeaturesCharPrefChangedSuccess,                   // 2
    GeckoFeaturesCharPrefChangedError,                     // 3
    GeckoFeaturesIntPrefChangedSuccess,                    // 4
    GeckoFeaturesIntPrefChangedError,                      // 5
    GeckoFeaturesSetCardInfoManagerDelegateSuccess,        // 6
    GeckoFeaturesSetCardInfoManagerDelegateError,          // 7
    GeckoFeaturesSetNetworkManagerDelegateSuccess,         // 8
    GeckoFeaturesSetNetworkManagerDelegateError,           // 9
    GeckoFeaturesSetPowerManagerDelegateSuccess,           // 10
    GeckoFeaturesSetPowerManagerDelegateError,             // 11
    CardInfoManagerDelegateGetCardInfo(i64, CardInfoType), // 12
    CardInfoManagerDelegateGetMncMcc(i64, bool),           // 13
    NetworkManagerDelegateGetNetworkInfo,                  // 14
    PowerManagerDelegateSetScreenEnabled(bool),            // 15
}
