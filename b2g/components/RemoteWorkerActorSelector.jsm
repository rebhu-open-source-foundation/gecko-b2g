/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

function RemoteWorkerActorSelector() {
  Services.obs.addObserver(embedderSelector => {
    this.embedderSelector = embedderSelector.wrappedJSObject;
  }, "web-embedder-set-remote-worker-actor-selector");
}

RemoteWorkerActorSelector.prototype = {
  classID: Components.ID("{cf4d5ab8-fbff-11eb-a8d5-e341e5172886}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsIRemoteWorkerActorSelector]),

  /**
   * Get a suggested content process for service worker to spawn at.
   * aScope: Scope of service worker which will be spawned.
   */
  getSuggestedPid(aScope) {
    // If delegate is not presented, return 0 as default
    // (which is select a hosting content process randomly).
    if (!this.embedderSelector) {
      return 0;
    }
    return this.embedderSelector.suggestedForHostingServiceWorker(aScope);
  },
};

var EXPORTED_SYMBOLS = ["RemoteWorkerActorSelector"];
