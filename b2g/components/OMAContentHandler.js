/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */

"use strict";

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

function debug(aMsg) {
  //dump("--*-- OMAContentHandler: " + aMsg + "\n");
}

const NS_ERROR_WONT_HANDLE_CONTENT = 0x805d0001;

function OMAContentHandler() {}

OMAContentHandler.prototype = {
  classID: Components.ID("{a6b2ab13-9037-423a-9897-dde1081be323}"),

  _xpcom_factory: {
    createInstance: function createInstance(outer, iid) {
      if (outer != null) {
        throw Components.Exception("", Cr.NS_ERROR_NO_AGGREGATION);
      }
      return new OMAContentHandler().QueryInterface(iid);
    },
  },

  handleContent: function handleContent(aMimetype, aContext, aRequest) {
    if (!(aRequest instanceof Ci.nsIChannel)) {
      throw NS_ERROR_WONT_HANDLE_CONTENT;
    }

    let detail = {
      type: aMimetype,
      url: aRequest.URI.spec,
    };
    Services.cpmm.sendAsyncMessage("content-handler", detail);

    aRequest.cancel(Cr.NS_BINDING_ABORTED);
  },

  QueryInterface: ChromeUtils.generateQI([Ci.nsIContentHandler]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([OMAContentHandler]);
