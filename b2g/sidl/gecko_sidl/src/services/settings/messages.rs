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
    "5d48958d28487986f180bb35534c8345ef72e0d593552c7c9b121e7f6ee3c";

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug)]
pub enum GetErrorReason {
    UnknownError, // #0
    NonExistingSetting, // #1
}
impl Copy for GetErrorReason {}

#[derive(Clone, Deserialize, Serialize, Debug)]
pub struct GetError {
    pub name: String,
    pub reason: GetErrorReason,
}

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
    SettingsFactoryGetError(GetError),       // 5
    SettingsFactoryRemoveObserverSuccess,    // 6
    SettingsFactoryRemoveObserverError,      // 7
    SettingsFactorySetSuccess,               // 8
    SettingsFactorySetError,                 // 9
    SettingsFactoryChangeEvent(SettingInfo), // 10
    SettingObserverCallback(SettingInfo),    // 11
}
