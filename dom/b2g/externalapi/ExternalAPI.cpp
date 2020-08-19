/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ExternalAPI.h"
#include "mozilla/dom/ExternalAPIBinding.h"

#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/dom/WebSocket.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "mozilla/dom/WorkerScope.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsIGlobalObject.h"
#include "nsIPermission.h"
#include "nsIPermissionManager.h"
#include "js/JSON.h"

#ifdef MOZ_WIDGET_GONK
#  include <cutils/properties.h>
#else
#  include "prenv.h"
#endif

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(ExternalAPI, mGlobal, mPromises)

NS_IMPL_CYCLE_COLLECTING_ADDREF(ExternalAPI)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ExternalAPI)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ExternalAPI)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
NS_INTERFACE_MAP_END

class PrepareMessageRunnable : public WorkerMainThreadRunnable {
  nsACString& mOrigin;
  nsACString& mToken;
  Sequence<nsString>& mPermissions;

 public:
  PrepareMessageRunnable(WorkerPrivate* aWorkerPrivate, nsACString& aOrigin,
                         nsACString& aToken, Sequence<nsString>& aPermissions)
      : WorkerMainThreadRunnable(aWorkerPrivate,
                                 "ExternalAPI :: Prepare Message"_ns),
        mOrigin(aOrigin),
        mToken(aToken),
        mPermissions(aPermissions) {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  bool MainThreadRun() override {
    AssertIsOnMainThread();

    nsIPrincipal* principal = mWorkerPrivate->GetPrincipal();
    if (!principal) {
      // Walk up to our containing page
      WorkerPrivate* wp = mWorkerPrivate;
      while (wp->GetParent()) {
        wp = wp->GetParent();
      }

      principal = wp->GetPrincipal();
    }

    ExternalAPI::GetOrigin(principal, mOrigin);
    ExternalAPI::GetPermissions(principal, mPermissions);
    ExternalAPI::GenerateToken(mToken);

    return true;
  }
};

class GetWebSocketServerInfoRunnable : public WorkerMainThreadRunnable {
  nsAString& mURL;
  nsAString& mProtocols;

 public:
  GetWebSocketServerInfoRunnable(WorkerPrivate* aWorkerPrivate, nsAString& aURL,
                                 nsAString& aProtocols)
      : WorkerMainThreadRunnable(aWorkerPrivate,
                                 "ExternalAPI :: Get websocket info"_ns),
        mURL(aURL),
        mProtocols(aProtocols) {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  bool MainThreadRun() override {
    AssertIsOnMainThread();

    ExternalAPI::GetWebSocketServerInfo(mURL, mProtocols);

    return true;
  }
};

ExternalAPI::ExternalAPI(nsIGlobalObject* aGlobal) : mGlobal(aGlobal) {
  MOZ_ASSERT(mGlobal);
}

ExternalAPI::~ExternalAPI() {
  NS_ASSERT_OWNINGTHREAD(ExternalAPI);

  for (uint32_t i = 0; i < mPromises.Length(); ++i) {
    mPromises[i]->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  }
  mPromises.Clear();
  mTokens.Clear();
}

/* static */ already_AddRefed<ExternalAPI> ExternalAPI::Create(
    nsIGlobalObject* aGlobal) {
  RefPtr<ExternalAPI> external = new ExternalAPI(aGlobal);
  return external.forget();
}

already_AddRefed<Promise> ExternalAPI::GetToken() {
  NS_ASSERT_OWNINGTHREAD(ExternalAPI);

  ErrorResult rv;
  RefPtr<Promise> promise = Promise::Create(mGlobal, rv);
  if (NS_WARN_IF(rv.Failed())) {
    return nullptr;
  }

#ifdef MOZ_WIDGET_GONK
  nsAutoString nsSecretKey;
  nsresult prv =
      Preferences::GetString("b2g.services.runtime.token", nsSecretKey);
  if (NS_FAILED(prv) || nsSecretKey.IsEmpty()) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }
  const char* secretKey = ToNewCString(nsSecretKey);
#else
  char* secretKey = PR_GetEnv("WS_RUNTIME_TOKEN");
  if (!secretKey) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }
#endif

