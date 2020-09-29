/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "GeckoEditableSupport.h"

#include "PuppetWidget.h"
#include "nsIContent.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPrefs_intl.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "mozilla/TextEvents.h"
#include "mozilla/ToString.h"
#include "mozilla/dom/BrowserChild.h"
#include "nsIDocShell.h"
#include "nsIGlobalObject.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/IMELog.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/InputMethodServiceChild.h"
#include "nsFocusManager.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "mozilla/dom/HTMLTextAreaElement.h"
#include "nsReadableUtils.h"
#include "mozilla/dom/Document.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/Element.h"
#include "nsHashPropertyBag.h"
#include "nsIDocumentEncoder.h"
#include "nsRange.h"

using namespace mozilla::dom;

namespace mozilla {

namespace widget {

bool isContentEditable(Element* aElement) {
  if (!aElement) {
    return false;
  }
  bool isContentEditable = false;
  bool isEditingOn = false;
  RefPtr<nsGenericHTMLElement> htmlElement =
      nsGenericHTMLElement::FromNodeOrNull(aElement);
  if (htmlElement) {
    isContentEditable = htmlElement->IsContentEditable();
  }
  RefPtr<Document> document = aElement->OwnerDoc();
  if (document) {
    isEditingOn = document->IsEditingOn();
  }
  if (isContentEditable || isEditingOn) {
    return true;
  }

  document = document->OwnerDoc();
  if (document) {
    return false;
  }

  return document->IsEditingOn();
}

bool isPlainTextField(Element* aElement) {
  if (!aElement) {
    return false;
  }
  RefPtr<HTMLTextAreaElement> textArea =
      HTMLTextAreaElement::FromNodeOrNull(aElement);
  if (textArea) {
    return true;
  }
  RefPtr<HTMLInputElement> input = HTMLInputElement::FromNodeOrNull(aElement);
  if (input) {
    return input->MozIsTextField(false);
  }
  return false;
}

already_AddRefed<nsIDocumentEncoder> getDocumentEncoder(Element* aElement) {
  nsCOMPtr<Document> document = aElement->OwnerDoc();
  nsAutoString contentType;
  document->GetContentType(contentType);
  nsCOMPtr<nsIDocumentEncoder> docEncoder = do_createDocumentEncoder(
      PromiseFlatCString(NS_ConvertUTF16toUTF8(contentType)).get());
  uint32_t flags = nsIDocumentEncoder::SkipInvisibleContent |
                   nsIDocumentEncoder::OutputRaw |
                   nsIDocumentEncoder::OutputDropInvisibleBreak |
                   nsIDocumentEncoder::OutputLFLineBreak;
  docEncoder->Init(document, u"text/plain"_ns, flags);
  return docEncoder.forget();
}

nsresult getContentEditableText(Element* aElement, nsAString& aText) {
  if (!aElement || !isContentEditable(aElement)) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<Document> document = aElement->OwnerDoc();
  ErrorResult rv;
  RefPtr<nsRange> range = document->CreateRange(rv);
  if (NS_WARN_IF(rv.Failed())) {
    return rv.StealNSResult();
  }
  range->SelectNodeContents(*aElement, rv);
  if (NS_WARN_IF(rv.Failed())) {
    return rv.StealNSResult();
  }
  nsCOMPtr<nsIDocumentEncoder> docEncoder = getDocumentEncoder(aElement);
  if (!docEncoder) {
    return NS_ERROR_FAILURE;
  }
  docEncoder->SetRange(range);
  return docEncoder->EncodeToString(aText);
}

uint32_t getSelectionStart(Element* aElement) {
  uint32_t start = 0;
  nsAutoString attributeValue;
  if (isPlainTextField(aElement)) {
    ErrorResult rv;
    Nullable<uint32_t> _start;
    RefPtr<HTMLTextAreaElement> textArea =
        HTMLTextAreaElement::FromNodeOrNull(aElement);
    if (textArea) {
      _start = textArea->GetSelectionStart(rv);
    }
    RefPtr<HTMLInputElement> input = HTMLInputElement::FromNodeOrNull(aElement);
    if (input) {
      _start = input->GetSelectionStart(rv);
    }
    if (!_start.IsNull()) {
      start = _start.Value();
    }
  } else if (isContentEditable(aElement)) {
    nsCOMPtr<Document> document = aElement->OwnerDoc();
    nsCOMPtr<nsPIDOMWindowOuter> window = document->GetWindow();
    RefPtr<Selection> selection = window->GetSelection();
    if (selection && selection->RangeCount() > 0) {
      ErrorResult rv;
      RefPtr<nsRange> range = document->CreateRange(rv);
      if (NS_WARN_IF(rv.Failed())) {
        return 0;
      }
      range->SetStart(aElement, 0);
      range->SetEnd(selection->GetAnchorNode(), selection->AnchorOffset());
      nsCOMPtr<nsIDocumentEncoder> docEncoder = getDocumentEncoder(aElement);
      if (!docEncoder) {
        return 0;
      }
      docEncoder->SetRange(range);
      nsAutoString buffer;
      docEncoder->EncodeToString(buffer);
      start = buffer.Length();
    }
  }
  return start;
}

uint32_t getSelectionEnd(Element* aElement) {
  uint32_t end = 0;
  nsAutoString attributeValue;
  if (isPlainTextField(aElement)) {
    ErrorResult rv;
    Nullable<uint32_t> _end;
    RefPtr<HTMLTextAreaElement> textArea =
        HTMLTextAreaElement::FromNodeOrNull(aElement);
    if (textArea) {
      _end = textArea->GetSelectionEnd(rv);
    }
    RefPtr<HTMLInputElement> input = HTMLInputElement::FromNodeOrNull(aElement);
    if (input) {
      _end = input->GetSelectionEnd(rv);
    }
    if (!_end.IsNull()) {
      end = _end.Value();
    }
  } else if (isContentEditable(aElement)) {
    nsCOMPtr<Document> document = aElement->OwnerDoc();
    nsCOMPtr<nsPIDOMWindowOuter> window = document->GetWindow();
    RefPtr<Selection> selection = window->GetSelection();
    if (selection && selection->RangeCount() > 0) {
      uint32_t start = getSelectionStart(aElement);
      nsCOMPtr<nsIDocumentEncoder> docEncoder = getDocumentEncoder(aElement);
      if (!docEncoder) {
        return 0;
      }
      ErrorResult rv;
      docEncoder->SetRange(selection->GetRangeAt(0, rv));
      nsAutoString buffer;
      docEncoder->EncodeToString(buffer);
      uint32_t length = buffer.Length();
      end = start + length;
    }
  }
  return end;
}

HandleFocusRequest CreateFocusRequestFromPropBag(nsIPropertyBag2* aPropBag) {
  nsAutoString typeValue, inputTypeValue, valueValue, maxValue, minValue,
      langValue, inputModeValue, nameValue;
  uint32_t selectionStartValue, selectionEndValue;
  bool voiceInputSupportedValue;
  aPropBag->GetPropertyAsAString(u"type"_ns, typeValue);
  aPropBag->GetPropertyAsAString(u"inputType"_ns, inputTypeValue);
  aPropBag->GetPropertyAsAString(u"value"_ns, valueValue);
  aPropBag->GetPropertyAsAString(u"max"_ns, maxValue);
  aPropBag->GetPropertyAsAString(u"min"_ns, minValue);
  aPropBag->GetPropertyAsAString(u"lang"_ns, langValue);
  aPropBag->GetPropertyAsAString(u"inputMode"_ns, inputModeValue);
  aPropBag->GetPropertyAsBool(u"voiceInputSupported"_ns,
                              &voiceInputSupportedValue);
  aPropBag->GetPropertyAsAString(u"name"_ns, nameValue);
  aPropBag->GetPropertyAsUint32(u"selectionStart"_ns, &selectionStartValue);
  aPropBag->GetPropertyAsUint32(u"selectionEnd"_ns, &selectionEndValue);

  HandleFocusRequest request(
      nsString(typeValue), nsString(inputTypeValue), nsString(valueValue),
      nsString(maxValue), nsString(minValue), nsString(langValue),
      nsString(inputModeValue), voiceInputSupportedValue, nsString(nameValue),
      selectionStartValue, selectionEndValue);
  return request;
}

NS_IMPL_ISUPPORTS(GeckoEditableSupport, TextEventDispatcherListener,
                  nsIEditableSupportListener, nsISupportsWeakReference)

GeckoEditableSupport::GeckoEditableSupport(nsIGlobalObject* aGlobal,
                                           nsWindow* aWindow)
    : mWindow(aWindow), mIsRemote(!aWindow), mGlobal(aGlobal) {}

nsresult GeckoEditableSupport::NotifyIME(
    TextEventDispatcher* aTextEventDispatcher,
    const IMENotification& aNotification) {
  IME_LOGD("NotifyIME, TextEventDispatcher:[%p]", aTextEventDispatcher);
  nsresult result;
  switch (aNotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      IME_LOGD("IME: REQUEST_TO_COMMIT_COMPOSITION");
      break;
    }

    case REQUEST_TO_CANCEL_COMPOSITION: {
      IME_LOGD("IME: REQUEST_TO_CANCEL_COMPOSITION");
      break;
    }

    case NOTIFY_IME_OF_FOCUS: {
      IME_LOGD("IME: NOTIFY_IME_OF_FOCUS");
      mDispatcher = aTextEventDispatcher;
      IME_LOGD("NotifyIME, mDispatcher:[%p]", mDispatcher.get());
      result = BeginInputTransaction(mDispatcher);
      NS_ENSURE_SUCCESS(result, result);
      RefPtr<nsHashPropertyBag> propBag = new nsHashPropertyBag();
      GetInputContextBag(propBag);
      {
        // oop
        ContentChild* contentChild = ContentChild::GetSingleton();
        if (contentChild) {
          InputMethodServiceChild* child = new InputMethodServiceChild();
          child->SetEditableSupportListener(this);
          contentChild->SendPInputMethodServiceConstructor(child);
          child->SendRequest(CreateFocusRequestFromPropBag(propBag));
        } else {
          // Call from parent process (or in-proces app).
          RefPtr<InputMethodService> service =
              dom::InputMethodService::GetInstance();
          service->HandleFocus(this, static_cast<nsIPropertyBag2*>(propBag));
        }
      }
      break;
    }

    case NOTIFY_IME_OF_BLUR: {
      IME_LOGD("IME: NOTIFY_IME_OF_BLUR");
      mDispatcher->EndInputTransaction(this);
      OnRemovedFrom(mDispatcher);
      {
        // oop
        ContentChild* contentChild = ContentChild::GetSingleton();
        if (contentChild) {
          InputMethodServiceChild* child = new InputMethodServiceChild();
          child->SetEditableSupportListener(this);
          contentChild->SendPInputMethodServiceConstructor(child);
          HandleBlurRequest request(contentChild->GetID());
          child->SendRequest(request);
        } else {
          // Call from parent process (or in-proces app).
          RefPtr<InputMethodService> service =
              dom::InputMethodService::GetInstance();
          service->HandleBlur(this);
        }
      }
      break;
    }

    case NOTIFY_IME_OF_SELECTION_CHANGE: {
      IME_LOGD("IME: NOTIFY_IME_OF_SELECTION_CHANGE: SelectionChangeData=%s",
               ToString(aNotification.mSelectionChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_TEXT_CHANGE: {
      // TODO
      IME_LOGD("IME: NOTIFY_IME_OF_TEXT_CHANGE: TextChangeData=%s",
               ToString(aNotification.mTextChangeData).c_str());
      break;
    }

    case NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED: {
      IME_LOGD("IME: NOTIFY_IME_OF_COMPOSITION_EVENT_HANDLED");
      break;
    }

    default:
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP_(IMENotificationRequests)
GeckoEditableSupport::GetIMENotificationRequests() {
  // Assume we support all change just as TextInputProcessor
  return IMENotificationRequests(
      IMENotificationRequests::NOTIFY_TEXT_CHANGE |
      IMENotificationRequests::NOTIFY_POSITION_CHANGE);
}

void GeckoEditableSupport::OnRemovedFrom(
    TextEventDispatcher* aTextEventDispatcher) {
  mDispatcher = nullptr;
}

void GeckoEditableSupport::WillDispatchKeyboardEvent(
    TextEventDispatcher* aTextEventDispatcher,
    WidgetKeyboardEvent& aKeyboardEvent, uint32_t aIndexOfKeypress,
    void* aData) {}

/* static */
void GeckoEditableSupport::SetOnBrowserChild(dom::BrowserChild* aBrowserChild,
                                             nsIGlobalObject* aGlobal) {
  MOZ_ASSERT(!XRE_IsParentProcess());
  NS_ENSURE_TRUE_VOID(aBrowserChild);
  IME_LOGD("IME: SetOnBrowserChild");
  RefPtr<widget::PuppetWidget> widget(aBrowserChild->WebWidget());
  RefPtr<widget::TextEventDispatcherListener> listener =
      widget->GetNativeTextEventDispatcherListener();

  if (!listener ||
      listener.get() ==
          static_cast<widget::TextEventDispatcherListener*>(widget)) {
    // We need to set a new listener.
    RefPtr<widget::GeckoEditableSupport> editableSupport =
        new widget::GeckoEditableSupport(aGlobal, nullptr);

    // Tell PuppetWidget to use our listener for IME operations.
    widget->SetNativeTextEventDispatcherListener(editableSupport);

    return;
  }
}

// nsIEditableSupportListener methods.
NS_IMETHODIMP
GeckoEditableSupport::DoSetComposition(const nsAString& aText) {
  IME_LOGD("-- GeckoEditableSupport::DoSetComposition");
  IME_LOGD("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (aText.Length() == 0) {
    mDispatcher->SetPendingCompositionString(EmptyString());
  } else {
    mDispatcher->SetPendingCompositionString(aText);
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->FlushPendingComposition(status);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoEndComposition(const nsAString& aText) {
  IME_LOGD("-- GeckoEditableSupport::DoEndComposition");
  IME_LOGD("-- EditableSupport aText:[%s]", NS_ConvertUTF16toUTF8(aText).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (!mDispatcher->IsComposing()) {
    return NS_ERROR_FAILURE;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->CommitComposition(status, &aText);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoKeydown(const nsAString& aKey) {
  IME_LOGD("-- GeckoEditableSupport::DoKeydown");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eKeyDown, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport OnKeydown: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  if (mDispatcher->DispatchKeyboardEvent(eKeyDown, event, status)) {
    mDispatcher->MaybeDispatchKeypressEvents(event, status);
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoKeyup(const nsAString& aKey) {
  IME_LOGD("-- GeckoEditableSupport::DoKeyup");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eKeyUp, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport OnKeyup: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  mDispatcher->DispatchKeyboardEvent(eKeyUp, event, status);
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoSendKey(const nsAString& aKey) {
  IME_LOGD("-- EditableSupport DoSendKey");
  IME_LOGD("-- EditableSupport aKey:[%s]", NS_ConvertUTF16toUTF8(aKey).get());
  IME_LOGD("-- EditableSupport mDispatcher:[%p]", mDispatcher.get());
  // Handle value
  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsCOMPtr<nsIWidget> widget = mDispatcher->GetWidget();
  WidgetKeyboardEvent event(true, eVoidEvent, widget);
  event.mKeyNameIndex = WidgetKeyboardEvent::GetKeyNameIndex(aKey);
  if (event.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING) {
    event.mKeyValue = aKey;
  } else if (event.mCodeNameIndex == CODE_NAME_INDEX_UNKNOWN) {
    event.mKeyCode = WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        event.mKeyNameIndex);
    IME_LOGD("-- EditableSupport DoSendKey: mKeyCode:[%d]", event.mKeyCode);
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  if (mDispatcher->DispatchKeyboardEvent(eKeyDown, event, status)) {
    mDispatcher->MaybeDispatchKeypressEvents(event, status);
  }
  mDispatcher->DispatchKeyboardEvent(eKeyUp, event, status);
  return NS_OK;
}

nsresult GeckoEditableSupport::GetInputContextBag(nsHashPropertyBag* aBagOut) {
  nsFocusManager* focusManager = nsFocusManager::GetFocusManager();
  if (!focusManager) return NS_ERROR_ABORT;
  Element* focusedElement = focusManager->GetFocusedElement();
  if (!focusedElement) return NS_ERROR_ABORT;

  nsAutoString attributeValue;

  // type
  focusedElement->GetTagName(attributeValue);
  ToLowerCase(attributeValue);
  if (attributeValue.IsEmpty()) {
    attributeValue.SetIsVoid(true);
  }
  aBagOut->SetPropertyAsAString(u"type"_ns, attributeValue);
  IME_LOGD("InputContextBag: type:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // inputType
  focusedElement->GetAttribute(u"type"_ns, attributeValue);
  ToLowerCase(attributeValue);
  if (attributeValue.IsEmpty()) {
    attributeValue.SetIsVoid(true);
  }
  aBagOut->SetPropertyAsAString(u"inputType"_ns, attributeValue);
  IME_LOGD("InputContextBag: inputType:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // value
  focusedElement->GetAttribute(u"value"_ns, attributeValue);
  aBagOut->SetPropertyAsAString(u"value"_ns, attributeValue);
  IME_LOGD("InputContextBag: value:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // max Using string since if inputType is date then max could be string.
  focusedElement->GetAttribute(u"max"_ns, attributeValue);
  aBagOut->SetPropertyAsAString(u"max"_ns, attributeValue);
  IME_LOGD("InputContextBag: max:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // min Same as max
  focusedElement->GetAttribute(u"min"_ns, attributeValue);
  aBagOut->SetPropertyAsAString(u"min"_ns, attributeValue);
  IME_LOGD("InputContextBag: min:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // lang
  focusedElement->GetAttribute(u"lang"_ns, attributeValue);
  aBagOut->SetPropertyAsAString(u"lang"_ns, attributeValue);
  IME_LOGD("InputContextBag: lang:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // inputMode
  focusedElement->GetAttribute(u"x-inputmode"_ns, attributeValue);
  ToLowerCase(attributeValue);
  if (attributeValue.IsEmpty()) {
    attributeValue.SetIsVoid(true);
  }
  aBagOut->SetPropertyAsAString(u"inputMode"_ns, attributeValue);
  IME_LOGD("InputContextBag: inputMode:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // voiceInputSupported
  focusedElement->GetAttribute(u"voiceInputSupported"_ns, attributeValue);
  aBagOut->SetPropertyAsBool(u"voiceInputSupported"_ns,
                             attributeValue.EqualsIgnoreCase("true"));
  IME_LOGD("InputContextBag: voiceInputSupported:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // name
  focusedElement->GetAttribute(u"name"_ns, attributeValue);
  ToLowerCase(attributeValue);
  if (attributeValue.IsEmpty()) {
    attributeValue.SetIsVoid(true);
  }
  aBagOut->SetPropertyAsAString(u"name"_ns, attributeValue);
  IME_LOGD("InputContextBag: name:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // selectionStart
  uint32_t start = getSelectionStart(focusedElement);
  aBagOut->SetPropertyAsUint32(u"selectionStart"_ns, start);
  IME_LOGD("InputContextBag: selectionStart:[%lu]", start);

  // selectionEnd
  uint32_t end = getSelectionEnd(focusedElement);
  aBagOut->SetPropertyAsUint32(u"selectionEnd"_ns, end);
  IME_LOGD("InputContextBag: selectionEnd:[%lu]", end);

  // Treat contenteditable element as a special text area field
  if (isContentEditable(focusedElement)) {
    aBagOut->SetPropertyAsAString(u"type"_ns, u"contenteditable"_ns);
    aBagOut->SetPropertyAsAString(u"inputType"_ns, u"textarea"_ns);
    nsresult rv = getContentEditableText(focusedElement, attributeValue);
    if (rv != NS_OK) {
      return rv;
    }
    aBagOut->SetPropertyAsAString(u"value"_ns, attributeValue);
    IME_LOGD("InputContextBag: value:[%s]",
             NS_ConvertUTF16toUTF8(attributeValue).get());
  }

  // TODO Should carry `choices` for HTMLSelectElement

  return NS_OK;
}

}  // namespace widget
}  // namespace mozilla
