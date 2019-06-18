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

#ifndef mozilla_dom_workerkaiOS_h__
#define mozilla_dom_workerkaiOS_h__

#include "nsWrapperCache.h"
#ifdef HAS_KOOST_MODULES
#  include "mozilla/dom/ExternalAPI.h"
#endif

namespace mozilla {
namespace dom {

class WorkerKaiOS final : public nsWrapperCache {
#ifdef HAS_KOOST_MODULES
  RefPtr<ExternalAPI> mExternalAPI;
#endif

  WorkerKaiOS();
  ~WorkerKaiOS();

 public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WorkerKaiOS)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WorkerKaiOS)

  static already_AddRefed<WorkerKaiOS> Create();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  nsISupports* GetParentObject() const { return nullptr; }
#ifdef HAS_KOOST_MODULES
  ExternalAPI* GetExternalapi(ErrorResult& aRv);
#endif
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_workerkaiOS_h__
