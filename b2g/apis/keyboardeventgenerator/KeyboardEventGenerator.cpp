/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "KeyboardEventGenerator.h"
#include "KeyboardEvent.h"

#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsITextInputProcessor.h"
#include "nsITextInputProcessorCallback.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/KeyboardEventGeneratorBinding.h"

class nsITextInputProcessorNotification;

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(KeyboardEventGenerator, mWindow)

NS_IMPL_CYCLE_COLLECTING_ADDREF(KeyboardEventGenerator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(KeyboardEventGenerator)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(KeyboardEventGenerator)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

already_AddRefed<KeyboardEventGenerator>
KeyboardEventGenerator::Constructor(const GlobalObject& aGlobal,
                                    ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindowInner> inner =
    do_QueryInterface(aGlobal.GetAsSupports());
  if (NS_WARN_IF(!inner)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<KeyboardEventGenerator> keg =
    new KeyboardEventGenerator(inner);

  return keg.forget();
}

KeyboardEventGenerator::KeyboardEventGenerator(nsPIDOMWindowInner* aWindow)
  : mWindow(aWindow)
{
}

KeyboardEventGenerator::~KeyboardEventGenerator()
{
}

JSObject*
KeyboardEventGenerator::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return KeyboardEventGenerator_Binding::Wrap(aCx, this, aGivenProto);
}

// A simple callback for the TextInputProcessor.
// In our use case it's never called.
class TipCallback final: public nsITextInputProcessorCallback {
  NS_DECL_ISUPPORTS

  NS_IMETHOD OnNotify(nsITextInputProcessor *aTextInputProcessor, nsITextInputProcessorNotification *aNotification, bool *_retval) override {
    return NS_OK;
  }

  private:
    ~TipCallback() {}
};

NS_IMPL_ISUPPORTS(TipCallback, nsITextInputProcessorCallback)

void
KeyboardEventGenerator::Generate(KeyboardEvent& aEvent, ErrorResult& aRv)
{
  if (NS_WARN_IF(!XRE_IsParentProcess())) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  // Use the TextInputProcessor for the current window to send the key events.
  nsresult rv;
  nsCOMPtr<nsITextInputProcessor> tip = do_CreateInstance(TEXT_INPUT_PROCESSOR_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  // We have to begin composition to set up the TextInputProcessor.
  // See example 8 of nsITextInputProcessor.idl
  RefPtr<TipCallback> callback = new TipCallback();

  bool begin_rv;
  tip->BeginInputTransaction(mWindow, callback, &begin_rv);
  if (NS_WARN_IF(!begin_rv)) {
      aRv.Throw(NS_ERROR_FAILURE);
      return;
  }

  // Check if we want to send a keydown or keyup event and act accordingly.
  nsAutoString event_type;
  aEvent.GetType(event_type);

  if (event_type.EqualsLiteral("keydown")) {
    uint32_t keydown_rv;
    tip->Keydown(&aEvent, nsITextInputProcessor::KEY_NON_PRINTABLE_KEY, 0, &keydown_rv);
  } else if (event_type.EqualsLiteral("keyup")) {
    bool keyup_rv;
    tip->Keyup(&aEvent, nsITextInputProcessor::KEY_NON_PRINTABLE_KEY, 0, &keyup_rv);
  } else {
      aRv.Throw(NS_ERROR_FAILURE);
      return;
  }
}

} // namespace dom
} // namespace mozilla
