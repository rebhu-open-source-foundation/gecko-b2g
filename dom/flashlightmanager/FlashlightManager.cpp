/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "FlashlightManager.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Hal.h"
#include "mozilla/dom/FlashlightManagerBinding.h"
//#include "nsIDOMClassInfo.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(FlashlightManager, DOMEventTargetHelper,
                                   mPendingFlashlightPromises)

NS_IMPL_ADDREF_INHERITED(FlashlightManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FlashlightManager, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FlashlightManager)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

FlashlightManager::FlashlightManager(nsPIDOMWindowInner* aWindow)
  : DOMEventTargetHelper(aWindow)
  , mFlashlightEnabled(false)
{
}

FlashlightManager::~FlashlightManager()
{
}

void
FlashlightManager::Init()
{
  hal::RegisterFlashlightObserver(this);
}

void
FlashlightManager::Shutdown()
{
  hal::UnregisterFlashlightObserver(this);

  for (uint32_t i = 0; i < mPendingFlashlightPromises.Length(); ++i) {
    mPendingFlashlightPromises[i]->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  }
  mPendingFlashlightPromises.Clear();
}

JSObject*
FlashlightManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return FlashlightManager_Binding::Wrap(aCx, this, aGivenProto);
}

void
FlashlightManager::Notify(const hal::FlashlightInformation& aFlashlightInfo)
{
  bool hasChanged = aFlashlightInfo.enabled() != mFlashlightEnabled;
  mFlashlightEnabled = aFlashlightInfo.enabled();

  for (uint32_t i = 0; i < mPendingFlashlightPromises.Length(); ++i) {
    mPendingFlashlightPromises[i]->MaybeResolve(this);
  }
  mPendingFlashlightPromises.Clear();

  if (hasChanged) {
    DispatchTrustedEvent(NS_LITERAL_STRING("flashlightchange"));
  }
}

already_AddRefed<Promise>
FlashlightManager::GetPromise(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(GetOwner());
  if (!go) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<Promise> promise = Promise::Create(go, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  mPendingFlashlightPromises.AppendElement(promise);
  hal::RequestCurrentFlashlightState();

  return promise.forget();
}

void
FlashlightManager::SetFlashlightEnabled(bool aEnabled)
{
  hal::SetFlashlightEnabled(aEnabled);
}

bool
FlashlightManager::FlashlightEnabled() const
{
  return mFlashlightEnabled;
}

} // namespace dom
} // namespace mozilla
