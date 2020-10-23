/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "InputMethodServiceChild.h"

#include "mozilla/dom/ContentChild.h"
#include "nsIInputMethodListener.h"
#include "mozilla/dom/IMELog.h"
namespace mozilla {
namespace dom {

InputMethodServiceChild::InputMethodServiceChild() {
  IME_LOGD("InputMethodServiceChild::Constructor[%p]", this);
}

InputMethodServiceChild::~InputMethodServiceChild() {
  IME_LOGD("InputMethodServiceChild::Destructor[%p]", this);
}

mozilla::ipc::IPCResult InputMethodServiceChild::RecvResponse(
    const InputMethodServiceResponse& aResponse) {
  IME_LOGD("InputMethodServiceChild::RecvResponse");
  switch (aResponse.type()) {
    case InputMethodServiceResponse::TSetCompositionResponse: {
      const SetCompositionResponse& response = aResponse;
      mInputMethodListener->OnSetComposition(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TEndCompositionResponse: {
      const EndCompositionResponse& response = aResponse;
      mInputMethodListener->OnEndComposition(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TKeydownResponse: {
      const KeydownResponse& response = aResponse;
      mInputMethodListener->OnKeydown(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TKeyupResponse: {
      const KeyupResponse& response = aResponse;
      mInputMethodListener->OnKeyup(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TSendKeyResponse: {
      const SendKeyResponse& response = aResponse;
      mInputMethodListener->OnSendKey(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TDeleteBackwardResponse: {
      const DeleteBackwardResponse& response = aResponse;
      mInputMethodListener->OnDeleteBackward(response.status());
      Unused << Send__delete__(this);
      break;
    }
    case InputMethodServiceResponse::TDoSetCompositionResponse: {
      const DoSetCompositionResponse& response = aResponse;
      mEditableSupportListener->DoSetComposition(response.text());
      break;
    }
    case InputMethodServiceResponse::TDoEndCompositionResponse: {
      const DoEndCompositionResponse& response = aResponse;
      mEditableSupportListener->DoEndComposition(response.text());
      break;
    }
    case InputMethodServiceResponse::TDoKeydownResponse: {
      const DoKeydownResponse& response = aResponse;
      mEditableSupportListener->DoKeydown(response.key());
      break;
    }
    case InputMethodServiceResponse::TDoKeyupResponse: {
      const DoKeyupResponse& response = aResponse;
      mEditableSupportListener->DoKeyup(response.key());
      break;
    }
    case InputMethodServiceResponse::TDoSendKeyResponse: {
      const DoSendKeyResponse& response = aResponse;
      mEditableSupportListener->DoSendKey(response.key());
      break;
    }
    case InputMethodServiceResponse::TDoDeleteBackwardResponse: {
      mEditableSupportListener->DoDeleteBackward();
      break;
    }
    case InputMethodServiceResponse::TDoSetSelectedOptionResponse: {
      const DoSetSelectedOptionResponse& response = aResponse;
      mEditableSupportListener->DoSetSelectedOption(response.optionIndex());
      break;
    }
    case InputMethodServiceResponse::TDoSetSelectedOptionsResponse: {
      const DoSetSelectedOptionsResponse& response = aResponse;
      mEditableSupportListener->DoSetSelectedOptions(response.optionIndexes());
      break;
    }
    case InputMethodServiceResponse::TCommonResponse: {
      const CommonResponse& response = aResponse;
      if (response.responseName() == u"DoRemoveFocus"_ns) {
        mEditableSupportListener->DoRemoveFocus();
      }

      break;
    }
    default: {
      return IPC_FAIL(this, "Unknown InputMethodService action type.");
    }
  }

  return IPC_OK();
}

void InputMethodServiceChild::SetEditableSupportListener(
    nsIEditableSupportListener* aListener) {
  mEditableSupportListener = aListener;
}

void InputMethodServiceChild::SetInputMethodListener(
    nsIInputMethodListener* aListener) {
  mInputMethodListener = aListener;
}

}  // namespace dom
}  // namespace mozilla
