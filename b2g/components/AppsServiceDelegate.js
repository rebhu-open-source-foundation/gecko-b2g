/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { ComponentUtils } = ChromeUtils.import(
  "resource://gre/modules/ComponentUtils.jsm"
);

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

  onBoot: function ad_onboot(aManifestUrl, aValue) {
    // TODO: call to the related components to finish the registration
    log("onBoot manifest url: " + aManifestUrl);
  },
  onInstall: function ad_oninstall(aManifestUrl, aValue) {
    // TODO: call to the related components to finish the registration
    log("onInstall: " + aManifestUrl);
  },
  onUpdate: function ad_onupdate(aManifestUrl, aValue) {
    // TODO: call to the related components to finish the registration
    log("onUpdate: " + aManifestUrl);
  },
  onUninstall: function ad_onuninstall(aManifestUrl) {
    // TODO: call to the related components to finish the registration
    log("onUninstall: " + aManifestUrl);
  }
};

this.NSGetFactory = ComponentUtils.generateNSGetFactory([AppsServiceDelegate]);
