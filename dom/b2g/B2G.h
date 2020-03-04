/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_B2G_h
#define mozilla_dom_B2G_h

#include "mozilla/dom/BindingDeclarations.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/TetheringManagerBinding.h"
#ifdef HAS_KOOST_MODULES
#  include "mozilla/dom/ExternalAPI.h"
#endif
#ifdef MOZ_B2G_BT
#  include "mozilla/dom/bluetooth/BluetoothManager.h"
#endif

namespace mozilla {
namespace dom {

class B2G final : public nsISupports, public nsWrapperCache {
  nsCOMPtr<nsIGlobalObject> mOwner;

 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(B2G)

  explicit B2G(nsIGlobalObject* aGlobal);

  nsIGlobalObject* GetParentObject() const { return mOwner; }
  virtual JSObject* WrapObject(JSContext* aCtx,
                               JS::Handle<JSObject*> aGivenProto) override;

  TetheringManager* GetTetheringManager(ErrorResult& aRv);

#ifdef HAS_KOOST_MODULES
  ExternalAPI* GetExternalapi(ErrorResult& aRv);
#endif
#ifdef MOZ_B2G_BT
  bluetooth::BluetoothManager* GetBluetooth(ErrorResult& aRv);
#endif
  // Shutting down.
  void Shutdown();

 private:
  ~B2G();
  RefPtr<TetheringManager> mTetheringManager;
#ifdef HAS_KOOST_MODULES
  RefPtr<ExternalAPI> mExternalAPI;
#endif

#ifdef MOZ_B2G_BT
  RefPtr<bluetooth::BluetoothManager> mBluetooth;
#endif
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_B2G_h
