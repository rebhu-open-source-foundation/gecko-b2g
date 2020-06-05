// Client for the Gecko Bridge service.

use super::messages::*;
use super::power_manager_delegate::*;
use crate::common::client_object::*;
use crate::common::core::BaseMessage;
use crate::common::sidl_task::*;
use crate::common::traits::{Shared, TrackerId};
use crate::common::uds_transport::*;
use crate::services::core::service::*;
use log::{debug, error};
use moz_task::{TaskRunnable, ThreadPtrHandle, ThreadPtrHolder};
use nserror::{nsresult, NS_OK};
use nsstring::*;
use parking_lot::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use xpcom::{
    interfaces::{nsIGeckoBridge, nsIPowerManagerDelegate, nsISidlDefaultResponse},
    RefPtr,
};

type SetPowerManagerDelegateTask = (
    SidlCallTask<(), (), nsISidlDefaultResponse>,
    ThreadPtrHandle<nsIPowerManagerDelegate>,
);

type OnCharPrefChangedTask = (
    SidlCallTask<(), (), nsISidlDefaultResponse>,
    (String, String),
);

type OnIntPrefChangedTask = (SidlCallTask<(), (), nsISidlDefaultResponse>, (String, i32));

type OnBoolPrefChangedTask = (SidlCallTask<(), (), nsISidlDefaultResponse>, (String, bool));

// The tasks that can be dispatched to the calling thread.
enum GeckoBridgeTask {
    SetPowerManagerDelegate(SetPowerManagerDelegateTask),
    CharPrefChanged(OnCharPrefChangedTask),
    IntPrefChanged(OnIntPrefChangedTask),
    BoolPrefChanged(OnBoolPrefChangedTask),
}

struct GeckoBridgeImpl {
    // The underlying UDS transport we are connected to.
    transport: UdsTransport,
    // The service ID of this instance.
    service_id: TrackerId,
    // Helper struct to send tasks.
    sender: TaskSender,
    // The next usable object_id when we create delegates.
    current_object_id: TrackerId,
    // The power manager delegate.
    power_manager_delegate: Option<ClientObject>,
}

impl SessionObject for GeckoBridgeImpl {
    fn on_request(&mut self, _request: BaseMessage, _id: u64) {
        debug!("GeckoBridgeImpl::on_request");
    }

    fn on_event(&mut self, _event_data: Vec<u8>) {
        debug!("GeckoBridgeImpl::on_event");
    }

    fn get_ids(&self) -> (u32, u32) {
        (self.service_id, 0)
    }
}

impl ServiceClientImpl<GeckoBridgeTask> for GeckoBridgeImpl {
    fn new(transport: UdsTransport, service_id: TrackerId) -> Self {
        let sender = TaskSender::new(transport.clone(), service_id, 0 /* service object */);

        Self {
            transport,
            service_id,
            sender,
            // current_object_id starts at 1 since 0 is reserved for the service object itself.
            current_object_id: 1,
            power_manager_delegate: None,
        }
    }

    fn dispatch_queue(
        &mut self,
        queue: &Shared<Vec<GeckoBridgeTask>>,
        _pending_listeners: &PendingListeners,
    ) {
        let mut queue = queue.lock();
        debug!("Running the {} pending tasks", queue.len());

        // drain the queue.
        for task in queue.drain(..) {
            match task {
                GeckoBridgeTask::SetPowerManagerDelegate(task) => {
                    let _ = self.set_power_manager_delegate(task);
                }
                GeckoBridgeTask::CharPrefChanged(task) => {
                    let _ = self.char_pref_changed(task);
                }
                GeckoBridgeTask::IntPrefChanged(task) => {
                    let _ = self.int_pref_changed(task);
                }
                GeckoBridgeTask::BoolPrefChanged(task) => {
                    let _ = self.bool_pref_changed(task);
                }
            }
        }
    }
}

