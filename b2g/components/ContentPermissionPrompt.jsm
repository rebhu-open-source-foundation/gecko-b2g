/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

function debug(str) {
  //dump("-*- ContentPermissionPrompt: " + str + "\n");
}

const PROMPT_FOR_UNKNOWN = [
  "audio-capture",
  "desktop-notification",
  "geolocation",
  "video-capture",
];

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

var secMan = Services.scriptSecurityManager;

/**
 * Determine if a permission should be prompt to user or not.
 *
 * @param aPerm requested permission
 * @param aAction the action according to principal
 * @return true if prompt is required
 */
function shouldPrompt(aPerm, aAction) {
  return (
    aAction == Ci.nsIPermissionManager.PROMPT_ACTION ||
    (aAction == Ci.nsIPermissionManager.UNKNOWN_ACTION &&
      PROMPT_FOR_UNKNOWN.includes(aPerm))
  );
}

/**
 * Create the default choices for the requested permissions
 *
 * @param aTypesInfo requested permissions
 * @return the default choices for permissions with options, return
 *         undefined if no option in all requested permissions.
 */
function buildDefaultChoices(aTypesInfo) {
  let choices;
  for (let type of aTypesInfo) {
    if (type.options.length > 0) {
      if (!choices) {
        choices = {};
      }
      choices[type.permission] = type.options[0];
    }
  }
  return choices;
}

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {
  handleExistingPermission: function handleExistingPermission(
    request,
    typesInfo
  ) {
    typesInfo.forEach(function(type) {
      type.action = Services.perms.testExactPermissionFromPrincipal(
        request.principal,
        type.permission
      );
      if (shouldPrompt(type.permission, type.action)) {
        type.action = Ci.nsIPermissionManager.PROMPT_ACTION;
      }
    });

    // If all permissions are allowed already, call allow() without prompting.
    let checkAllowPermission = function(type) {
      if (type.action == Ci.nsIPermissionManager.ALLOW_ACTION) {
        return true;
      }
      return false;
    };
    if (typesInfo.every(checkAllowPermission)) {
      debug("all permission requests are allowed");
      request.allow(buildDefaultChoices(typesInfo));
      return true;
    }

    // If all permissions are DENY_ACTION or UNKNOWN_ACTION, call cancel()
    // without prompting.
    let checkDenyPermission = function(type) {
      if (
        type.action == Ci.nsIPermissionManager.DENY_ACTION ||
        type.action == Ci.nsIPermissionManager.UNKNOWN_ACTION
      ) {
        return true;
      }
      return false;
    };
    if (typesInfo.every(checkDenyPermission)) {
      debug("all permission requests are denied");
      request.cancel();
      return true;
    }
    return false;
  },

  prompt(request) {
    // Initialize the typesInfo and set the default value.
    let typesInfo = [];
    let perms = request.types.QueryInterface(Ci.nsIArray);
    for (let idx = 0; idx < perms.length; idx++) {
      let perm = perms.queryElementAt(idx, Ci.nsIContentPermissionType);
      let tmp = {
        permission: perm.type,
        options: [],
        deny: true,
        action: Ci.nsIPermissionManager.UNKNOWN_ACTION,
      };

      // Append available options, if any.
      let options = perm.options.QueryInterface(Ci.nsIArray);
      for (let i = 0; i < options.length; i++) {
        let option = options.queryElementAt(i, Ci.nsISupportsString).data;
        tmp.options.push(option);
      }
      typesInfo.push(tmp);
    }

    if (request.principal.isSystemPrincipal) {
      request.allow(buildDefaultChoices(typesInfo));
      return;
    }

    if (typesInfo.length == 0) {
      request.cancel();
      return;
    }

    // returns true if the request was handled
    this.handleExistingPermission(request, typesInfo);
  },

  classID: Components.ID("{8c719f03-afe0-4aac-91ff-6c215895d467}"),

  QueryInterface: ChromeUtils.generateQI([Ci.nsIContentPermissionPrompt]),
};

//module initialization
var EXPORTED_SYMBOLS = ["ContentPermissionPrompt"];
