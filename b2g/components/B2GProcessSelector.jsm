/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Process selector for b2g.

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

function B2GProcessSelector() {
  Services.obs.addObserver(embedderSelector => {
    this.embedderSelector = embedderSelector.wrappedJSObject;
  }, "web-embedder-set-process-selector");
}

B2GProcessSelector.prototype = {
  classID: Components.ID("{dd87f882-9d09-49e5-989d-cfaaaf4425be}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsIContentProcessProvider]),

  provideProcess(aType, aProcesses, aMaxCount) {
    // Delegates to the embedder if possible, or just defaults to create a new process.
    if (!this.embedderSelector) {
      return Ci.nsIContentProcessProvider.NEW_PROCESS;
    }
    return this.embedderSelector.provideProcess(aType, aProcesses, aMaxCount);
  },

  suggestServiceWorkerProcess(aScope) {
    // If delegate is not presented, return 0 as default
    // (select a hosting content process randomly).
    if (!this.embedderSelector) {
      return 0;
    }
    return this.embedderSelector.suggestServiceWorkerProcess(aScope);
  },
};

var EXPORTED_SYMBOLS = ["B2GProcessSelector"];
