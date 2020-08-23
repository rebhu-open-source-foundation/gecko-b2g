/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { AppConstants } = ChromeUtils.import(
  "resource://gre/modules/AppConstants.jsm"
);

const DEBUG = true;

function debug(aMsg) {
  if (DEBUG) {
    dump("-- InputMethodService: " + aMsg + "\n");
  }
}

this.EXPORTED_SYMBOLS = ["InputMethodService"];

this.InputMethodService = {
  _formMM: null,      // The current web page message manager.
  _keyboardMM: null,  // The keyboard app message manager.
  _messageNames: [
    // InputMethodProxy.jsm
    "InputMethod:SetComposition",
    "InputMethod:EndComposition",
    "InputMethod:SendKey",
    "InputMethod:Keydown",
    "InputMethod:Keyup",
    // From geckosupporteditable proxy
    "Forms:Focus",
    "Forms:SetComposition:Return:OK",
    "Forms:SetComposition:Return:KO",
    "Forms:EndComposition:Return:OK",
    "Forms:EndComposition:Return:KO",
    "Forms:SendKey:Return:OK",
    "Forms:SendKey:Return:KO",
    "Forms:Keydown:Return:OK",
    "Forms:Keydown:Return:KO",
    "Forms:Keyup:Return:OK",
    "Forms:Keyup:Return:KO",
  ],

  get formMM() {
    if (this._formMM)
      return this._formMM;

    return null;
  },

  set formMM(mm) {
    this._formMM = mm;
  },

  sendToForm: function(name, data) {
    if (!this.formMM) {
      debug("No form message manager for: " + name);
      return;
    }
    try {
      this.formMM.sendAsyncMessage(name, data);
    } catch(e) {
      debug("Error while sendToForm");
    }
  },

  get inputMethodMM() {
    if (this._inputMethodMM)
      return this._inputMethodMM;

    return null;
  },

  set inputMethodMM(mm) {
    this._inputMethodMM = mm;
  },

  sendToInputMethod: function(name, data) {
    if (!this.inputMethodMM) {
      debug("No inputMethod message manager for: " + name);
      return;
    }
    try {
      this.inputMethodMM.sendAsyncMessage(name, data);
    } catch(e) {
      debug("Error while sendToInputMethod");
    }
  },

  init: function inputmethod_init() {
    debug("init()");
    this._messageNames.forEach(function(msgName) {
      Services.ppmm.addMessageListener(msgName, this);
    }, this);

    Services.obs.addObserver(this, "xpcom-shutdown");
    this.callers = {};
  },

  observe: function inputmethod_observe(aSubject, aTopic, aData) {
    debug("observe(): " + aTopic);
    switch (aTopic) {
      case "xpcom-shutdown":
        this.uninit();
        break;
    }
  },

  receiveMessage: function inputmethod_receiveMessage(aMessage) {
    debug("receiveMessage(): " + aMessage.name);
    let mm = aMessage.target;
    let msg = aMessage.json;

    switch (aMessage.name) {
      case 'Forms:Focus':
        this.handleFocus(mm, msg);
        break;
      case 'Forms:Blur':
        this.handleBlur(mm, msg);
        break;
      case "InputMethod:SetComposition":
        this.setComposition(mm, msg);
        break;
      case "InputMethod:EndComposition":
        this.endComposition(mm, msg);
        break;
      case "InputMethod:SendKey":
        this.sendKey(mm, msg);
        break;
      case "InputMethod:Keydown":
        this.keydown(mm, msg);
        break;
      case "InputMethod:Keyup":
        this.keyup(mm, msg);
        break;
      case 'Forms:SetComposition:Return:OK':
      case 'Forms:EndComposition:Return:OK':
      case 'Forms:SetComposition:Return:KO':
      case 'Forms:EndComposition:Return:KO':
      case 'Forms:SendKey:Return:KO':
      case 'Forms:SendKey:Return:KO':
      case 'Forms:Keydown:Return:KO':
      case 'Forms:Keydown:Return:KO':
      case 'Forms:Keyup:Return:KO':
      case 'Forms:Keyup:Return:KO':
        let name = aMessage.name.replace(/^Forms/, 'InputMethod');
        this.sendToInputMethod(name, {
          requestId: msg.requestId,
          result: msg.result,
          error: msg.error,
        });
        break;
    }
  },

  handleFocus: function keyboardHandleFocus(aMessageManager, msg) {
    debug("handleFocus()");
    if (aMessageManager === this.formMM) {
      debug("Double focus, early return");
      return;
    }

    this.formMM = aMessageManager;

    // Notify system to show keyboard when focus
    Services.obs.notifyObservers(
        { wrappedJSObject: msg },
        "inputmethod-contextchange"
    );
    debug("Notify system to show keyboard when focus");
  },

  handleBlur: function keyboardHandleBlur(aMessageManager, msg) {
    debug("handleBlur()");
    if (aMessageManager !== this.formMM) {
      debug("Blur with different target of focus");
      return;
    }

    this.formMM = null;

    // Notify system to hide keyboard when blur
    Services.obs.notifyObservers(
        { wrappedJSObject: msg },
        "inputmethod-contextchange"
    );
    debug("Notify system to show keyboard when blur");
  },

  setComposition: function inputmethod_setcomposition(aMessageManager, aMsg) {
    debug("setComposition() with: " + JSON.stringify(aMsg));
    if (aMessageManager != this.inputMethodMM) {
      this.inputMethodMM = aMessageManager;
    }
    this.sendToForm('Forms:SetComposition', aMsg);
  },

  endComposition: function inputmethod_endComposition(aMessageManager, msg) {
    debug("endComposition()");
    if (aMessageManager != this.inputMethodMM) {
      this.inputMethodMM = aMessageManager;
    }
    this.sendToForm('Forms:EndComposition', msg);
  },

  sendKey: function inputmethod_sendKey(aMessageManager, msg) {
    debug("sendKey()");
    if (aMessageManager != this.inputMethodMM) {
      this.inputMethodMM = aMessageManager;
    }
    this.sendToForm('Forms:SendKey', msg);
  },

  keydown: function inputmethod_keydown(aMessageManager, msg) {
    debug("keydown()");
    if (aMessageManager != this.inputMethodMM) {
      this.inputMethodMM = aMessageManager;
    }
    this.sendToForm('Forms:Keydown', msg);
  },

  keyup: function inputmethod_keyup(aMessageManager, msg) {
    debug("keyup()");
    if (aMessageManager != this.inputMethodMM) {
      this.inputMethodMM = aMessageManager;
    }
    this.sendToForm('Forms:Keyup', msg);
  },

  uninit: function inputmethod_uninit() {
    debug("uninit()");
    this._messageNames.forEach(function(msgName) {
      Services.ppmm.removeMessageListener(msgName, this);
    }, this);

    Services.obs.removeObserver(this, "xpcom-shutdown");
  },


};

InputMethodService.init();