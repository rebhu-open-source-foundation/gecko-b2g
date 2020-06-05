/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Log } = ChromeUtils.import("chrome://marionette/content/log.js");
XPCOMUtils.defineLazyGetter(this, "logger", Log.get);

this.EXPORTED_SYMBOLS = ["MarionetteHelper"];

class MarionetteHelper {
  constructor(contentWindow) {
    this.content = contentWindow;
  }

  get messageManager() {
    return this.content.messageManager;
  }

  get tabs() {
    let web_views = Array.from(
      this.content.document.querySelectorAll("web-view")
    );
    return web_views
  }

  get selectedTab() {
    let web_views = Array.from(
      this.content.document.querySelectorAll("web-view")
    );
    let active = web_views.find(tab => {
      // logger.info(`${tab.src} active=${tab.active} visible=${tab.visible}`);
      return tab.active;
    });

    // Hack: select the first tab if none is marked as active.
    if (!active && web_views.length > 0) {
      active = web_views[0];
    }

    return active;
  }

  set selectedTab(tab) {
    logger.info(`MarionetteHelper set selectedTab to ${tab}`);
    let current = this.selectedTab;
    current.active = false;
    tab.active = true;
  }
}

this.MarionetteHelper = MarionetteHelper;
