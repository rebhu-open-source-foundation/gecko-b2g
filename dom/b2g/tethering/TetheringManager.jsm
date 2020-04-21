/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { DOMRequestIpcHelper } = ChromeUtils.import(
  "resource://gre/modules/DOMRequestHelper.jsm"
);

XPCOMUtils.defineLazyGetter(this, "cpmm", () => {
  return Cc["@mozilla.org/childprocessmessagemanager;1"].getService();
});

const DEBUG = false;

const TETHERING_TYPE_WIFI = "wifi";
const TETHERING_TYPE_BLUETOOTH = "bt";
const TETHERING_TYPE_USB = "usb";

function TetheringManager() {
  this.defineEventHandlerGetterSetter("ontetheringstatuschange");
}

TetheringManager.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,
  classDescription: "TetheringManager",
  classID: Components.ID("{bd8a831c-d8ec-4f00-8803-606e50781097}"),
  contractID: "@mozilla.org/dom/tetheringmanager;1",
  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIDOMGlobalPropertyInitializer,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),

  _window: null,

  init(aWindow) {
    const messages = [
      "WifiManager:setWifiTethering:Return:OK",
      "WifiManager:setWifiTethering:Return:NO",
      "TetheringService:setUsbTethering:Return:OK",
      "TetheringService:setUsbTethering:Return:NO",
      "TetheringService:tetheringstatuschange",
    ];

    this._window = aWindow;
    this.initDOMRequestHelper(aWindow, messages);
    var state = cpmm.sendSyncMessage("TetheringService:getStatus")[0];
    if (state) {
      this._wifiTetheringState = state.wifiTetheringState;
      this._usbTetheringState = state.usbTetheringState;
    } else {
      this._wifiTetheringState = Ci.nsITetheringService.TETHERING_STATE_INACTIVE;
      this._usbTetheringState = Ci.nsITetheringService.TETHERING_STATE_INACTIVE;
    }
  },

  receiveMessage(aMessage) {
    switch (aMessage.name) {
      case "WifiManager:setWifiTethering:Return:OK":
      case "TetheringService:setUsbTethering:Return:OK":
        this.handlePromiseMessage(true, aMessage);
        break;
      case "WifiManager:setWifiTethering:Return:NO":
      case "TetheringService:setUsbTethering:Return:NO":
        this.handlePromiseMessage(false, aMessage);
        break;
      case "TetheringService:tetheringstatuschange":
        let msg = aMessage.json;
        if (msg.mid && msg.mid != this._id) {
          return;
        }

        let request;
        if (msg.rid) {
          request = this.takeRequest(msg.rid);
          if (!request) {
            return;
          }
        }
        this._wifiTetheringState = msg.wifiTetheringState;
        this._usbTetheringState = msg.usbTetheringState;
        this._fireTetheringStatusChangeEvent(msg);
        break;
    }
  },

  setTetheringEnabled: function setTetheringEnabled(aEnabled, aType, aConfig) {
    let self = this;
    switch (aType) {
      case TETHERING_TYPE_WIFI:
        return this.createPromiseWithId(function(aResolverId) {
          let data = {
            resolverId: aResolverId,
            enabled: aEnabled,
            config: aConfig,
          };
          cpmm.sendAsyncMessage("WifiManager:setWifiTethering", { data });
        });
      case TETHERING_TYPE_USB:
        return this.createPromiseWithId(function(aResolverId) {
          let data = {
            resolverId: aResolverId,
            enabled: aEnabled,
            config: aConfig,
          };
          cpmm.sendAsyncMessage("TetheringService:setUsbTethering", {
            data,
          });
        });
      case TETHERING_TYPE_BLUETOOTH:
      default:
        debug("tethering type(" + aType + ") doesn't support");
        return this.createPromiseWithId(function(aResolverId) {
          self.takePromiseResolver(aResolverId).reject();
        });
    }
  },

  handlePromiseMessage: function handlePromise(aResolve, aMessage) {
    let data = aMessage.data.data;
    let resolver = this.takePromiseResolver(data.resolverId);
    if (!resolver) {
      return;
    }
    if (aResolve) {
      resolver.resolve(data);
    } else {
      resolver.reject(data.reason);
    }
  },

  get wifiTetheringState() {
    return this._wifiTetheringState;
  },

  get usbTetheringState() {
    return this._usbTetheringState;
  },

  _fireTetheringStatusChangeEvent: function onTetheringStatusChangeEvent(info) {
    var event = new this._window.TetheringStatusChangeEvent(
      "tetheringstatuschange",
      { tetheringState: info.tetheringState }
    );
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  defineEventHandlerGetterSetter(name) {
    Object.defineProperty(this, name, {
      get() {
        return this.__DOM_IMPL__.getEventHandler(name);
      },
      set(handler) {
        this.__DOM_IMPL__.setEventHandler(name, handler);
      },
    });
  },
};

this.EXPORTED_SYMBOLS = ["TetheringManager"];

var debug;
if (DEBUG) {
  debug = function(s) {
    console.log("-*- TetheringManager component: ", s, "\n");
  };
} else {
  debug = function(s) {};
}
