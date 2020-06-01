/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = [];

const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import(
  "resource://gre/modules/Services.jsm"
);

XPCOMUtils.defineLazyServiceGetter(this, "notificationStorage",
                                   "@mozilla.org/notificationStorage;1",
                                   "nsINotificationStorage");

XPCOMUtils.defineLazyServiceGetter(this, "serviceWorkerManager",
                                   "@mozilla.org/serviceworkers/manager;1",
                                   "nsIServiceWorkerManager");

XPCOMUtils.defineLazyGetter(this, "ppmm", function() {
  return Services.ppmm;
});

function debug(str) {
  dump("=*= AlertsHelper.jsm : " + str + "\n");
}

const kNotificationIconSize = 128;

const kDesktopNotificationPerm = "desktop-notification";

const kDesktopNotification      = "desktop-notification";
const kDesktopNotificationShow  = "desktop-notification-show";
const kDesktopNotificationClick = "desktop-notification-click";
const kDesktopNotificationClose = "desktop-notification-close";

const kTopicAlertClickCallback = "alertclickcallback";
const kTopicAlertShow          = "alertshow";
const kTopicAlertFinished      = "alertfinished";

const kMessageAppNotificationSend    = "app-notification-send";
const kMessageAppNotificationReturn  = "app-notification-return";
const kMessageAlertNotificationSend  = "alert-notification-send";
const kMessageAlertNotificationClose = "alert-notification-close";

const kMessages = [
  kMessageAppNotificationSend,
  kMessageAlertNotificationSend,
  kMessageAlertNotificationClose
];

