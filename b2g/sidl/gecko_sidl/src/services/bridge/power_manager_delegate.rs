/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsIPowerManagerDelegate interface.

use super::messages::*;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::UdsTransport;
use crate::common::uds_transport::{from_base_message, SessionObject};
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use xpcom::interfaces::nsIPowerManagerDelegate;

pub struct PowerManagerDelegate {
    xpcom: ThreadPtrHandle<nsIPowerManagerDelegate>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
}

impl PowerManagerDelegate {
    pub fn new(
        xpcom: ThreadPtrHandle<nsIPowerManagerDelegate>,
        service_id: TrackerId,
        object_id: TrackerId,
        transport: &UdsTransport,
    ) -> Self {
        Self {
            xpcom,
            service_id,
            object_id,
            transport: transport.clone(),
        }
    }
}

impl SessionObject for PowerManagerDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) {
        // Unpack the request.
        if let Ok(GeckoBridgeToClient::PowerManagerDelegateSetScreenEnabled(value)) =
            from_base_message(&request)
        {
            // Dispatch the setting change to the xpcom observer.
            let task = PowerManagerDelegateTask {
                xpcom: self.xpcom.clone(),
                command: PowerManagerCommand::SetScreenEnabled(value),
            };
            let _ = TaskRunnable::new("PowerManagerDelegate", Box::new(task))
                .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
            // Unconditionally Return a success response for now.
            let payload = GeckoBridgeFromClient::PowerManagerDelegateSetScreenEnabledSuccess;
            let mut bincode = bincode::config();

            // Wrap the payload in a base message and send it.
            let message = BaseMessage {
                service: self.service_id,
                object: self.object_id,
                kind: BaseMessageKind::Response(request_id),
                content: bincode.big_endian().serialize(&payload).unwrap(),
            };
            let _ = self.transport.send_message(&message);
        } else {
            error!(
                "PowerManagerDelegate::on_request unexpected message: {:?}",
                request.content
            );
        }
    }

    fn on_event(&mut self, _event: Vec<u8>) {}

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, self.object_id)
    }
}

// Commands supported by the power manager delegate.
#[derive(Clone)]
enum PowerManagerCommand {
    SetScreenEnabled(bool),
}

// A Task to dispatch commands to the delegate.
#[derive(Clone)]
struct PowerManagerDelegateTask {
    xpcom: ThreadPtrHandle<nsIPowerManagerDelegate>,
    command: PowerManagerCommand,
}

impl Task for PowerManagerDelegateTask {
    fn run(&self) {
        // Call the method on the initial thread.
        debug!("PowerManagerDelegateTask::run");
        if let Some(object) = self.xpcom.get() {
            match self.command {
                PowerManagerCommand::SetScreenEnabled(value) => unsafe {
                    object.SetScreenEnabled(value);
                },
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
