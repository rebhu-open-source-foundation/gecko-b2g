/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VirtualCursorProxy.h"

#include "nsLayoutUtils.h"
#include "nsSupportsPrimitives.h"

extern mozilla::LazyLogModule gVirtualCursorLog;

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(VirtualCursorProxy, nsIObserver)

VirtualCursorProxy::VirtualCursorProxy(nsPIDOMWindowInner* aWindow,
                                       nsIVirtualCursor* aDelegate,
                                       const LayoutDeviceIntPoint& aOffset)
    : mInnerWindow(aWindow) {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
          ("VirtualCursorProxy construct win id=%llu", aWindow->WindowID()));
  mSimulator = new CursorSimulator(aWindow->GetOuterWindow(), aDelegate);
  mSimulator->UpdateChromeOffset(aOffset);
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "inner-window-destroyed", false);
  }
}

VirtualCursorProxy::~VirtualCursorProxy() {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
          ("VirtualCursorProxy::~VirtualCursorProxy"));
}

void VirtualCursorProxy::DestroyCursor() {
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, "inner-window-destroyed");
  }
  mSimulator->Destroy();
  VirtualCursorService::RemoveCursor(mInnerWindow);
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug, ("VirtualCursorProxy::Destroy"));
}

void VirtualCursorProxy::RequestEnable() {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
          ("VirtualCursorProxy::RequestEnable"));
  mSimulator->Enable();
};

void VirtualCursorProxy::RequestDisable() {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
          ("VirtualCursorProxy::RequestDisable"));
  mSimulator->Disable();
};

void VirtualCursorProxy::IsEnabled(bool* aEnabled) {
  *aEnabled = mSimulator->isEnabled();
}

void VirtualCursorProxy::UpdateChromeOffset(
    const LayoutDeviceIntPoint& aChromeOffset) {
  mSimulator->UpdateChromeOffset(aChromeOffset);
}

NS_IMETHODIMP VirtualCursorProxy::Observe(nsISupports* aSubject,
                                          const char* aTopic,
                                          const char16_t* aData) {
  MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
          ("VirtualCursorProxy observed %s", aTopic));
  if (!strcmp(aTopic, "inner-window-destroyed")) {
    nsCOMPtr<nsISupportsPRUint64> wrapper = do_QueryInterface(aSubject);
    NS_ENSURE_TRUE(wrapper, NS_ERROR_FAILURE);
    uint64_t outerID;
    Unused << wrapper->GetData(&outerID);
    MOZ_LOG(gVirtualCursorLog, LogLevel::Debug,
            ("inner-window-destroyed id %llu self %llu", outerID,
             mInnerWindow->WindowID()));
    if (mInnerWindow->WindowID() == outerID) {
      DestroyCursor();
    }
  }
  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla
