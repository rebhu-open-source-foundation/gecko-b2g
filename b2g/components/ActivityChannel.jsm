/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(
  this,
  "contentSecManager",
  "@mozilla.org/contentsecuritymanager;1",
  "nsIContentSecurityManager"
);

this.EXPORTED_SYMBOLS = ["ActivityChannel"];

this.ActivityChannel = function(aURI, aLoadInfo, aName, aDetails) {
  this._activityName = aName;
  this._activityDetails = aDetails;
  this.originalURI = aURI;
  this.URI = aURI;
  this.loadInfo = aLoadInfo;
};

this.ActivityChannel.prototype = {
  originalURI: null,
  URI: null,
  owner: null,
  notificationCallbacks: null,
  securityInfo: null,
  contentType: null,
  contentCharset: null,
  contentLength: 0,
  contentDisposition: Ci.nsIChannel.DISPOSITION_INLINE,
  contentDispositionFilename: null,
  contentDispositionHeader: null,
  loadInfo: null,

  open() {
    throw Components.Exception("", Cr.NS_ERROR_NOT_IMPLEMENTED);
  },

  open2() {
    throw Components.Exception("", Cr.NS_ERROR_NOT_IMPLEMENTED);
  },

  asyncOpen(aListener, aContext) {
    Services.cpmm.sendAsyncMessage(this._activityName, this._activityDetails);
    // Let the listener cleanup.
    aListener.onStopRequest(this, aContext, Cr.NS_OK);
  },

  asyncOpen2(aListener) {
    // throws an error if security checks fail
    var outListener = contentSecManager.performSecurityCheck(this, aListener);
    this.asyncOpen(outListener, null);
  },

  QueryInterface2: ChromeUtils.generateQI([Ci.nsIChannel]),
};
