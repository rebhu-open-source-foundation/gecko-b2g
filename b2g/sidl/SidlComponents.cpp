/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsCOMPtr.h"
#include "SidlComponents.h"

namespace {

// Implemented in Rust.
extern "C" {
void gecko_bridge_construct(nsIGeckoBridge** aResult);
void settings_manager_construct(nsISettingsManager** aResult);
void daemon_manager_construct(nsIDaemonManager** aResult);
void time_construct(nsITime** aResult);
}

}  // namespace

namespace sidl {

already_AddRefed<nsIGeckoBridge> ConstructGeckoBridge() {
  nsCOMPtr<nsIGeckoBridge> bridge;
  gecko_bridge_construct(getter_AddRefs(bridge));
  return bridge.forget();
}

already_AddRefed<nsISettingsManager> ConstructSettingsManager() {
  nsCOMPtr<nsISettingsManager> manager;
  settings_manager_construct(getter_AddRefs(manager));
  return manager.forget();
}

already_AddRefed<nsIDaemonManager> ConstructDaemonManager() {
  nsCOMPtr<nsIDaemonManager> manager;
  daemon_manager_construct(getter_AddRefs(manager));
  return manager.forget();
}

already_AddRefed<nsITime> ConstructTime() {
  nsCOMPtr<nsITime> time;
  time_construct(getter_AddRefs(time));
  return time.forget();
}

}  // namespace sidl
