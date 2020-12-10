/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Wrapper for the nsISettingsObserver interface.

use super::messages::*;
use super::setting_info::SettingInfoXpcom;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::{from_base_message, SessionObject, XpcomSessionObject};
use bincode::Options;
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use std::any::Any;
use xpcom::interfaces::nsISettingsObserver;

pub struct ObserverWrapper {
    xpcom: ThreadPtrHandle<nsISettingsObserver>,
    service_id: TrackerId,
    object_id: TrackerId,
}

impl ObserverWrapper {
    pub fn new(
        xpcom: ThreadPtrHandle<nsISettingsObserver>,
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

impl SessionObject for ObserverWrapper {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) -> Option<BaseMessage> {
        // Unpack the request.
        if let Ok(SettingsManagerToClient::SettingObserverCallback(setting_info)) =
            from_base_message(&request)
        {
            // Dispatch the setting change to the xpcom observer.
            let task = ObserverTask {
                xpcom: self.xpcom.clone(),
                setting_info,
            };
            let _ = TaskRunnable::new("ObserverWrapper", Box::new(task))
                .and_then(|r| TaskRunnable::dispatch(r, self.xpcom.owning_thread()));
            // Unconditionally Return a success response for now.
            let payload = SettingsManagerFromClient::SettingObserverCallbackSuccess;

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
                "ObserverWrapper::on_request unexpected message: {:?}",
                request.content
            );
            None
        }
    }

    fn on_event(&mut self, _event: Vec<u8>) {}

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, self.object_id)
    }
}

impl XpcomSessionObject for ObserverWrapper {
    fn as_xpcom(&self) -> &dyn Any {
        &self.xpcom
    }
}

// A Task to dispatch settings changes to the observer.
#[derive(Clone)]
struct ObserverTask {
    xpcom: ThreadPtrHandle<nsISettingsObserver>,
    setting_info: SettingInfo,
}

impl Task for ObserverTask {
    fn run(&self) {
        // Call the method on the initial thread.
        debug!("ObserverTask::run");
        if let Some(object) = self.xpcom.get() {
            let setting = SettingInfoXpcom::new(&self.setting_info);
            unsafe {
                object.ObserveSetting(setting.coerce());
            }
        }
    }

    fn done(&self) -> Result<(), nsresult> {
        // We don't return a result to the calling thread, so nothing to do.
        Ok(())
    }
}
