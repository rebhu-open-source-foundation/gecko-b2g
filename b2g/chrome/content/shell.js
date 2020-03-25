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
ChromeUtils.import("resource://gre/modules/DownloadService.jsm");

const isGonk = AppConstants.platform === "gonk";

if (isGonk) {
  XPCOMUtils.defineLazyGetter(this, "libcutils", () => {
    const { libcutils } = ChromeUtils.import(
      "resource://gre/modules/systemlibs.js"
    );
    return libcutils;
  });
}

function debug(str) {
  console.log(`-*- Shell.js: ${str}`);
}

var shell = {
  get startURL() {
    let url = Services.prefs.getCharPref("b2g.system_startup_url");
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
    isGonk && libcutils.property_set("sys.boot_completed", "1");
    // This will cause Gonk Widget to remove boot animation from the screen
    // and reveals the page.
    Services.obs.notifyObservers(null, "browser-ui-startup-complete");
  },
};

function toggle_bool_pref(name) {
  let current = Services.prefs.getBoolPref(name);
  Services.prefs.setBoolPref(name, !current);
  debug(`${name} is now ${!current}`);
}

document.addEventListener(
  "DOMContentLoaded",
  () => {
    if (shell.hasStarted()) {
      // Shoudl never happen!
      console.error("Shell has already started but didn't initialize!!!");
      return;
    }

    document.addEventListener(
      "keydown",
      event => {
        if (event.key == "AudioVolumeUp") {
          console.log("Toggling GPU profiler display");
          toggle_bool_pref("gfx.webrender.debug.profiler");
          toggle_bool_pref("gfx.webrender.debug.compact-profiler");
        }
      },
      true
    );

    // eslint-disable-next-line no-undef
    RemoteDebugger.init(window);

    Services.obs.addObserver(browserWindowImpl => {
      debug("New web embedder created.");
      window.browserDOMWindow = browserWindowImpl;

      // Notify the the shell is ready at the next event loop tick to
      // let the embedder user a chance to add event listeners.
      window.setTimeout(() => {
        Services.obs.notifyObservers(window, "shell-ready");
      }, 0);
    }, "web-embedder-created");

    // Initialize Marionette server
    Services.tm.idleDispatchToMainThread(() => {
      Services.obs.notifyObservers(null, "marionette-startup-requested");
    });

    shell.start();
  },
  { once: true }
);

// Install the self signed certificate for locally served apps.
function setup_local_https() {
  const { LocalDomains } = ChromeUtils.import(
    "resource://gre/modules/LocalDomains.jsm"
  );

  LocalDomains.init();
  if (LocalDomains.scan()) {
    debug(`Updating local domains list to: ${LocalDomains.get()}`);
    LocalDomains.update();
  }
}

// We need to set this up early to be able to launch the homescreen.
setup_local_https();
