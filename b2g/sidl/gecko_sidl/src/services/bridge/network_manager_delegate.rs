/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsIPowerManagerDelegate interface.

use super::messages::*;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::UdsTransport;
use crate::common::uds_transport::{from_base_message, SessionObject, XpcomSessionObject};
use bincode::Options;
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::{nsresult, NS_OK};
use std::any::Any;
use xpcom::interfaces::nsINetworkManagerDelegate;

impl From<i32> for NetworkState {
    fn from(value: i32) -> NetworkState {
        match value {
            // NETWORK_STATE_UNKNOWN is -1 defined in nsINetworkInterface.idl.
            -1 => NetworkState::NetworkStateUnknown,
            0 => NetworkState::NetworkStateConnecting,
            1 => NetworkState::NetworkStateConnected,
            2 => NetworkState::NetworkStateDisconnecting,
            3 => NetworkState::NetworkStateDisconnected,
            4 => NetworkState::NetworkStateEnabled,
            5 => NetworkState::NetworkStateDisabled,
            6 => NetworkState::NetworkStateSuspended,
            _ => NetworkState::NetworkStateUnknown,
        }
    }
}

impl From<i32> for NetworkType {
    fn from(value: i32) -> NetworkType {
        match value {
            // NETWORK_TYPE_UNKNOWN is -1 defined in nsINetworkInterface.idl.
            -1 => NetworkType::NetworkTypeUnknown,
            0 => NetworkType::NetworkTypeWifi,
            1 => NetworkType::NetworkTypeMobile,
            2 => NetworkType::NetworkTypeMobileMms,
            3 => NetworkType::NetworkTypeMobileSupl,
            4 => NetworkType::NetworkTypeWifiP2p,
            5 => NetworkType::NetworkTypeMobileIms,
            6 => NetworkType::NetworkTypeMobileDun,
            7 => NetworkType::NetworkTypeMobileFota,
            8 => NetworkType::NetworkTypeEthernet,
            9 => NetworkType::NetworkTypeMobileHipri,
            10 => NetworkType::NetworkTypeMobileCbs,
            11 => NetworkType::NetworkTypeMobileIa,
            12 => NetworkType::NetworkTypeMobileEcc,
            13 => NetworkType::NetworkTypeMobileXcap,
            _ => NetworkType::NetworkTypeUnknown,
        }
    }
}

pub struct NetworkManagerDelegate {
    xpcom: ThreadPtrHandle<nsINetworkManagerDelegate>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
}

impl NetworkManagerDelegate {
    pub fn new(
        xpcom: ThreadPtrHandle<nsINetworkManagerDelegate>,
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

    fn post_task(&mut self, command: NetworkManagerCommand, request_id: u64) {
        let task = NetworkManagerDelegateTask {
            xpcom: self.xpcom.clone(),
            command,
            transport: self.transport.clone(),
            service_id: self.service_id,
            object_id: self.object_id,
            request_id,
        };
        let _ = TaskRunnable::new("NetworkManagerDelegate", Box::new(task))
            .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
    }
}

impl SessionObject for NetworkManagerDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) -> Option<BaseMessage> {
        // Unpack the request.
        if let Ok(GeckoBridgeToClient::NetworkManagerDelegateGetNetworkInfo) =
            from_base_message(&request)
        {
            self.post_task(NetworkManagerCommand::GetNetworkInfo(), request_id);
        } else {
            error!(
                "NetworkManagerDelegate::on_request unexpected message: {:?}",
                request.content
            );
        }
        None
    }

    fn on_event(&mut self, _event: Vec<u8>) {}

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, self.object_id)
    }
}

impl XpcomSessionObject for NetworkManagerDelegate {
    fn as_xpcom(&self) -> &dyn Any {
        &self.xpcom
    }
}

// Commands supported by the power manager delegate.
#[derive(Clone)]
enum NetworkManagerCommand {
    GetNetworkInfo(),
}

// A Task to dispatch commands to the delegate.
#[derive(Clone)]
struct NetworkManagerDelegateTask {
    xpcom: ThreadPtrHandle<nsINetworkManagerDelegate>,
    command: NetworkManagerCommand,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
    request_id: u64,
}

impl NetworkManagerDelegateTask {
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

impl Task for NetworkManagerDelegateTask {
    fn run(&self) {
        // Call the method on the initial thread.
        debug!("NetworkManagerDelegateTask::run");
        if let Some(object) = self.xpcom.get() {
            match self.command {
                NetworkManagerCommand::GetNetworkInfo() => {
                    let network_state = Box::new(0);
                    let network_type = Box::new(0);
                    let p_state = Box::into_raw(network_state);
                    let p_type = Box::into_raw(network_type);

                    let status;
                    unsafe { status = object.GetNetworkInfo(p_state, p_type) };

                    let payload = match status {
                        NS_OK => {
                            let network_state = unsafe { Box::from_raw(p_state) };
                            let network_type = unsafe { Box::from_raw(p_type) };

                            GeckoBridgeFromClient::NetworkManagerDelegateGetNetworkInfoSuccess(
                                NetworkInfo {
                                    network_state: NetworkState::from(*network_state),
                                    network_type: NetworkType::from(*network_type),
                                },
                            )
                        }
                        _ => GeckoBridgeFromClient::NetworkManagerDelegateGetNetworkInfoError,
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
