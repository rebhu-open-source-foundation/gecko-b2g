// Implementation of the nsISettings xpcom interface

use crate::common::client_object::*;
use crate::common::core::BaseMessage;
use crate::common::event_manager::*;
use crate::common::sidl_task::*;
use crate::common::traits::{Shared, TrackerId};
use crate::common::uds_transport::*;
use crate::services::core::service::*;
use crate::services::settings::messages::*;
use crate::services::settings::observer::*;
use log::{debug, error};
use moz_task::{TaskRunnable, ThreadPtrHandle, ThreadPtrHolder};
use nserror::{nsresult, NS_ERROR_INVALID_ARG, NS_OK};
use nsstring::*;
use parking_lot::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use thin_vec::ThinVec;
use xpcom::{
    interfaces::{
        nsISettingInfo, nsISettingsGetResponse, nsISettingsManager, nsISettingsObserver,
        nsISidlDefaultResponse, nsISidlEventListener,
    },
    RefPtr,
};
use super::setting_info::*;
use crate::as_isettingsinfo;

type GetSuccessType = SettingInfo;

sidl_callback_for!(nsISidlDefaultResponse);
sidl_callback_for!(nsISettingsGetResponse, GetSuccessType, as_isettingsinfo);

// Task types
type ClearTask = SidlCallTask<(), (), nsISidlDefaultResponse>;
type SetTask = (
    SidlCallTask<(), (), nsISidlDefaultResponse>,
    Vec<SettingInfo>, /* settings */
);
type GetTask = (
    SidlCallTask<GetSuccessType, (), nsISettingsGetResponse>,
    String, /* name */
);
type AddObserverTask = (
    SidlCallTask<(), (), nsISidlDefaultResponse>,
    (String, ThreadPtrHandle<nsISettingsObserver>, usize), /* (name, observer, key) */
);
type RemoveObserverTask = (
    SidlCallTask<(), (), nsISidlDefaultResponse>,
    (String, usize), /* (name, key) */
);

// The tasks that can be dispatched to the calling thread.
enum SettingsTask {
    Clear(ClearTask),
    Set(SetTask),
    Get(GetTask),
    AddObserver(AddObserverTask),
    RemoveObserver(RemoveObserverTask),
}

struct SettingsManagerImpl {
    // The underlying UDS transport we are connected to.
    transport: UdsTransport,
    // The service ID of this instance.
    service_id: TrackerId,
    // The core service we use to manage event listeners.
    core_service: Arc<Mutex<CoreService>>,
    // Helper struct to send tasks.
    sender: TaskSender,
    // Events management.
    event_manager: Shared<EventManager>,
    // The registered observers. (object_id, key, object, name)
    observers: Vec<(TrackerId, usize, ClientObject, String)>,
    // The next usable object_id when we create an observer.
    current_object_id: TrackerId,
}

impl SessionObject for SettingsManagerImpl {
    fn on_request(&mut self, _request: BaseMessage, _id: u64) {
        debug!("SettingsManagerImpl::on_request");
    }

    fn on_event(&mut self, event_data: Vec<u8>) {
        debug!("SettingsManagerImpl::on_event");
        // TODO: Extract that to some helper.
        let mut bincode = bincode::config();
        match bincode
            .big_endian()
            .deserialize::<SettingsManagerToClient>(&event_data)
        {
            Ok(SettingsManagerToClient::SettingsFactoryChangeEvent(event_info)) => {
                let event_manager = self.event_manager.lock();
                let listeners = event_manager
                    .handlers
                    .get(&(nsISettingsManager::CHANGE_EVENT as _));
                let count = if let Some(listeners) = listeners {
                    listeners.len()
                } else {
                    0
                };
                debug!(
                    "Received event {:?}, will dispatch to {} listeners.",
                    event_info, count
                );
                if count == 0 {
                    return;
                }

                for listener in listeners.unwrap() {
                    // We are not on the calling thread, so dispatch a task.
                    let mut task = SidlEventTask::new(listener.0.clone());
                    task.set_value(event_info.clone());
                    let _ = TaskRunnable::new("SidlEvent", Box::new(task.clone()))
                        .and_then(|r| TaskRunnable::dispatch(r, task.thread()));
                }
            }
            Ok(other) => error!("Unexpected setting event: {:?}", other),
            Err(err) => error!("Failed to deserialize settings event: {}", err),
        }
    }

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, 0)
    }
}

