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

pub static SETTINGS_CHANGE_EVENT: u32 = 0;

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
