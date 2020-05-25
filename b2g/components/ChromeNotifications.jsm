/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["ChromeNotifications"];

const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import(
  "resource://gre/modules/Services.jsm"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "appNotifier",
  "@mozilla.org/system-alerts-service;1",
  "nsIAppNotificationService"
);

const DEBUG = false;

function debug(s) {
  dump("-*- ChromeNotifications.jsm: " + s + "\n");
}

var ChromeNotifications = {

  init: function () {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.cpmm.addMessageListener(
      "Notification:GetAllCrossOrigin:Return:OK",
      this
    );
  },

  performResend: function (notifications) {
    let resentNotifications = 0;

    notifications.forEach(function (notification) {
      let behavior;
      try {
        behavior = JSON.parse(notification.mozbehavior);
        behavior.resend = true;
      } catch (e) {
        behavior = {
          resend: true
        };
      }

      if (behavior.showOnlyOnce === true) {
        return;
      }

      appNotifier.showAppNotification(
        notification.icon,
        notification.title,
        notification.body,
        null,
        {
          id: notification.alertName,
          dir: notification.dir,
          lang: notification.lang,
          tag: notification.tag,
          dbId: notification.id,
          timestamp: notification.timestamp,
          data: notification.data,
          mozbehavior: behavior,
          serviceWorkerRegistrationScope: notification.serviceWorkerRegistrationScope
        }
      );
      resentNotifications++;
    });

    try {
      this.resendCallback && this.resendCallback(resentNotifications);
    } catch (ex) {
      if (DEBUG) debug("Content sent exception: " + ex);
    }
  },

  resendAllNotifications: function (resendCallback) {
    this.resendCallback = resendCallback;
    Services.cpmm.sendAsyncMessage("Notification:GetAllCrossOrigin", {});
  },

  receiveMessage: function (message) {
    switch (message.name) {
      case "Notification:GetAllCrossOrigin:Return:OK":
        this.performResend(message.data.notifications);
        break;

      default:
        if (DEBUG) { debug("Unrecognized message: " + message.name); }
        break;
    }
  },

  observe: function (aSubject, aTopic, aData) {
    if (DEBUG) debug("Topic: " + aTopic);
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
      Services.cpmm.removeMessageListener("Notification:GetAllCrossOrigin:Return:OK", this);
    }
  },
}

ChromeNotifications.init();
