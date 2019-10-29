/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

var EXPORTED_SYMBOLS = ["WebViewChild"];

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const kLongestReturnedString = 128;

var WebViewChild = {
  // Prints arguments separated by a space and appends a new line.
  log(...args) {
    dump("WebViewChild: ");
    for (let a of args) {
      dump(a + " ");
    }
    dump("\n");
  },

  init(global) {
    this.global = global;

    // A cache of the menuitem dom objects keyed by the id we generate
    // and pass to the embedder
    this._ctxHandlers = {};
    // Counter of contextmenu events fired
    this._ctxCounter = 0;

    let els = Services.els;

    // We are using the system group for those events so if something in the
    // content called .stopPropagation() this will still be called.
    els.addSystemEventListener(
      global,
      "DOMWindowClose",
      this.windowCloseHandler.bind(this),
      /* useCapture = */ false
    );
    els.addSystemEventListener(
      global,
      "DOMWindowCreated",
      this.windowCreatedHandler.bind(this),
      /* useCapture = */ true
    );
    els.addSystemEventListener(
      global,
      "DOMWindowResize",
      this.windowResizeHandler.bind(this),
      /* useCapture = */ false
    );
    els.addSystemEventListener(
      global,
      "contextmenu",
      this.contextmenuHandler.bind(this),
      /* useCapture = */ false
    );
    els.addSystemEventListener(
      global,
      "scroll",
      this.scrollEventHandler.bind(this),
      /* useCapture = */ false
    );

    global.addMessageListener(
      "WebView::fire-ctx-callback",
      this.recvFireCtxCallback.bind(this)
    );

    let metachange_handler = this.metaChangeHandler.bind(this);
    this.global.addEventListener(
      "DOMMetaAdded",
      metachange_handler,
      /* useCapture = */ true,
      /* wantsUntrusted = */ false
    );

    this.global.addEventListener(
      "DOMMetaChanged",
      metachange_handler,
      /* useCapture = */ true,
      /* wantsUntrusted = */ false
    );

    this.global.addEventListener(
      "DOMMetaRemoved",
      metachange_handler,
      /* useCapture = */ true,
      /* wantsUntrusted = */ false
    );

    // Remote the value of the background color since the parent can't get
    // it directly in its progress listener.
    // This will be dispatched before the parent's loadend so we can use
    // this value in the loadend event handler of the <web-view> element.
    let seenLoadStart = false;
    let seenLoadEnd = false;
    let progress_listener = {
      QueryInterface: ChromeUtils.generateQI([
        Ci.nsIWebProgressListener,
        Ci.nsISupportsWeakReference,
      ]),

      onStateChange(webProgress, request, stateFlags, status) {
        if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
          seenLoadStart = true;
        }

        if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          let backgroundcolor = "transparent";
          try {
            backgroundcolor = global.content
              .getComputedStyle(global.content.document.body)
              .getPropertyValue("background-color");
          } catch (e) {}
          if (seenLoadStart && !seenLoadEnd) {
            global.sendAsyncMessage("WebView::backgroundcolor", {
              backgroundcolor,
            });
            seenLoadEnd = true;
          }
        }
      },
    };

    this.global.docShell
      .QueryInterface(Ci.nsIWebProgress)
      .addProgressListener(
        progress_listener,
        Ci.nsIWebProgress.NOTIFY_STATE_WINDOW
      );
  },

  metaChangeHandler(event) {
    let win = event.target.ownerGlobal;
    if (win != this.global.content) {
      return;
    }

    let name = event.target.name;
    let property = event.target.getAttributeNS(null, "property");

    if (!name && !property) {
      return;
    }

    this.log(`Got metaChanged: (${name || property}) ${event.target.content}`);

    let generic_handler = this.genericMetaHandler.bind(this);

    let handlers = {
      viewmode: generic_handler,
      "theme-color": generic_handler,
      "theme-group": generic_handler,
      "application-name": this.applicationNameChangedHandler.bind(this),
    };
    let handler = handlers[name];

    if ((property || name).match(/^og:/)) {
      name = property || name;
      handler = generic_handler;
    }

    if (handler) {
      handler(name, event.type, event.target);
    }
  },

  genericMetaHandler(name, eventType, target) {
    let meta = {
      name,
      content: target.content,
      type: eventType.replace("DOMMeta", "").toLowerCase(),
    };
    this.global.sendAsyncMessage("WebView::metachange", meta);
  },

  applicationNameChangedHandler(name, eventType, target) {
    if (eventType !== "DOMMetaAdded") {
      // Bug 1037448 - Decide what to do when <meta name="application-name">
      // changes
      return;
    }

    let meta = { name, content: target.content };

    let lang;
    let elm;

    for (
      elm = target;
      !lang && elm && elm.nodeType == target.ELEMENT_NODE;
      elm = elm.parentNode
    ) {
      if (elm.hasAttribute("lang")) {
        lang = elm.getAttribute("lang");
        continue;
      }

      if (elm.hasAttributeNS("http://www.w3.org/XML/1998/namespace", "lang")) {
        lang = elm.getAttributeNS(
          "http://www.w3.org/XML/1998/namespace",
          "lang"
        );
        continue;
      }
    }

    // No lang has been detected.
    if (!lang && elm.nodeType == target.DOCUMENT_NODE) {
      lang = elm.contentLanguage;
    }

    if (lang) {
      meta.lang = lang;
    }

    this.global.sendAsyncMessage("WebView::metachange", meta);
  },

  addMozAfterPaintHandler(callback) {
    let self = this;
    function onMozAfterPaint() {
      let uri = self.global.docShell.QueryInterface(Ci.nsIWebNavigation)
        .currentURI;
      if (uri.spec != "about:blank") {
        self.log(`Got afterpaint event: ${uri.spec}`);
        self.global.removeEventListener(
          "MozAfterPaint",
          onMozAfterPaint,
          /* useCapture = */ true
        );
        callback();
      }
    }
    this.global.addEventListener(
      "MozAfterPaint",
      onMozAfterPaint,
      /* useCapture = */ true
    );
    return onMozAfterPaint;
  },

  windowCreatedHandler(event) {
    let targetDocShell = event.target.defaultView.docShell;
    if (targetDocShell != this.global.docShell) {
      return;
    }

    let uri = this.global.docShell.QueryInterface(Ci.nsIWebNavigation)
      .currentURI;
    this.log("Window created: " + uri.spec);
    if (uri.spec != "about:blank") {
      this.addMozAfterPaintHandler(() => {
        this.global.sendAsyncMessage("WebView::documentfirstpaint");
      });
    }
  },

  windowCloseHandler(event) {
    let win = event.target;
    if (win != this.global.content || event.defaultPrevented) {
      return;
    }

    this.log("Closing window " + win);
    this.global.sendAsyncMessage("WebView::close");

    // Inform the window implementation that we handled this close ourselves.
    event.preventDefault();
  },

  windowResizeHandler(event) {
    let win = event.target;
    if (win != this.global.content || event.defaultPrevented) {
      return;
    }

    this.log("resizing window " + win);
    this.global.sendAsyncMessage("WebView::resize", {
      width: event.detail.width,
      height: event.detail.height,
    });

    // Inform the window implementation that we handled this resize ourselves.
    event.preventDefault();
  },

  contextmenuHandler(event) {
    this.log(event.type);
    if (event.defaultPrevented) {
      return;
    }

    this._ctxCounter++;
    this._ctxHandlers = {};

    let elem = event.target;
    let menuData = { systemTargets: [], contextmenu: null };
    let ctxMenuId = null;
    let clipboardPlainTextOnly = Services.prefs.getBoolPref(
      "clipboard.plainTextOnly"
    );
    var copyableElements = {
      image: false,
      link: false,
      hasElements: () => {
        return this.image || this.link;
      },
    };

    // Set the event target as the copy image command needs it to
    // determine what was context-clicked on.
    this.global.docShell.contentViewer
      .QueryInterface(Ci.nsIContentViewerEdit)
      .setCommandNode(elem);

    while (elem && elem.parentNode) {
      let ctxData = this.getSystemCtxMenuData(elem);
      if (ctxData) {
        menuData.systemTargets.push({
          nodeName: elem.nodeName,
          data: ctxData,
        });
      }

      if (
        !ctxMenuId &&
        "hasAttribute" in elem &&
        elem.hasAttribute("contextmenu")
      ) {
        ctxMenuId = elem.getAttribute("contextmenu");
      }

      // Enable copy image/link option
      if (elem.nodeName == "IMG") {
        copyableElements.image = !clipboardPlainTextOnly;
      } else if (elem.nodeName == "A") {
        copyableElements.link = true;
      }

      elem = elem.parentNode;
    }

    if (ctxMenuId || copyableElements.hasElements()) {
      var menu = null;
      if (ctxMenuId) {
        menu = event.target.ownerDocument.getElementById(ctxMenuId);
      }
      menuData.contextmenu = this.buildMenuObj(menu, "", copyableElements);
    }

    // Pass along the position where the context menu should be located
    menuData.clientX = event.clientX;
    menuData.clientY = event.clientY;
    menuData.screenX = event.screenX;
    menuData.screenY = event.screenY;

    // The value returned by the contextmenu sync call is true if the embedder
    // called preventDefault() on its contextmenu event.
    //
    // We call preventDefault() on our contextmenu event if the embedder called
    // preventDefault() on /its/ contextmenu event.  This way, if the embedder
    // ignored the contextmenu event, TabChild will fire a click.
    if (this.global.sendSyncMessage("WebView::contextmenu", menuData)[0]) {
      event.preventDefault();
    } else {
      this._ctxHandlers = {};
    }
  },

  maybeCopyAttribute(src, target, attribute) {
    if (src.getAttribute(attribute)) {
      target[attribute] = src.getAttribute(attribute);
    }
  },

  buildMenuObj(menu, idPrefix, copyableElements) {
    let menuObj = { type: "menu", customized: false, items: [] };
    // Customized context menu
    if (menu) {
      this.maybeCopyAttribute(menu, menuObj, "label");

      for (let i = 0, child; (child = menu.children[i++]); ) {
        if (child.nodeName === "MENU") {
          menuObj.items.push(
            this.buildMenuObj(child, idPrefix + i + "_", false)
          );
        } else if (child.nodeName === "MENUITEM") {
          let id = this._ctxCounter + "_" + idPrefix + i;
          let menuitem = { id, type: "menuitem" };
          this.maybeCopyAttribute(child, menuitem, "label");
          this.maybeCopyAttribute(child, menuitem, "icon");
          this._ctxHandlers[id] = child;
          menuObj.items.push(menuitem);
        }
      }

      if (menuObj.items.length) {
        menuObj.customized = true;
      }
    }
    // Note: Display "Copy Link" first in order to make sure "Copy Image" is
    //       put together with other image options if elem is an image link.
    // "Copy Link" menu item
    if (copyableElements.link) {
      menuObj.items.push({ id: "copy-link" });
    }
    // "Copy Image" menu item
    if (copyableElements.image) {
      menuObj.items.push({ id: "copy-image" });
    }

    return menuObj;
  },

  getSystemCtxMenuData(elem) {
    let documentURI = this.global.docShell.QueryInterface(Ci.nsIWebNavigation)
      .currentURI.spec;
    let content = this.global.content;
    if (
      (elem instanceof content.HTMLAnchorElement && elem.href) ||
      (elem instanceof content.HTMLAreaElement && elem.href)
    ) {
      return {
        uri: elem.href,
        documentURI,
        text: elem.textContent.substring(0, kLongestReturnedString),
      };
    }
    if (elem instanceof Ci.nsIImageLoadingContent && elem.currentURI) {
      return { uri: elem.currentURI.spec, documentURI };
    }
    if (elem instanceof content.HTMLImageElement) {
      return { uri: elem.src, documentURI };
    }
    if (elem instanceof content.HTMLMediaElement) {
      let hasVideo = !(
        elem.readyState >= elem.HAVE_METADATA &&
        (elem.videoWidth == 0 || elem.videoHeight == 0)
      );
      return {
        uri: elem.currentSrc || elem.src,
        hasVideo,
        documentURI,
      };
    }
    if (elem instanceof content.HTMLInputElement && elem.hasAttribute("name")) {
      // For input elements, we look for a parent <form> and if there is
      // one we return the form's method and action uri.
      let parent = elem.parentNode;
      while (parent) {
        if (
          parent instanceof content.HTMLFormElement &&
          parent.hasAttribute("action")
        ) {
          let actionHref = this.global.docShell
            .QueryInterface(Ci.nsIWebNavigation)
            .currentURI.resolve(parent.getAttribute("action"));
          let method = parent.hasAttribute("method")
            ? parent.getAttribute("method").toLowerCase()
            : "get";
          return {
            documentURI,
            action: actionHref,
            method,
            name: elem.getAttribute("name"),
          };
        }
        parent = parent.parentNode;
      }
    }
    return false;
  },

  recvFireCtxCallback(data) {
    this.log(`Received fireCtxCallback message: (${data.json.menuitem})`);

    let doCommandIfEnabled = command => {
      if (this.global.docShell.isCommandEnabled(command)) {
        this.global.docShell.doCommand(command);
      }
    };

    if (data.json.menuitem == "copy-image") {
      doCommandIfEnabled("cmd_copyImage");
    } else if (data.json.menuitem == "copy-link") {
      doCommandIfEnabled("cmd_copyLink");
    } else if (data.json.menuitem in this._ctxHandlers) {
      this._ctxHandlers[data.json.menuitem].click();
      this._ctxHandlers = {};
    } else {
      // We silently ignore if the embedder uses an incorrect id in the callback
      this.log("Ignored invalid contextmenu invocation");
    }
  },

  scrollEventHandler(event) {
    let win = event.target;
    if (win != this.global.content || event.defaultPrevented) {
      return;
    }

    this.log("scroll event " + win);
    this.global.sendAsyncMessage("WebView::scroll", {
      top: win.scrollY,
      left: win.scrollX,
    });
  },
};
