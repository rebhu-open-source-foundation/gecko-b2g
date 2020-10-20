/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "InputMethodServiceParent.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/IMELog.h"
#include "mozilla/dom/nsInputContext.h"
namespace mozilla {
namespace dom {

void CarryIntoInputContext(const HandleFocusRequest& aRequest,
                           nsInputContext* aInputContext) {
  aInputContext->SetType(aRequest.type());
  aInputContext->SetInputType(aRequest.inputType());
  aInputContext->SetValue(aRequest.value());
  aInputContext->SetMax(aRequest.max());
  aInputContext->SetMin(aRequest.min());
  aInputContext->SetLang(aRequest.lang());
  aInputContext->SetInputMode(aRequest.inputMode());
  aInputContext->SetVoiceInputSupported(aRequest.voiceInputSupported());
  aInputContext->SetName(aRequest.name());
  aInputContext->SetSelectionStart(aRequest.selectionStart());
  aInputContext->SetSelectionEnd(aRequest.selectionEnd());

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

  RefPtr<nsInputContextChoices> inputContextChoices =
      new nsInputContextChoices();
  nsTArray<RefPtr<nsIInputContextChoicesInfo>> choices;
  choices.Clear();
  inputContextChoices->SetMultiple(aRequest.choices().multiple());

  for (uint32_t i = 0; i < aRequest.choices().choices().Length(); ++i) {
    const OptionDetailCollection& choice = aRequest.choices().choices()[i];
    if (choice.type() == OptionDetailCollection::TOptionDetail) {
      const OptionDetail& optionDetail = choice.get_OptionDetail();
      RefPtr<nsInputContextChoicesInfo> optInfo =
          new nsInputContextChoicesInfo();
      optInfo->SetGroup(optionDetail.group());
      optInfo->SetInGroup(optionDetail.inGroup());
      optInfo->SetValue(optionDetail.text());
      optInfo->SetText(optionDetail.text());
      optInfo->SetDisabled(optionDetail.disabled());
      optInfo->SetSelected(optionDetail.selected());
      optInfo->SetDefaultSelected(optionDetail.defaultSelected());
      optInfo->SetOptionIndex(optionDetail.optionIndex());
      choices.AppendElement(optInfo);
      IME_LOGD("HandleFocusRequest: OptionDetail:[%s]",
               NS_ConvertUTF16toUTF8(optionDetail.text()).get());
    } else {  // OptionGroupDetail
      const OptionGroupDetail& optionGroupDetail =
          choice.get_OptionGroupDetail();
      RefPtr<nsInputContextChoicesInfo> groupInfo =
          new nsInputContextChoicesInfo();
      groupInfo->SetGroup(optionGroupDetail.group());
      groupInfo->SetLabel(optionGroupDetail.label());
      groupInfo->SetDisabled(optionGroupDetail.disabled());
      choices.AppendElement(groupInfo);
      IME_LOGD("HandleFocusRequest: OptionGroupDetail:[%s]",
               NS_ConvertUTF16toUTF8(optionGroupDetail.label()).get());
    }
  }
  inputContextChoices->SetChoices(choices);
  aInputContext->SetInputContextChoices(inputContextChoices);
}

NS_IMPL_ISUPPORTS(InputMethodServiceParent, nsIInputMethodListener,
                  nsIEditableSupportListener)

InputMethodServiceParent::InputMethodServiceParent() {
  IME_LOGD("InputMethodServiceParent::Constructor[%p]", this);
}

InputMethodServiceParent::~InputMethodServiceParent() {
  IME_LOGD("InputMethodServiceParent::Destructor[%p]", this);
}

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
    case InputMethodServiceRequest::TDeleteBackwardRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:DeleteBackwardRequest");
      const DeleteBackwardRequest& request = aRequest;
      service->DeleteBackward(this);
      break;
    }
    case InputMethodServiceRequest::THandleFocusRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:HandleFocusRequest");
      const HandleFocusRequest& request = aRequest;
      RefPtr<nsInputContext> inputContext = new nsInputContext();
      CarryIntoInputContext(request, inputContext);
      service->HandleFocus(this, static_cast<nsIInputContext*>(inputContext));
      break;
    }
    case InputMethodServiceRequest::THandleBlurRequest: {
      IME_LOGD("InputMethodServiceParent::RecvRequest:HandleBlurRequest");
      service->HandleBlur(this);
      break;
    }
    case InputMethodServiceRequest::TSetSelectedOptionRequest: {
      IME_LOGD(
          "InputMethodServiceParent::RecvRequest:SetSelectedOptionRequest");
      const SetSelectedOptionRequest& request = aRequest;
      service->SetSelectedOption(request.optionIndex());
      break;
    }
    case InputMethodServiceRequest::TSetSelectedOptionsRequest: {
      IME_LOGD(
          "InputMethodServiceParent::RecvRequest:SetSelectedOptionsRequest");
      const SetSelectedOptionsRequest& request = aRequest;
      service->SetSelectedOptions(request.optionIndexes());
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

NS_IMETHODIMP
InputMethodServiceParent::OnDeleteBackward(nsresult aStatus) {
  IME_LOGD("InputMethodServiceParent::OnDeleteBackward");
  DeleteBackwardResponse response(aStatus);
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

NS_IMETHODIMP
InputMethodServiceParent::DoDeleteBackward() {
  IME_LOGD("InputMethodServiceParent::DoDeleteBackward");
  DoDeleteBackwardResponse response;
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoSetSelectedOption(int32_t aOptionIndex) {
  IME_LOGD("InputMethodServiceParent::DoSetSelectedOption");
  DoSetSelectedOptionResponse response(aOptionIndex);
  Unused << SendResponse(response);
  return NS_OK;
}

NS_IMETHODIMP
InputMethodServiceParent::DoSetSelectedOptions(
    const nsTArray<int32_t>& aOptionIndexes) {
  IME_LOGD("InputMethodServiceParent::DoSetSelectedOptions");
  DoSetSelectedOptionsResponse response(aOptionIndexes);
  Unused << SendResponse(response);
  return NS_OK;
}

}  // namespace dom
}  // namespace mozilla
