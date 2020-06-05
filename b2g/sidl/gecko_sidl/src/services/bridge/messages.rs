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

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeFromClient {
    GeckoFeaturesBoolPrefChanged(String, bool),      // 0
    GeckoFeaturesCharPrefChanged(String, String),    // 1
    GeckoFeaturesIntPrefChanged(String, i64),        // 2
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef), // 3
    PowerManagerDelegateSetScreenEnabledSuccess,     // 4
    PowerManagerDelegateSetScreenEnabledError,       // 5
}

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeToClient {
    GeckoFeaturesBoolPrefChangedSuccess,         // 0
    GeckoFeaturesBoolPrefChangedError,           // 1
    GeckoFeaturesCharPrefChangedSuccess,         // 2
    GeckoFeaturesCharPrefChangedError,           // 3
    GeckoFeaturesIntPrefChangedSuccess,          // 4
    GeckoFeaturesIntPrefChangedError,            // 5
    GeckoFeaturesSetPowerManagerDelegateSuccess, // 6
    GeckoFeaturesSetPowerManagerDelegateError,   // 7
    PowerManagerDelegateSetScreenEnabled(bool),  // 8
}
