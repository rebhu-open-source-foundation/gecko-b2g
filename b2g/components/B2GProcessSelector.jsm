// Process selector for b2g.

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");

function B2GProcessSelector() {
  Services.obs.addObserver(embedderSelector => {
    this.embedderSelector = embedderSelector.wrappedJSObject;
  }, "web-embedder-set-process-selector");
}

B2GProcessSelector.prototype = {
  classID: Components.ID("{dd87f882-9d09-49e5-989d-cfaaaf4425be}"),
  QueryInterface: ChromeUtils.generateQI([Ci.nsIContentProcessProvider]),

  provideProcess(aType, aOpener, aProcesses, aMaxCount) {
    // Delegates to the embedder if possible, or just defaults to create a new process.
    if (!this.embedderSelector) {
      return Ci.nsIContentProcessProvider.NEW_PROCESS;
    }
    return this.embedderSelector.provideProcess(
      aType,
      aOpener,
      aProcesses,
      aMaxCount
    );
  },
};

var EXPORTED_SYMBOLS = ["B2GProcessSelector"];