  int len = strlen(secretKey);
  nsAutoString url;
  nsAutoString protocols;
  if (NS_IsMainThread()) {
    GetWebSocketServerInfo(url, protocols);
  } else {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
    ErrorResult rv;
    RefPtr<GetWebSocketServerInfoRunnable> runnable =
        new GetWebSocketServerInfoRunnable(workerPrivate, url, protocols);
    runnable->Dispatch(dom::WorkerStatus::Canceling, rv);

    if (rv.Failed()) {
      promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return promise.forget();
    }
  }

  url.Append(NS_ConvertUTF8toUTF16(Substring(secretKey, len)));

#ifdef MOZ_WIDGET_GONK
  free((void*)secretKey);
#endif

  AutoJSAPI jsapi;
  if (!jsapi.Init(mGlobal)) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }
  JSContext* cx = jsapi.cx();
  GlobalObject global(cx, mGlobal->GetGlobalJSObject());

  StringOrStringSequence protocols_param;
  protocols_param.SetAsString().AssignLiteral(protocols.get(),
                                              protocols.Length());
  RefPtr<WebSocket> socket =
      WebSocket::Constructor(global, url, protocols_param, rv);
  if (NS_WARN_IF(rv.Failed())) {
    promise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return promise.forget();
  }

  nsCOMPtr<EventTarget> target;
  target = socket;
  target->AddEventListener(u"open"_ns, this, true);
  target->AddEventListener(u"message"_ns, this, true);
  target->AddEventListener(u"error"_ns, this, true);
  target->AddEventListener(u"close"_ns, this, true);

  mPromises.AppendElement(promise);

  return promise.forget();
}

nsISupports* ExternalAPI::GetParentObject() const { return mGlobal; }

JSObject* ExternalAPI::WrapObject(JSContext* aContext,
                                  JS::Handle<JSObject*> aGivenProto) {
  return ExternalAPI_Binding::Wrap(aContext, this, aGivenProto);
}

// Remove all the event listeners and close the Web Socket
void ExternalAPI::CloseWebSocket(WebSocket* aWebSocket) {
  nsCOMPtr<EventTarget> target;
  target = aWebSocket;
  target->RemoveEventListener(u"open"_ns, this, true);
  target->RemoveEventListener(u"message"_ns, this, true);
  target->RemoveEventListener(u"error"_ns, this, true);
  target->RemoveEventListener(u"close"_ns, this, true);
  ErrorResult result;
  Optional<uint16_t> code;
  Optional<nsAString> reason;
  aWebSocket->Close(code, reason, result);
}

NS_IMETHODIMP
ExternalAPI::HandleEvent(Event* aEvent) {
  NS_ASSERT_OWNINGTHREAD(ExternalAPI);

  nsCOMPtr<EventTarget> target = aEvent->GetCurrentTarget();
  RefPtr<WebSocket> socket = static_cast<WebSocket*>(target.get());
  if (!socket) {
    PopAndRejectPromise();
    return NS_OK;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("open")) {
    // Connection has successful established with api-daemon server, so start
    // sending message to server.
    if (NS_WARN_IF(NS_FAILED(SendMessage(socket)))) {
      PopAndRejectPromise();
    }
    target->RemoveEventListener(u"open"_ns, this, true);
  } else if (eventType.EqualsLiteral("message")) {
    // Receive response from api-daemon server.
    RefPtr<MessageEvent> messageEvent = static_cast<MessageEvent*>(aEvent);
    if (NS_WARN_IF(NS_FAILED(ReceiveMessage(messageEvent)))) {
      PopAndRejectPromise();
      // Either rejected by server or something goes wrong during processing
      // server response, we should pop a token from token's stack.
      if (!mTokens.IsEmpty()) {
        mTokens.RemoveElementAt(0);
      }
    }
    // We expect a single response from the daemon, so close the socket now.
    CloseWebSocket(socket);
  } else if (eventType.EqualsLiteral("error") ||
             eventType.EqualsLiteral("close")) {
    PopAndRejectPromise();
    // A token is pushed to mTokens after successful sending the message to
    // server. If server is closed or returned with error before sending
    // message, there's no need to pop a token here.
    if (!mTokens.IsEmpty() && mTokens.Length() > mPromises.Length()) {
      mTokens.RemoveElementAt(0);
    }
    CloseWebSocket(socket);
  }

  return NS_OK;
}

