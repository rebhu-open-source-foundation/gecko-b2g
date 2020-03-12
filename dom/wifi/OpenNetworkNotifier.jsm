/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["OpenNetworkNotifier"];

var gDebug = false;

this.OpenNetworkNotifier = (function() {
  var openNetworkNotifier = {};

  const NUM_SCANS_BEFORE_ACTUALLY_SCANNING = 3;
  const NOTIFICATION_REPEAT_DELAY_MS = 900 * 1000;
  var settingEnabled = true;
  var notificationRepeatTime = 0;
  var numScansSinceNetworkStateChange = 0;

  openNetworkNotifier.setOpenNetworkNotifyEnabled = setOpenNetworkNotifyEnabled;
  openNetworkNotifier.handleScanResults = handleScanResults;
  openNetworkNotifier.clearPendingNotification = clearPendingNotification;
  openNetworkNotifier.setDebug = setDebug;

  function setDebug(aDebug) {
    gDebug = aDebug;
  }

  function debug(aMsg) {
    if (gDebug) {
      dump("-*- OpenNetworkNotifier: " + aMsg);
    }
  }

  function setOpenNetworkNotifyEnabled(enable) {
    debug("setOpenNetworkNotifyEnabled: " + enable);
    settingEnabled = enable;
    clearPendingNotification();
  }

  function handleScanResults(aResults) {
    if (!settingEnabled) {
      return;
    }

    if (aResults) {
      let numOpenNetworks = 0;
      let lines = aResults.split("\n");
      for (let i = 1; i < lines.length; ++i) {
        let match = /([\S]+)\s+([\S]+)\s+([\S]+)\s+(\[[\S]+\])?\s(.*)/.exec(
          lines[i]
        );
        let flags = match[4];
        if (flags !== null && flags == "[ESS]") {
          numOpenNetworks++;
        }
      }

      if (numOpenNetworks > 0) {
        if (
          ++numScansSinceNetworkStateChange > NUM_SCANS_BEFORE_ACTUALLY_SCANNING
        ) {
          setNotificationVisible(true);
        }
        return;
      }
    }
    // No open networks in range, remove the notification
    setNotificationVisible(false);
  }

  function setNotificationVisible(visible) {
    debug("setNotificationVisible: visible = " + visible);
    if (visible) {
      let now = Date.now();
      debug(
        "now = " + now + " , notificationRepeatTime = " + notificationRepeatTime
      );
      // Not enough time has passed to show the notification again
      if (now < notificationRepeatTime) {
        return;
      }
      notificationRepeatTime = now + NOTIFICATION_REPEAT_DELAY_MS;
      notify("opennetworknotification", { enabled: true });
    } else {
      notify("opennetworknotification", { enabled: false });
    }
  }

  function clearPendingNotification() {
    notificationRepeatTime = 0;
    numScansSinceNetworkStateChange = 0;
    setNotificationVisible(false);
  }

  function notify(eventName, eventObject) {
    var handler = openNetworkNotifier["on" + eventName];
    if (!handler) {
      return;
    }
    if (!eventObject) {
      eventObject = {};
    }
    handler.call(eventObject);
  }

  return openNetworkNotifier;
})();
