/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

use parking_lot::{Mutex, MutexGuard};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::hash::Hash;
use std::sync::Arc;

pub type DispatcherId = u32;

pub trait EventIds {
    fn ids(&self) -> (u32, u32, u32);
}

// A composite key to identify an event target.
#[derive(Debug, Eq, PartialEq, Hash)]
pub struct EventMapKey {
    service: u32,
    object: u32,
    event: u32,
}

impl EventMapKey {
    pub fn new(service: u32, object: u32, event: u32) -> Self {
        Self {
            service,
            object,
            event,
        }
    }

    pub fn from_ids<T: EventIds>(source: &T) -> Self {
        let (service, object, event) = source.ids();
        Self {
            service, object, event,
        }
    }
}

pub type SharedEventMap = Shared<HashMap<EventMapKey, bool>>;

#[derive(Debug)]
pub struct IdFactory {
    current: u64,
}

impl IdFactory {
    // Creates a new factory with a base index. The base is used
    // to distinguish id sources and prevent collisions.
    pub fn new(base: u32) -> Self {
        IdFactory {
            current: u64::from(base),
        }
    }

    pub fn next(&mut self) -> u64 {
        // The server side generates even request ids.
        self.current += 2;
        self.current
    }
}

pub type SharedIdFactory = Shared<IdFactory>;

pub type TrackerId = u32; // Simple tracker type.

pub trait SimpleObjectTracker {
    fn id(&self) -> TrackerId {
        0
    }
}

pub trait ObjectTrackerKey: Copy {
    fn first() -> Self;
    fn next(&self) -> Self;
}

impl ObjectTrackerKey for TrackerId {
    fn first() -> Self {
        1 // Starting at 1 because 0 is reserved for "no object" in the protocol.
    }

    fn next(&self) -> Self {
        self + 1
    }
}

// Tracks objects while also keeping track of the session.
#[derive(Hash, PartialEq, Clone, Copy, Debug, Deserialize, Serialize)]
pub struct SessionTrackerId {
    session: u32,
    id: u32,
}

impl ObjectTrackerKey for SessionTrackerId {
    fn first() -> Self {
        SessionTrackerId { session: 1, id: 1 }
    }

    fn next(&self) -> Self {
        SessionTrackerId {
            session: self.session,
            id: self.id + 1,
        }
    }
}

impl Eq for SessionTrackerId {}

impl SessionTrackerId {
    pub fn from(session: u32, id: u32) -> Self {
        SessionTrackerId { session, id }
    }

    pub fn service(self) -> u32 {
        self.id
    }

    pub fn session(self) -> u32 {
        self.session
    }

    pub fn set_session(&mut self, session: u32) {
        self.session = session;
    }
}

pub trait ObjectTrackerMethods<T, K: Eq + Hash + ObjectTrackerKey> {
    fn next_id(&self) -> K;
    fn track(&mut self, obj: T) -> K;
    fn untrack(&mut self, id: K) -> bool;
    fn get(&self, id: K) -> Option<&T>;
    fn get_mut(&mut self, id: K) -> Option<&mut T>;
    fn clear(&mut self);
    fn track_with(&mut self, obj: T, key: K);
}

// Utility to simplify Arc<Mutex<T>> patterns.
pub struct Shared<T> {
    inner: Arc<Mutex<T>>,
}

impl<T> Shared<T> {
    pub fn lock(&self) -> MutexGuard<T> {
        self.inner.lock()
    }

    pub fn adopt(what: T) -> Self {
        Shared {
            inner: Arc::new(Mutex::new(what)),
        }
    }
}

impl<T> Clone for Shared<T> {
    fn clone(&self) -> Self {
        Shared {
            inner: Arc::clone(&self.inner),
        }
    }
}

impl<T> Default for Shared<T>
where
    T: Default,
{
    fn default() -> Self {
        Shared {
            inner: Arc::new(Mutex::new(T::default())),
        }
    }
}