impl ServiceClientImpl<SettingsTask> for SettingsManagerImpl {
    fn new(mut transport: UdsTransport, service_id: TrackerId) -> Self {
        let core_service = Arc::new(Mutex::new(CoreService::new(&transport)));
        transport.register_session_object(core_service.clone());
        let sender = TaskSender::new(
            transport.clone(),
            service_id,
            0, /* service object */
        );

        let event_manager = Shared::adopt(EventManager::new(
            core_service.clone(),
            service_id,
            0, /* object id */
        ));

        Self {
            transport,
            service_id,
            core_service,
            sender,
            event_manager,
            observers: vec![],
            // current_object_id starts at 1 since 0 is reserved for the service object itself.
            current_object_id: 1,
        }
    }

    fn dispatch_queue(
        &mut self,
        queue: &Shared<Vec<SettingsTask>>,
        pending_listeners: &PendingListeners,
    ) {
        let mut queue = queue.lock();
        debug!("Running the {} pending tasks", queue.len());

        // Manage event listeners.
        {
            let mut event_manager = self.event_manager.lock();
            for (event, handler, key) in pending_listeners.lock().drain(..) {
                event_manager.add_event_listener(event, handler, key);
            }
        }

        // drain the queue.
        for task in queue.drain(..) {
            match task {
                SettingsTask::Clear(task) => {
                    let _ = self.clear(task);
                }
                SettingsTask::Set(task) => {
                    let _ = self.set(task);
                }
                SettingsTask::Get(task) => {
                    let _ = self.get(task);
                }
                SettingsTask::AddObserver(task) => {
                    let _ = self.add_observer(task);
                }
                SettingsTask::RemoveObserver(task) => {
                    let _ = self.remove_observer(task);
                }
            }
        }
    }
}

impl SettingsManagerImpl {
    fn clear(&mut self, task: ClearTask) -> Result<(), nsresult> {
        debug!("SettingsManager::clear");

        let request = SettingsManagerFromClient::SettingsFactoryClear;
        self.sender.send_task(&request, ClearTaskReceiver { task });
        Ok(())
    }

    fn set(&mut self, task: SetTask) -> Result<(), nsresult> {
        debug!("SettingsManager::set");
        let (task, settings) = task;
        let request = SettingsManagerFromClient::SettingsFactorySet(settings);
        self.sender.send_task(&request, SetTaskReceiver { task });
        Ok(())
    }

    fn get(&mut self, task: GetTask) -> Result<(), nsresult> {
        debug!("SettingsManager::get");

        let (task, name) = task;
        let request = SettingsManagerFromClient::SettingsFactoryGet(name);
        self.sender.send_task(&request, GetTaskReceiver { task });
        Ok(())
    }

    fn add_observer(&mut self, task: AddObserverTask) -> Result<(), nsresult> {
        debug!("SettingsManager::add_observer");
        let object_id = self.next_object_id();

        let (task, (name, observer, key)) = task;
        // Create a lightweight xpcom wrapper + session proxy that manages object release for us.
        let wrapper = ObserverWrapper::new(observer, self.service_id, object_id, &self.transport);
        let proxy = ClientObject::new(wrapper, &mut self.transport);

        self.observers.push((object_id, key, proxy, name.clone()));

        let request = SettingsManagerFromClient::SettingsFactoryAddObserver(name, object_id.into());
        self.sender
            .send_task(&request, AddObserverTaskReceiver { task });

        Ok(())
    }

    fn remove_observer(&mut self, task: RemoveObserverTask) -> Result<(), nsresult> {
        debug!("SettingsManager::remove_observer");

        let (task, (name, key)) = task;
        let object_id = self.get_observer_id(key);

        if let Some(pos) = self
            .observers
            .iter()
            .position(|item| item.1 == key && item.3 == name)
        {
            self.observers.remove(pos);
        }

        if let Some(object_id) = object_id {
            let request =
                SettingsManagerFromClient::SettingsFactoryRemoveObserver(name, object_id.into());
            self.sender
                .send_task(&request, RemoveObserverTaskReceiver { task });
        }
        Ok(())
    }

