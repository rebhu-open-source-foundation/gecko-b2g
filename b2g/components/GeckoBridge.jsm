/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["GeckoBridge"];

/*
  Preferences listed in kWatchedPrefs can be accessed by sidl.
  Please put the preferences in alphabetical order.
  When adding a new preference name, please follow the naming conventions:
  - Use dot "." to hierarchically distinguish different types of groups.
  - Use hyphen "-" to join two or more words together into a compound term.
  If the preference is already in upstream, then just follow upstream.
*/
const kWatchedPrefs = [
  "app.update.custom",
  "b2g.version",
  "device.bt",
  "device.cdma-apn",
  "device.commercial.ref",
  "device.flip",
  "device.fm.recorder",
  "device.gps",
  "device.group-message",
  "device.key.camera",
  "device.key.endcall",
  "device.key.volume",
  "device.parental-control",
  "device.qwerty",
  "device.readout",
  "device.rtt",
  "device.sim-hotswap",
  "device.storage.size",
  "device.tethering",
  "device.torch",
  "device.vilte",
  "device.volte",
  "device.vowifi",
  "device.wifi",
  "oem.software.version",
  "ril.support.primarysim.switch",
];

function log(msg) {
  dump(`GeckoBridge: ${msg}\n`);
}

this.GeckoBridge = {
  start() {
    log(`Starting`);
    try {
      this._bridge = Cc["@mozilla.org/sidl-native/bridge;1"].getService(
        Ci.nsIGeckoBridge
      );

      this._powerManagerDelegate = Cc[
        "@mozilla.org/sidl-native/powermanager;1"
      ].getService(Ci.nsIPowerManagerDelegate);

      this._cardInfoManagerDelegate = Cc[
        "@mozilla.org/sidl-native/cardinfomanager;1"
      ].getService(Ci.nsICardInfoManagerDelegate);

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
      this.setPref(pref);
    });

    this.setPowerManagerDelegate();
    this.setCardInfoManagerDelegate();
  },

  observe(subject, topic, data) {
    this.setPref(data);
  },

  generateLoggingCallback(delegate) {
    return {
      resolve() {
        log(`Success setting ${delegate}`);
      },
      reject() {
        log(`Failure setting ${delegate}`);
      },
    }
  },

  setCardInfoManagerDelegate() {
    if (!this._bridge || !this._cardInfoManagerDelegate) {
      log(`Invalid cardinfomanager delegate`);
      return;
    }

    if (this._bridge) {
      log(`Setting CardInfoManagerDelegate`);
      this._bridge.setCardInfoManagerDelegate(
        this._cardInfoManagerDelegate,
        this.generateLoggingCallback("CardInfoManagerDelegate")
      );
    }
  },

  setPowerManagerDelegate() {
    if (!this._bridge || !this._powerManagerDelegate) {
      log(`Invalid powermanager delegate`);
      return;
    }

    if (this._bridge) {
      log(`Setting PowerManagerDelegate`);
      this._bridge.setPowerManagerDelegate(
        this._powerManagerDelegate,
        this.generateLoggingCallback("PowerManagerDelegate")
      );
    }
  },

  setPref(pref) {
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
