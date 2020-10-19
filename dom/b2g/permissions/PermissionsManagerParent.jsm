/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

function debug(s) {
  //dump("-*- PermissionsManagerParent: " + s + "\n");
}

this.EXPORTED_SYMBOLS = ["PermissionsManagerParent"];

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { PermissionsTable, permissionsReverseTable } = ChromeUtils.import(
  "resource://gre/modules/PermissionsTable.jsm"
);

const { PermissionsHelper } = ChromeUtils.import(
  "resource://gre/modules/PermissionsInstaller.jsm"
);

class PermissionsManagerParent extends JSWindowActorParent {
  receiveMessage(aMessage) {
    debug(`receiveMessage ${aMessage.name}`);
    let msg = aMessage.data;

    switch (aMessage.name) {
      case "PermissionsManager:AddPermission":
        let success = this._internalAddPermission(msg);

        // TODO: check if we can get back this mm permission check.
        // let mm = aMessage.target;
        // if (mm.assertPermission("permissions")) {
        //   success = this._internalAddPermission(msg);
        //   if (!success) {
        //     // Just kill the calling process
        //     mm.assertPermission("permissions-modify-implicit");
        //     errorMsg =
        //       " had an implicit permission change. Child process killed.";
        //   }
        // }
        if (!success) {
          let errorMsg =
            "PermissionsManagerParent message ${msg.type} from a content process with no 'permissions' privileges.";
          Cu.reportError(errorMsg);
          return Promise.reject(String(errorMsg));
        }
        return Promise.resolve();
      case "PermissionsManager:IsExplicit":
        return this.isExplicitInPermissionsTable(msg.origin, msg.type);
    }
    return undefined;
  }

  isExplicitInPermissionsTable(aOrigin, aPermName) {
    let appType = "pwa";

    if (aOrigin.endsWith(".local")) {
      if (PermissionsHelper.isCoreApp(aOrigin) === true) {
        appType = "core";
      } else {
        appType = "signed";
      }
    }

    let realPerm = permissionsReverseTable[aPermName];

    if (realPerm) {
      return (
        PermissionsTable[realPerm][appType] ==
        Ci.nsIPermissionManager.PROMPT_ACTION
      );
    }
    return false;
  }

  _isChangeAllowed(aOrigin, aPrincipal, aPermName, aAction) {
    // Bug 812289:
    // Change is allowed from a child process when all of the following
    // conditions stand true:
    //   * the action isn't "unknown" (so the change isn't a delete) if the app
    //     is installed
    //   * the permission already exists on the database
    //   * the permission is marked as explicit on the permissions table
    // Note that we *have* to check the first two conditions here because
    // permissionManager doesn't know if it's being called as a result of
    // a parent process or child process request. We could check
    // if the permission is actually explicit (and thus modifiable) or not
    // on permissionManager also but we currently don't.

    let perm = Services.perms.testExactPermissionFromPrincipal(
      aPrincipal,
      aPermName
    );
    let isExplicit = this.isExplicitInPermissionsTable(aOrigin, aPermName);

    return (
      aAction === "unknown" ||
      (aAction !== "unknown" &&
        perm !== Ci.nsIPermissionManager.UNKNOWN_ACTION &&
        isExplicit)
    );
  }

  _internalAddPermission(aData) {
    let principal = Services.scriptSecurityManager.createContentPrincipalFromOrigin(
      aData.origin
    );

    let action;
    switch (aData.value) {
      case "unknown":
        action = Ci.nsIPermissionManager.UNKNOWN_ACTION;
        break;
      case "allow":
        action = Ci.nsIPermissionManager.ALLOW_ACTION;
        break;
      case "deny":
        action = Ci.nsIPermissionManager.DENY_ACTION;
        break;
      case "prompt":
        action = Ci.nsIPermissionManager.PROMPT_ACTION;
        break;
      default:
        debug(`Unsupported Action ${aData.value}`);
        action = Ci.nsIPermissionManager.UNKNOWN_ACTION;
    }

    if (
      this._isChangeAllowed(aData.origin, principal, aData.type, aData.value)
    ) {
      debug(`add: ${aData.origin} ${action}`);
      Services.perms.addFromPrincipal(principal, aData.type, action);
      return true;
    }
    debug(`add Failure: ${aData.origin} ${action}`);
    return false; // This isn't currently used, see comment on setPermission
  }
}
