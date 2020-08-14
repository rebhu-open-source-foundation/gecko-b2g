/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

pub mod client_object;
#[macro_use]
pub mod connection_observer;
pub mod core;
pub mod daemon_manager;
pub mod default_response;
#[macro_use]
pub mod event_manager;
pub mod frame;
#[macro_use]
pub mod sidl_task;
pub mod traits;
pub mod uds_transport;

use bincode::Options;
use nsstring::nsString;
use serde::de::Visitor;
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use std::fmt;
use std::ops::Deref;

pub fn get_bincode() -> impl Options {
    bincode::options().with_big_endian().with_varint_encoding()
}

pub fn deserialize_bincode<'a, T>(input: &'a [u8]) -> Result<T, bincode::Error>
where
    T: Deserialize<'a>,
{
    get_bincode().deserialize(input)
}

// A wrapper around a JsonValue to help with the encoding/decoding of JSON strings.
#[derive(Clone, Debug)]
pub struct JsonValue(serde_json::Value);

impl<'de> Deserialize<'de> for JsonValue {
    fn deserialize<D>(deserializer: D) -> Result<JsonValue, D::Error>
    where
        D: Deserializer<'de>,
    {
        use serde::de::Error;
        use std::str::FromStr;

        struct JsonVisitor;
        impl<'de> Visitor<'de> for JsonVisitor {
            type Value = String;

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "a JSON string")
            }

            fn visit_str<E>(self, s: &str) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                Ok(s.to_owned())
            }
        }

        let json_str = deserializer.deserialize_str(JsonVisitor)?;
        let value = serde_json::Value::from_str(&json_str)
            .map_err(|err| D::Error::custom(format!("{}", err)))?;

        Ok(JsonValue(value))
    }
}

impl Serialize for JsonValue {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(&self.0.to_string())
    }
}

impl Deref for JsonValue {
    type Target = serde_json::Value;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl From<serde_json::Value> for JsonValue {
    fn from(v: serde_json::Value) -> Self {
        JsonValue(v)
    }
}

impl From<JsonValue> for String {
    fn from(v: JsonValue) -> Self {
        v.to_string()
    }
}

impl From<JsonValue> for nsString {
    fn from(v: JsonValue) -> Self {
        let s = &v.to_string();
        s.into()
    }
}