impl GeckoBridgeImpl {
    fn set_power_manager_delegate(
        &mut self,
        task: SetPowerManagerDelegateTask,
    ) -> Result<(), nsresult> {
        debug!("GeckoBridge::set_power_manager_delegate");
        let object_id = self.next_object_id();

        let (task, delegate) = task;

        // Create a lightweight xpcom wrapper + session proxy that manages object release for us.
        let wrapper =
            PowerManagerDelegate::new(delegate, self.service_id, object_id, &self.transport);
        self.power_manager_delegate = Some(ClientObject::new(wrapper, &mut self.transport));

        let request = GeckoBridgeFromClient::GeckoFeaturesSetPowerManagerDelegate(object_id.into());
        self.sender
            .send_task(&request, SetPowerManagerDelegateTaskReceiver { task });

        Ok(())
    }

    fn char_pref_changed(&mut self, task: OnCharPrefChangedTask) -> Result<(), nsresult> {
        debug!("GeckoBridge::char_pref_changed");

        let (task, (name, value)) = task;
        let request = GeckoBridgeFromClient::GeckoFeaturesCharPrefChanged(name, value);
        self.sender
            .send_task(&request, OnCharPrefChangedTaskReceiver { task });
        Ok(())
    }

    fn int_pref_changed(&mut self, task: OnIntPrefChangedTask) -> Result<(), nsresult> {
        debug!("GeckoBridge::int_pref_changed");

        let (task, (name, value)) = task;
        let request = GeckoBridgeFromClient::GeckoFeaturesIntPrefChanged(name, value.into());
        self.sender
            .send_task(&request, OnIntPrefChangedTaskReceiver { task });
        Ok(())
    }

    fn bool_pref_changed(&mut self, task: OnBoolPrefChangedTask) -> Result<(), nsresult> {
        debug!("GeckoBridge::bool_pref_changed");

        let (task, (name, value)) = task;
        let request = GeckoBridgeFromClient::GeckoFeaturesBoolPrefChanged(name, value);
        self.sender
            .send_task(&request, OnBoolPrefChangedTaskReceiver { task });
        Ok(())
    }

    fn next_object_id(&mut self) -> TrackerId {
        let res = self.current_object_id;
        self.current_object_id += 1;
        res
    }
}

task_receiver!(
    SetPowerManagerDelegateTaskReceiver,
    nsISidlDefaultResponse,
    GeckoBridgeToClient,
    GeckoFeaturesSetPowerManagerDelegateSuccess,
    GeckoFeaturesSetPowerManagerDelegateError
);

task_receiver!(
    OnCharPrefChangedTaskReceiver,
    nsISidlDefaultResponse,
    GeckoBridgeToClient,
    GeckoFeaturesCharPrefChangedSuccess,
    GeckoFeaturesCharPrefChangedError
);

task_receiver!(
    OnIntPrefChangedTaskReceiver,
    nsISidlDefaultResponse,
    GeckoBridgeToClient,
    GeckoFeaturesIntPrefChangedSuccess,
    GeckoFeaturesIntPrefChangedError
);

task_receiver!(
    OnBoolPrefChangedTaskReceiver,
    nsISidlDefaultResponse,
    GeckoBridgeToClient,
    GeckoFeaturesBoolPrefChangedSuccess,
    GeckoFeaturesBoolPrefChangedError
);

#[derive(xpcom)]
#[xpimplements(nsIGeckoBridge)]
#[refcnt = "atomic"]
struct InitGeckoBridgeXpcom {
    // The underlying UDS transport we are connected to.
    transport: UdsTransport,
    // The core service we use to get an instance of the GeckoBridge service.
    core_service: Arc<Mutex<CoreService>>,
    // The implementation registered as a session object with the transport.
    inner: Shared<Option<Arc<Mutex<GeckoBridgeImpl>>>>,
    // The list of pending tasks queued while we are waiting on the service id
    // to be available.
    pending_tasks: Shared<Vec<GeckoBridgeTask>>,
    // The list of pending event listeners to add.
    pending_listeners: PendingListeners,
    // Flag to know if are already fetching the service id.
    getting_service: AtomicBool,
}

