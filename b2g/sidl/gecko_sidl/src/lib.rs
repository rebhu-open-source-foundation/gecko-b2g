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