/* static */ void ExternalAPI::GenerateToken(nsACString& aToken) {
  MOZ_ASSERT(NS_IsMainThread());

  nsID uuid;
  char uuidBuffer[NSID_LENGTH];
  nsCString asciiString;
  ErrorResult rv;

  rv = nsContentUtils::GenerateUUIDInPlace(uuid);
  if (rv.Failed()) {
    aToken.AssignLiteral("");
    return;
  }

  uuid.ToProvidedString(uuidBuffer);
  asciiString.AssignASCII(uuidBuffer);

  // Remove {} and the null terminator
  aToken.Assign(Substring(asciiString, 1, NSID_LENGTH - 3));
}

/* static */ void ExternalAPI::GetOrigin(nsIPrincipal* aPrincipal,
                                         nsACString& aOrigin) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!aPrincipal) {
    return;
  }

  ErrorResult rv;
  bool isNullPrincipal;
  rv = aPrincipal->GetIsNullPrincipal(&isNullPrincipal);
  if (NS_WARN_IF(rv.Failed()) || NS_WARN_IF(isNullPrincipal)) {
    return;
  }

  rv = aPrincipal->GetOriginNoSuffix(aOrigin);
}

/* static */ void ExternalAPI::GetPermissions(
    nsIPrincipal* aPrincipal, Sequence<nsString>& aPermissions) {
  MOZ_ASSERT(NS_IsMainThread());

  if (!aPrincipal) {
    return;
  }

  nsCOMPtr<nsIPermissionManager> permManager = services::GetPermissionManager();
  if (!permManager) {
    return;
  }

  // The permission set for the calling principal.
  nsTArray<RefPtr<nsIPermission>> permissions;

  if (aPrincipal->IsSystemPrincipal()) {
    // If this is the system principal the call was initiated by the system app.
    // In that situation we use a content principal for the system app url instead
    // to match with the PermissionInstaller code.
    nsAutoCString systemAppUrl;
    if (NS_FAILED(
            Preferences::GetCString("b2g.system_startup_url", systemAppUrl))) {
      // Should never happen...
      printf_stderr("Failed to retrieve value of b2g.system_startup_url !!\n");
      return;
    };

    nsCOMPtr<nsIPrincipal> contentPrincipal =
        BasePrincipal::CreateContentPrincipal(systemAppUrl);
    if (!contentPrincipal) {
      printf_stderr(
          "Failed to create content principal for the system app !!\n");
      return;
    }

    if (NS_FAILED(permManager->GetAllForPrincipal(contentPrincipal, permissions))) {
      return;
    }
  } else {
    if (NS_FAILED(permManager->GetAllForPrincipal(aPrincipal, permissions))) {
      return;
    }
  }

  // Add all the permissions granted to this principal.
  // TODO: filter only the ones that are managed by api-daemon services.
  for (const auto& permission : permissions) {
    if (!permission) {
      continue;
    }

    nsAutoCString permissionType;
    if (NS_FAILED(permission->GetType(permissionType))) {
      continue;
    }

    uint32_t capability = 0;
    if (NS_SUCCEEDED(permission->GetCapability(&capability)) &&
        capability == nsIPermissionManager::ALLOW_ACTION) {
      if (!aPermissions.AppendElement(NS_ConvertUTF8toUTF16(permissionType),
                                      fallible)) {
        // Allocation failure...
        return;
      }
    }
  }
}

