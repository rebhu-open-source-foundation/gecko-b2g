/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsIPowerManagerDelegate interface.

use super::messages::*;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::{from_base_message, SessionObject};
use bincode::Options;
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use std::any::Any;
use xpcom::interfaces::nsIPowerManagerDelegate;

pub struct PowerManagerDelegate {
    xpcom: ThreadPtrHandle<nsIPowerManagerDelegate>,
    service_id: TrackerId,
    object_id: TrackerId,
}

impl PowerManagerDelegate {
    pub fn new(
        xpcom: ThreadPtrHandle<nsIPowerManagerDelegate>,
        service_id: TrackerId,
        object_id: TrackerId,
    ) -> Self {
        Self {
            xpcom,
            service_id,
            object_id,
        }
    }
}

impl SessionObject for PowerManagerDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) -> Option<BaseMessage> {
        debug!("PowerManagerDelegate on_request {}", request_id);
        // Unpack the request.
        if let Ok(GeckoBridgeToClient::PowerManagerDelegateSetScreenEnabled(
            value,
            is_external_screen,
        )) = from_base_message(&request)
        {
            debug!("PowerManagerDelegate set_screen_enabled {}", value);
            // Dispatch the setting change to the xpcom observer.
            let task = PowerManagerDelegateTask {
                xpcom: self.xpcom.clone(),
                command: PowerManagerCommand::SetScreenEnabled(value, is_external_screen),
            };
            let _ = TaskRunnable::new("PowerManagerDelegate", Box::new(task))
                .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));

            // Unconditionally Return a success response for now.
            let payload = GeckoBridgeFromClient::PowerManagerDelegateSetScreenEnabledSuccess;

            // Wrap the payload in a base message and send it.
            let message = BaseMessage {
                service: self.service_id,
                object: self.object_id,
                kind: BaseMessageKind::Response(request_id),
                content: crate::common::get_bincode().serialize(&payload).unwrap(),
            };
            Some(message)
        } else {
            error!(
                "PowerManagerDelegate::on_request unexpected message: {:?}",
                request.content
            );
            None
        }
    }

    fn on_event(&mut self, _event: Vec<u8>) {}

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, self.object_id)
    }

    fn maybe_xpcom(&self) -> Option<&dyn Any> {
        Some(&self.xpcom)
    }
}

// Commands supported by the power manager delegate.
#[derive(Clone)]
enum PowerManagerCommand {
    SetScreenEnabled(bool, bool),
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
                PowerManagerCommand::SetScreenEnabled(value, is_external_screen) => unsafe {
                    object.SetScreenEnabled(value, is_external_screen);
                },
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
