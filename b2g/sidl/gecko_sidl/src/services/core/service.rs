/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// The "core service" client.
use crate::common::core::{
    BaseMessage, CoreRequest, CoreResponse, DisableEventListenerRequest,
    EnableEventListenerRequest, GetServiceRequest, GetServiceResponse, HasServiceRequest,
    ReleaseObjectRequest,
};
use crate::common::traits::TrackerId;
use crate::common::uds_transport::*;
use log::{debug, error};
use serde::Serialize;

pub struct CoreService {
    transport: UdsTransport,
}

impl SessionObject for CoreService {
    fn on_request(&mut self, _request: BaseMessage, _id: u64) -> Option<BaseMessage> {
        debug!("CoreService::on_request");
        None
    }

    fn on_event(&mut self, _event: Vec<u8>) {
        debug!("CoreService::on_event");
    }

    fn get_ids(&self) -> (u32, u32) {
        (0, 0)
    }
}

pub trait CoreGetServiceReceiverDelegate: Send {
    fn on_success(&mut self, service_id: TrackerId);
    fn on_error(&mut self);
}

pub struct CoreGetServiceReceiver {
    service_name: Option<String>,
    delegate: Box<dyn CoreGetServiceReceiverDelegate>,
}

impl CoreGetServiceReceiver {
    pub fn new(delegate: Box<dyn CoreGetServiceReceiverDelegate>) -> Self {
        Self {
            delegate,
            service_name: None,
        }
    }
}

impl ResponseReceiver for CoreGetServiceReceiver {
    fn on_message(&mut self, message: BaseMessage) {
        match from_base_message(&message) {
            Ok(CoreResponse::GetService(response)) => {
                let service_name = self.service_name.as_ref().unwrap();
                if let GetServiceResponse::Success(service) = response {
                    debug!("GetService success: id for {} is {}", service_name, service);
                    self.delegate.on_success(service);
                } else {
                    error!("Failed to get service {}: {:?}", service_name, response);
                    self.delegate.on_error();
                }
            }
            other => {
                error!("Unexpected CoreResponse: {:?}", other);
                self.delegate.on_error();
            }
        }
    }
}

pub trait CoreHasServiceReceiverDelegate: Send {
    fn on_success(&mut self, result: bool);
    fn on_error(&mut self);
}

pub struct CoreHasServiceReceiver {
    service_name: Option<String>,
    delegate: Box<dyn CoreHasServiceReceiverDelegate>,
}

impl CoreHasServiceReceiver {
    pub fn new(delegate: Box<dyn CoreHasServiceReceiverDelegate>) -> Self {
        Self {
            delegate,
            service_name: None,
        }
    }
}

impl ResponseReceiver for CoreHasServiceReceiver {
    fn on_message(&mut self, message: BaseMessage) {
        match from_base_message(&message) {
            Ok(CoreResponse::HasService(response)) => {
                let service_name = self.service_name.as_ref().unwrap();
                if response.success {
                    debug!("HasService {} is {}", service_name, response.success);
                    self.delegate.on_success(response.success);
                } else {
                    error!("Failed to check has_service {}", service_name);
                    self.delegate.on_error();
                }
            }
            other => {
                error!("Unexpected CoreResponse: {:?}", other);
                self.delegate.on_error();
            }
        }
    }
}

pub trait CoreSimpleReceiverDelegate: Send {
    fn on_success(&mut self);
    fn on_error(&mut self);
}

pub struct CoreAddEventListenerReceiver {
    params: Option<(u32, u32, u32)>, // (service, object, event)
    delegate: Box<dyn CoreSimpleReceiverDelegate>,
}

impl CoreAddEventListenerReceiver {
    pub fn new(delegate: Box<dyn CoreSimpleReceiverDelegate>) -> Self {
        Self {
            delegate,
            params: None,
        }
    }
}

impl ResponseReceiver for CoreAddEventListenerReceiver {
    fn on_message(&mut self, message: BaseMessage) {
        match from_base_message(&message) {
            Ok(CoreResponse::EnableEvent(response)) => {
                if response.success {
                    debug!(
                        "Event listener added on {:?} is {}",
                        self.params, response.success
                    );
                    self.delegate.on_success();
                } else {
                    error!("Failed to add event listener on {:?}", self.params);
                    self.delegate.on_error();
                }
            }
            other => {
                error!("Unexpected CoreResponse: {:?}", other);
                self.delegate.on_error();
            }
        }
    }
}

