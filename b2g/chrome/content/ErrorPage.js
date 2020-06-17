/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global docShell, sendAsyncMessage, addMessageListener, content */
"use strict";

console.log("############ ErrorPage.js\n");

var ErrorPageHandler = {
  _reload() {
    docShell
      .QueryInterface(Ci.nsIWebNavigation)
      .reload(Ci.nsIWebNavigation.LOAD_FLAGS_NONE);
  },

  _certErrorPageEventHandler(e) {
    let target = e.originalTarget;
    let errorDoc = target.ownerDocument;

    // If the event came from an ssl error page, it is one of the "Add
    // Exception…" buttons.
    if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      let permanent = errorDoc.getElementById("permanentExceptionButton");
      let temp = errorDoc.getElementById("temporaryExceptionButton");
      if (target == temp || target == permanent) {
        sendAsyncMessage("ErrorPage:AddCertException", {
          url: errorDoc.location.href,
          isPermanent: target == permanent,
        });
      }
    }
  },

  _bindPageEvent(target) {
    if (!target) {
      return;
    }

    if (/^about:certerror/.test(target.documentURI)) {
      let errorPageEventHandler = this._certErrorPageEventHandler.bind(this);
      addEventListener("click", errorPageEventHandler, true, false);
      let listener = function() {
        removeEventListener("click", errorPageEventHandler, true);
        removeEventListener("pagehide", listener, true);
      };

      addEventListener("pagehide", listener, true);
    }
  },

  domContentLoadedHandler(e) {
    let target = e.originalTarget;
    let targetDocShell = target.defaultView
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation);
    if (targetDocShell != docShell) {
      return;
    }
    this._bindPageEvent(target);
  },

  init() {
    addMessageListener("ErrorPage:ReloadPage", this._reload.bind(this));
    addEventListener(
      "DOMContentLoaded",
      this.domContentLoadedHandler.bind(this),
      true
    );
    this._bindPageEvent(content.document);
  },
};

ErrorPageHandler.init();
