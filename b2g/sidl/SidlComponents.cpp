#include "nsCOMPtr.h"
#include "SidlComponents.h"

namespace {

// Implemented in Rust.
extern "C" {
void gecko_bridge_construct(nsIGeckoBridge** aResult);
void settings_manager_construct(nsISettingsManager** aResult);

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

}  // namespace sidl