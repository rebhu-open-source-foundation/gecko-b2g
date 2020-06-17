/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// A client side UDS transport.
use crate::adopt_current_thread;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::frame::{Error as FrameError, Frame};
use crate::common::traits::Shared;
use log::{debug, error};
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::net::Shutdown;
use std::os::unix::net::UnixStream;
use std::result::Result as StdResult;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::Arc;
use std::thread;
use thiserror::Error as ThisError;

#[derive(ThisError, Debug)]
pub enum UdsError {
    #[error("Framing error")]
    Frame(#[from] FrameError),
    #[error("Generic Error: {0}")]
    Generic(String),
}

impl From<&str> for UdsError {
    fn from(val: &str) -> Self {
        Self::Generic(val.into())
    }
}

impl From<String> for UdsError {
    fn from(val: String) -> Self {
        Self::Generic(val)
    }
}

pub type UdsResult<T> = StdResult<T, UdsError>;

// A response dispatcher.
pub trait ResponseReceiver: Send {
    fn on_message(&mut self, message: BaseMessage);
}

// Trait that needs to be implemented by session objects, that can receive
// events and raw requests/responses.
// These methods may be called on a different thread than the caller one.
pub trait SessionObject: Send {
    // Requests are messages initiated by the service to the client, used
    // when calling methods on callbacks.
    fn on_request(&mut self, request: BaseMessage, request_id: u64);
    // Events are messages for all events on this object.
    fn on_event(&mut self, event: Vec<u8>);
    // Returns the (service, object) tuple.
    fn get_ids(&self) -> (u32, u32);
}

pub type SharedSessionObject = Arc<Mutex<dyn SessionObject>>;

#[derive(Default)]
struct SessionData {
    // The map from request IDs to messages for the client.
    pending_responses: HashMap<u64, Box<dyn ResponseReceiver>>,
    // The current request id used when sending requests.
    request_id: u64,
    // Session objects. The key is (service, object)
    session_objects: HashMap<(u32, u32), SharedSessionObject>,
    // Is that transport really usable?
    zombie: bool,
}

pub struct UdsTransport {
    // The connection to the Unix Domain Socket.
    stream: UnixStream,
    // The book keeping data for this session.
    session_data: Shared<SessionData>,
    // Custom addref/release to know when to close the underlying socket.
    ref_count: Arc<AtomicU32>,
}

impl Clone for UdsTransport {
    fn clone(&self) -> Self {
        self.ref_count.fetch_add(1, Ordering::SeqCst);
        Self {
            stream: self.stream.try_clone().unwrap(),
            session_data: self.session_data.clone(),
            ref_count: Arc::clone(&self.ref_count),
        }
    }
}

impl UdsTransport {
    pub fn open() -> Option<Self> {
        #[cfg(target_os = "android")]
        let path = "/dev/socket/api-daemon";
        #[cfg(not(target_os = "android"))]
        let path = "/tmp/api-daemon-socket";

        match UnixStream::connect(path) {
            Ok(stream) => {
                // TODO: fail gracefully.
                let mut recv_stream = stream.try_clone().expect("Failed to clone UDS stream");

                let transport = Self {
                    stream,
                    session_data: Shared::adopt(SessionData::default()),
                    ref_count: Arc::new(AtomicU32::new(0)),
                };

                let mut transport_inner = transport.clone();

                // Start the reading thread.
                thread::spawn(move || {
                    // Needed to make this thread known from Gecko, since we can dispatch Tasks
                    // from there.
                    let _thread = adopt_current_thread();

                    loop {
                        // TODO: check if we need a BufReader for performance reasons.
                        match Frame::deserialize_from(&mut recv_stream) {
                            Ok(data) => {
                                transport_inner.dispatch(data);
                            }
                            Err(err) => {
                                // That can happen if the server side is closed under us, eg. if
                                // the daemon restarts.
                                // In that case we need to mark this transport as "zombie" and try
                                // to reconnect it.
                                let mut data = transport_inner.session_data.lock();
                                error!(
                                    "Failed to read frame (zombie: {}): {}, closing uds session.",
                                    data.zombie, err
                                );
                                data.zombie = true;
                                break;
                            }
                        }
                    }
                });

                Some(transport)
            }
            Err(err) => {
                error!("Failed to connect to UDS socket {} : {}", path, err);
                None
            }
        }
    }

