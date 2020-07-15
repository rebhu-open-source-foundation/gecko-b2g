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
    "6a1e68bb0df899394ca849e09ad8046ebe4cfd690af334284d84efd8bbb32";

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug)]
pub enum CardInfoType {
    Imsi, // #0
    Imei, // #1
    Msisdn, // #2
}

impl Copy for CardInfoType {}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct NetworkOperator {
    pub mnc: String,
    pub mcc: String,
}

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeFromClient {
    GeckoFeaturesBoolPrefChanged(String,bool,), // 0
    GeckoFeaturesCharPrefChanged(String,String,), // 1
    GeckoFeaturesIntPrefChanged(String,i64,), // 2
    GeckoFeaturesSetCardInfoManagerDelegate(ObjectRef,), // 3
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef,), // 4
    CardInfoManagerDelegateGetCardInfoSuccess(String), // 5
    CardInfoManagerDelegateGetCardInfoError, // 6
    CardInfoManagerDelegateGetMncMccSuccess(NetworkOperator), // 7
    CardInfoManagerDelegateGetMncMccError, // 8
    PowerManagerDelegateSetScreenEnabledSuccess, // 9
    PowerManagerDelegateSetScreenEnabledError, // 10
}

#[derive(Debug, Deserialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess, // 0
    GeckoFeaturesBoolPrefChangedError, // 1
    GeckoFeaturesCharPrefChangedSuccess, // 2
    GeckoFeaturesCharPrefChangedError, // 3
    GeckoFeaturesIntPrefChangedSuccess, // 4
    GeckoFeaturesIntPrefChangedError, // 5
    GeckoFeaturesSetCardInfoManagerDelegateSuccess, // 6
    GeckoFeaturesSetCardInfoManagerDelegateError, // 7
    GeckoFeaturesSetPowerManagerDelegateSuccess, // 8
    GeckoFeaturesSetPowerManagerDelegateError, // 9
    CardInfoManagerDelegateGetCardInfo(i64,CardInfoType,), // 10
    CardInfoManagerDelegateGetMncMcc(i64,bool,), // 11
    PowerManagerDelegateSetScreenEnabled(bool,), // 12
}
