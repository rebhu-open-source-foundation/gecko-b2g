#include "nsCOMPtr.h"
#include "SettingsManager.h"

namespace {
extern "C" {

// Implemented in Rust.
void settings_manager_construct(nsISettingsManager** aResult);
}
}  // namespace

namespace sidl {

already_AddRefed<nsISettingsManager> ConstructSettingsManager() {
  nsCOMPtr<nsISettingsManager> manager;
  settings_manager_construct(getter_AddRefs(manager));
  return manager.forget();
}

}  // namespace sidl