/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

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
