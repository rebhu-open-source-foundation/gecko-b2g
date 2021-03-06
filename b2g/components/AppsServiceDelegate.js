/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { ComponentUtils } = ChromeUtils.import(
  "resource://gre/modules/ComponentUtils.jsm"
);

const { PermissionsInstaller } = ChromeUtils.import(
  "resource://gre/modules/PermissionsInstaller.jsm"
);

const { ServiceWorkerAssistant } = ChromeUtils.import(
  "resource://gre/modules/ServiceWorkerAssistant.jsm"
);

const { AppsUtils } = ChromeUtils.import(
  "resource://gre/modules/AppsUtils.jsm"
);

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

const DEBUG = 1;
var log = DEBUG
  ? function log_dump(msg) {
      dump("AppsServiceDelegate: " + msg + "\n");
    }
  : function log_noop(msg) {};

const inParent =
  Services.appinfo.processType === Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

function AppsServiceDelegate() {}

AppsServiceDelegate.prototype = {
  classID: Components.ID("{a4a8d542-c877-11ea-81c6-87c0ade42646}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsIAppsServiceDelegate]),
  _xpcom_factory: ComponentUtils.generateSingletonFactory(AppsServiceDelegate),

  apps_list: new Map(),

  _addOrUpdateAppsList(aManifestUrl, aManifest) {
    // Will add PWA app into the list only.
    if (Services.io.newURI(aManifestUrl).host.endsWith(".localhost")) {
      return;
    }
    // A quick check if the manifest is valid.
    if (!aManifest.start_url && !aManifest.scope) {
      return;
    }
    this.apps_list.set(aManifestUrl, aManifest.scope);
  },

  _removeFromAppsList(aManifestUrl) {
    this.apps_list.delete(aManifestUrl);
  },

  _installPermissions(aFeatures, aManifestUrl, aReinstall, aState) {
    try {
      PermissionsInstaller.installPermissions(
        aFeatures,
        aManifestUrl,
        aReinstall,
        null /* onerror */
      );
    } catch (e) {
      log(`Error with PermissionsInstaller in ${aState}: ${e}`);
    }
  },

  _processServiceWorker(aManifestUrl, aFeatures, aState) {
    try {
      switch (aState) {
        case "onBoot":
        case "onInstall":
          ServiceWorkerAssistant.register(aManifestUrl, aFeatures);
          break;
        case "onUpdate":
          ServiceWorkerAssistant.update(aManifestUrl, aFeatures);
          break;
        case "onUninstall":
          ServiceWorkerAssistant.unregister(aManifestUrl);
          break;
      }
    } catch (e) {
      log(`Error with ServiceWorkerAssistant in ${aState}: ${e}`);
    }
  },

  getManifestUrlByScopeUrl(aUrl) {
    if (!inParent) {
      log("Return call from non-parent process.");
      return null;
    }
    let found = null;
    this.apps_list.forEach((scope, key, map) => {
      // If there are multiple scopes for the same origin,
      // try to match the longer one.
      if (
        aUrl.startsWith(scope) &&
        (!found || scope.length > map[found].length)
      ) {
        found = key;
      }
    });
    return found;
  },

  onBoot(aManifestUrl, aManifest) {
    log(`onBoot: ${aManifestUrl}`);
    try {
      let manifest = JSON.parse(aManifest);
      // To compatible with when b2g_features only is passed.
      let features = manifest.b2g_features || manifest;
      this._installPermissions(features, aManifestUrl, false, "onBoot");
      this._processServiceWorker(aManifestUrl, features, "onBoot");
      this._addOrUpdateAppsList(aManifestUrl, manifest);
    } catch (e) {
      log(`Error in onBoot: ${e}`);
    }
  },

  onBootDone() {
    log(`onBootDone`);
    Services.obs.notifyObservers(null, "on-boot-done");
    ServiceWorkerAssistant.waitForRegistrations();
  },

  onClear(aManifestUrl, aType, aManifest) {
    log(`onClear: ${aManifestUrl}: clear type: ${aType}`);
    switch (aType) {
      case "Browser":
        AppsUtils.clearBrowserData(aManifestUrl);
        break;
      case "Storage":
        AppsUtils.clearStorage(aManifestUrl);
        break;
      default:
    }

    // clearStorage removes everything stores per origin, re-register service
    // worker to create the cache db back for used by service worker.
    if (aManifest) {
      try {
        let manifest = JSON.parse(aManifest);
        // To compatible with when b2g_features only is passed.
        let features = manifest.b2g_features || manifest;
        ServiceWorkerAssistant.register(
          aManifestUrl,
          features,
          true /* serviceWorkerOnly */
        );
      } catch (e) {
        log(`Error when trying re-register sw in onClear: ${e}`);
      }
    }
  },

  onInstall(aManifestUrl, aManifest) {
    log(`onInstall: ${aManifestUrl}`);
    try {
      let manifest = JSON.parse(aManifest);
      // To compatible with when b2g_features only is passed.
      let features = manifest.b2g_features || manifest;
      this._installPermissions(features, aManifestUrl, false, "onInstall");
      this._processServiceWorker(aManifestUrl, features, "onInstall");
      this._addOrUpdateAppsList(aManifestUrl, manifest);
    } catch (e) {
      log(`Error in onInstall: ${e}`);
    }
  },

  onUpdate(aManifestUrl, aManifest) {
    log(`onUpdate: ${aManifestUrl}`);
    try {
      let manifest = JSON.parse(aManifest);
      // To compatible with when b2g_features only is passed.
      let features = manifest.b2g_features || manifest;
      this._installPermissions(features, aManifestUrl, true, "onUpdate");
      this._processServiceWorker(aManifestUrl, features, "onUpdate");
      this._addOrUpdateAppsList(aManifestUrl, manifest);
    } catch (e) {
      log(`Error in onUpdate: ${e}`);
    }
  },

  onUninstall(aManifestUrl) {
    log(`onUninstall: ${aManifestUrl}`);
    PermissionsInstaller.uninstallPermissions(aManifestUrl);
    this._processServiceWorker(aManifestUrl, undefined, "onUninstall");
    AppsUtils.clearBrowserData(aManifestUrl);
    AppsUtils.clearStorage(aManifestUrl);
    this._removeFromAppsList(aManifestUrl);
  },
};

this.NSGetFactory = ComponentUtils.generateNSGetFactory([AppsServiceDelegate]);
