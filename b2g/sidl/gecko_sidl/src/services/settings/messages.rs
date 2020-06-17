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
    "fc2921a98dc0e8b2b52c5bb0278b3a74bc6f98e18577b9fba445de59c60a7e9";

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct SettingInfo {
    pub name: String,
    pub value: JsonValue,
}

#[derive(Debug, Deserialize, Serialize)]
pub enum SettingsManagerFromClient {
    SettingsFactoryAddObserver(String, ObjectRef),    // 0
    SettingsFactoryClear,                             // 1
    SettingsFactoryGet(String),                       // 2
    SettingsFactoryRemoveObserver(String, ObjectRef), // 3
    SettingsFactorySet(Vec<SettingInfo>),             // 4
    SettingObserverCallbackSuccess,                   // 5
    SettingObserverCallbackError,                     // 6
}

#[derive(Debug, Deserialize)]
pub enum SettingsManagerToClient {
    SettingsFactoryAddObserverSuccess,       // 0
    SettingsFactoryAddObserverError,         // 1
    SettingsFactoryClearSuccess,             // 2
    SettingsFactoryClearError,               // 3
    SettingsFactoryGetSuccess(SettingInfo),  // 4
    SettingsFactoryGetError,                 // 5
    SettingsFactoryRemoveObserverSuccess,    // 6
    SettingsFactoryRemoveObserverError,      // 7
    SettingsFactorySetSuccess,               // 8
    SettingsFactorySetError,                 // 9
    SettingsFactoryChangeEvent(SettingInfo), // 10
    SettingObserverCallback(SettingInfo),    // 11
}