    // Returns the object id for a given observer.
    fn get_observer_id(&self, key: usize) -> Option<TrackerId> {
        debug!("SettingsManager::get_observer_id for {}", key);
        self.observers
            .iter()
            .find(|item| item.1 == key)
            .map(|item| item.0)
    }

    fn next_object_id(&mut self) -> TrackerId {
        let res = self.current_object_id;
        self.current_object_id += 1;
        res
    }

    impl_event_listener!();
}

impl Drop for SettingsManagerImpl {
    fn drop(&mut self) {
        self.transport
            .unregister_session_object(self.core_service.clone());
    }
}

task_receiver!(
    ClearTaskReceiver,
    nsISidlDefaultResponse,
    SettingsManagerToClient,
    SettingsFactoryClearSuccess,
    SettingsFactoryClearError
);

task_receiver!(
    SetTaskReceiver,
    nsISidlDefaultResponse,
    SettingsManagerToClient,
    SettingsFactorySetSuccess,
    SettingsFactorySetError
);

task_receiver_success!(
    GetTaskReceiver,
    nsISettingsGetResponse,
    SettingsManagerToClient,
    SettingsFactoryGetSuccess,
    SettingsFactoryGetError,
    GetSuccessType
);

task_receiver!(
    AddObserverTaskReceiver,
    nsISidlDefaultResponse,
    SettingsManagerToClient,
    SettingsFactoryAddObserverSuccess,
    SettingsFactoryAddObserverError
);

task_receiver!(
    RemoveObserverTaskReceiver,
    nsISidlDefaultResponse,
    SettingsManagerToClient,
    SettingsFactoryRemoveObserverSuccess,
    SettingsFactoryRemoveObserverError
);

#[derive(xpcom)]
#[xpimplements(nsISettingsManager)]
#[refcnt = "atomic"]
struct InitSettingsManagerXpcom {
    // The underlying UDS transport we are connected to.
    transport: UdsTransport,
    // The core service we use to get an instance of the SettingsManager service.
    core_service: Arc<Mutex<CoreService>>,
    // The implementation registered as a session object with the transport.
    inner: Shared<Option<Arc<Mutex<SettingsManagerImpl>>>>,
    // The list of pending tasks queued while we are waiting on the service id
    // to be available.
    pending_tasks: Shared<Vec<SettingsTask>>,
    // The list of pending event listeners to add.
    pending_listeners: PendingListeners,
    // Flag to know if are already fetching the service id.
    getting_service: AtomicBool,
}

impl SettingsManagerXpcom {
    fn new() -> Option<RefPtr<Self>> {
        debug!("SettingsManagerXpcom::new");
        if let Some(transport) = UdsTransport::open() {
            let core_service = Arc::new(Mutex::new(CoreService::new(&transport)));

            Some(Self::allocate(InitSettingsManagerXpcom {
                transport,
                core_service,
                pending_tasks: Shared::adopt(vec![]),
                pending_listeners: Shared::adopt(vec![]),
                inner: Shared::adopt(None),
                getting_service: AtomicBool::new(false),
            }))
        } else {
            error!("Failed to connect to api-daemon socket");
            None
        }
    }

