"use strict";

// A <web-view> custom element, wrapping a <xul:browser>

(function() {
    const {Services} = ChromeUtils.import("resource://gre/modules/Services.jsm");

    // Use a prefix to help with backward compatibility and ease UI porting.
    const EVENT_PREFIX = "mozbrowser";

    // The progress listener attached to the webview, managing some events.
    function ProgressListener(webview) {
        this.webview = webview;
    }

    ProgressListener.prototype =  {
        log(msg) {
            console.log(`<web-view-listener> ${msg}`);
        },

        error(msg) {
            console.error(`<web-view-listener> ${msg}`);
        },

        dispatchEvent(name, detail) {
            this.log(`dispatching ${EVENT_PREFIX}${name}`);
            let event = new CustomEvent(`${EVENT_PREFIX}${name}`, { bubbles: true, detail });
            this.webview.dispatchEvent(event);
        },

        QueryInterface: ChromeUtils.generateQI([Ci.nsIWebProgressListener,
                                                Ci.nsISupportsWeakReference]),
        _seenLoadStart: false,
        _seenLoadEnd: false,
    
        onLocationChange(webProgress, request, location, flags) {
          this.log(`onLocationChange ${location.spec}`);
    
          // Ignore locationchange events which occur before the first loadstart.
          // These are usually about:blank loads we don't care about.
          if (!this._seenLoadStart) {
            this.log(`loadstart not seen yet, not dispatching locationchange`);
            return;
          }
    
          // Remove password from uri.
          location = Services.uriFixup.createExposableURI(location);
    
          this.dispatchEvent(`locationchange`, { url: location.spec,
                                           canGoBack: this.webview.canGoBack,
                                           canGoForward: this.webview.canGoForward });
        },
    
        // eslint-disable-next-line complexity
        onStateChange(webProgress, request, stateFlags, status) {
          // this.log(`onStateChange ${stateFlags}`);

          if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
            !this._seenLoadStart && this.dispatchEvent("loadstart");
            this._seenLoadStart = true;
          }
    
          if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
            let backgroundColor = "transparent";
            // TODO: get the background color.
            // try {
            //     backgroundColor = content.getComputedStyle(content.document.body)
            //                              .getPropertyValue("background-color");
            // } catch (e) { this.error(e); }
            if (this._seenLoadStart && !this._seenLoadEnd) {
              this.dispatchEvent("loadend", { backgroundColor });
              this._seenLoadEnd = true;
            }

            switch (status) {
              case Cr.NS_OK :
              case Cr.NS_BINDING_ABORTED :
                // Ignoring NS_BINDING_ABORTED, which is set when loading page is
                // stopped.
              case Cr.NS_ERROR_PARSED_DATA_CACHED:
                return;
    
              // TODO See nsDocShell::DisplayLoadError to see what extra
              // information we should be annotating this first block of errors
              // with. Bug 1107091.
              case Cr.NS_ERROR_UNKNOWN_PROTOCOL :
                this.dispatchEvent("error", { type: "unknownProtocolFound" });
                return;
              case Cr.NS_ERROR_FILE_NOT_FOUND :
                this.dispatchEvent("error", { type: "fileNotFound" });
                return;
              case Cr.NS_ERROR_UNKNOWN_HOST :
                this.dispatchEvent("error", { type: "dnsNotFound" });
                return;
              case Cr.NS_ERROR_CONNECTION_REFUSED :
                this.dispatchEvent("error", { type: "connectionFailure" });
                return;
              case Cr.NS_ERROR_NET_INTERRUPT :
                this.dispatchEvent("error", { type: "netInterrupt" });
                return;
              case Cr.NS_ERROR_NET_TIMEOUT :
                this.dispatchEvent("error", { type: "netTimeout" });
                return;
              case Cr.NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION :
                this.dispatchEvent("error", { type: "cspBlocked" });
                return;
              case Cr.NS_ERROR_PHISHING_URI :
                this.dispatchEvent("error", { type: "deceptiveBlocked" });
                return;
              case Cr.NS_ERROR_MALWARE_URI :
                this.dispatchEvent("error", { type: "malwareBlocked" });
                return;
              case Cr.NS_ERROR_HARMFUL_URI :
                this.dispatchEvent("error", { type: "harmfulBlocked" });
                return;
              case Cr.NS_ERROR_UNWANTED_URI :
                this.dispatchEvent("error", { type: "unwantedBlocked" });
                return;
              case Cr.NS_ERROR_FORBIDDEN_URI :
                this.dispatchEvent("error", { type: "forbiddenBlocked" });
                return;
    
              case Cr.NS_ERROR_OFFLINE :
                this.dispatchEvent("error", { type: "offline" });
                return;
              case Cr.NS_ERROR_MALFORMED_URI :
                this.dispatchEvent("error", { type: "malformedURI" });
                return;
              case Cr.NS_ERROR_REDIRECT_LOOP :
                this.dispatchEvent("error", { type: "redirectLoop" });
                return;
              case Cr.NS_ERROR_UNKNOWN_SOCKET_TYPE :
                this.dispatchEvent("error", { type: "unknownSocketType" });
                return;
              case Cr.NS_ERROR_NET_RESET :
                this.dispatchEvent("error", { type: "netReset" });
                return;
              case Cr.NS_ERROR_DOCUMENT_NOT_CACHED :
                this.dispatchEvent("error", { type: "notCached" });
                return;
              case Cr.NS_ERROR_DOCUMENT_IS_PRINTMODE :
                this.dispatchEvent("error", { type: "isprinting" });
                return;
              case Cr.NS_ERROR_PORT_ACCESS_NOT_ALLOWED :
                this.dispatchEvent("error", { type: "deniedPortAccess" });
                return;
              case Cr.NS_ERROR_UNKNOWN_PROXY_HOST :
                this.dispatchEvent("error", { type: "proxyResolveFailure" });
                return;
              case Cr.NS_ERROR_PROXY_CONNECTION_REFUSED :
                this.dispatchEvent("error", { type: "proxyConnectFailure" });
                return;
              case Cr.NS_ERROR_INVALID_CONTENT_ENCODING :
                this.dispatchEvent("error", { type: "contentEncodingFailure" });
                return;
              case Cr.NS_ERROR_REMOTE_XUL :
                this.dispatchEvent("error", { type: "remoteXUL" });
                return;
              case Cr.NS_ERROR_UNSAFE_CONTENT_TYPE :
                this.dispatchEvent("error", { type: "unsafeContentType" });
                return;
              case Cr.NS_ERROR_CORRUPTED_CONTENT :
                this.dispatchEvent("error", { type: "corruptedContentErrorv2" });
                return;
              case Cr.NS_ERROR_BLOCKED_BY_POLICY :
                this.dispatchEvent("error", { type: "blockedByPolicy" });
                return;
    
              default:
                // getErrorClass() will throw if the error code passed in is not a NSS
                // error code.
                try {
                  let nssErrorsService = Cc["@mozilla.org/nss_errors_service;1"]
                                           .getService(Ci.nsINSSErrorsService);
                  if (nssErrorsService.getErrorClass(status)
                        == Ci.nsINSSErrorsService.ERROR_CLASS_BAD_CERT) {
                    // XXX Is there a point firing the event if the error page is not
                    // certerror? If yes, maybe we should add a property to the
                    // event to to indicate whether there is a custom page. That would
                    // let the embedder have more control over the desired behavior.
                    let errorPage = Services.prefs.getCharPref(CERTIFICATE_ERROR_PAGE_PREF, "");
    
                    if (errorPage == "certerror") {
                      this.dispatchEvent("error", { type: "certerror" });
                      return;
                    }
                  }
                } catch (e) {}
    
                this.dispatchEvent("error", { type: "other" });
            }
          }
        },
    
        onSecurityChange(webProgress, request, state) {
          var securityStateDesc;
          if (state & Ci.nsIWebProgressListener.STATE_IS_SECURE) {
            securityStateDesc = "secure";
          } else if (state & Ci.nsIWebProgressListener.STATE_IS_BROKEN) {
            securityStateDesc = "broken";
          } else if (state & Ci.nsIWebProgressListener.STATE_IS_INSECURE) {
            securityStateDesc = "insecure";
          } else {
            this.error(`Unexpected securitychange state: ${state}`);
            securityStateDesc = "???";
          }
    
          var mixedStateDesc;
          if (state & Ci.nsIWebProgressListener.STATE_BLOCKED_MIXED_ACTIVE_CONTENT) {
            mixedStateDesc = "blocked_mixed_active_content";
          } else if (state & Ci.nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT) {
            // Note that STATE_LOADED_MIXED_ACTIVE_CONTENT implies STATE_IS_BROKEN
            mixedStateDesc = "loaded_mixed_active_content";
          }
    
          var isEV = !!(state & Ci.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL);
          var isMixedContent = !!(state &
            (Ci.nsIWebProgressListener.STATE_BLOCKED_MIXED_ACTIVE_CONTENT |
            Ci.nsIWebProgressListener.STATE_LOADED_MIXED_ACTIVE_CONTENT));
    
          this.dispatchEvent("securitychange", {
            state: securityStateDesc,
            mixedState: mixedStateDesc,
            extendedValidation: isEV,
            mixedContent: isMixedContent,
          });
        },
      }

    class WebView extends HTMLElement {
        constructor() {
            super();
            this.log("constructor");

            this.browser = null;
            this.attrs = [];

            // Mark some functions used by the UI as unimplemented for now.
            ["addNextPaintListener",
             "removeNextPaintListener",
             "getScreenshot"].forEach(name => {
              this[name] = () => { this.log(`Unimplemented: ${name}`); }
            });
        }

        log(msg) {
            console.log(`<web-view> ${msg}`);
        }

        error(msg) {
            console.error(`<web-view> ${msg}`);
        }

        static get observedAttributes() {
            return ["src", "remote", "ignoreuserfocus", "transparent"];
        }

        attributeChangedCallback(name, old_value, new_value) {
            if (old_value === new_value) {
              return;
            }

            this.log(`attribute ${name} changed from ${old_value} to ${new_value}`);
            // If we have not created the browser yet, buffer the attribute modification.
            if (!this.browser) {
                this.attrs.push({ name, old_value, new_value });
            } else {
                this.update_attr(name, old_value, new_value);
            }
        }

        update_attr(name, old_value, new_value) {
            if (!this.browser) {
                this.error(`update_attr(${name}) called with no browser available.`);
                return;
            }
    
            if (new_value) {
                this.browser.setAttribute(name, new_value);
            } else {
                this.browser.removeAttribute(name);
            }
        }

        connectedCallback() {
            this.log(`connectedCallback`);
            if (!this.browser) {
                this.log(`creating xul:browser`);
                // Creates a xul:browser with default attributes.
                this.browser = document.createXULElement("browser");
                this.browser.setAttribute("src", "about:blank");
                this.browser.setAttribute("type", "content");
                this.browser.setAttribute("style", "border: none; width: 100%; height: 100%");

                let src = null;

                // Apply buffered attribute changes.
                this.attrs.forEach(attr => {
                  if (attr.name == "src") {
                    src = attr.new_value;
                    return;
                  }
                  this.update_attr(attr.name, attr.old_value, attr.new_value);
                });
                this.attrs = [];

                // Hack around failing to add the progress listener before construct() runs.
                this.browser.delayConnectedCallback = () => {
                    return false;
                }

                this.appendChild(this.browser);
                this.progressListener = new ProgressListener(this);
                this.browser.addProgressListener(this.progressListener);

                if (this.browser.isRemoteBrowser) {
                  this.browser.messageManager.addMessageListener("DOMTitleChanged", this);
                } else {
                  this.browser.addEventListener("DOMTitleChanged", this, false);
                }

                // Set the src to load once we have setup all listeners to not miss progress events
                // like loadstart.
                src && this.browser.setAttribute("src", src);
            }
        }

        disconnectedCallback() {
          if (this.browser.isRemoteBrowser) {
            this.browser.messageManager.removeMessageListener("DOMTitleChanged", this);
          } else {
            this.browser.removeEventListener("DOMTitleChanged", this, false);
          }

          this.browser.removeProgressListener(this.progressListener);
          this.progressListener = null;
        }

        dispatchCustomEvent(name, detail) {
          this.log(`dispatching ${EVENT_PREFIX}${name}`);
          let event = new CustomEvent(`${EVENT_PREFIX}${name}`, { bubbles: true, detail });
          this.dispatchEvent(event);
        }

        handleEvent(event) {
          switch (event.type) {
            case "DOMTitleChanged":
                this.dispatchCustomEvent("titlechange", { title: this.browser.contentTitle });
                break;
            default:
                this.error(`Unexpected event ${event.type}`);
          }
        }

        receiveMessage(message) {
          switch (message.name) {
            case "DOMTitleChanged":
              this.dispatchCustomEvent("titlechange", { title: message.data.title });
              break;
            default:
              this.error(`Unexpected message ${message.name} ${JSON.stringify(message.data)}`);
          }
        }

        get frame() {
          return this.browser;
        }

        set src(url) {
            this.log(`set src to ${url}`);
            // If we are not yet connected to the DOM, add that action to the list
            // of attribute changes.
            if (!this.browser) {
              this.attrs.push({ name: "src", new_value: url});
            } else {
              this.browser.setAttribute("src", url);
            }
        }

        get src() {
          return this.browser ? this.browser.getAttribute("src") : null;
        }

        get canGoBack() {
            return !!this.browser && this.browser.canGoBack;
        }

        goBack() {
            !!this.browser && this.browser.goBack();
        }

        get canGoForward() {
            return !!this.browser && this.browser.canGoForward;
        }

        goForward() {
            !!this.browser && this.browser.goForward();
        }

        focus() {
          this.log(`focus() browser available: ${!!this.browser}`);
          !!this.browser && this.browser.focus();
        }

        blur() {
          this.log(`blur() browser available: ${!!this.browser}`);
          !!this.browser && this.browser.blur();
        }

        get active() {
          return (!!this.browser && this.browser.docShellIsActive);
        }

        set active(val) {
          if (!!this.browser) {
            let current = this.browser.docShellIsActive;
            this.browser.docShellIsActive = val;
            if (current !== val) {
              this.dispatchCustomEvent("visibilitychange", { visible: val });
            }
          }
        }

        get visible() {
          return this.active;
        }

        set visible(val) {
          this.active = val;
        }
    };

    console.log(`Setting up <web-view> custom element`);
    window.customElements.define("web-view", WebView);
}());
