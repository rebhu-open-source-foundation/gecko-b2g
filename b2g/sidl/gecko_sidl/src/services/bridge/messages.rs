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
    "3dbad0dfaf9a25ece80ed85289a4e971115113fe3ba8d5c7abb29d9988bc5cf";

#[derive(Debug, Deserialize, Serialize)]
pub enum GeckoBridgeFromClient {
    GeckoFeaturesBoolPrefChanged(String, bool),      // 0
    GeckoFeaturesCharPrefChanged(String, String),    // 1
    GeckoFeaturesIntPrefChanged(String, i64),        // 2
    GeckoFeaturesSetPowerManagerDelegate(ObjectRef), // 3
    PowerManagerDelegateSetScreenEnabledSuccess,     // 4
    PowerManagerDelegateSetScreenEnabledError,       // 5
}

#[derive(Debug, Deserialize)]
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
