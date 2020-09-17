/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_CursorRemote_h
#define mozilla_dom_CursorRemote_h

#include "VirtualCursorService.h"
#include "VirtualCursorProxy.h"

class nsPIDOMWindowOuter;

namespace mozilla {
namespace dom {

class CursorRemote final : public dom::nsIVirtualCursor {
 public:
  NS_DECL_ISUPPORTS

  explicit CursorRemote(nsPIDOMWindowOuter* aWindow) : mOuterWindow(aWindow) {}
  virtual void UpdatePos(const LayoutDeviceIntPoint& aPos) override;
  virtual void CursorClick() override;
  virtual void CursorDown() override;
  virtual void CursorUp() override;
  virtual void CursorMove() override;
  virtual void CursorOut(bool aCheckActive = false) override;
  virtual void ShowContextMenu() override;

 private:
  ~CursorRemote();
  nsCOMPtr<nsPIDOMWindowOuter> mOuterWindow;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_CursorRemote_h
