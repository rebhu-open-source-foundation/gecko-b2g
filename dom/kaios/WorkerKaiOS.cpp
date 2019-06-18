/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#include "nsIGlobalObject.h"

#include "mozilla/dom/WorkerKaiOS.h"
#include "mozilla/dom/WorkerKaiOSBinding.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerScope.h"

#include "mozilla/dom/KaiOS.h"

namespace mozilla {
namespace dom {

using namespace workerinternals;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WorkerKaiOS);

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WorkerKaiOS, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WorkerKaiOS, Release)

WorkerKaiOS::WorkerKaiOS() {}

WorkerKaiOS::~WorkerKaiOS() {}

/* static */
already_AddRefed<WorkerKaiOS> WorkerKaiOS::Create() {
  RefPtr<WorkerKaiOS> kaiOS = new WorkerKaiOS();

  return kaiOS.forget();
}

JSObject* WorkerKaiOS::WrapObject(JSContext* aCx,
                                  JS::Handle<JSObject*> aGivenProto) {
  return WorkerKaiOS_Binding::Wrap(aCx, this, aGivenProto);
}

}  // namespace dom
}  // namespace mozilla
