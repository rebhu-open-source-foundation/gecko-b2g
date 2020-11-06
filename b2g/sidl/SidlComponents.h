/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef SIDL_COMPONENTS_H_
#define SIDL_COMPONENTS_H_

#include "nsIGeckoBridge.h"
#include "nsISettings.h"
#include "nsITime.h"
#include "nsCOMPtr.h"

namespace sidl {

already_AddRefed<nsIGeckoBridge> ConstructGeckoBridge();

already_AddRefed<nsISettingsManager> ConstructSettingsManager();

already_AddRefed<nsIDaemonManager> ConstructDaemonManager();

already_AddRefed<nsITime> ConstructTime();

}  // namespace sidl

#endif  // SIDL_COMPONENTS_H_
