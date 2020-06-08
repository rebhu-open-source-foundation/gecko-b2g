// Wrapper for the nsISettingsObserver interface.

use super::messages::*;
use super::setting_info::SettingInfoXpcom;
use crate::common::core::{BaseMessage, BaseMessageKind};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::UdsTransport;
use crate::common::uds_transport::{from_base_message, SessionObject};
use log::{debug, error};
use moz_task::{Task, TaskRunnable, ThreadPtrHandle};
use nserror::nsresult;
use xpcom::interfaces::nsISettingsObserver;

pub struct ObserverWrapper {
    xpcom: ThreadPtrHandle<nsISettingsObserver>,
    service_id: TrackerId,
    object_id: TrackerId,
    transport: UdsTransport,
}

impl ObserverWrapper {
    pub fn new(
        xpcom: ThreadPtrHandle<nsISettingsObserver>,
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

impl SessionObject for ObserverWrapper {
    fn on_request(&mut self, request: BaseMessage, request_id: u64) {
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
                "ObserverWrapper::on_request unexpected message: {:?}",
                request.content
            );
        }
    }

    fn on_event(&mut self, _event: Vec<u8>) {}

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, self.object_id)
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
