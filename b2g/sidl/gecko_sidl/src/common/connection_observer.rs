/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Macro providing the bulk of the nsISidlConnectionObserver implementation.
// Users still need to add and remove themselves as observers properly.

macro_rules! implement_connection_observer {
    ($name:expr) => {
    xpcom_method!(disconnected => Disconnected());
    fn disconnected(&self) -> Result<(), nsresult> {
        debug!("{} received observer notification: disconnected", $name);
        // TODO: figure out how to manage in-flight requests.

        // Re-initialize ourselves.
        // Get the set of tasks we want to queue and drain when reconnecting.
        if let Some(inner) = self.inner.lock().as_ref() {
            let mut really_inner = inner.lock();
            *self.pending_tasks.lock() = really_inner.get_tasks_to_reconnect();
            *self.pending_listeners.lock() = really_inner.get_event_listeners_to_reconnect();
        }
        self.getting_service.store(false, Ordering::Relaxed);
        *self.inner.lock() = None;

        Ok(())
    }

    xpcom_method!(reconnected => Reconnected());
    fn reconnected(&self) -> Result<(), nsresult> {
        debug!("{} received observer notification: reconnected", $name);

        // Start the process of getting a service instance. This will drain the queue
        // of pending tasks and attach pending listeners if any are available.
        *self.core_service.lock() = CoreService::new(&self.transport);
        self.run_or_queue_task(None);

        Ok(())
    }
    };
}
