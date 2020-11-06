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
    "53595ea015bc6fb157243e2b4468d079359529e5c27ce07ae834942cfeabd7b5";

#[derive(Clone, PartialEq, Deserialize, Serialize, Debug, Eq, Hash)]
pub enum CallbackReason {
    None,            // #0
    TimeChanged,     // #1
    TimezoneChanged, // #2
}
impl Copy for CallbackReason {}

#[derive(Debug, Deserialize, Serialize)]
pub enum TimeServiceFromClient {
    TimeAddObserver(CallbackReason, ObjectRef),    // 0
    TimeGet,                                       // 1
    TimeGetElapsedRealTime,                        // 2
    TimeRemoveObserver(CallbackReason, ObjectRef), // 3
    TimeSet(SystemTime),                           // 4
    TimeSetTimezone(String),                       // 5
    TimeObserverCallbackSuccess,                   // 6
    TimeObserverCallbackError,                     // 7
}

#[derive(Debug, Deserialize)]
pub enum TimeServiceToClient {
    TimeAddObserverSuccess,               // 0
    TimeAddObserverError,                 // 1
    TimeGetSuccess(SystemTime),           // 2
    TimeGetError,                         // 3
    TimeGetElapsedRealTimeSuccess(i64),   // 4
    TimeGetElapsedRealTimeError,          // 5
    TimeRemoveObserverSuccess,            // 6
    TimeRemoveObserverError,              // 7
    TimeSetSuccess,                       // 8
    TimeSetError,                         // 9
    TimeSetTimezoneSuccess,               // 10
    TimeSetTimezoneError,                 // 11
    TimeTimeChangedEvent,                 // 12
    TimeTimezoneChangedEvent,             // 13
    TimeObserverCallback(CallbackReason), // 14
}
