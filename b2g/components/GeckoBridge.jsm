/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["GeckoBridge"];

const kWatchedPrefs = [
  "app.update.custom",
  "b2g.version",
  "device.commercial.ref",
  "oem.software.version",
];

function log(msg) {
  dump(`GeckoBridge: ${msg}\n`);
}

this.GeckoBridge = {
  start() {
    log(`Starting GeckoBridge`);
    try {
      this._bridge = Cc["@mozilla.org/sidl-native/bridge;1"].getService(
        Ci.nsIGeckoBridge
      );
      this.setup();
    } catch (e) {
      log(`Failed to create GeckoBridge component: ${e}`);
    }
  },

  setup() {
    // Add a pref change listener for each of the watched prefs and sync the
    // initial value.
    kWatchedPrefs.forEach(pref => {
      Services.prefs.addObserver(pref, this);
      this.set_pref(pref);
    });
  },

  observe(subject, topic, data) {
    this.set_pref(data);
  },

  set_pref(pref) {
    let prefs = Services.prefs;
    let kind = prefs.getPrefType(pref);
    log(`Setting pref ${pref} (${kind})`);

    const callback = {
      resolve() {},
      reject() {
        log(`Failed to set pref ${pref}`);
      },
    };

    switch (kind) {
      case Ci.nsIPrefBranch.PREF_STRING:
        this._bridge.charPrefChanged(pref, prefs.getCharPref(pref), callback);
        break;
      case Ci.nsIPrefBranch.PREF_INT:
        this._bridge.intPrefChanged(pref, prefs.getIntPref(pref), callback);
        break;
      case Ci.nsIPrefBranch.PREF_BOOL:
        this._bridge.boolPrefChanged(pref, prefs.getBoolPref(pref), callback);
        break;
      default:
        log(`Unexpected pref type (${kind}) for ${pref}`);
    }
  },
};
