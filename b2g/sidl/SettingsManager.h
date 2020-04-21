#ifndef SETTINGS_MANAGER_H_
#define SETTINGS_MANAGER_H_

#include "nsISettings.h"
#include "nsCOMPtr.h"

namespace sidl {

already_AddRefed<nsISettingsManager> ConstructSettingsManager();

}  // namespace sidl

#endif  // SETTINGS_MANAGER_H_