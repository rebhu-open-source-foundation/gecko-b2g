#ifndef SIDL_COMPONENTS_H_
#define SIDL_COMPONENTS_H_

#include "nsIGeckoBridge.h"
#include "nsISettings.h"
#include "nsCOMPtr.h"

namespace sidl {

already_AddRefed<nsIGeckoBridge> ConstructGeckoBridge();

already_AddRefed<nsISettingsManager> ConstructSettingsManager();

}  // namespace sidl

#endif  // SIDL_COMPONENTS_H_