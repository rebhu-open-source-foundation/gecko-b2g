/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#[macro_use]
extern crate cstr;

#[macro_use]
extern crate xpcom;

#[macro_use]
pub mod common;
pub mod services;

use nserror::nsresult;
use xpcom::{getter_addrefs, interfaces::nsIThread, RefPtr};

// Helper to configure a Rust thread as a proper event target for Gecko.
extern "C" {
    fn NS_AdoptCurrentThread(result: *mut *const nsIThread) -> nsresult;
}

pub fn adopt_current_thread() -> Result<RefPtr<nsIThread>, nsresult> {
    getter_addrefs(|p| unsafe { NS_AdoptCurrentThread(p) })
}