/* static */ void ExternalAPI::GetWebSocketServerInfo(nsAString& aURL,
                                                      nsAString& aProtocols) {
  Preferences::GetString("externalAPI.websocket.url", aURL);
  Preferences::GetString("externalAPI.websocket.protocols", aProtocols);
}

nsresult ExternalAPI::SendMessage(WebSocket* aWebSocket) {
  NS_ASSERT_OWNINGTHREAD(ExternalAPI);

  nsAutoCString token;
  nsAutoCString origin;
  Sequence<nsString> permissions;
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIPrincipal> principal = mGlobal->PrincipalOrNull();
    GetOrigin(principal, origin);
    GetPermissions(principal, permissions);
    GenerateToken(token);
  } else {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();

    ErrorResult rv;
    RefPtr<PrepareMessageRunnable> runnable =
        new PrepareMessageRunnable(workerPrivate, origin, token, permissions);
    runnable->Dispatch(dom::WorkerStatus::Canceling, rv);

    if (rv.Failed()) {
      return NS_ERROR_FAILURE;
    }
  }

  if (token.IsEmpty() || origin.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  ExternalAPIClientInfo data;
  data.mToken.Construct(NS_ConvertUTF8toUTF16(token));
  data.mIdentity.Construct(NS_ConvertUTF8toUTF16(origin));
  data.mPermissions.Construct(permissions);

  nsAutoString json;
  data.ToJSON(json);

  // send message to api-daemon server in the following format:
  // { identity: "app_origin_no_suffix" , token: "new_generated_uuid"}
  ErrorResult rv;
  aWebSocket->Send(json, rv);
  if (NS_WARN_IF(rv.Failed())) {
    return NS_ERROR_FAILURE;
  }

  mTokens.AppendElement(token);

  return NS_OK;
}

nsresult ExternalAPI::ReceiveMessage(MessageEvent* aEvent) {
  NS_ASSERT_OWNINGTHREAD(ExternalAPI);

  if (!aEvent) {
    return NS_ERROR_FAILURE;
  }

  ErrorResult rv;
  AutoJSAPI jsapi;
  if (!jsapi.Init(mGlobal)) {
    return NS_ERROR_FAILURE;
  }
  JSContext* cx = jsapi.cx();
  JS::Rooted<JS::Value> data(cx);
  aEvent->GetData(cx, &data, rv);
  if (NS_WARN_IF(rv.Failed())) {
    return NS_ERROR_FAILURE;
  }

  JS::RootedString dataStr(cx, data.toString());
  JS::Rooted<JS::Value> json(cx);
  if (!JS_ParseJSON(cx, dataStr, &json)) {
    return NS_ERROR_FAILURE;
  }

  JS::RootedObject jsonObj(cx, &json.toObject());
  JS::Rooted<JS::Value> val(cx);
  if (!JS_GetProperty(cx, jsonObj, "result", &val) || !val.isBoolean()) {
    return NS_ERROR_FAILURE;
  }

  if (val.isFalse()) {
    // api-daemon server response with { "result": false }
    return NS_ERROR_FAILURE;
  }

  // api-daemon server response with { "result": true }
  // pop a promise from the stack(mPromises) and resolve it with a token,
  // which also poped from a stack of tokens(mTokens).
  if (mPromises.IsEmpty() || mTokens.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }
  mPromises[0]->MaybeResolve(NS_ConvertUTF8toUTF16(mTokens[0]));

  mPromises.RemoveElementAt(0);
  mTokens.RemoveElementAt(0);

  return NS_OK;
}

void ExternalAPI::PopAndRejectPromise() {
  if (!mPromises.IsEmpty()) {
    mPromises[0]->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    mPromises.RemoveElementAt(0);
  }
}

}  // namespace dom
}  // namespace mozilla