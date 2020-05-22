/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { AppConstants } = ChromeUtils.import(
  "resource://gre/modules/AppConstants.jsm"
);

const isGonk = AppConstants.platform === "gonk";

this.EXPORTED_SYMBOLS = ["LocalDomains"];

function log(msg) {
  dump(`LocalDomains: ${msg}\n`);
}

this.LocalDomains = {
  list: [],
  cert: null,

  // Initialize the list of local domains from the current value of the preference used
  // by Necko.
  init() {
    this.list = Services.prefs
      .getCharPref("network.dns.localDomains")
      .split(",")
      .map(item => `${item.trim()}`)
      .sort();
  },

  get() {
    log(`get: ${this.list.length} items.`);
    return this.list;
  },

  // Performs a scanning of the file system to find available apps.
  // Returns true if any change happened compared to the current state, false otherwise.
  scan() {
    log("scan");
    // On Gonk, scan /data/local/webapps or /system/b2g/webapps, and $profile/webapps on desktop.
    let path = "/data/local/webapps";
    if (isGonk) {
      let webapps = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
      webapps.initWithPath(path);
      if (!webapps.exists()) {
        path = "/system/b2g/webapps";
      }
    } else {
      let file = Services.dirsvc.get("ProfD", Ci.nsIFile);
      file.append("webapps");
      path = file.path;
    }
    log(`scanning ${path}`);
    let found = [];
    let webapps = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    try {
      webapps.initWithPath(path);
      if (!webapps.isDirectory()) {
        log(`${path} should be a directory!!`);
        return false;
      }
      let enumerator = webapps.directoryEntries;
      while (enumerator.hasMoreElements()) {
        let file = enumerator.nextFile;
        if (file.isDirectory()) {
          let name = `${file.leafName.trim()}.local`;
          log(`Found ${name}`);
          found.push(name);
        }
      }
    } catch(e) {
      // Something went wrong iterating the apps list. That can happen in empty
      // profiles that will only load the default UI.
      return false;
    }

    // Compare the current list with the found ones.
    if (this.list.length !== found.length) {
      this.list = found;
      return true;
    }

    let has_new = false;
    found.forEach(item => {
      if (!this.list.includes(item)) {
        has_new = true;
      }
    });

    if (has_new) {
      this.list = found;
    }
    return has_new;
  },

  // Makes sure the certificate is available to other methods.
  ensure_cert() {
    log("ensure_cert");

    if (this.cert !== null) {
      return;
    }

    const { NetUtil } = ChromeUtils.import(
      "resource://gre/modules/NetUtil.jsm"
    );

    // On desktop the certificate is located in the profile directory for now, and
    // on device it is in the default system folder.
    let file = Services.dirsvc.get(isGonk ? "DefRt" : "ProfD", Ci.nsIFile);
    file.append("local-cert.pem");
    log(`Loading certs from ${file.path}`);

    let fstream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(
      Ci.nsIFileInputStream
    );
    fstream.init(file, -1, 0, 0);
    let data = NetUtil.readInputStreamToString(fstream, fstream.available());
    fstream.close();

    // Get the base64 content only as a 1-liner.
    data = data
      .replace(/-----BEGIN CERTIFICATE-----/, "")
      .replace(/-----END CERTIFICATE-----/, "")
      .replace(/[\r\n]/g, "");

    let certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(
      Ci.nsIX509CertDB
    );
    try {
      certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(
        Ci.nsIX509CertDB2
      );
    } catch (e) {}

    this.cert = certdb.addCertFromBase64(data, "CT,CT,CT");
    log(`Certificate added for ${this.cert.subjectAltNames}`);
  },

  // Update the prefs to match the current set of local domains, and
  // registers the certificate overrides.
  update() {
    log("update");
    this.ensure_cert();

    // 1. Add the overrides.
    let overrideService = Cc["@mozilla.org/security/certoverride;1"].getService(
      Ci.nsICertOverrideService
    );

    // Reuse the list of hosts that we force to resolve to 127.0.0.1 to add the overrides.
    this.list.forEach(host => {
      overrideService.rememberValidityOverride(
        host,
        443,
        this.cert,
        overrideService.ERROR_UNTRUSTED |
          overrideService.ERROR_MISMATCH |
          overrideService.ERROR_TIME,
        false /* temporary */
      );
    });

    // 2. Update the preference.
    Services.prefs.setCharPref("network.dns.localDomains", this.list.join(","));
  },
};
