/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG
 * KONG) LIMITED or its affiliate company and may be registered in some
 * jurisdictions. All other trademarks are the property of their respective
 * owners.
 */

#ifndef mozilla_dom_WebActivityRequestHandler_h
#define mozilla_dom_WebActivityRequestHandler_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsWrapperCache.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

struct WebActivityOptions;

class WebActivityRequestHandler final : public nsWrapperCache

{
 public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebActivityRequestHandler)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebActivityRequestHandler)

  nsISupports* GetParentObject() const { return nullptr; }

  JSObject* WrapObject(JSContext* aCx,
                       JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<WebActivityRequestHandler> Create(
      nsIGlobalObject* aGlobal, const nsAString& aMessage);
  // WebIDL methods:

  void PostResult(JSContext* aCx, JS::Handle<JS::Value> aResult,
                  ErrorResult& aRv);

  void PostError(const nsAString& aError);

  void GetSource(JSContext* aCx, WebActivityOptions& aResult, ErrorResult& aRv);

 private:
  WebActivityRequestHandler(const nsAString& aMessage, const nsAString& aId,
                            bool aReturnValue);
  ~WebActivityRequestHandler();

  nsString mMessage;
  nsString mActivityId;
  bool mReturnValue;
};
}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_WebActivityRequestHandler_h
