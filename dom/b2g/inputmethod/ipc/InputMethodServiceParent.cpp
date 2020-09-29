/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "InputMethodServiceParent.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/IMELog.h"
#include "nsHashPropertyBag.h"
namespace mozilla {
namespace dom {

void CarryIntoBag(const HandleFocusRequest& aRequest,
                  nsHashPropertyBag* aProp) {
  aProp->SetPropertyAsAString(u"type"_ns, aRequest.type());
  aProp->SetPropertyAsAString(u"inputType"_ns, aRequest.inputType());
  aProp->SetPropertyAsAString(u"value"_ns, aRequest.value());
  aProp->SetPropertyAsAString(u"max"_ns, aRequest.max());
  aProp->SetPropertyAsAString(u"min"_ns, aRequest.min());
  aProp->SetPropertyAsAString(u"lang"_ns, aRequest.lang());
  aProp->SetPropertyAsAString(u"inputMode"_ns, aRequest.inputMode());
  aProp->SetPropertyAsBool(u"voiceInputSupported"_ns,
                           aRequest.voiceInputSupported());
  aProp->SetPropertyAsAString(u"name"_ns, aRequest.name());
  aProp->SetPropertyAsUint32(u"selectionStart"_ns, aRequest.selectionStart());
  aProp->SetPropertyAsUint32(u"selectionEnd"_ns, aRequest.selectionEnd());

  IME_LOGD("HandleFocusRequest: type:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.type()).get());
  IME_LOGD("HandleFocusRequest: inputType:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.inputType()).get());
  IME_LOGD("HandleFocusRequest: value:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.value()).get());
  IME_LOGD("HandleFocusRequest: max:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.max()).get());
  IME_LOGD("HandleFocusRequest: min:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.min()).get());
  IME_LOGD("HandleFocusRequest: lang:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.lang()).get());
  IME_LOGD("HandleFocusRequest: inputMode:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.inputMode()).get());
  IME_LOGD("HandleFocusRequest: voiceInputSupported:[%d]",
           aRequest.voiceInputSupported());
  IME_LOGD("HandleFocusRequest: name:[%s]",
           NS_ConvertUTF16toUTF8(aRequest.name()).get());
  IME_LOGD("HandleFocusRequest: selectionStart:[%lu]",
           aRequest.selectionStart());
  IME_LOGD("HandleFocusRequest: selectionEnd:[%lu]", aRequest.selectionEnd());
}

NS_IMPL_ISUPPORTS(InputMethodServiceParent, nsIInputMethodListener,
                  nsIEditableSupportListener)

InputMethodServiceParent::InputMethodServiceParent() {}

InputMethodServiceParent::~InputMethodServiceParent() {}

mozilla::ipc::IPCResult InputMethodServiceParent::RecvRequest(
    const InputMethodServiceRequest& aRequest) {
  IME_LOGD("InputMethodServiceParent::RecvRequest");
  RefPtr<InputMethodService> service = InputMethodService::GetInstance();
  MOZ_ASSERT(service);

  switch (aRequest.type()) {
    case InputMethodServiceRequest::TSetCompositionRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:SetCompositionRequest");
      const SetCompositionRequest& request = aRequest;
      service->SetComposition(request.text(), this);
      break;
    }
    case InputMethodServiceRequest::TEndCompositionRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:EndCompositionRequest");
      const EndCompositionRequest& request = aRequest;
      service->EndComposition(request.text(), this);
      break;
    }
    case InputMethodServiceRequest::TKeydownRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:KeydownRequest");
      const KeydownRequest& request = aRequest;
      service->Keydown(request.key(), this);
      break;
    }
    case InputMethodServiceRequest::TKeyupRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:KeyupRequest");
      const KeyupRequest& request = aRequest;
      service->Keyup(request.key(), this);
      break;
    }
    case InputMethodServiceRequest::TSendKeyRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:SendKeyRequest");
      const SendKeyRequest& request = aRequest;
      service->SendKey(request.key(), this);
      break;
    }
    case InputMethodServiceRequest::THandleFocusRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:HandleFocusRequest");
      const HandleFocusRequest& request = aRequest;
      RefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();
      CarryIntoBag(request, props);
      service->HandleFocus(this, static_cast<nsIPropertyBag2*>(props));
      break;
    }
    case InputMethodServiceRequest::THandleBlurRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:HandleBlurRequest");
      service->HandleBlur(this);
      break;
    }
    default: {
      return IPC_FAIL(this, "Unknown InputMethodService action type.");
    }
  }

  return IPC_OK();
}

// nsIInputMethodListener methods.
NS_IMETHODIMP
InputMethodServiceParent::OnSetComposition(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnSetComposition");
  SetCompositionResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::OnEndComposition(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnEndComposition");
  EndCompositionResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::OnKeydown(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnKeydown");
  KeydownResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::OnKeyup(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnKeyup");
  KeyupResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::OnSendKey(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnSendKey");
  SendKeyResponse response(aStatus);
  Unused << SendResponse(response);
  return NS_OK;
}

// nsIEditableSupportListener methods.
NS_IMETHODIMP
InputMethodServiceParent::DoSetComposition(const nsAString& aText) {
  IME_LOGD("InputMethodServiceParent::DoSetComposition");
  nsString text(aText);
  DoSetCompositionResponse response(text);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoEndComposition(const nsAString& aText) {
  IME_LOGD("InputMethodServiceParent::DoEndComposition");
  nsString text(aText);
  DoEndCompositionResponse response(text);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoKeydown(const nsAString& aKey) {
  IME_LOGD("InputMethodServiceParent::DoKeydown");
  nsString key(aKey);
  DoKeydownResponse response(key);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoKeyup(const nsAString& aKey) {
  IME_LOGD("InputMethodServiceParent::DoKeyup");
  nsString key(aKey);
  DoKeyupResponse response(key);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoSendKey(const nsAString& aKey) {
  IME_LOGD("InputMethodServiceParent::DoSendKey");
  nsString key(aKey);
  DoSendKeyResponse response(key);
  Unused << SendResponse(response);
  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla
