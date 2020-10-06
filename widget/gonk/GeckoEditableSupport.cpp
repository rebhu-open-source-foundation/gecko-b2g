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
#include "mozilla/dom/IMELog.h"
#include "mozilla/dom/InputMethodService.h"
#include "mozilla/dom/InputMethodServiceChild.h"
#include "nsFocusManager.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "mozilla/dom/HTMLTextAreaElement.h"
#include "mozilla/dom/HTMLSelectElement.h"
#include "mozilla/dom/HTMLOptGroupElement.h"
#include "mozilla/dom/HTMLOptionElement.h"
#include "mozilla/dom/Document.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/Element.h"
#include "nsIDocumentEncoder.h"
#include "nsRange.h"
#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/nsInputContext.h"

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

bool isIgnoredInputTypes(nsAString& inputType) {
  // nsAutoString inputType(aType);
  return inputType.EqualsIgnoreCase("button") ||
         inputType.EqualsIgnoreCase("file") ||
         inputType.EqualsIgnoreCase("checkbox") ||
         inputType.EqualsIgnoreCase("radio") ||
         inputType.EqualsIgnoreCase("reset") ||
         inputType.EqualsIgnoreCase("submit") ||
         inputType.EqualsIgnoreCase("image") ||
         inputType.EqualsIgnoreCase("range");
}

bool isFocusableElement(Element* aElement) {
  if (!aElement) {
    return false;
  }
  RefPtr<HTMLSelectElement> selectElement =
      HTMLSelectElement::FromNodeOrNull(aElement);
  if (selectElement) {
    return true;
  }
  RefPtr<HTMLTextAreaElement> textAreaElement =
      HTMLTextAreaElement::FromNodeOrNull(aElement);
  if (textAreaElement) {
    return true;
  }
  RefPtr<HTMLOptionElement> optionElement =
      HTMLOptionElement::FromNodeOrNull(aElement);
  if (optionElement) {
    RefPtr<HTMLSelectElement> optionParent =
        HTMLSelectElement::FromNodeOrNull(optionElement->GetParentNode());
    if (optionParent) {
      return true;
    }
  }
  nsAutoString inputType;
  RefPtr<HTMLInputElement> inputElement =
      HTMLInputElement::FromNodeOrNull(aElement);
  if (inputElement) {
    if (inputElement->ReadOnly()) {
      return false;
    }
    inputElement->GetType(inputType);
    if (isIgnoredInputTypes(inputType)) {
      return false;
    }
    return true;
  }
  return false;
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

HandleFocusRequest CreateFocusRequestFromInputContext(
    nsIInputContext* aInputContext) {
  nsAutoString typeValue, inputTypeValue, valueValue, maxValue, minValue,
      langValue, inputModeValue, nameValue;
  uint32_t selectionStartValue, selectionEndValue;
  bool voiceInputSupportedValue;

  aInputContext->GetType(typeValue);
  aInputContext->GetInputType(inputTypeValue);
  aInputContext->GetValue(valueValue);
  aInputContext->GetMax(maxValue);
  aInputContext->GetMin(minValue);
  aInputContext->GetLang(langValue);
  aInputContext->GetInputMode(inputModeValue);
  aInputContext->GetVoiceInputSupported(&voiceInputSupportedValue);
  aInputContext->GetName(nameValue);
  aInputContext->GetSelectionStart(&selectionStartValue);
  aInputContext->GetSelectionEnd(&selectionEndValue);
  nsCOMPtr<nsIInputContextChoices> choices;
  aInputContext->GetChoices(getter_AddRefs(choices));
  SelectionChoices selectionChoices;
  nsTArray<OptionDetailCollection> optionDetailArray;
  if (choices) {
    bool isMultiple;
    choices->GetMultiple(&isMultiple);
    nsTArray<RefPtr<nsIInputContextChoicesInfo>> infos;
    choices->GetChoices(infos);

    for (const auto& info : infos) {
      bool isGroup;
      info->GetGroup(&isGroup);
      if (isGroup) {  // OptionGroupDetail
        OptionGroupDetail groupDetail;
        bool isGroupDisabled;
        info->GetDisabled(&isGroupDisabled);
        nsAutoString label;
        info->GetLabel(label);
        groupDetail.group() = isGroup;
        groupDetail.label() = nsString(label);
        groupDetail.disabled() = isGroupDisabled;
        IME_LOGD("groupDetail.group:[%s]", isGroup ? "true" : "false");
        IME_LOGD("groupDetail.disabled:[%s]",
                 isGroupDisabled ? "true" : "false");
        IME_LOGD("groupDetail.label:[%s]", NS_ConvertUTF16toUTF8(label).get());
        optionDetailArray.AppendElement(groupDetail);
      } else {  // OptionDetail
        OptionDetail optionDetail;
        bool isInGroup, isDisabled, isSelected, isDefaultSelected;
        nsAutoString text;
        int32_t optionIndex;
        info->GetInGroup(&isInGroup);
        info->GetDisabled(&isDisabled);
        info->GetSelected(&isSelected);
        info->GetDefaultSelected(&isDefaultSelected);
        info->GetText(text);
        info->GetOptionIndex(&optionIndex);
        optionDetail.group() = isGroup;
        optionDetail.inGroup() = isInGroup;
        optionDetail.disabled() = isDisabled;
        optionDetail.selected() = isSelected;
        optionDetail.defaultSelected() = isDefaultSelected;
        optionDetail.text() = nsString(text);
        optionDetail.optionIndex() = optionIndex;
        optionDetailArray.AppendElement(optionDetail);
        IME_LOGD("optionDetail.group:[%s]", isGroup ? "true" : "false");
        IME_LOGD("optionDetail.inGroup:[%s]", isInGroup ? "true" : "false");
        IME_LOGD("optionDetail.disabled:[%s]", isDisabled ? "true" : "false");
        IME_LOGD("optionDetail.selected:[%s]", isSelected ? "true" : "false");
        IME_LOGD("optionDetail.defaultSelected:[%s]",
                 isDefaultSelected ? "true" : "false");
        IME_LOGD("optionDetail.text:[%s]", NS_ConvertUTF16toUTF8(text).get());
        IME_LOGD("optionDetail.optionIndex:[%ld]", optionIndex);
      }
    }
    selectionChoices.multiple() = isMultiple;
    selectionChoices.choices() = optionDetailArray.Clone();
  }

  HandleFocusRequest request(
      nsString(typeValue), nsString(inputTypeValue), nsString(valueValue),
      nsString(maxValue), nsString(minValue), nsString(langValue),
      nsString(inputModeValue), voiceInputSupportedValue, nsString(nameValue),
      selectionStartValue, selectionEndValue, selectionChoices);
  return request;
}

NS_IMPL_ISUPPORTS(GeckoEditableSupport, TextEventDispatcherListener,
                  nsIEditableSupportListener, nsIDOMEventListener, nsIObserver,
                  nsISupportsWeakReference)

GeckoEditableSupport::GeckoEditableSupport(nsPIDOMWindowOuter* aDOMWindow,
                                           nsWindow* aWindow)
    : mWindow(aWindow), mIsRemote(!aWindow) {
  IME_LOGD("GeckoEditableSupport::Constructor[%p]", this);
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "outer-window-destroyed", false);
  }
  nsCOMPtr<EventTarget> mChromeEventHandler =
      aDOMWindow->GetChromeEventHandler();
  if (mChromeEventHandler) {
    IME_LOGD("IME: AddEventListener: focus & blur");
    mChromeEventHandler->AddEventListener(u"focus"_ns, this,
                                          /* useCapture = */ true);
    mChromeEventHandler->AddEventListener(u"blur"_ns, this,
                                          /* useCapture = */ true);
  }
}

GeckoEditableSupport::~GeckoEditableSupport() {
  IME_LOGD("GeckoEditableSupport::Destructor[%p]", this);
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(this, "outer-window-destroyed");
  }
}

NS_IMETHODIMP
GeckoEditableSupport::Observe(nsISupports* aSubject, const char* aTopic,
                              const char16_t* aData) {
  IME_LOGD("GeckoEditableSupport[%p], Observe[%s]", this, aTopic);
  if (!strcmp(aTopic, "outer-window-destroyed")) {
    mChromeEventHandler->RemoveEventListener(u"focus"_ns, this,
                                             /* useCapture = */ true);
    mChromeEventHandler->RemoveEventListener(u"blur"_ns, this,
                                             /* useCapture = */ true);
  }
}

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
      break;
    }

    case NOTIFY_IME_OF_BLUR: {
      IME_LOGD("IME: NOTIFY_IME_OF_BLUR");
      mDispatcher->EndInputTransaction(this);
      OnRemovedFrom(mDispatcher);
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
                                             nsPIDOMWindowOuter* aDOMWindow) {
  MOZ_ASSERT(!XRE_IsParentProcess());
  NS_ENSURE_TRUE_VOID(aBrowserChild);

  RefPtr<widget::PuppetWidget> widget(aBrowserChild->WebWidget());
  RefPtr<widget::TextEventDispatcherListener> listener =
      widget->GetNativeTextEventDispatcherListener();

  if (!listener ||
      listener.get() ==
          static_cast<widget::TextEventDispatcherListener*>(widget)) {
    // We need to set a new listener.
    RefPtr<widget::GeckoEditableSupport> editableSupport =
        new widget::GeckoEditableSupport(aDOMWindow);

    // Tell PuppetWidget to use our listener for IME operations.
    widget->SetNativeTextEventDispatcherListener(editableSupport);
  }
}

NS_IMETHODIMP
GeckoEditableSupport::HandleEvent(Event* aEvent) {
  if (!aEvent) {
    return NS_OK;
  }
  nsCOMPtr<EventTarget> target = aEvent->GetComposedTarget();
  nsCOMPtr<nsINode> node(do_QueryInterface(target));
  if (!node || !node->IsHTMLElement()) {
    return NS_OK;
  }
  nsCOMPtr<Element> ele = node->AsElement();
  if (!ele) {
    return NS_OK;
  }
  if (!isContentEditable(ele) && !isFocusableElement(ele)) {
    return NS_OK;
  }
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if (eventType.EqualsLiteral("focus")) {
    IME_LOGD("-- GeckoEditableSupport::HandleEvent : focus");
    HandleFocus();
  }

  if (eventType.EqualsLiteral("blur")) {
    IME_LOGD("-- GeckoEditableSupport::HandleEvent : blur");
    HandleBlur();
  }
  return NS_OK;
}

void GeckoEditableSupport::HandleFocus() {
  RefPtr<nsInputContext> inputContext = new nsInputContext();
  nsresult rv = GetInputContextBag(inputContext);
  if (NS_FAILED(rv)) {
    return;
  }
  // oop
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetEditableSupportListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    child->SendRequest(CreateFocusRequestFromInputContext(inputContext));
    // TODO Unable to delete here, find somewhere else to do so.
  } else {
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = dom::InputMethodService::GetInstance();
    service->HandleFocus(this, static_cast<nsIInputContext*>(inputContext));
  }
}

