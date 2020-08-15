/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["ExternalAPIService"];

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { NetUtil } = ChromeUtils.import("resource://gre/modules/NetUtil.jsm");
const { FileUtils } = ChromeUtils.import(
  "resource://gre/modules/FileUtils.jsm"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "uuidgen",
  "@mozilla.org/uuid-generator;1",
  "nsIUUIDGenerator"
);

function writeFile(aPath, aData, callback) {
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(aPath);

  // Initialize the file output stream
  let ostream = FileUtils.openSafeFileOutputStream(file);

  // Obtain a converter to convert our data to a UTF-8 encoded input stream.
  let converter = Cc[
    "@mozilla.org/intl/scriptableunicodeconverter"
  ].createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";

  // Asynchronously copy the data to the file.
  let istream = converter.convertToInputStream(aData);
  NetUtil.asyncCopy(istream, ostream, aResult => {
    if (!Components.isSuccessCode(aResult)) {
      callback({ error: aResult });
    } else {
      callback();
    }
  });
}

this.ExternalAPIService = {
  init() {
    let secretToken = uuidgen
      .generateUUID()
      .toString()
      .replace(/[{}]/g, "");
    Services.prefs.setCharPref("b2g.services.runtime.token", secretToken);
    // Write the token in the root owned file /data/local/service/api-daemon/.runtime_token
    writeFile(
      "/data/local/service/api-daemon/.runtime_token",
      secretToken,
      res => {
        if (res && res.error) {
          Cu.reportError(
            `Unable to write b2g.services.runtime.token : ${res.error}`
          );
        }
      }
    );
  },
};

ExternalAPIService.init();
