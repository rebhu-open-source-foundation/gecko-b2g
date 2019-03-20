/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- /
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");

function debug(str) {
  console.log(`-*- Shell.js: ${str}`);
}

var shell = {

  get startURL() {
    let url = Services.prefs.getCharPref("b2g.system_startup_url");
    debug(`startURL is ${url}`);
    return "http://localhost:8081/system/index.html";
  },

  _started: false,
  hasStarted: function() {
    return this._started;
  },

  start: function() {
   let allowed = Services.perms.testPermission(Services.io.newURI("http://localhost:8081/system/index.html"), "browser");

    debug("In start permission=" + allowed);
    this._started = true;

    // This forces the initialization of the cookie service before we hit the
    // network.
    // See bug 810209
    let cookies = Cc["@mozilla.org/cookieService;1"];

    let startURL = this.startURL;
    
    // <html:iframe id="systemapp"
    //              mozbrowser="true" allowfullscreen="true"
    //              style="overflow: hidden; height: 100%; width: 100%; border: none;"
    //              src="data:text/html;charset=utf-8,%3C!DOCTYPE html>%3Cbody style="background:black;">"/>
    let systemAppFrame =
      document.createElementNS("http://www.w3.org/1999/xhtml", "html:iframe");
    systemAppFrame.setAttribute("id", "systemapp");
    systemAppFrame.setAttribute("mozbrowser", "true");
    systemAppFrame.setAttribute("allowfullscreen", "true");
    systemAppFrame.setAttribute("src", "blank.html");
    let container = document.getElementById("container");

    debug(`Loading blank.html`);

    this.contentBrowser = container.appendChild(systemAppFrame);
    
    // On firefox mulet, shell.html is loaded in a tab
    // and we have to listen on the chrome event handler
    // to catch key events
    let chromeEventHandler = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIWebNavigation)
                                   .QueryInterface(Ci.nsIDocShell)
                                   .chromeEventHandler || window;
    // Capture all key events so we can filter out hardware buttons
    // And send them to Gaia via mozChromeEvents.
    // Ideally, hardware buttons wouldn"t generate key events at all, or
    // if they did, they would use keycodes that conform to DOM 3 Events.
    // See discussion in https://bugzilla.mozilla.org/show_bug.cgi?id=762362
    chromeEventHandler.addEventListener("keydown", this, true);
    chromeEventHandler.addEventListener("keyup", this, true);

    window.addEventListener("MozAfterPaint", this);
    window.addEventListener("sizemodechange", this);
    window.addEventListener("unload", this);
    this.contentBrowser.addEventListener("mozbrowserloadstart", this, true);
    this.contentBrowser.addEventListener("mozbrowserscrollviewchange", this, true);
    this.contentBrowser.addEventListener("mozbrowsercaretstatechanged", this);

    CustomEventManager.init();

    debug(`Setting system url to ${startURL}`);

    this.contentBrowser.src = startURL;
    this._isEventListenerReady = false;
    
  },

  stop: function() {
    window.removeEventListener("unload", this);
    window.removeEventListener("keydown", this, true);
    window.removeEventListener("keyup", this, true);
    window.removeEventListener("sizemodechange", this);
    this.contentBrowser.removeEventListener("mozbrowserloadstart", this, true);
    this.contentBrowser.removeEventListener("mozbrowserscrollviewchange", this, true);
    this.contentBrowser.removeEventListener("mozbrowsercaretstatechanged", this);
  },

  handleEvent: function(evt) {
    debug(`event: ${event.type}`);

    function checkReloadKey() {
      // "b2g.reload_key" is only defined for Graphene.
      if (!AppConstants.MOZ_GRAPHENE) {
        return false;
      }

      if (evt.type !== "keyup") {
        return false;
      }

      try {
        let key = JSON.parse(Services.prefs.getCharPref("b2g.reload_key"));
        return (evt.keyCode  == key.key   &&
                evt.ctrlKey  == key.ctrl  &&
                evt.altKey   == key.alt   &&
                evt.shiftKey == key.shift &&
                evt.metaKey  == key.meta);
      } catch(e) {
        debug("Failed to get key: " + e);
      }

      return false;
    }

    let content = this.contentBrowser.contentWindow;
    switch (evt.type) {
      case "keydown":
      case "keyup":
        if (checkReloadKey()) {
          clearCacheAndReload();
        } else {
          this.broadcastHardwareKeys(evt);
        }
        break;
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
      case "load":
        if (content.document.location == "about:blank") {
          return;
        }
        content.removeEventListener("load", this, true);
        this.notifyContentWindowLoaded();
        break;
      case "mozbrowserloadstart":
        if (content.document.location == "about:blank") {
          this.contentBrowser.addEventListener("mozbrowserlocationchange", this, true);
          return;
        }

        this.notifyContentStart();
        break;
      case "mozbrowserlocationchange":
        if (content.document.location == "about:blank") {
          return;
        }

        this.notifyContentStart();
       break;
      case "mozbrowserscrollviewchange":
        this.sendChromeEvent({
          type: "scrollviewchange",
          detail: evt.detail,
        });
        break;
      case "mozbrowsercaretstatechanged":
        {
          let elt = evt.target;
          let win = elt.ownerDocument.defaultView;
          let offsetX = win.mozInnerScreenX - window.mozInnerScreenX;
          let offsetY = win.mozInnerScreenY - window.mozInnerScreenY;

          let rect = elt.getBoundingClientRect();
          offsetX += rect.left;
          offsetY += rect.top;

          let data = evt.detail;
          data.offsetX = offsetX;
          data.offsetY = offsetY;
          data.sendDoCommandMsg = null;

          shell.sendChromeEvent({
            type: "caretstatechanged",
            detail: data,
          });
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

  sendCustomEvent: function(type, details) {
    SystemAppProxy._sendCustomEvent(type, details);
  },

  sendChromeEvent: function shell_sendChromeEvent(details) {
    this.sendCustomEvent("mozChromeEvent", details);
  },

  notifyContentStart: function shell_notifyContentStart() {
    debug("notifyContentStart");
    this.contentBrowser.removeEventListener("mozbrowserloadstart", this, true);
    this.contentBrowser.removeEventListener("mozbrowserlocationchange", this, true);

    let content = this.contentBrowser.contentWindow;
    content.addEventListener("load", this, true);

    Services.obs.notifyObservers(null, "content-start", null);
  },

  // This gets called when window.onload fires on the System app content window,
  // which means things in <html> are parsed and statically referenced <script>s
  // and <script defer>s are loaded and run.
  notifyContentWindowLoaded: function shell_notifyContentWindowLoaded() {
    debug("notifyContentWindowLoaded");
    // isGonk && libcutils.property_set("sys.boot_completed", "1");

    // This will cause Gonk Widget to remove boot animation from the screen
    // and reveals the page.
    Services.obs.notifyObservers(null, "browser-ui-startup-complete", "");

    // SystemAppProxy.setIsLoaded();
  },

  // This gets called when the content sends us system-message-listener-ready
  // mozContentEvent, OR when an observer message tell us we should consider
  // the content as ready.
  notifyEventListenerReady: function shell_notifyEventListenerReady() {
    if (this._isEventListenerReady) {
      Cu.reportError("shell.js: SystemApp has already been declared as being ready.");
      return;
    }
    this._isEventListenerReady = true;
  }
};

Services.obs.addObserver(function(subject, topic, data) {
  shell.notifyEventListenerReady();
}, "system-message-listener-ready", false);

var permissionMap = new Map([
  ["unknown", Services.perms.UNKNOWN_ACTION],
  ["allow", Services.perms.ALLOW_ACTION],
  ["deny", Services.perms.DENY_ACTION],
  ["prompt", Services.perms.PROMPT_ACTION],
]);
var permissionMapRev = new Map(Array.from(permissionMap.entries()).reverse());

var CustomEventManager = {
  init: function custevt_init() {
    window.addEventListener("ContentStart", (function(evt) {
      let content = shell.contentBrowser.contentWindow;
      content.addEventListener("mozContentEvent", this, false, true);
    }).bind(this), false);
  },

  handleEvent: function custevt_handleEvent(evt) {
    let detail = evt.detail;
    dump("XXX FIXME : Got a mozContentEvent: " + detail.type + "\n");

    switch(detail.type) {
      case "system-message-listener-ready":
        Services.obs.notifyObservers(null, "system-message-listener-ready", null);
        break;
      case "captive-portal-login-cancel":
        CaptivePortalLoginHelper.handleEvent(detail);
        break;
      case "inputmethod-update-layouts":
      case "inputregistry-add":
      case "inputregistry-remove":
        KeyboardHelper.handleEvent(detail);
        break;
      case "copypaste-do-command":
        Services.obs.notifyObservers({ wrappedJSObject: shell.contentBrowser },
                                     "ask-children-to-execute-copypaste-command", detail.cmd);
        break;
      case "add-permission":
        Services.perms.add(Services.io.newURI(detail.uri, null, null),
                           detail.permissionType, permissionMap.get(detail.permission));
        break;
      case "remove-permission":
        Services.perms.remove(Services.io.newURI(detail.uri, null, null),
                              detail.permissionType);
        break;
      case "test-permission":
        let result = Services.perms.testExactPermission(
          Services.io.newURI(detail.uri, null, null), detail.permissionType);
        // Not equal check here because we want to prevent default only if it"s not set
        if (result !== permissionMapRev.get(detail.permission)) {
          evt.preventDefault();
        }
        break;
      case "shutdown-application":
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                           .getService(Ci.nsIAppStartup);
        appStartup.quit(appStartup.eAttemptQuit);
        break;
      case "toggle-fullscreen-native-window":
        window.fullScreen = !window.fullScreen;
        Services.prefs.setBoolPref("b2g.nativeWindowGeometry.fullscreen",
                                   window.fullScreen);
        break;
      case "minimize-native-window":
        window.minimize();
        break;
      case "clear-cache-and-reload":
        clearCacheAndReload();
        break;
      case "clear-cache-and-restart":
        clearCache();
        restart();
        break;
      case "restart":
        restart();
        break;
    }
  }
}

document.addEventListener("DOMContentLoaded", function dom_loaded() {
  document.removeEventListener("DOMContentLoaded", dom_loaded);
  
  try {
  if (!shell.hasStarted()) {
    // Give the browser permission to the shell and the system app.
    Services.perms.add(Services.io.newURI("chrome://b2g/content/shell.html"), "browser", Services.perms.ALLOW_ACTION);
    Services.perms.add(Services.io.newURI("http://localhost:8081"), "browser", Services.perms.ALLOW_ACTION);

    debug("About to start");
    shell.start();
  }
} catch(e) { debug(e); }
});