var AlertsHelper = {

  _listeners: {},
  _embedderNotifications: {},

  init: function() {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver((embedderNotifications) => {
      this._embedderNotifications = embedderNotifications.wrappedJSObject;
    }, "web-embedder-notifications");
    for (let message of kMessages) {
      ppmm.addMessageListener(message, this);
    }
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        Services.obs.removeObserver(this, "xpcom-shutdown");
        for (let message of kMessages) {
          ppmm.removeMessageListener(message, this);
        }
        break;
    }
  },

  handleEvent: function(evt) {
    let detail = evt.detail;

    switch (detail.type) {
      case kDesktopNotificationShow:
      case kDesktopNotificationClick:
      case kDesktopNotificationClose:
        this.handleNotificationEvent(detail);
        break;
      default:
        debug("FIXME: Unhandled notification event: " + detail.type);
        break;
    }
  },

  handleNotificationEvent: function(detail) {
    if (!detail || !detail.id) {
      return;
    }

    let uid = detail.id;
    let listener = this._listeners[uid];
    if (!listener) {
      return;
    }

    let topic;
    if (detail.type === kDesktopNotificationClick) {
      topic = kTopicAlertClickCallback;
    } else if (detail.type === kDesktopNotificationShow) {
      topic = kTopicAlertShow;
    } else {
      /* kDesktopNotificationClose */
      topic = kTopicAlertFinished;
    }

    if (listener.cookie) {
      try {
        listener.observer.observe(null, topic, listener.cookie);
      } catch (e) { }
    } else {
      try {
        listener.mm.sendAsyncMessage(kMessageAppNotificationReturn, {
          uid: uid,
          topic: topic,
          target: listener.target,
          extra: detail.action
        });
      } catch (e) {
        // The non-empty serviceWorkerRegistrationID means the notification
        // is issued by service worker, so deal with this listener
        // via serviceWorkerManager
        if (listener.serviceWorkerRegistrationID.length &&
            detail.type !== kDesktopNotificationShow) {
          let appId = appsService.getAppLocalIdByManifestURL(listener.manifestURL);
          let originSuffix = "^appId=" + appId;
          let eventName;
          let userAction = "";

          if (detail.type === kDesktopNotificationClick) {
            eventName = "notificationclick";
            if (detail.action && typeof detail.action === 'string') {
              userAction = detail.action;
            }
          } else if (detail.type === kDesktopNotificationClose) {
            eventName = "notificationclose";
          }

          if (eventName) {
            serviceWorkerManager.sendNotificationEvent(
              eventName,
              originSuffix,
              listener.serviceWorkerRegistrationID,
              listener.dbId,
              listener.title,
              listener.dir,
              listener.lang,
              listener.text,
              listener.tag,
              listener.imageURL,
              listener.dataObj || undefined,
              listener.mozbehavior,
              listener.requireInteraction,
              listener.actions,
              userAction
            );
          }
        }
        if (detail.type === kDesktopNotificationClose && listener.dbId) {
          notificationStorage.delete(listener.manifestURL, listener.dbId);
        }
      }
    }

    // we"re done with this notification
    if (detail.type === kDesktopNotificationClose) {
      delete this._listeners[uid];
    }
  },

  registerListener: function(alertId, cookie, alertListener) {
    this._listeners[alertId] = { observer: alertListener, cookie: cookie };
  },

  registerAppListener: function(uid, listener) {
    this._listeners[uid] = listener;
  },

  deserializeStructuredClone: function(dataString) {
    if (!dataString) {
      return null;
    }
    let scContainer = Cc["@mozilla.org/docshell/structured-clone-container;1"].
      createInstance(Ci.nsIStructuredCloneContainer);

    // The maximum supported structured-clone serialization format version
    // as defined in "js/public/StructuredClone.h"
    let JS_STRUCTURED_CLONE_VERSION = 4;
    scContainer.initFromBase64(dataString, JS_STRUCTURED_CLONE_VERSION);
    let dataObj = scContainer.deserializeToVariant();

    // We have to check whether dataObj contains DOM objects (supported by
    // nsIStructuredCloneContainer, but not by Cu.cloneInto), e.g. ImageData.
    // After the structured clone callback systems will be unified, we'll not
    // have to perform this check anymore.
    try {
      let data = Cu.cloneInto(dataObj, {});
    } catch(e) { dataObj = null; }

    return dataObj;
  },

  showNotification: function(imageURL, title, text, textClickable, cookie,
                             uid, dir, lang, dataObj, manifestURL, timestamp,
                             behavior, requireInteraction, actions,
                             serviceWorkerRegistrationScope) {
    if (!this._embedderNotifications || !this._embedderNotifications.showNotification) {
      debug(`No embedder support for 'showNotification()'`);
      return;
    }

    this._embedderNotifications.showNotification({
      type: kDesktopNotification,
      id: uid,
      icon: imageURL,
      title: title,
      text: text,
      dir: dir,
      lang: lang,
      appName: null,
      appIcon: null,
      manifestURL: manifestURL,
      timestamp: timestamp,
      data: dataObj,
      mozbehavior: behavior,
      requireInteraction: requireInteraction,
      serviceWorkerRegistrationScope: serviceWorkerRegistrationScope
    });
  },

  showAlertNotification: function(aMessage) {
    let data = aMessage.data;
    let currentListener = this._listeners[data.name];
    if (currentListener && currentListener.observer) {
      currentListener.observer.observe(null, kTopicAlertFinished, currentListener.cookie);
    }

    let dataObj = this.deserializeStructuredClone(data.dataStr);
    this.registerListener(data.name, data.cookie, data.alertListener);
    this.showNotification(data.imageURL, data.title, data.text,
                          data.textClickable, data.cookie, data.name, data.dir,
                          data.lang, dataObj, null, data.inPrivateBrowsing);
  },

  showAppNotification: function(aMessage) {
    let data = aMessage.data;
    let details = data.details;
    let listener = {
      mm: aMessage.target,
      title: data.title,
      text: data.text,
      manifestURL: details.manifestURL,
      imageURL: data.imageURL,
      lang: details.lang || undefined,
      id: details.id || undefined,
      dbId: details.dbId || undefined,
      dir: details.dir || undefined,
      tag: details.tag || undefined,
      timestamp: details.timestamp || undefined,
      dataObj: details.data || undefined,
      requireInteraction: details.requireInteraction || false,
      actions: details.actions || "[]",
      serviceWorkerRegistrationScope: details.serviceWorkerRegistrationScope
    };
    this.registerAppListener(data.uid, listener);
    this.showNotification(data.imageURL, data.title, data.text,
                          details.textClickable, null, data.uid, details.dir,
                          details.lang, details.data, details.manifestURL,
                          details.timestamp, details.mozbehavior,
                          details.requireInteraction, details.actions,
                          details.serviceWorkerRegistrationScope);
  },

  closeAlert: function(name) {},

  receiveMessage: function(aMessage) {
    // TODO: Need a DesktopNotification permission check in here which gecko48
    // is doing

    switch(aMessage.name) {
      case kMessageAppNotificationSend:
        this.showAppNotification(aMessage);
        break;

      case kMessageAlertNotificationSend:
        this.showAlertNotification(aMessage);
        break;

      case kMessageAlertNotificationClose:
        this.closeAlert(aMessage.data.name);
        break;
    }

  },
}

AlertsHelper.init();
