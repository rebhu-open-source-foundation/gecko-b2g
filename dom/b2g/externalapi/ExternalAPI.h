/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_ExternalAPI_h
#define mozilla_dom_ExternalAPI_h

#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ExternalAPIBinding.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;
class WebSocket;
class MessageEvent;
class PrepareMessageRunnable;
class GetWebSocketServerInfoRunnable;

class ExternalAPI final : public nsIDOMEventListener,
                          public nsWrapperCache

{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ExternalAPI)

  static already_AddRefed<ExternalAPI> Create(nsIGlobalObject* aGlobal);

  // webidl
  already_AddRefed<Promise> GetToken();

  // binding methods
  nsISupports* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aContext,
                               JS::Handle<JSObject*> aGivenProto) override;

  friend PrepareMessageRunnable;
  friend GetWebSocketServerInfoRunnable;

 private:
  explicit ExternalAPI(nsIGlobalObject* aGlobal);
  ~ExternalAPI();

  static void GenerateToken(nsACString& aToken);
  static void GetOrigin(nsIPrincipal* aPrincipal, nsACString& aOrigin);
  static void GetPermissions(nsIPrincipal* aPrincipal,
                             Sequence<nsString>& aPermissions);
  static void GetWebSocketServerInfo(nsAString& aURL, nsAString& aProtocols);
  nsresult SendMessage(WebSocket* aWebSocket);
  nsresult ReceiveMessage(MessageEvent* aEvent);
  void PopAndRejectPromise();
  void CloseWebSocket(WebSocket* aWebSocket);

  nsCOMPtr<nsIGlobalObject> mGlobal;
  nsTArray<RefPtr<Promise>> mPromises;
  nsTArray<nsCString> mTokens;
};

}  // namespace dom
}  // namespace mozilla
#endif
