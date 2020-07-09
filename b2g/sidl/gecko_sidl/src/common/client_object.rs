/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Manages an object that has a client side lifetime.
// This ensures we properly release the object when it is dropped.

use crate::common::traits::TrackerId;
use crate::common::uds_transport::{SessionObject, UdsTransport};
use crate::services::core::service::*;
use log::debug;
use moz_task::ThreadPtrHandle;
use parking_lot::Mutex;
use std::sync::Arc;
use xpcom::XpCom;

pub struct ClientObject {
    inner: Arc<Mutex<dyn SessionObject>>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
    should_release: bool,
}

impl ClientObject {
    pub fn new<T>(source: T, transport: &mut UdsTransport) -> Self
    where
        T: SessionObject + 'static,
    {
        let (service_id, object_id) = source.get_ids();
        let inner = Arc::new(Mutex::new(source));
        transport.register_session_object(inner.clone());
        Self {
            inner,
            service_id,
            object_id,
            transport: transport.clone(),
            should_release: true,
        }
    }

    pub fn object_ref(&self) -> TrackerId {
        self.object_id
    }

    pub fn maybe_xpcom<T: XpCom + 'static>(&self) -> Option<ThreadPtrHandle<T>> {
        if let Some(xpcom) = self.inner.lock().maybe_xpcom() {
            if let Some(obj) = xpcom.downcast_ref::<ThreadPtrHandle<T>>() {
                Some(obj.clone())
            } else {
                None
            }
        } else {
            None
        }
    }

    pub fn dont_release(&mut self) {
        self.should_release = false;
    }
}

impl Drop for ClientObject {
    // Release the object on the remote side.
    fn drop(&mut self) {
        self.transport.unregister_session_object(self.inner.clone());

        if self.should_release {
            struct DummyDelegate {};
            impl CoreSimpleReceiverDelegate for DummyDelegate {
                fn on_success(&mut self) {
                    debug!("Success releasing object");
                }
                fn on_error(&mut self) {
                    debug!("Error releasing object");
                }
            }

            let mut core = CoreService::new(&self.transport);

            let responder = Box::new(CoreReleaseObjectReceiver::new(Box::new(DummyDelegate {})));
            let _ = core.release_object(self.service_id, self.object_id, responder);
        }
    }
}
