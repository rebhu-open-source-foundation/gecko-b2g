/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { FileUtils } = ChromeUtils.import(
  "resource://gre/modules/FileUtils.jsm"
);
const { NetUtil } = ChromeUtils.import(
  "resource://gre/modules/NetUtil.jsm"
);

this.EXPORTED_SYMBOLS = ["WifiConfigStore"];

var gDebug = false;

this.WifiConfigStore = (function() {
  var wifiConfigStore = {};

  const WIFI_CONFIG_PATH = "/data/misc/wifi/wifi_config.json";

  // WifiConfigStore parameters
  // configManager.QUALITY_NETWORK_SELECTION_DISABLE_REASON = QUALITY_NETWORK_SELECTION_DISABLE_REASON;

  // WifiConfigStore functions
  wifiConfigStore.read = read;
  wifiConfigStore.write = write;
  wifiConfigStore.setDebug = setDebug;

  function setDebug(aDebug) {
    gDebug = aDebug;
  }

  function debug(aMsg) {
    if (gDebug) {
      dump("-*- WifiConfigStore: " + aMsg);
    }
  }

  function read() {
    let wifiConfigFile = new FileUtils.File(WIFI_CONFIG_PATH);
    let networks;
    if (wifiConfigFile.exists()) {
      let fstream = Cc[
        "@mozilla.org/network/file-input-stream;1"
      ].createInstance(Ci.nsIFileInputStream);
      let sstream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(
        Components.interfaces.nsIScriptableInputStream
      );

      const RO = 0x01;
      const READ_OTHERS = 4;

      fstream.init(wifiConfigFile, RO, READ_OTHERS, 0);
      sstream.init(fstream);
      let data = sstream.read(sstream.available());
      sstream.close();
      fstream.close();
      networks = JSON.parse(data);
    } else {
      networks = null;
    }

    return networks;
  }

  function write(aNetworks, aCallback) {
    let wifiConfigFile = new FileUtils.File(WIFI_CONFIG_PATH);
    // Initialize the file output stream.
    let ostream = FileUtils.openSafeFileOutputStream(wifiConfigFile);

    // Obtain a converter to convert our data to a UTF-8 encoded input stream.
    let converter = Cc[
      "@mozilla.org/intl/scriptableunicodeconverter"
    ].createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    // Asynchronously copy the data to the file.
    let istream = converter.convertToInputStream(JSON.stringify(aNetworks));
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      FileUtils.closeSafeFileOutputStream(ostream);
      if (aCallback) {
        aCallback();
      }
    });
  }

  return wifiConfigStore;
})();
