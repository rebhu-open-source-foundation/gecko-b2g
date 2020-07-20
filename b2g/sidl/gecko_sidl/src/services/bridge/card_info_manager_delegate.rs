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
use nserror::{nsresult, NS_OK};
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

    fn post_task(&mut self, command: CardInfoManagerCommand, request_id: u64) {
        let task = CardInfoManagerDelegateTask {
            xpcom: self.xpcom.clone(),
            command,
            transport: self.transport.clone(),
            service_id: self.service_id,
            object_id: self.object_id,
            request_id,
        };
        let _ = TaskRunnable::new("CardInfoManagerDelegate", Box::new(task))
            .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
    }
}

impl SessionObject for CardInfoManagerDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) {
        // Unpack the request.
        match from_base_message(&request) {
            Ok(GeckoBridgeToClient::CardInfoManagerDelegateGetCardInfo(id, info_type)) => {
                self.post_task(
                    CardInfoManagerCommand::GetCardInfo(id, info_type),
                    request_id,
                );
            }
            Ok(GeckoBridgeToClient::CardInfoManagerDelegateGetMncMcc(id, is_sim)) => {
                self.post_task(CardInfoManagerCommand::GetMncMcc(id, is_sim), request_id);
            }
            _ => error!(
                "CardInfoManagerDelegate::on_request unexpected message: {:?}",
                request.content
            ),
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
    GetCardInfo(i64, CardInfoType), // Get imsi/imei/msisdn by service id.
    GetMncMcc(i64, bool),           // Get mnc mcc from simcard or network by service id.
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

impl CardInfoManagerDelegateTask {
    fn reply(&self, payload: GeckoBridgeFromClient) {
        let message = BaseMessage {
            service: self.service_id,
            object: self.object_id,
            kind: BaseMessageKind::Response(self.request_id),
            content: crate::common::get_bincode().serialize(&payload).unwrap(),
        };
        let mut t = self.transport.clone();
        let _ = t.send_message(&message);
    }
}

impl Task for CardInfoManagerDelegateTask {
    fn run(&self) {
        // Call the method on the initial thread.
        debug!("CardInfoManagerDelegateTask::run");
        if let Some(object) = self.xpcom.get() {
            match self.command {
                CardInfoManagerCommand::GetCardInfo(id, info_type) => {
                    let mut card_info = nsString::new();
                    let status;
                    unsafe {
                        status = object.GetCardInfo(id as i32, info_type as i32, &mut *card_info)
                    };

                    let payload = match status {
                        NS_OK => GeckoBridgeFromClient::CardInfoManagerDelegateGetCardInfoSuccess(
                            card_info.to_string(),
                        ),
                        _ => GeckoBridgeFromClient::CardInfoManagerDelegateGetCardInfoError,
                    };
                    self.reply(payload);
                }
                CardInfoManagerCommand::GetMncMcc(id, is_sim) => {
                    let mut _mnc = nsString::new();
                    let mut _mcc = nsString::new();
                    let status;
                    unsafe { status = object.GetMncMcc(id as i32, is_sim, &mut *_mnc, &mut *_mcc) };

                    let payload = match status {
                        NS_OK => GeckoBridgeFromClient::CardInfoManagerDelegateGetMncMccSuccess(
                            NetworkOperator {
                                mnc: _mnc.to_string(),
                                mcc: _mcc.to_string(),
                            },
                        ),
                        _ => GeckoBridgeFromClient::CardInfoManagerDelegateGetMncMccError,
                    };
                    self.reply(payload);
                }
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
