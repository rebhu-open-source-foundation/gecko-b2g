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

const isGonk = AppConstants.platform === "gonk";

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

    CustomEventManager.init();

    debug(`Setting system url to ${startURL}`);

    this.contentBrowser.src = startURL;

    if (isGonk) {
      ChromeUtils.import("resource://gre/modules/ExternalAPIService.jsm");
    }
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
        // This event should be sent before System app returns with
        // system-message-listener-ready mozContentEvent, because it"s on
        // the critical launch path of the app.
        // SystemAppProxy._sendCustomEvent("mozChromeEvent", {
        //   type: "system-first-paint"
        // }, /* noPending */ true);
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
    // isGonk && libcutils.property_set("sys.boot_completed", "1");

    // This will cause Gonk Widget to remove boot animation from the screen
    // and reveals the page.
    Services.obs.notifyObservers(null, "browser-ui-startup-complete");
  },
};

var CustomEventManager = {
  init: function custevt_init() {
    window.addEventListener(
      "ContentStart",
      function(evt) {
        let content = shell.contentBrowser.contentWindow;
        content.addEventListener("mozContentEvent", this, false, true);
      }.bind(this)
    );
  },

  handleEvent: function custevt_handleEvent(evt) {
    let detail = evt.detail;
    debug(`XXX FIXME : Got a mozContentEvent:  ${detail.type}`);

    switch (detail.type) {
      case "captive-portal-login-cancel":
        // CaptivePortalLoginHelper.handleEvent(detail);
        break;
      case "copypaste-do-command":
        Services.obs.notifyObservers(
          { wrappedJSObject: shell.contentBrowser },
          "ask-children-to-execute-copypaste-command",
          detail.cmd
        );
        break;
    }
  },
};

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
          console.log("Toggle fps display");
          let current = Services.prefs.getBoolPref(
            "layers.acceleration.draw-fps"
          );
          Services.prefs.setBoolPref("layers.acceleration.draw-fps", !current);
        }
      },
      true
    );

    // Loads the Keyboard API.
    if (Services.prefs.getBoolPref("dom.mozInputMethod.enabled")) {
      debug(`Loading Keyboard API`);
      ChromeUtils.import("resource://gre/modules/Keyboard.jsm");

      let mm = Services.mm;
      mm.loadFrameScript("chrome://global/content/forms.js", false, true);
      mm.loadFrameScript(
        "chrome://global/content/formsVoiceInput.js",
        false,
        true
      );
    }

    // debug(`Input Method API enabled: ${Services.prefs.getBoolPref("dom.mozInputMethod.enabled")}`);
    // debug(`Input Method implementation: ${Cc["@mozilla.org/b2g-inputmethod;1"].createInstance()}`);

    // Give the needed permissions to the system app.
    // TODO: switch to perms.addFromPrincipal if needed.
    // ["browser", "input", "input-manage"].forEach(permission => {
    //   Services.perms.add(Services.io.newURI(shell.startURL), permission, Services.perms.ALLOW_ACTION);
    // });

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

    shell.start();
  },
  { once: true }
);

// Install the self signed certificate for locally served apps.
function setup_local_https() {
  const { NetUtil } = ChromeUtils.import("resource://gre/modules/NetUtil.jsm");

  // On desktop the certificate is located in the profile directory for now, and
  // on device it is in the default system folder.
  let file = Services.dirsvc.get(isGonk ? "DefRt" : "ProfD", Ci.nsIFile);
  file.append("local-cert.pem");
  debug(`Loading certs from ${file.path}`);

  let fstream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(
    Ci.nsIFileInputStream
  );
  fstream.init(file, -1, 0, 0);
  let data = NetUtil.readInputStreamToString(fstream, fstream.available());
  fstream.close();

  // Get the base64 content only as a 1-liner.
  data = data
    .replace(/-----BEGIN CERTIFICATE-----/, "")
    .replace(/-----END CERTIFICATE-----/, "")
    .replace(/[\r\n]/g, "");

  let certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(
    Ci.nsIX509CertDB
  );
  try {
    certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(
      Ci.nsIX509CertDB2
    );
  } catch (e) {}

  let cert = certdb.addCertFromBase64(data, "CT,CT,CT");

  debug(`Certificate added for ${cert.subjectAltNames}`);

  let overrideService = Cc["@mozilla.org/security/certoverride;1"].getService(
    Ci.nsICertOverrideService
  );

  // Reuse the list of hosts that we force to resolve to 127.0.0.1 to add the overrides.
  Services.prefs
    .getCharPref("network.dns.localDomains")
    .split(",")
    .forEach(host => {
      host = host.trim();
      overrideService.rememberValidityOverride(
        host,
        8081,
        cert,
        overrideService.ERROR_UNTRUSTED | overrideService.ERROR_MISMATCH,
        false /* temporary */
      );
    });
}

// We need to set this up early to be able to launch the homescreen.
setup_local_https();
