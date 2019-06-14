"use strict";

// A base class to use by Web Embedders, providing an ergonomic
// api over Gecko specific various hooks.
// It runs with chrome privileges in the system app.

(function() {

const {Services} = ChromeUtils.import("resource://gre/modules/Services.jsm");

function _webembed_log(msg) {
    console.log(`WebEmbedder: ${msg}`);
}

class WebEmbedder extends EventTarget {
    constructor() {
        super();

        Services.obs.addObserver(() => {
            this.dispatchEvent(new CustomEvent("runtime-ready"));
        }, "shell-ready");

        // Notify the shell that a new embedder was created.
        Services.obs.notifyObservers(this, "web-embedder-created");
    }

    launch_preallocated_process() {
        _webembed_log(`launch_preallocated_process`);
        return Services.appinfo.ensureContentProcess();
    }
}

window.WebEmbedder = WebEmbedder;

}());