    xpcom_method!(clear => Clear(callback: *const nsISidlDefaultResponse));
    fn clear(&self, callback: &nsISidlDefaultResponse) -> Result<(), nsresult> {
        debug!("SettingsManagerXpcom::clear");

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = SidlCallTask::new(callback);

        if !self.ensure_service() {
            self.queue_task(SettingsTask::Clear(task));
            return Ok(());
        }

        // The service is ready, send the request right away.
        debug!("SettingsManager::clear direct call");
        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().clear(task);
        } else {
            error!("Unable to get SettingsManagerImpl");
        }
        Ok(())
    }

    xpcom_method!(set => Set(settings: *const ThinVec<RefPtr<nsISettingInfo>>, callback: *const nsISidlDefaultResponse));
    fn set(
        &self,
        settings: &ThinVec<RefPtr<nsISettingInfo>>,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("SettingsManagerXpcom::set");

        // Validate parameters.
        let settings_info: Vec<SettingInfo> = settings
            .iter()
            .filter_map(|item| {
                let mut name = nsString::new();
                let mut value = nsString::new();
                unsafe {
                    if item.GetName(&mut *name) != NS_OK {
                        return None;
                    }
                    if item.GetValue(&mut *value) != NS_OK {
                        return None;
                    };
                }

                let json: serde_json::Value = match serde_json::from_str(&value.to_string()) {
                    Ok(json) => json,
                    Err(err) => {
                        error!("Invalid json `{}` : {}", value, err);
                        return None;
                    }
                };
                Some(SettingInfo {
                    name: name.to_string(),
                    value: json.into(),
                })
            })
            .collect();

        // If we failed to convert at least one element of the array, bail out.
        if settings_info.len() != settings.len() {
            return Err(NS_ERROR_INVALID_ARG);
        }

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), settings_info);

        if !self.ensure_service() {
            self.queue_task(SettingsTask::Set(task));
            return Ok(());
        }

        // The service is ready, send the request right away.
        debug!("SettingsManager::set direct call");
        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().set(task);
        } else {
            error!("Unable to get SettingsManagerImpl");
        }
        Ok(())
    }

    xpcom_method!(get => Get(name: *const nsAString, callback: *const nsISettingsGetResponse));
    fn get(&self, name: &nsAString, callback: &nsISettingsGetResponse) -> Result<(), nsresult> {
        debug!("SettingsManagerXpcom::get");

        let callback =
            ThreadPtrHolder::new(cstr!("nsISettingsGetResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), name.to_string());

        if !self.ensure_service() {
            self.queue_task(SettingsTask::Get(task));
            return Ok(());
        }

        // The service is ready, send the request right away.
        debug!("SettingsManager::get direct call");
        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().get(task);
        } else {
            error!("Unable to get SettingsManagerImpl");
        }

        Ok(())
    }

    xpcom_method!(add_observer => AddObserver(name: *const nsAString, observer: *const nsISettingsObserver, callback: *const nsISidlDefaultResponse));
    fn add_observer(
        &self,
        name: &nsAString,
        observer: &nsISettingsObserver,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("SettingsManagerXpcom::add_observer {}", name);

        let key: usize = unsafe { std::mem::transmute(observer) };
        let observer =
            ThreadPtrHolder::new(cstr!("nsISettingsObserver"), RefPtr::new(observer)).unwrap();

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (
            SidlCallTask::new(callback),
            (name.to_string(), observer, key),
        );

        if !self.ensure_service() {
            self.queue_task(SettingsTask::AddObserver(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().add_observer(task);
        } else {
            error!("Unable to get SettingsManagerImpl");
        }

        Ok(())
    }

    xpcom_method!(remove_observer => RemoveObserver(name: *const nsAString, observer: *const nsISettingsObserver, callback: *const nsISidlDefaultResponse));
    fn remove_observer(
        &self,
        name: &nsAString,
        observer: &nsISettingsObserver,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("SettingsManagerXpcom::remove_observer {}", name);

        let key: usize = unsafe { std::mem::transmute(observer) };

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), (name.to_string(), key));

        if !self.ensure_service() {
            self.queue_task(SettingsTask::RemoveObserver(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().remove_observer(task);
        } else {
            error!("Unable to get SettingsManagerImpl");
        }

        Ok(())
    }

    ensure_service_and_queue!(SettingsTask, "SettingsManager", SERVICE_FINGERPRINT);
    xpcom_sidl_event_target!();
}

impl Drop for SettingsManagerXpcom {
    fn drop(&mut self) {
        if let Some(inner) = self.inner.lock().as_ref() {
            self.transport.unregister_session_object(inner.clone());
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn settings_manager_construct(result: &mut *const nsISettingsManager) {
    if let Some(inst) = SettingsManagerXpcom::new() {
        *result = inst.coerce::<nsISettingsManager>();
        std::mem::forget(inst);
    } else {
        *result = std::ptr::null();
    }
}
