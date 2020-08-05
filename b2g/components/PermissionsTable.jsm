/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = [
  "PermissionsTable",
  "expandPermissions",
  "appendAccessToPermName",
  "AllPossiblePermissions",
];

// Permission access flags
const READONLY = "readonly";
const CREATEONLY = "createonly";
const READCREATE = "readcreate";
const READWRITE = "readwrite";

const ALLOW_ACTION = Ci.nsIPermissionManager.ALLOW_ACTION;
const DENY_ACTION = Ci.nsIPermissionManager.DENY_ACTION;
const PROMPT_ACTION = Ci.nsIPermissionManager.PROMPT_ACTION;

this.PermissionsTable = {
  "desktop-notification": {
    pwa: PROMPT_ACTION,
    packaged: PROMPT_ACTION,
    certified: PROMPT_ACTION,
  },
  "device-storage:apps": {
    pwa: DENY_ACTION,
    packaged: DENY_ACTION,
    certified: ALLOW_ACTION,
    access: ["read"],
  },
  "device-storage:apps-storage": {
    pwa: DENY_ACTION,
    packaged: DENY_ACTION,
    certified: ALLOW_ACTION,
    access: ["read"],
  },
  "device-storage:crashes": {
    pwa: DENY_ACTION,
    packaged: DENY_ACTION,
    certified: ALLOW_ACTION,
    access: ["read"],
  },
  "device-storage:music": {
    pwa: DENY_ACTION,
    packaged: ALLOW_ACTION,
    certified: ALLOW_ACTION,
    access: ["read", "write", "create"],
  },
  "device-storage:pictures": {
    pwa: DENY_ACTION,
    packaged: ALLOW_ACTION,
    certified: ALLOW_ACTION,
    access: ["read", "write", "create"],
  },
  "device-storage:sdcard": {
    pwa: DENY_ACTION,
    packaged: ALLOW_ACTION,
    certified: ALLOW_ACTION,
    access: ["read", "write", "create"],
  },
  "device-storage:videos": {
    pwa: DENY_ACTION,
    packaged: ALLOW_ACTION,
    certified: ALLOW_ACTION,
    access: ["read", "write", "create"],
  },
  geolocation: {
    pwa: PROMPT_ACTION,
    packaged: PROMPT_ACTION,
    certified: PROMPT_ACTION,
  },
};

/**
 * Append access modes to the permission name as suffixes.
 *   e.g. permission name 'device-storage:sdcard' with ['read', 'write'] =
 *   ['device-storage:sdcard:read', 'device-storage:sdcard:write']
 * @param string aPermName
 * @param array aAccess
 * @returns array containing access:appended permission names.
 **/
this.appendAccessToPermName = (aPermName, aAccess) => {
  if (!aAccess.length) {
    return [aPermName];
  }
  return aAccess.map(function(aMode) {
    return aPermName + ":" + aMode;
  });
};

/**
 * Expand an access string into multiple permission names,
 *   e.g: permission name 'device-storage:sdcard' with 'readwrite' =
 *   ['device-storage:sdcard:read', 'device-storage:sdcard:create', 'device-storage:sdcard:write']
 * @param string aPermName
 * @param string aAccess (optional)
 * @returns array containing expanded permission names.
 **/
this.expandPermissions = (aPermName, aAccess) => {
  if (!PermissionsTable[aPermName]) {
    let errorMsg =
      "PermissionsTable.jsm: expandPermissions: Unknown Permission: " +
      aPermName;
    Cu.reportError(errorMsg);
    dump(errorMsg);
    return [];
  }

  const tableEntry = PermissionsTable[aPermName];

  if ((!aAccess && tableEntry.access) || (aAccess && !tableEntry.access)) {
    let errorMsg =
      "PermissionsTable.jsm: expandPermissions: Invalid access for permission " +
      aPermName +
      ": " +
      aAccess +
      "\n";
    Cu.reportError(errorMsg);
    dump(errorMsg);
    return [];
  }

  let expandedPermNames = [];

  if (tableEntry.access && aAccess) {
    let requestedSuffixes = [];
    switch (aAccess) {
      case READONLY:
        requestedSuffixes.push("read");
        break;
      case CREATEONLY:
        requestedSuffixes.push("create");
        break;
      case READCREATE:
        requestedSuffixes.push("read", "create");
        break;
      case READWRITE:
        requestedSuffixes.push("read", "create", "write");
        break;
      default:
        return [];
    }

    let permArr = appendAccessToPermName(aPermName, requestedSuffixes);

    // Only add the suffixed version if the suffix exists in the table.
    for (let idx in permArr) {
      let suffix = requestedSuffixes[idx % requestedSuffixes.length];
      if (tableEntry.access.includes(suffix)) {
        expandedPermNames.push(permArr[idx]);
      }
    }
  } else if (tableEntry.substitute) {
    expandedPermNames = expandedPermNames.concat(tableEntry.substitute);
  } else {
    expandedPermNames.push(aPermName);
  }

  return expandedPermNames;
};

this.AllPossiblePermissions = [];

(function() {
  // We also need a list of all the possible permissions for things like the
  // settingsmanager, so construct that while we're at it.
  for (let permName in PermissionsTable) {
    let permAliases = [];
    if (PermissionsTable[permName].access) {
      permAliases = expandPermissions(permName, "readwrite");
    } else if (!PermissionsTable[permName].substitute) {
      permAliases = expandPermissions(permName);
    }
    for (let i = 0; i < permAliases.length; i++) {
      AllPossiblePermissions.push(permAliases[i]);
    }
  }
})();
