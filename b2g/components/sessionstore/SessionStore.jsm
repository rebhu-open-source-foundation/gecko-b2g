/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

var EXPORTED_SYMBOLS = ["SessionStore"];

var SessionStore = {
  updateSessionStoreFromTablistener(
    aBrowser,
    aBrowsingContext,
    aPermanentKey,
    aData
  ) {
    // No return value expected.
  },

  // Used by remote/cdp/domains/parent/Page.jsm
  getSessionHistory(tab, updatedCallback) {
    // Don't do anything for now.
  },
};

// Freeze the SessionStore object. We don't want anyone to modify it.
Object.freeze(SessionStore);
