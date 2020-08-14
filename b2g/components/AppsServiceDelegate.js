/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { ComponentUtils } = ChromeUtils.import(
  "resource://gre/modules/ComponentUtils.jsm"
);

const { PermissionsInstaller } = ChromeUtils.import(
  "resource://gre/modules/PermissionsInstaller.jsm"
);

const { LocalDomains } = ChromeUtils.import(
  "resource://gre/modules/LocalDomains.jsm"
);

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

const DEBUG = 1;
var log = DEBUG
  ? function log_dump(msg) {
      dump("AppsServiceDelegate: " + msg + "\n");
    }
  : function log_noop(msg) {};

function AppsServiceDelegate() {}

AppsServiceDelegate.prototype = {
  classID: Components.ID("{a4a8d542-c877-11ea-81c6-87c0ade42646}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsIAppsServiceDelegate]),
  _xpcom_factory: ComponentUtils.generateSingletonFactory(AppsServiceDelegate),

  _installPermissions(aFeatures, aManifestUrl, aReinstall, aState) {
    try {
      let features = JSON.parse(aFeatures);

      PermissionsInstaller.installPermissions(
        features,
        aManifestUrl,
        aReinstall,
        null /* onerror */
      );
    } catch (e) {
      log(`Error in ${aState}: ${e}`);
    }
  },

  onBoot(aManifestUrl, aFeatures) {
    // TODO: call to the related components to finish the registration
    log(`onBoot: ${aManifestUrl}`);
    log(aFeatures);
    this._installPermissions(aFeatures, aManifestUrl, false, "onBoot");
  },

  onInstall(aManifestUrl, aFeatures) {
    // TODO: call to the related components to finish the registration
    log(`onInstall: ${aManifestUrl}`);
    log(aFeatures);
    this._installPermissions(aFeatures, aManifestUrl, false, "onInstall");
    let uri = Services.io.newURI(aManifestUrl);
    LocalDomains.add(uri.host);
  },

  onUpdate(aManifestUrl, aFeatures) {
    log(`onUpdate: ${aManifestUrl}`);
    log(aFeatures);
    this._installPermissions(aFeatures, aManifestUrl, true, "onUpdate");
  },

  onUninstall(aManifestUrl) {
    // TODO: call to the related components to finish the registration
    log(`onUninstall: ${aManifestUrl}`);
    PermissionsInstaller.uninstallPermissions(aManifestUrl);
    let uri = Services.io.newURI(aManifestUrl);
    LocalDomains.remove(uri.host);
  },
};

this.NSGetFactory = ComponentUtils.generateNSGetFactory([AppsServiceDelegate]);