pub struct CoreRemoveEventListenerReceiver {
    params: Option<(u32, u32, u32)>, //(service, object, event)
    delegate: Box<dyn CoreSimpleReceiverDelegate>,
}

impl CoreRemoveEventListenerReceiver {
    pub fn new(delegate: Box<dyn CoreSimpleReceiverDelegate>) -> Self {
        Self {
            delegate,
            params: None,
        }
    }
}

impl ResponseReceiver for CoreRemoveEventListenerReceiver {
    fn on_message(&mut self, message: BaseMessage) {
        match from_base_message(&message) {
            Ok(CoreResponse::DisableEvent(response)) => {
                if response.success {
                    debug!(
                        "Event listener removed on {:?} is {}",
                        self.params, response.success
                    );
                    self.delegate.on_success();
                } else {
                    error!("Failed to remove event listener on {:?}", self.params);
                    self.delegate.on_error();
                }
            }
            other => {
                error!("Unexpected CoreResponse: {:?}", other);
                self.delegate.on_error();
            }
        }
    }
}

pub struct CoreReleaseObjectReceiver {
    params: Option<(u32, u32)>, //(service, object)
    delegate: Box<dyn CoreSimpleReceiverDelegate>,
}

impl CoreReleaseObjectReceiver {
    pub fn new(delegate: Box<dyn CoreSimpleReceiverDelegate>) -> Self {
        Self {
            delegate,
            params: None,
        }
    }
}

impl ResponseReceiver for CoreReleaseObjectReceiver {
    fn on_message(&mut self, message: BaseMessage) {
        match from_base_message(&message) {
            Ok(CoreResponse::ReleaseObject(response)) => {
                if response.success {
                    debug!("Object released {:?} is {}", self.params, response.success);
                    self.delegate.on_success();
                } else {
                    error!("Failed to release object {:?}", self.params);
                    self.delegate.on_error();
                }
            }
            other => {
                error!("Unexpected CoreResponse: {:?}", other);
                self.delegate.on_error();
            }
        }
    }
}

impl CoreService {
    pub fn new(transport: &UdsTransport) -> Self {
        Self {
            transport: transport.clone(),
        }
    }

    // Pre-process a CoreResponse.
    fn send_request<S: Serialize>(
        &mut self,
        payload: &S,
        response_task: Box<dyn ResponseReceiver>,
    ) -> UdsResult<()> {
        self.transport.send_request(
            0, /* core service */
            0, /* core object */
            payload,
            response_task,
        )
    }

    // Returns the service id if successful.
    pub fn get_service(
        &mut self,
        service_name: &str,
        service_fingerprint: &str,
        mut response_task: Box<CoreGetServiceReceiver>,
    ) -> UdsResult<()> {
        response_task.service_name = Some(service_name.to_owned());

        // Build a `get_service` call.
        let request = CoreRequest::GetService(GetServiceRequest {
            name: service_name.into(),
            fingerprint: service_fingerprint.into(),
        });

        self.send_request(&request, response_task)
    }

    // Returns whether the service exists.
    pub fn has_service(
        &mut self,
        service_name: &str,
        mut response_task: Box<CoreHasServiceReceiver>,
    ) -> UdsResult<()> {
        response_task.service_name = Some(service_name.to_owned());

        // Build a `get_service` call.
        let request = CoreRequest::HasService(HasServiceRequest {
            name: service_name.into(),
        });

        self.send_request(&request, response_task)
    }

    pub fn enable_event_listeners(
        &mut self,
        service: u32,
        object: u32,
        event: u32,
        mut response_task: Box<CoreAddEventListenerReceiver>,
    ) -> UdsResult<()> {
        response_task.params = Some((service, object, event));
        let request = CoreRequest::EnableEvent(EnableEventListenerRequest {
            service,
            object,
            event,
        });

        self.send_request(&request, response_task)
    }

    pub fn disable_event_listeners(
        &mut self,
        service: u32,
        object: u32,
        event: u32,
        mut response_task: Box<CoreRemoveEventListenerReceiver>,
    ) -> UdsResult<()> {
        response_task.params = Some((service, object, event));
        let request = CoreRequest::DisableEvent(DisableEventListenerRequest {
            service,
            object,
            event,
        });

        self.send_request(&request, response_task)
    }

    pub fn release_object(
        &mut self,
        service: u32,
        object: u32,
        mut response_task: Box<CoreReleaseObjectReceiver>,
    ) -> UdsResult<()> {
        response_task.params = Some((service, object));
        let request = CoreRequest::ReleaseObject(ReleaseObjectRequest { service, object });

        self.send_request(&request, response_task)
    }
}
