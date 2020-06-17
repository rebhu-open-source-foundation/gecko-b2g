/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var EXPORTED_SYMBOLS = ["BinderServices"];

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

var BinderServices = {};

XPCOMUtils.defineLazyGetter(BinderServices, "connectivity", function() {
#ifdef HAS_KOOST_MODULES
  try {
    return Cc["@mozilla.org/b2g/connectivitybinderservice;1"].getService(
      Ci.nsIConnectivityBinderService);
  } catch (e) {}
#endif
  // Sync from nsIConnectivityBinderService.idl.
  return {
    onCaptivePortalChanged(wifiState, usbState) {},
    onTetheringChanged(captivePortalLanding) {},
  };
});

XPCOMUtils.defineLazyServiceGetters(BinderServices, {});
