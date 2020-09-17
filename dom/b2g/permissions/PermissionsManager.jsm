/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

function debug(aMsg) {
  //dump(`-*- PermissionsManager: ${aMsg}\n`);
}

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

const PERMISSIONSETTINGS_CID = Components.ID(
  "{cd2cf7a1-f4c1-487b-8c1b-1a71c7097431}"
);

this.EXPORTED_SYMBOLS = ["PermissionsManager"];

function PermissionsManager() {
  debug("Constructor");
}

PermissionsManager.prototype = {
  // nsIDOMGlobalPropertyInitializer implementation
  init(aWindow) {
    this.window = aWindow;
  },

  get(aPermName, aOrigin) {
    debug(`get ${aPermName} ${aOrigin}`);
    let principal = Services.scriptSecurityManager.createContentPrincipalFromOrigin(
      aOrigin
    );
    let result = Services.perms.testExactPermissionFromPrincipal(
      principal,
      aPermName
    );

    switch (result) {
      case Ci.nsIPermissionManager.UNKNOWN_ACTION:
        return "unknown";
      case Ci.nsIPermissionManager.ALLOW_ACTION:
        return "allow";
      case Ci.nsIPermissionManager.DENY_ACTION:
        return "deny";
      case Ci.nsIPermissionManager.PROMPT_ACTION:
        return "prompt";
      default:
        debug(
          `Unsupported PermissionsManager origin: ${aOrigin} perm: ${aPermName} action: ${result}`
        );
        return "unknown";
    }
  },

  async isExplicit(aPermName, aOrigin) {
    let actor = this.window.windowGlobalChild.getActor("PermissionsManager");
    let v = await actor.isExplicit({
      type: aPermName,
      origin: aOrigin,
    });
    debug(`isExplicit: ${aPermName} ${aOrigin} ${v}`);
    return v;
  },

  set(aPermName, aPermValue, aOrigin) {
    debug(`set ${aPermName} ${aOrigin} ${aPermValue}`);
    let currentPermValue = this.get(aPermName, aOrigin);
    // Check for invalid calls so that we throw an exception rather than get
    // killed by parent process
    if (
      currentPermValue === "unknown" ||
      aPermValue === "unknown" ||
      !this.isExplicit(aPermName, aOrigin)
    ) {
      let errorMsg = `PermissionsManager: ${aPermName} is an implicit permission for ${aOrigin} or the permission isn't set`;
      Cu.reportError(errorMsg);
      throw new Components.Exception(errorMsg);
    }
    let actor = this.window.windowGlobalChild.getActor("PermissionsManager");

    actor
      .addPermission({
        type: aPermName,
        origin: aOrigin,
        value: aPermValue,
      })
      .then(
        () => {
          debug(`set ${aPermName} from ${aOrigin} success`);
        },
        aMsg => {
          debug(`set ${aPermName} from ${aOrigin} fail: ${aMsg}`);
        }
      );
  },

  remove(aPermName, aOrigin) {
    // PermissionsManagerParent.jsm handles delete when value is "unknown"
    let actor = this.window.windowGlobalChild.getActor("PermissionsManager");

    actor
      .addPermission({
        type: aPermName,
        origin: aOrigin,
        value: "unknown",
      })
      .then(
        () => {
          debug(`remove ${aPermName} from ${aOrigin} success`);
        },
        aMsg => {
          debug(`remove ${aPermName} from ${aOrigin} fail: ${aMsg}`);
        }
      );
  },

  classID: PERMISSIONSETTINGS_CID,
  QueryInterface: ChromeUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer]),
};
