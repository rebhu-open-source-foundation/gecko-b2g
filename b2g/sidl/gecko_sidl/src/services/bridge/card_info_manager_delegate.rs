/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsICardInfoManagerDelegate interface.

use super::messages::*;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::UdsTransport;
use crate::common::uds_transport::{from_base_message, SessionObject};
use bincode::Options;
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use nsstring::*;
use xpcom::interfaces::nsICardInfoManagerDelegate;

pub struct CardInfoManagerDelegate {
    xpcom: ThreadPtrHandle<nsICardInfoManagerDelegate>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
}

impl CardInfoManagerDelegate {
    pub fn new(
        xpcom: ThreadPtrHandle<nsICardInfoManagerDelegate>,
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

impl SessionObject for CardInfoManagerDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) {
        // Unpack the request.
        if let Ok(GeckoBridgeToClient::CardInfoManagerDelegateGetCardInfo(info_type, id)) =
            from_base_message(&request)
        {
            // Dispatch the setting change to the xpcom observer.
            let task = CardInfoManagerDelegateTask {
                xpcom: self.xpcom.clone(),
                command: CardInfoManagerCommand::GetCardInfo(info_type, id),
                transport: self.transport.clone(),
                service_id: self.service_id,
                object_id: self.object_id,
                request_id: request_id,
            };
            let _ = TaskRunnable::new("CardInfoManagerDelegate", Box::new(task))
                .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
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

// Commands supported by the card info manager delegate.
#[derive(Clone)]
enum CardInfoManagerCommand {
    GetCardInfo(CardInfoType,i64),
}

// A Task to dispatch commands to the delegate.
#[derive(Clone)]
struct CardInfoManagerDelegateTask {
    xpcom: ThreadPtrHandle<nsICardInfoManagerDelegate>,
    command: CardInfoManagerCommand,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
    request_id: u64,
}

impl Task for CardInfoManagerDelegateTask {
    fn run(&self) {
        // Call the method on the initial thread.
        debug!("CardInfoManagerDelegateTask::run");
        if let Some(object) = self.xpcom.get() {
            match self.command {
                CardInfoManagerCommand::GetCardInfo(info_type, id) => {
                    let mut result = nsString::new();
                    unsafe { object.GetCardInfo(info_type as i32, id as i32, &mut *result) };

                    let payload = GeckoBridgeFromClient::CardInfoManagerDelegateGetCardInfoSuccess(result.to_string());
                    // Wrap the payload in a base message and send it.
                    let message = BaseMessage {
                        service: self.service_id,
                        object: self.object_id,
                        kind: BaseMessageKind::Response(self.request_id),
                        content: crate::common::get_bincode().serialize(&payload).unwrap(),
                    };
                    let mut t = self.transport.clone();
                    let _ = t.send_message(&message);
                },
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
