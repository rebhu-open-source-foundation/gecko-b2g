/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

const DEBUG = true;

function debug(aMsg) {
  if (DEBUG) {
    dump(`-- IME GeckoEditableSupportProxy: ${aMsg}\n`);
  }
}

/*
 * GeckoEditableSupportProxy is a helper of passing requests from
 * GeckoEditableSupport.cpp to InputMethodService.jsm, receiving results from
 * InputMethodService.jsm and send it back to GeckoEditableSupport.cpp.
 */

function GeckoEditableSupportProxy() {}

GeckoEditableSupportProxy.prototype = {
  init: function init() {
    debug("init");
    // A <windowId, {window}> map.
    this._windows = new Map();
    // A <messageName, {windowId, callback}> map.
    this._requests = new Map();
    this._messageNames = [
      "Forms:SetComposition",
      "Forms:EndComposition",
      "Forms:SendKey",
      "Forms:Keydown",
      "Forms:Keyup",
    ];
    this._messageNames.forEach(aName => {
      Services.cpmm.addMessageListener(aName, this);
    });
    Services.obs.addObserver(
      this,
      "inner-window-destroyed",
      /* weak-ref */ true
    );
    Services.obs.addObserver(this, "xpcom-shutdown", /* weak-ref */ true);
  },

  attach: function attach(aOwner) {
    debug("attatch");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
    }
  },

  detach: function detach(aId) {
    debug("detatch" + aId);
    if (this._windows.has(aId)) {
      this._windows.delete(aId);
    }
  },

  setComposition(aOwner, aCallback) {
    debug("setComposition()");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:SetComposition", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  endComposition(aOwner, aCallback) {
    debug("endComposition()");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:EndComposition", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  sendKey(aOwner, aCallback) {
    debug("sendKey()");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:SendKey", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  keydown(aOwner, aCallback) {
    debug("keydown()");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:Keydown", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  keyup(aOwner, aCallback) {
    debug("keyup()");
    if (aOwner) {
      let innerWindowID = aOwner.windowGlobalChild.innerWindowId;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:Keyup", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  handleFocus() {
    debug("handleFocus");
    Services.cpmm.sendAsyncMessage("Forms:Focus", {
      isFocus: true,
    });
  },

  handleBlur() {
    debug("handleBlur");
    Services.cpmm.sendAsyncMessage("Forms:Blur", {
      isFocus: false,
    });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage: " + aMessage.name);
    let json = aMessage.json ? aMessage.json : {};
    let request = this._requests.get(aMessage.name);
    if (!request) {
      debug("no request found for [" + aMessage.name + "]");
      return;
    }
    let win = this._windows.get(request.windowId);
    if (!win) {
      debug("no window found for windowId: " + request.windowId);
      return;
    }
    switch (aMessage.name) {
      case "Forms:SetComposition":
        debug("Forms:SetComposition with: " + JSON.stringify(json));
        request.callback.onSetComposition(json.text);
        Services.cpmm.sendAsyncMessage("Forms:SetComposition:Return:OK", {
          requestId: json.requestId,
          result: json.result,
        });
        break;
      case "Forms:EndComposition":
        debug("Forms:EndComposition with: " + JSON.stringify(json));
        request.callback.onEndComposition(json.text);
        Services.cpmm.sendAsyncMessage("Forms:EndComposition:Return:OK", {
          requestId: json.requestId,
          result: json.result,
        });
        break;
      case "Forms:SendKey":
        debug("Forms:SendKey with: " + JSON.stringify(json));
        request.callback.onSendKey(json.key);
        Services.cpmm.sendAsyncMessage("Forms:SendKey:Return:OK", {
          requestId: json.requestId,
          result: json.result,
        });
        break;
      case "Forms:Keydown":
        debug("Forms:Keydown with: " + JSON.stringify(json));
        request.callback.onKeydown(json.key);
        Services.cpmm.sendAsyncMessage("Forms:Keydown:Return:OK", {
          requestId: json.requestId,
          result: json.result,
        });
        break;
      case "Forms:Keyup":
        debug("Forms:Keyup with: " + JSON.stringify(json));
        request.callback.onKeyup(json.key);
        Services.cpmm.sendAsyncMessage("Forms:Keyup:Return:OK", {
          requestId: json.requestId,
          result: json.result,
        });
        break;
      default:
        debug("No matching message: " + aMessage.name);
        break;
    }
  },

  observe(aSubject, aTopic, aData) {
    debug("observe " + aTopic);
    if (aTopic === "inner-window-destroyed") {
      let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (!this._windows.has(wId)) {
        return;
      }
      this.detach(wId);
    }
    if (aTopic === "xpcom-shutdown") {
      Services.obs.removeObserver(this, "inner-window-destroyed");
      Services.obs.removeObserver(this, "xpcom-shutdown");
      this._messageNames.forEach(aName => {
        Services.cpmm.removeMessageListener(aName, this);
      });
      this._requests.clear();
      this._windows.clear();
    }
  },

  contractID: "@mozilla.org/dom/inputmethod/supportproxy;1",

  classID: Components.ID("{821361d8-e4ff-11ea-89db-9f28f78da76a}"),

  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIGeckoEditableSupportProxy,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

var EXPORTED_SYMBOLS = ["GeckoEditableSupportProxy"];
