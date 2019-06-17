"use strict";

// A base class to use by Web Embedders, providing an ergonomic
// api over Gecko specific various hooks.
// It runs with chrome privileges in the system app.

(function() {

const {Services} = ChromeUtils.import("resource://gre/modules/Services.jsm");

function _webembed_log(msg) {
    console.log(`WebEmbedder: ${msg}`);
}

function BrowserDOMWindow(embedder) {
    _webembed_log(`Creating BrowserDOMWindow implementing ${Ci.nsIBrowserDOMWindow}`);
    this.embedder = embedder;
}

BrowserDOMWindow.prototype = {
  QueryInterface: ChromeUtils.generateQI([Ci.nsIBrowserDOMWindow]),

  openURI: function(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp) {
    _webembed_log(`BrowserDOMWindow::openURI ${aURI.spec}`);
    if (this.embedder && this.embedder.browserDomWindow) {
        return this.embedder.browserDomWindow.openURI(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp);
    }
    throw "NOT IMPLEMENTED";
  },

  createContentWindow: function(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp) {
    _webembed_log(`BrowserDOMWindow::createContentWindow ${aURI.spec}`);
    if (this.embedder && this.embedder.browserDomWindow) {
        return this.embedder.browserDomWindow.createContentWindow(aURI, aOpener, aWhere, aFlags, aTriggeringPrincipal, aCsp);
    }
    throw "NOT IMPLEMENTED";
  },

  openURIInFrame: function(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName) {
    // We currently ignore aNextRemoteTabId on mobile.  This needs to change
    // when Fennec starts to support e10s.  Assertions will fire if this code
    // isn't fixed by then.
    //
    // We also ignore aName if it is set, as it is currently only used on the
    // e10s codepath.
    _webembed_log(`BrowserDOMWindow::openURIInFrame ${aURI.spec}`);
    if (this.embedder && this.embedder.browserDomWindow) {
        let res = this.embedder.browserDomWindow.openURIInFrame(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName);
        if (res) {
            return res.frame();
        }
    }
    throw "NOT IMPLEMENTED";
  },

  createContentWindowInFrame: function(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName) {
    _webembed_log(`BrowserDOMWindow::createContentWindowInFrame ${aURI.spec}`);
    if (this.embedder && this.embedder.browserDomWindow) {
        let res = this.embedder.browserDomWindow.createContentWindowInFrame(aURI, aParams, aWhere, aFlags, aNextRemoteTabId, aName);
        if (res) {
            return res.frame();
        }
    }
    throw "NOT IMPLEMENTED";
  },

  isTabContentWindow: function(aWindow) {
    _webembed_log(`BrowserDOMWindow::isTabContentWindow`);
    if (this.embedder && this.embedder.browserDomWindow) {
        return this.embedder.browserDomWindow.isTabContentWindow(aWindow);
    }
    return false;
  },

  canClose() {
    _webembed_log(`BrowserDOMWindow::canClose`);
    if (this.embedder && this.embedder.browserDomWindow) {
        return this.embedder.browserDomWindow.canClose();
    }
    return true;
  },
};

class WebEmbedder extends EventTarget {
    constructor(windowProviderImpl, processSelectorImpl) {
        super();

        _webembed_log(`constructor in ${window}`);

        this.browserDomWindow = windowProviderImpl;

        Services.obs.addObserver((shell_window) => {
            this.dispatchEvent(new CustomEvent("runtime-ready"));
        }, "shell-ready");

        // Hook up the process provider implementation.
        // First make sure the service was started so it can receive the observer notification.
        let _ = Cc["@mozilla.org/ipc/processselector;1"].getService(Ci.nsIContentProcessProvider);
        Services.obs.notifyObservers({ wrappedJSObject: processSelectorImpl }, "web-embedder-set-process-selector");

        // Notify the shell that a new embedder was created and send it the window provider.
        Services.obs.notifyObservers(new BrowserDOMWindow(this), "web-embedder-created");
    }

    launch_preallocated_process() {
        _webembed_log(`launch_preallocated_process`);
        return Services.appinfo.ensureContentProcess();
    }
}

window.WebEmbedder = WebEmbedder;

}());