    pub fn dispatch(&mut self, message: BaseMessage) {
        match message.kind {
            BaseMessageKind::Response(id) => {
                let maybe_receiver = {
                    let mut session_data = self.session_data.lock();
                    if let Some(receiver) = session_data.pending_responses.remove(&id) {
                        Some(receiver)
                    } else {
                        error!("No receiver object available for request #{}", id);
                        None
                    }
                };

                if let Some(mut receiver) = maybe_receiver {
                    receiver.on_message(message);
                }
            }
            BaseMessageKind::Event => {
                // Send the message to the correct handler.
                debug!(
                    "Received event for service #{} object #{}",
                    message.service, message.object,
                );
                let session_data = self.session_data.lock();
                match session_data
                    .session_objects
                    .get(&(message.service, message.object))
                {
                    Some(handler) => handler.lock().on_event(message.content),
                    None => error!(
                        "No event handler for service #{} object #{}",
                        message.service, message.object
                    ),
                }
            }
            BaseMessageKind::Request(id) => {
                // Look for the session object that should receive this request.
                let key = (message.service, message.object);
                let session_data = self.session_data.lock();
                if let Some(target) = session_data.session_objects.get(&key) {
                    log::debug!("Found session object!");
                    target.lock().on_request(message, id);
                } else {
                    error!(
                        "No session object found for service #{} object #{}",
                        message.service, message.object
                    );
                }
            }
        }
    }

    // Returns a base message matching this request payload.
    pub fn send_request<S: Serialize>(
        &mut self,
        service: u32,
        object: u32,
        payload: &S,
        response_task: Box<dyn ResponseReceiver>,
    ) -> UdsResult<()> {
        let request_id = {
            let mut session_data = self.session_data.lock();
            session_data.request_id += 1;
            session_data.request_id
        };

        let mut bincode = bincode::config();

        // Wrap the payload in a base message and send it.
        let message = BaseMessage {
            service,
            object,
            kind: BaseMessageKind::Request(request_id),
            content: bincode.big_endian().serialize(payload).unwrap(),
        };

        // Register the sender in the requests HashMap.
        let mut session_data = self.session_data.lock();
        session_data
            .pending_responses
            .insert(request_id, response_task);

        // Send the message.
        Frame::serialize_to(&message, &mut self.stream).map_err(|e| e.into())
    }

    // Sends a BaseMessage fully build.
    pub fn send_message(&mut self, message: &BaseMessage) -> UdsResult<()> {
        Frame::serialize_to(&message, &mut self.stream).map_err(|e| e.into())
    }

    fn close(&mut self) {
        let mut session_data = self.session_data.lock();
        session_data.zombie = true;
        session_data.pending_responses.clear();
        session_data.session_objects.clear();

        let _ = self.stream.shutdown(Shutdown::Both);
    }

    pub fn register_session_object(&mut self, object: SharedSessionObject) {
        let ids = object.lock().get_ids();
        self.session_data
            .lock()
            .session_objects
            .insert(ids, object.clone());
    }

    pub fn unregister_session_object(&mut self, object: SharedSessionObject) {
        let ids = object.lock().get_ids();
        self.session_data.lock().session_objects.remove(&ids);
    }
}

impl Drop for UdsTransport {
    fn drop(&mut self) {
        self.ref_count.fetch_sub(1, Ordering::SeqCst);
        if self.ref_count.load(Ordering::SeqCst) == 0 {
            self.close();
        }
    }
}

pub fn from_base_message<'a, T: Deserialize<'a>>(message: &'a BaseMessage) -> UdsResult<T> {
    let mut bincode = bincode::config();
    bincode
        .big_endian()
        .deserialize(&message.content)
        .map_err(|err| UdsError::Generic(format!("Failed to deserialize base message: {}", err)))
}