void GeckoEditableSupport::HandleBlur() {
  // oop
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (contentChild) {
    InputMethodServiceChild* child = new InputMethodServiceChild();
    child->SetEditableSupportListener(this);
    contentChild->SendPInputMethodServiceConstructor(child);
    HandleBlurRequest request(contentChild->GetID());
    child->SendRequest(request);
    Unused << child->Send__delete__(child);
  } else {
    // Call from parent process (or in-proces app).
    RefPtr<InputMethodService> service = dom::InputMethodService::GetInstance();
    service->HandleBlur(this);
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

NS_IMETHODIMP
GeckoEditableSupport::DoSetSelectedOption(int32_t aOptionIndex) {
  IME_LOGD("-- EditableSupport DoSetSelectedOption");
  IME_LOGD("-- EditableSupport aOptionIndex:[%ld]", aOptionIndex);
  if (aOptionIndex < 0) {
    return NS_ERROR_ABORT;
  }
  nsFocusManager* focusManager = nsFocusManager::GetFocusManager();
  if (!focusManager) return NS_ERROR_ABORT;
  Element* focusedElement = focusManager->GetFocusedElement();
  if (!focusedElement) return NS_ERROR_ABORT;

  RefPtr<HTMLSelectElement> selectElement =
      HTMLSelectElement::FromNodeOrNull(focusedElement);
  if (!selectElement) {
    return NS_ERROR_ABORT;
  }
  // Safe to convert to unit since index is not negative
  if ((uint32_t)aOptionIndex > selectElement->Length()) {
    return NS_ERROR_ABORT;
  }
  if (selectElement->SelectedIndex() == aOptionIndex) {
    return NS_OK;
  }

  selectElement->SetSelectedIndex(aOptionIndex);

  nsCOMPtr<Document> document = selectElement->OwnerDoc();
  RefPtr<Event> event = NS_NewDOMEvent(document, nullptr, nullptr);
  event->InitEvent(u"change"_ns, true, true);
  event->SetTrusted(true);

  ErrorResult rv;
  document->DispatchEvent(*event);
  if (rv.Failed()) {
    IME_LOGD("Failed to dispatch change event!!!");
    return NS_ERROR_ABORT;
  }
  return NS_OK;
}

NS_IMETHODIMP
GeckoEditableSupport::DoSetSelectedOptions(
    const nsTArray<int32_t>& aOptionIndexes) {
  IME_LOGD("-- EditableSupport DoSetSelectedOption");
  IME_LOGD("-- EditableSupport aOptionIndexes.Length():[%ld]",
           aOptionIndexes.Length());
  nsFocusManager* focusManager = nsFocusManager::GetFocusManager();
  if (!focusManager) return NS_ERROR_ABORT;

  Element* focusedElement = focusManager->GetFocusedElement();
  if (!focusedElement) return NS_ERROR_ABORT;

  if (aOptionIndexes.Length() == 0) {
    return NS_ERROR_ABORT;
  }

  RefPtr<HTMLSelectElement> selectElement =
      HTMLSelectElement::FromNodeOrNull(focusedElement);
  if (!selectElement) {
    return NS_ERROR_ABORT;
  }

  // only fire onchange event if any selected option is changed
  bool changed = false;
  for (const auto& index : aOptionIndexes) {
    if (index < 0) {
      continue;
    }
    if (!selectElement->Item(index)) {
      continue;
    }
    if (!selectElement->Item(index)->Selected()) {
      changed = true;
      selectElement->Item(index)->SetSelected(true);
    }
  }
  if (!changed) {
    return NS_OK;
  }
  nsCOMPtr<Document> document = selectElement->OwnerDoc();
  RefPtr<Event> event = NS_NewDOMEvent(document, nullptr, nullptr);
  event->InitEvent(u"change"_ns, true, true);
  event->SetTrusted(true);

  ErrorResult rv;
  document->DispatchEvent(*event);
  if (rv.Failed()) {
    IME_LOGD("Failed to dispatch change event!!!");
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

nsresult GeckoEditableSupport::GetInputContextBag(
    nsInputContext* aInputContext) {
  nsFocusManager* focusManager = nsFocusManager::GetFocusManager();
  if (!focusManager) return NS_ERROR_ABORT;
  Element* focusedElement = focusManager->GetFocusedElement();
  if (!focusedElement) return NS_ERROR_ABORT;

  nsAutoString attributeValue;

  // type
  focusedElement->GetTagName(attributeValue);
  aInputContext->SetType(attributeValue);
  IME_LOGD("InputContext: type:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // inputType
  focusedElement->GetAttribute(u"type"_ns, attributeValue);
  aInputContext->SetInputType(attributeValue);
  IME_LOGD("InputContext: inputType:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // value
  focusedElement->GetAttribute(u"value"_ns, attributeValue);
  aInputContext->SetValue(attributeValue);
  IME_LOGD("InputContext: value:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // max Using string since if inputType is date then max could be string.
  focusedElement->GetAttribute(u"max"_ns, attributeValue);
  aInputContext->SetMax(attributeValue);
  IME_LOGD("InputContext: max:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // min Same as max
  focusedElement->GetAttribute(u"min"_ns, attributeValue);
  aInputContext->SetMin(attributeValue);
  IME_LOGD("InputContext: min:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // lang
  focusedElement->GetAttribute(u"lang"_ns, attributeValue);
  aInputContext->SetLang(attributeValue);
  IME_LOGD("InputContext: lang:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // inputMode
  focusedElement->GetAttribute(u"x-inputmode"_ns, attributeValue);
  aInputContext->SetInputMode(attributeValue);
  IME_LOGD("InputContext: inputMode:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // voiceInputSupported
  focusedElement->GetAttribute(u"voiceInputSupported"_ns, attributeValue);
  aInputContext->SetVoiceInputSupported(
      attributeValue.EqualsIgnoreCase("true"));
  IME_LOGD("InputContext: voiceInputSupported:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // name
  focusedElement->GetAttribute(u"name"_ns, attributeValue);
  aInputContext->SetName(attributeValue);
  IME_LOGD("InputContext: name:[%s]",
           NS_ConvertUTF16toUTF8(attributeValue).get());

  // selectionStart
  uint32_t start = getSelectionStart(focusedElement);
  aInputContext->SetSelectionStart(start);
  IME_LOGD("InputContext: selectionStart:[%lu]", start);

  // selectionEnd
  uint32_t end = getSelectionEnd(focusedElement);
  aInputContext->SetSelectionEnd(end);
  IME_LOGD("InputContext: selectionEnd:[%lu]", end);

  // Treat contenteditable element as a special text area field
  if (isContentEditable(focusedElement)) {
    aInputContext->SetType(u"contenteditable"_ns);
    aInputContext->SetInputType(u"textarea"_ns);
    nsresult rv = getContentEditableText(focusedElement, attributeValue);
    if (rv != NS_OK) {
      return rv;
    }
    aInputContext->SetValue(attributeValue);
    IME_LOGD("InputContext: value:[%s]",
             NS_ConvertUTF16toUTF8(attributeValue).get());
  }

  // Choices
  RefPtr<HTMLSelectElement> selectElement =
      HTMLSelectElement::FromNodeOrNull(focusedElement);
  if (!selectElement) {
    return NS_OK;
  }

  RefPtr<nsInputContextChoices> inputContextChoices =
      new nsInputContextChoices();
  nsTArray<RefPtr<nsIInputContextChoicesInfo>> choices;
  choices.Clear();

  bool isMultiple = selectElement->Multiple();
  inputContextChoices->SetMultiple(isMultiple);
  IME_LOGD("HTMLSelectElement: multiple:[%s]", isMultiple ? "true" : "false");
  nsCOMPtr<nsINodeList> children;
  children = focusedElement->ChildNodes();
  for (uint32_t i = 0; i < children->Length(); i++) {
    nsCOMPtr<nsINode> child = children->Item(i);
    RefPtr<HTMLOptGroupElement> optGroupElement =
        HTMLOptGroupElement::FromNodeOrNull(child);
    RefPtr<HTMLOptionElement> optElement =
        HTMLOptionElement::FromNodeOrNull(child);
    if (optGroupElement) {
      RefPtr<nsInputContextChoicesInfo> groupInfo =
          new nsInputContextChoicesInfo();
      groupInfo->SetGroup(true);
      IME_LOGD("groupInfo: group:[true]");
      optGroupElement->GetAttribute(u"label"_ns, attributeValue);
      groupInfo->SetLabel(attributeValue);
      IME_LOGD("groupInfo: label:[%s]",
               NS_ConvertUTF16toUTF8(attributeValue).get());
      groupInfo->SetDisabled(optGroupElement->Disabled());
      IME_LOGD("groupInfo: disabled:[%s]",
               optGroupElement->Disabled() ? "true" : "false");
      choices.AppendElement(groupInfo);
      nsCOMPtr<nsINodeList> subChildren;
      subChildren = optGroupElement->ChildNodes();
      for (uint32_t j = 0; j < subChildren->Length(); j++) {
        nsCOMPtr<nsINode> subChild = subChildren->Item(j);
        RefPtr<HTMLOptionElement> subOptElement =
            HTMLOptionElement::FromNodeOrNull(subChild);
        if (!subOptElement) {
          continue;
        }
        RefPtr<nsInputContextChoicesInfo> subOptInfo =
            new nsInputContextChoicesInfo();
        subOptInfo->SetGroup(false);
        IME_LOGD("subOptInfo: group:[false]");
        subOptInfo->SetInGroup(true);
        IME_LOGD("subOptInfo: inGroup:[true]");
        subOptElement->GetAttribute(u"label"_ns, attributeValue);
        subOptInfo->SetLabel(attributeValue);
        IME_LOGD("subOptInfo: label:[%s]",
                 NS_ConvertUTF16toUTF8(attributeValue).get());
        subOptElement->GetAttribute(u"value"_ns, attributeValue);
        subOptInfo->SetValue(attributeValue);
        IME_LOGD("subOptInfo: value:[%s]",
                 NS_ConvertUTF16toUTF8(attributeValue).get());
        subOptElement->GetText(attributeValue);
        subOptInfo->SetText(attributeValue);
        IME_LOGD("subOptInfo: text:[%s]",
                 NS_ConvertUTF16toUTF8(attributeValue).get());
        subOptInfo->SetDisabled(subOptElement->Disabled());
        IME_LOGD("subOptInfo: Disabled:[%s]",
                 subOptElement->Disabled() ? "true" : "false");
        subOptInfo->SetSelected(subOptElement->Selected());
        IME_LOGD("subOptInfo: Selected:[%s]",
                 subOptElement->Selected() ? "true" : "false");
        subOptInfo->SetDefaultSelected(subOptElement->DefaultSelected());
        IME_LOGD("subOptInfo: DefaultSelected:[%s]",
                 subOptElement->DefaultSelected() ? "true" : "false");
        int32_t subIndex = subOptElement->Index();
        subOptInfo->SetOptionIndex(subIndex);
        IME_LOGD("subOptInfo: optionIndex:[%ld]", subIndex);
        choices.AppendElement(subOptInfo);
      }
    } else if (optElement) {
      RefPtr<nsInputContextChoicesInfo> optInfo =
          new nsInputContextChoicesInfo();
      optInfo->SetGroup(false);
      IME_LOGD("optInfo: group:[false]");
      optInfo->SetInGroup(false);
      IME_LOGD("optInfo: inGroup:[false]");
      optElement->GetAttribute(u"label"_ns, attributeValue);
      optInfo->SetLabel(attributeValue);
      IME_LOGD("optInfo: label:[%s]",
               NS_ConvertUTF16toUTF8(attributeValue).get());
      optElement->GetAttribute(u"value"_ns, attributeValue);
      optInfo->SetValue(attributeValue);
      IME_LOGD("optInfo: value:[%s]",
               NS_ConvertUTF16toUTF8(attributeValue).get());
      optElement->GetText(attributeValue);
      optInfo->SetText(attributeValue);
      IME_LOGD("optInfo: text:[%s]",
               NS_ConvertUTF16toUTF8(attributeValue).get());
      optInfo->SetDisabled(optElement->Disabled());
      IME_LOGD("optInfo: Disabled:[%s]",
               optElement->Disabled() ? "true" : "false");
      optInfo->SetSelected(optElement->Selected());
      IME_LOGD("optInfo: Selected:[%s]",
               optElement->Selected() ? "true" : "false");
      optInfo->SetDefaultSelected(optElement->DefaultSelected());
      IME_LOGD("optInfo: DefaultSelected:[%s]",
               optElement->DefaultSelected() ? "true" : "false");
      int32_t optionIndex = optElement->Index();
      optInfo->SetOptionIndex(optionIndex);
      IME_LOGD("optInfo: optionIndex:[%ld]", optionIndex);
      choices.AppendElement(optInfo);
    }
  }
  inputContextChoices->SetChoices(choices);
  aInputContext->SetInputContextChoices(inputContextChoices);

  return NS_OK;
}

}  // namespace widget
}  // namespace mozilla
