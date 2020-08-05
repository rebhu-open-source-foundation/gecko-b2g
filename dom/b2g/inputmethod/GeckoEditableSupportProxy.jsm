/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

const DEBUG = true;

function debug(aMsg) {
  if (DEBUG) {
    dump("-- GeckoEditableSupportProxy: " + aMsg + "\n");
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
    ];
    this._messageNames.forEach(aName => {
      Services.cpmm.addMessageListener(aName, this);
    });
    Services.obs.addObserver(this, "inner-window-destroyed", 
                             /* weak-ref */ true);
    Services.obs.addObserver(this, "xpcom-shutdown", 
                             /* weak-ref */ true);
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

  setComposition(aOwner, aCallback) {
    debug("setComposition()");
    if (aOwner) {
      let util = aOwner.windowUtils;
      let innerWindowID = util.currentInnerWindowID;
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
      let util = aOwner.windowUtils;
      let innerWindowID = util.currentInnerWindowID;
      this._windows.set(innerWindowID, aOwner);
      this._requests.set("Forms:EndComposition", {
        windowId: innerWindowID ? innerWindowID : {},
        callback: aCallback,
      });
    }
  },

  handleFocus() {
    debug("handleFocus");
    Services.cpmm.sendAsyncMessage("Forms:Focus",{
      isFocus: true,
    });
  },

  handleBlur() {
    debug("handleBlur");
    Services.cpmm.sendAsyncMessage("Forms:Blur",{
      isFocus: false,
    });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage: " + aMessage.name);
    let mm = aMessage.target;
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
        Services.cpmm.sendAsyncMessage("Forms:SetComposition:Return:OK",{
          requestId: json.requestId,
          result: json.result,
        });
        break;
      case "Forms:EndComposition":
        debug("Forms:EndComposition with: " + JSON.stringify(json));
        request.callback.onEndComposition(json.text);
        Services.cpmm.sendAsyncMessage("Forms:EndComposition:Return:OK",{
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

  classID: Components.ID("{ef8d32a2-e34c-11ea-bbc6-171ffb5b97bf}"),

  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIGeckoEditableSupportProxy,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

var EXPORTED_SYMBOLS = ["GeckoEditableSupportProxy"];