impl GeckoBridgeXpcom {
    fn new() -> Option<RefPtr<Self>> {
        debug!("GeckoBridgeXpcom::new");
        if let Some(transport) = UdsTransport::open() {
            let core_service = Arc::new(Mutex::new(CoreService::new(&transport)));

            Some(Self::allocate(InitGeckoBridgeXpcom {
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

    xpcom_method!(set_power_manager_delegate => SetPowerManagerDelegate(delegate: *const nsIPowerManagerDelegate, callback: *const nsISidlDefaultResponse));
    fn set_power_manager_delegate(
        &self,
        delegate: &nsIPowerManagerDelegate,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("GeckoBridgeXpcom::set_power_manager_delegate");

        let delegate =
            ThreadPtrHolder::new(cstr!("nsIPowerManagerDelegate"), RefPtr::new(delegate)).unwrap();

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), delegate);

        if !self.ensure_service() {
            self.queue_task(GeckoBridgeTask::SetPowerManagerDelegate(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().set_power_manager_delegate(task);
        } else {
            error!("Unable to get GeckoBridgeImpl");
        }

        Ok(())
    }

    xpcom_method!(char_pref_changed => CharPrefChanged(name: *const nsACString, value: *const nsACString, callback: *const nsISidlDefaultResponse));
    fn char_pref_changed(
        &self,
        name: &nsACString,
        value: &nsACString,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("GeckoBridgeXpcom::char_pref_changed");

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (
            SidlCallTask::new(callback),
            (name.to_string(), value.to_string()),
        );

        if !self.ensure_service() {
            self.queue_task(GeckoBridgeTask::CharPrefChanged(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().char_pref_changed(task);
        } else {
            error!("Unable to get GeckoBridgeImpl");
        }

        Ok(())
    }

    xpcom_method!(int_pref_changed => IntPrefChanged(name: *const nsACString, value: i32, callback: *const nsISidlDefaultResponse));
    fn int_pref_changed(
        &self,
        name: &nsACString,
        value: i32,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("GeckoBridgeXpcom::int_pref_changed");

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), (name.to_string(), value));

        if !self.ensure_service() {
            self.queue_task(GeckoBridgeTask::IntPrefChanged(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().int_pref_changed(task);
        } else {
            error!("Unable to get GeckoBridgeImpl");
        }

        Ok(())
    }

    xpcom_method!(bool_pref_changed => BoolPrefChanged(name: *const nsACString, value: bool, callback: *const nsISidlDefaultResponse));
    fn bool_pref_changed(
        &self,
        name: &nsACString,
        value: bool,
        callback: &nsISidlDefaultResponse,
    ) -> Result<(), nsresult> {
        debug!("GeckoBridgeXpcom::bool_pref_changed");

        let callback =
            ThreadPtrHolder::new(cstr!("nsISidlDefaultResponse"), RefPtr::new(callback)).unwrap();
        let task = (SidlCallTask::new(callback), (name.to_string(), value));

        if !self.ensure_service() {
            self.queue_task(GeckoBridgeTask::BoolPrefChanged(task));
            return Ok(());
        }

        if let Some(inner) = self.inner.lock().as_ref() {
            return inner.lock().bool_pref_changed(task);
        } else {
            error!("Unable to get GeckoBridgeImpl");
        }

        Ok(())
    }

    ensure_service_and_queue!(GeckoBridgeTask, "GeckoBridge");
}

impl Drop for GeckoBridgeXpcom {
    fn drop(&mut self) {
        if let Some(inner) = self.inner.lock().as_ref() {
            self.transport.unregister_session_object(inner.clone());
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn gecko_bridge_construct(result: &mut *const nsIGeckoBridge) {
    if let Some(inst) = GeckoBridgeXpcom::new() {
        *result = inst.coerce::<nsIGeckoBridge>();
        std::mem::forget(inst);
    } else {
        *result = std::ptr::null();
    }
}
