/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

const DEBUG = true;

function debug(aMsg) {
  if (DEBUG) {
    dump("-- InputMethodProxy: " + aMsg + "\n");
  }
}

/*
 * InputMethodProxy is a helper of passing requests from InputMethod.cpp to
 * InputMethodService.jsm, receiving results from InputMethodService.jsm and
 * send it back to InputMethod.cpp.
 */

function InputMethodProxy() {}

InputMethodProxy.prototype = {
  init: function init() {
    debug("init");

    // A <requestId, {windowId, callback}> map.
    this._requests = new Map();
    // A <windowId, {window}> map.
    this._windows = new Map();
    this._counter = 0;
    this._messageNames = [
      "InputMethod:SetComposition:Return:OK",
      "InputMethod:SetComposition:Return:KO",
      "InputMethod:EndComposition:Return:OK",
      "InputMethod:EndComposition:Return:KO",
      "InputMethod:SendKey:Return:OK",
      "InputMethod:SendKey:Return:KO",
      "InputMethod:Keydown:Return:OK",
      "InputMethod:Keydown:Return:KO",
      "InputMethod:Keyup:Return:OK",
      "InputMethod:Keyup:Return:KO",
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
      let util = aOwner.windowUtils;
      let innerWindowID = util.currentInnerWindowID;
      this._windows.set(innerWindowID, aOwner);
    }
  },

  detach: function detach(aId) {
    debug("detatch" + aId);
    if (this._windows.has(aId)) {
      this._windows.delete(aId);
    }
  },

  setComposition(aOwner, aText, aCallback) {
    debug("setComposition: [" + aText + "]");
    debug("setComposition: aCallback[" + aCallback + "]");
    this._counter++;
    let requestId = this._counter;
    let util;
    let innerWindowID;
    if (aOwner) {
      util = aOwner.windowUtils;
      innerWindowID = util.currentInnerWindowID;
    }

    this._requests.set(requestId, {
      windowId: innerWindowID ? innerWindowID : {},
      callback: aCallback,
    });
    debug("setComposition: requestId:[" + requestId + "]");
    debug("setComposition: windowId:[" + innerWindowID + "]");
    Services.cpmm.sendAsyncMessage("InputMethod:SetComposition", {
      requestId,
      text: aText,
    });
  },

  endComposition(aOwner, aText, aCallback) {
    debug("endComposition: [" + aText + "]");
    debug("endComposition: aCallback[" + aCallback + "]");
    this._counter++;
    let requestId = this._counter;
    let util;
    let innerWindowID;
    if (aOwner) {
      util = aOwner.windowUtils;
      innerWindowID = util.currentInnerWindowID;
    }

    this._requests.set(requestId, {
      windowId: innerWindowID ? innerWindowID : {},
      callback: aCallback,
    });
    debug("endComposition: requestId:[" + requestId + "]");
    debug("endComposition: windowId:[" + innerWindowID + "]");
    Services.cpmm.sendAsyncMessage("InputMethod:EndComposition", {
      requestId,
      text: aText || "",
    });
  },

  sendKey(aOwner, aKey, aCallback) {
    debug("SendKey: [" + aKey + "]");
    debug("SendKey: aCallback[" + aCallback + "]");
    this._counter++;
    let requestId = this._counter;
    let util;
    let innerWindowID;
    if (aOwner) {
      util = aOwner.windowUtils;
      innerWindowID = util.currentInnerWindowID;
    }

    this._requests.set(requestId, {
      windowId: innerWindowID ? innerWindowID : {},
      callback: aCallback,
    });
    debug("SendKey: requestId:[" + requestId + "]");
    debug("SendKey: windowId:[" + innerWindowID + "]");
    Services.cpmm.sendAsyncMessage("InputMethod:SendKey", {
      requestId,
      key: aKey,
    });
  },

  keydown(aOwner, aKey, aCallback) {
    debug("Keydown: [" + aKey + "]");
    debug("Keydown: aCallback[" + aCallback + "]");
    this._counter++;
    let requestId = this._counter;
    let util;
    let innerWindowID;
    if (aOwner) {
      util = aOwner.windowUtils;
      innerWindowID = util.currentInnerWindowID;
    }

    this._requests.set(requestId, {
      windowId: innerWindowID ? innerWindowID : {},
      callback: aCallback,
    });
    debug("Keydown: requestId:[" + requestId + "]");
    debug("Keydown: windowId:[" + innerWindowID + "]");
    Services.cpmm.sendAsyncMessage("InputMethod:Keydown", {
      requestId,
      key: aKey,
    });
  },

  keyup(aOwner, aKey, aCallback) {
    debug("Keyup: [" + aKey + "]");
    debug("Keyup: aCallback[" + aCallback + "]");
    this._counter++;
    let requestId = this._counter;
    let util;
    let innerWindowID;
    if (aOwner) {
      util = aOwner.windowUtils;
      innerWindowID = util.currentInnerWindowID;
    }

    this._requests.set(requestId, {
      windowId: innerWindowID ? innerWindowID : {},
      callback: aCallback,
    });
    debug("Keyup: requestId:[" + requestId + "]");
    debug("Keyup: windowId:[" + innerWindowID + "]");
    Services.cpmm.sendAsyncMessage("InputMethod:Keyup", {
      requestId,
      key: aKey,
    });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage: " + aMessage.name);
    let json = aMessage.json ? aMessage.json : {};
    let request = this._requests.get(json.requestId);
    if (!request) {
      debug("no request found for requestId: " + json.requestId);
      return;
    }
    let win = this._windows.get(request.windowId);
    if (!win) {
      debug("no window found for windowId: " + request.windowId);
      return;
    }
    debug(
      "Request: [" + aMessage.name + "] with requestId:[" + json.requestId + "]"
    );
    switch (aMessage.name) {
      case "InputMethod:SetComposition:Return:OK":
        request.callback.onSetComposition(
          Cr.NS_OK,
          Cu.cloneInto(json.result, win)
        );
        break;
      case "InputMethod:SetComposition:Return:KO":
        request.callback.onSetComposition(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, win)
        );
        break;
      case "InputMethod:EndComposition:Return:OK":
        request.callback.onEndComposition(
          Cr.NS_OK,
          Cu.cloneInto(json.result, win)
        );
        break;
      case "InputMethod:EndComposition:Return:KO":
        request.callback.onEndComposition(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, win)
        );
        break;
      case "InputMethod:SendKey:Return:OK":
        request.callback.onSendKey(Cr.NS_OK, Cu.cloneInto(json.result, win));
        break;
      case "InputMethod:SendKey:Return:KO":
        request.callback.onSendKey(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, win)
        );
        break;
      case "InputMethod:Keydown:Return:OK":
        request.callback.onKeydown(Cr.NS_OK, Cu.cloneInto(json.result, win));
        break;
      case "InputMethod:Keydown:Return:KO":
        request.callback.onKeydown(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, win)
        );
        break;
      case "InputMethod:Keyup:Return:OK":
        request.callback.onKeyup(Cr.NS_OK, Cu.cloneInto(json.result, win));
        break;
      case "InputMethod:Keyup:Return:KO":
        request.callback.onKeyup(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, win)
        );
        break;
      default:
        debug("No matching message: " + aMessage.name);
        break;
    }
    // Delete mapping when request is fulfilled.
    this._requests.delete(json.requestId);
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

  contractID: "@mozilla.org/dom/inputmethod/proxy;1",

  classID: Components.ID("{960e39b0-e4ff-11ea-8c0e-b7c5f54cec64}"),

  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIInputMethodProxy,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

var EXPORTED_SYMBOLS = ["InputMethodProxy"];
