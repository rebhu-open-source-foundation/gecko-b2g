/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { ComponentUtils } = ChromeUtils.import(
  "resource://gre/modules/ComponentUtils.jsm"
);
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

// Small helper to expose nsICommandLine object to chrome code

function CommandlineHandler() {
  this.wrappedJSObject = this;
}

CommandlineHandler.prototype = {
  handle(cmdLine) {
    this.cmdLine = cmdLine;
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    if (win && win.shell) {
      win.shell.handleCmdLine();
    }
  },

  helpInfo: "",
  classID: Components.ID("{385993fe-8710-4621-9fb1-00a09d8bec37}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsICommandLineHandler]),
};

this.NSGetFactory = ComponentUtils.generateNSGetFactory([CommandlineHandler]);
