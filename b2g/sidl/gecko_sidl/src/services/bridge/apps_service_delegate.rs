/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsIAppsServiceDelegate interface.

use super::messages::*;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::UdsTransport;
use crate::common::uds_transport::{from_base_message, SessionObject};
use bincode::Options;
use log::{debug, error, info};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use nsstring::*;
use xpcom::interfaces::nsIAppsServiceDelegate;

pub struct AppsServiceDelegate {
    xpcom: ThreadPtrHandle<nsIAppsServiceDelegate>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
}

impl AppsServiceDelegate {
    pub fn new(
        xpcom: ThreadPtrHandle<nsIAppsServiceDelegate>,
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

    fn post_task(&mut self, command: AppsServiceCommand, request_id: u64) {

        // Dispatch the setting change to the xpcom observer.
        let task = AppsServiceDelegateTask {
            xpcom: self.xpcom.clone(),
            command: command,
        };
        let _ = TaskRunnable::new("AppsServiceDelegate", Box::new(task))
            .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
        // Unconditionally Return a success response for now.
        let payload = GeckoBridgeFromClient::AppsServiceDelegateOnBootSuccess;

        // Wrap the payload in a base message and send it.
        let message = BaseMessage {
            service: self.service_id,
            object: self.object_id,
            kind: BaseMessageKind::Response(request_id),
            content: crate::common::get_bincode().serialize(&payload).unwrap(),
        };
        let _ = self.transport.send_message(&message);
    }
}

impl SessionObject for AppsServiceDelegate {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) {
        info!("AppsServiceDelegate on_request id: {}", request_id);
        // Unpack the request.
        match from_base_message(&request) {
            Ok(GeckoBridgeToClient::AppsServiceDelegateOnBoot(manifest_url, value)) => {
                let value_string = match serde_json::to_string(&value) {
                    Ok(value_string) => value_string,
                    Err(err) => {
                        error!("AppsServiceDelegateOnBoot to string error: {:?}", err);
                        return;
                    }
                };
                debug!("AppsServiceDelegate on_request OnBoot value string: {}", &value_string);
                self.post_task(AppsServiceCommand::OnBoot(manifest_url, value_string),
                              request_id);
            },
            Ok(GeckoBridgeToClient::AppsServiceDelegateOnInstall(manifest_url, value)) => {
                let value_string = match serde_json::to_string(&value) {
                    Ok(value_string) => value_string,
                    Err(err) => {
                        error!("AppsServiceDelegateOnBoot to string error: {:?}", err);
                        return;
                    }
                };
                debug!("AppsServiceDelegate on_request OnBoot value string: {}", &value_string);
                self.post_task(AppsServiceCommand::OnInstall(manifest_url, value_string),
                              request_id);
            },
            Ok(GeckoBridgeToClient::AppsServiceDelegateOnUpdate(manifest_url, value)) => {
                let value_string = match serde_json::to_string(&value) {
                    Ok(value_string) => value_string,
                    Err(err) => {
                        error!("AppsServiceDelegateOnBoot to string error: {:?}", err);
                        return;
                    }
                };
                debug!("AppsServiceDelegate on_request OnBoot value string: {}", &value_string);
                self.post_task(AppsServiceCommand::OnUpdate(manifest_url, value_string),
                              request_id);
            },
            Ok(GeckoBridgeToClient::AppsServiceDelegateOnUninstall(manifest_url)) => {
                self.post_task(AppsServiceCommand::OnUninstall(manifest_url),
                              request_id);
            },
            _ => {
                error!(
                    "AppsServiceDelegate::on_request unexpected message: {:?}",
                    request.content
                );
            }
        }
    }

    fn on_event(&mut self, _event: Vec<u8>) {
        info!("AppsServiceDelegate on_request event");
    }

    fn get_ids(&self) -> (u32, u32) {
        info!("AppsServiceDelegate get_ids, {}, {}", self.service_id, self.object_id);
        (self.service_id, self.object_id)
    }
}

// Commands supported by the power manager delegate.
#[derive(Clone)]
enum AppsServiceCommand {
    OnBoot(
        String, // manifest_url
        String, // b2g_features
    ),
    OnInstall(
        String, // manifest_url
        String, // b2g_features
    ),
    OnUpdate(
        String, // manifest_url
        String, // b2g_features
    ),
    OnUninstall(
        String, // manifest_url
    ),
}

// A Task to dispatch commands to the delegate.
#[derive(Clone)]
struct AppsServiceDelegateTask {
    xpcom: ThreadPtrHandle<nsIAppsServiceDelegate>,
    command: AppsServiceCommand,
}

impl Task for AppsServiceDelegateTask {
    fn run(&self) {
        // Call the method on the initial thread.
        info!("AppsServiceDelegateTask::run");
        if let Some(object) = self.xpcom.get() {
            match &self.command {
                AppsServiceCommand::OnBoot(manifest_url, value) => {
                    let manifest_url = nsString::from(manifest_url);
                    let value = nsString::from(value);
                    info!("AppsServiceDelegateTask OnBoot manifest_url: {}",  manifest_url);
                    info!("AppsServiceDelegateTask OnBoot value: {:?}",  value);
                    unsafe {
                        object.OnBoot(&*manifest_url as &nsAString, &*value as &nsAString);
                    }
                },
                AppsServiceCommand::OnInstall(manifest_url, value) => {
                    let manifest_url = nsString::from(manifest_url);
                    let value = nsString::from(value);
                    unsafe {
                        object.OnInstall(&*manifest_url as &nsAString, &*value as &nsAString);
                    }
                },
                AppsServiceCommand::OnUpdate(manifest_url, value) => {
                    let manifest_url = nsString::from(manifest_url);
                    let value = nsString::from(value);
                    unsafe {
                        object.OnUpdate(&*manifest_url as &nsAString, &*value as &nsAString);
                    }
                },
                AppsServiceCommand::OnUninstall(manifest_url) => {
                    let url = nsString::from(manifest_url);
                    unsafe {
                        object.OnUninstall(&*url as &nsAString);
                    }
                },
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        info!("AppsServiceDelegateTask::Done");
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
