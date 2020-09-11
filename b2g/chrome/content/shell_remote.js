/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- /
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { AppConstants } = ChromeUtils.import(
  "resource://gre/modules/AppConstants.jsm"
);
ChromeUtils.import("resource://gre/modules/ActivitiesService.jsm");
ChromeUtils.import("resource://gre/modules/AlarmService.jsm");
ChromeUtils.import("resource://gre/modules/DownloadService.jsm");

const isGonk = AppConstants.platform === "gonk";

function debug(aStr) {
  //console.log(`-*- ShellRemote.js: ${aStr}`);
}

var remoteShell = {
  get startURL() {
    let url = Services.prefs.getCharPref("b2g.multiscreen.system_remote_url");
    if (!url) {
      console.error(
        `Please set the b2g.system_startup_url preference properly`
      );
    }
    return url;
  },

  _started: false,
  hasStarted() {
    return this._started;
  },

  start() {
    this._started = true;

    // This forces the initialization of the cookie service before we hit the
    // network.
    // See bug 810209
    let cookies = Cc["@mozilla.org/cookieService;1"].getService();
    if (!cookies) {
      debug("No cookies service!");
    }

    let startURL = this.startURL;

    let systemAppFrame = document.createXULElement("browser");
    systemAppFrame.setAttribute("type", "chrome");
    systemAppFrame.setAttribute("id", "systemapp");
    systemAppFrame.setAttribute("src", "blank.html");

    debug(`Loading blank.html`);
    document.body.appendChild(systemAppFrame);

    this.contentBrowser = systemAppFrame;

    window.addEventListener("MozAfterPaint", this);
    window.addEventListener("sizemodechange", this);
    window.addEventListener("unload", this);

    let stop_url = null;

    // Listen for loading events on the system app xul:browser
    let listener = {
      onLocationChange: (webProgress, request, location, flags) => {
        // debug(`LocationChange: ${location.spec}`);
      },

      onProgressChange: () => {
        // debug(`ProgressChange`);
      },

      onSecurityChange: () => {
        // debug(`SecurityChange`);
      },

      onStateChange: (webProgress, request, stateFlags, status) => {
        // debug(`StateChange ${stateFlags}`);
        if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
          if (!stop_url) {
            stop_url = request.name;
          }
        }

        if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          // debug(`Done loading ${request.name}`);
          if (stop_url && request.name == stop_url) {
            this.contentBrowser.removeProgressListener(listener);
            this.notifyContentWindowLoaded();
          }
        }
      },

      onStatusChange: () => {
        // debug(`StatusChange`);
      },

      QueryInterface: ChromeUtils.generateQI([
        Ci.nsIWebProgressListener2,
        Ci.nsIWebProgressListener,
        Ci.nsISupportsWeakReference,
      ]),
    };
    this.contentBrowser.addProgressListener(listener);

    debug(`Setting system url to ${startURL}`);

    this.contentBrowser.src = startURL;

    try {
      ChromeUtils.import("resource://gre/modules/ExternalAPIService.jsm");
    } catch (e) {}
  },

  stop() {
    window.removeEventListener("unload", this);
    window.removeEventListener("sizemodechange", this);
  },

  handleEvent(event) {
    debug(`event: ${event.type}`);

    // let content = this.contentBrowser.contentWindow;
    switch (event.type) {
      case "sizemodechange":
        // Due to bug 4657, the default behavior of video/audio playing from web
        // sites should be paused when this browser tab has sent back to
        // background or phone has flip closed.
        if (window.windowState == window.STATE_MINIMIZED) {
          this.contentBrowser.setVisible(false);
        } else {
          this.contentBrowser.setVisible(true);
        }
        break;
      case "MozAfterPaint":
        window.removeEventListener("MozAfterPaint", this);
        break;
      case "unload":
        this.stop();
        break;
    }
  },

  // This gets called when window.onload fires on the System app content window,
  // which means things in <html> are parsed and statically referenced <script>s
  // and <script defer>s are loaded and run.
  notifyContentWindowLoaded() {
    debug("notifyContentWindowLoaded");
  },
};

document.addEventListener(
  "DOMContentLoaded",
  () => {
    if (remoteShell.hasStarted()) {
      // Shoudl never happen!
      console.error("ShellRemote has already started but didn't initialize!!!");
      return;
    }
    Services.obs.addObserver(browserWindowImpl => {
      debug("New web embedder created.");
      window.browserDOMWindow = browserWindowImpl;

      // Notify the the shell is ready at the next event loop tick to
      // let the embedder user a chance to add event listeners.
      window.setTimeout(() => {
        Services.obs.notifyObservers(window, "shell-ready");
      }, 0);
    }, "web-embedder-created");

    remoteShell.start();
  },
  { once: true }
);
