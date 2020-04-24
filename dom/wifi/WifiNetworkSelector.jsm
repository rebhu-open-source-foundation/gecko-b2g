/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { WifiConfigManager } = ChromeUtils.import(
  "resource://gre/modules/WifiConfigManager.jsm"
);
const { WifiWorker } = ChromeUtils.import(
  "resource://gre/modules/WifiWorker.jsm"
);

this.EXPORTED_SYMBOLS = ["WifiNetworkSelector"];

var gDebug = false;

const INVALID_TIME_STAMP = -1;

function BssidBlacklistStatus() {}

BssidBlacklistStatus.prototype = {
  // How many times it is requested to be blacklisted
  // (association rejection trigger this)
  counter: 0,
  isBlacklisted: false,
  blacklistedTimeStamp: INVALID_TIME_STAMP,
};

this.WifiNetworkSelector = (function() {
  var wifiNetworkSelector = {};

  const INVALID_NETWORK_ID = -1;

  // Minimum time gap between last successful Network Selection and
  // new selection attempt usable only when current state is connected state.
  const MINIMUM_NETWORK_SELECTION_INTERVAL = 10 * 1000;
  const MINIMUM_LAST_USER_SELECTION_INTERVAL = 30 * 1000;

  const RSSI_THRESHOLD_GOOD_24G = -60;
  const RSSI_THRESHOLD_LOW_24G = -73;
  const RSSI_THRESHOLD_BAD_24G = -83;
  const RSSI_THRESHOLD_GOOD_5G = -57;
  const RSSI_THRESHOLD_LOW_5G = -70;
  const RSSI_THRESHOLD_BAD_5G = -80;

  const RSSI_SCORE_OFFSET = 85;
  const RSSI_SCORE_SLOPE = 4;

  const BAND_AWARD_5GHZ = 40;
  const LAST_SELECTION_AWARD = 480;
  const CURRENT_NETWORK_BOOST = 16;
  const SAME_BSSID_AWARD = 24;
  const SECURITY_AWARD = 80;
  const NO_INTERNET_PENALTY =
    (RSSI_THRESHOLD_GOOD_24G + RSSI_SCORE_OFFSET) * RSSI_SCORE_SLOPE +
    BAND_AWARD_5GHZ +
    CURRENT_NETWORK_BOOST +
    SAME_BSSID_AWARD +
    SECURITY_AWARD;

  const BSSID_BLACKLIST_THRESHOLD = 3;
  const BSSID_BLACKLIST_EXPIRE_TIME = 30 * 60 * 1000;

  const REASON_AP_UNABLE_TO_HANDLE_NEW_STA = 17;

  var lastNetworkSelectionTimeStamp = INVALID_TIME_STAMP;
  var enableAutoJoinWhenAssociated = true;
  var bssidBlacklist = new Map();

  // WifiNetworkSelector parameters
  wifiNetworkSelector.RSSI_THRESHOLD_LOW_24G = RSSI_THRESHOLD_LOW_24G;
  wifiNetworkSelector.RSSI_THRESHOLD_LOW_5G = RSSI_THRESHOLD_LOW_5G;

  // WifiNetworkSelector functions
  wifiNetworkSelector.bssidBlacklist = bssidBlacklist;
  wifiNetworkSelector.updateBssidBlacklist = updateBssidBlacklist;
  wifiNetworkSelector.selectNetwork = selectNetwork;
  wifiNetworkSelector.trackBssid = trackBssid;
  wifiNetworkSelector.setDebug = setDebug;

  function setDebug(aDebug) {
    gDebug = aDebug;
  }

  function debug(aMsg) {
    if (gDebug) {
      dump("-*- WifiNetworkSelector: " + aMsg);
    }
  }

  function selectNetwork(
    scanResults,
    configuredNetworks,
    isLinkDebouncing,
    wifiState,
    wifiInfo,
    callback
  ) {
    debug("==========start Network Selection==========");

    if (scanResults.length == 0) {
      debug("Empty connectivity scan result");
      return callback(null);
    }

    // Shall we start network selection at all?
    if (!isNetworkSelectionNeeded(isLinkDebouncing, wifiState, wifiInfo)) {
      return callback(null);
    }

    var lastUserSelectedNetwork = WifiConfigManager.getLastSelectedNetwork();
    var lastUserSelectedNetworkTimeStamp = WifiConfigManager.getLastSelectedTimeStamp();
    var highestScore = 0;
    var scanResultCandidate = null;

    updateSavedNetworkSelectionStatus(configuredNetworks);

    var filteredResults = filterScanResults(
      scanResults,
      wifiState,
      wifiInfo.bssid
    );

    // iterate all scan results and find the best candidate with the highest score
    for (let i in filteredResults) {
      let result = filteredResults[i];
      // If network disabled, it didn't need to calculate bssid score.
      if (configuredNetworks[result.networkKey].networkSelectionStatus) {
        continue;
      }
      var score = calculateBssidScore(
        result,
        lastUserSelectedNetwork == null
          ? false
          : lastUserSelectedNetwork == result.netId,
        wifiInfo.networkId == result.netId,
        wifiInfo.bssid == null ? false : wifiInfo.bssid == result.bssid,
        lastUserSelectedNetworkTimeStamp
      );

      if (score > highestScore) {
        highestScore = score;
        scanResultCandidate = result;
      }
    }

    if (scanResultCandidate == null) {
      debug("Can not find any suitable candidates");
      return callback(null);
    }

    lastNetworkSelectionTimeStamp = Date.now();
    return callback(scanResultCandidate);
  }

  function calculateBssidScore(
    scanResult,
    sameSelect,
    sameNetworkId,
    sameBssid,
    lastUserSelectedNetworkTimeStamp
  ) {
    var score = 0;
    // calculate the RSSI score
    var rssi =
      scanResult.signalStrength <= RSSI_THRESHOLD_GOOD_24G
        ? scanResult.signalStrength
        : RSSI_THRESHOLD_GOOD_24G;
    score += (parseInt(rssi, 10) + RSSI_SCORE_OFFSET) * RSSI_SCORE_SLOPE;
    debug("RSSI score: " + score);
    if (scanResult.is5G) {
      // 5GHz band
      score += BAND_AWARD_5GHZ;
      debug("5GHz bonus: " + BAND_AWARD_5GHZ);
    }
    // last user selection award
    if (sameSelect) {
      var timeDifference = Date.now() - lastUserSelectedNetworkTimeStamp;

      if (timeDifference > 0) {
        var bonus = LAST_SELECTION_AWARD - timeDifference / 1000 / 60;
        score += bonus > 0 ? bonus : 0;
        debug(
          " User selected it last time " +
            timeDifference / 1000 / 60 +
            " minutes ago, bonus:" +
            bonus
        );
      }
    }
    // same network award
    if (sameNetworkId) {
      score += CURRENT_NETWORK_BOOST;
      debug(
        "Same network with current associated. Bonus: " + CURRENT_NETWORK_BOOST
      );
    }
    // same BSSID award
    if (sameBssid) {
      score += SAME_BSSID_AWARD;
      debug("Same BSSID with current association. Bonus: " + SAME_BSSID_AWARD);
    }
    // security award
    if (scanResult.security !== "") {
      score += SECURITY_AWARD;
      debug("Secure network Bonus: " + SECURITY_AWARD);
    }
    // Penalty for no internet network. Make sure if there is any network with
    // Internet.However, if there is no any other network with internet, this
    // network can be chosen.
    // FIXME: network validation is not ready
    // if (
    //   typeof scanResult.hasInternet !== "undefined" &&
    //   !scanResult.hasInternet
    // ) {
    //   score -= NO_INTERNET_PENALTY;
    //   debug(" No internet Penalty:-" + NO_INTERNET_PENALTY);
    // }

    debug(
      " Score for scanResult: " + uneval(scanResult) + " final score:" + score
    );
    return score;
  }

  function isNetworkSelectionNeeded(isLinkDebouncing, wifiState, wifiInfo) {
    // Do not trigger Network Selection during link debouncing procedure
    if (isLinkDebouncing) {
      debug("Need not Network Selection during link debouncing");
      return false;
    }

    if (wifiState == "connected" || wifiState == "associated") {
      if (!wifiInfo) {
        return false;
      }

      // Is roaming allowed?
      if (!enableAutoJoinWhenAssociated) {
        debug(
          "Switching networks in connected state is not allowed." +
            " Skip network selection."
        );
        return false;
      }

      // Has it been at least the minimum interval since last network selection?
      if (lastNetworkSelectionTimeStamp != INVALID_TIME_STAMP) {
        var now = Date.now();
        var gap = now - lastNetworkSelectionTimeStamp;
        if (gap < MINIMUM_NETWORK_SELECTION_INTERVAL) {
          debug(
            "Too short since last network selection: " +
              gap +
              " ms." +
              " Skip network selection"
          );
          return false;
        }
      }

      if (isCurrentNetworkSufficient(wifiInfo)) {
        debug("Current network already sufficient. Skip network selection.");
        return false;
      }
      debug("Current connected network is not sufficient.");
      return true;
    } else if (wifiState == "disconnected") {
      return true;
    }
    debug("Wifi is neither connected or disconnected. Skip network selection");
    return false;
  }

  function isCurrentNetworkSufficient(wifiInfo) {
    if (wifiInfo.networkId == INVALID_NETWORK_ID) {
      debug("WifiWorker in connected state but WifiInfo is not");
      return false;
    }

    debug(
      "Current connected network: " +
        wifiInfo.wifiSsid +
        " ,ID is: " +
        wifiInfo.networkId
    );

    // Open network is not qualified.
    if (wifiInfo.security == "OPEN") {
      debug("Current network is a open one");
      return false;
    }

    // current network is recently user-selected.
    let lastNetwork = WifiConfigManager.getLastSelectedNetwork();
    let lastTimeStamp = WifiConfigManager.getLastSelectedTimeStamp();
    if (
      lastNetwork == wifiInfo.networkId &&
      Date.now() - lastTimeStamp < MINIMUM_LAST_USER_SELECTION_INTERVAL
    ) {
      return true;
    }

    // TODO: 1. 2.4GHz networks is not qualified whenever 5GHz is available.
    //       2. Tx/Rx Success rate shall be considered.
    let currentRssi = wifiInfo.rssi;
    let hasQualifiedRssi =
      (wifiInfo.is24G && currentRssi > RSSI_THRESHOLD_LOW_24G) ||
      (wifiInfo.is5G && currentRssi > RSSI_THRESHOLD_LOW_5G);

    if (!hasQualifiedRssi) {
      debug(
        "Current network RSSI[" +
          currentRssi +
          "]-acceptable but not qualified."
      );
      return false;
    }
    return true;
  }

  function filterScanResults(scanResults, wifiState, currentBssid) {
    let filteredResults = [];
    let resultsContainCurrentBssid = false;

    for (let i in scanResults) {
      // skip not saved network
      if (!scanResults[i].known) {
        continue;
      }

      // skip bad scan result
      if (scanResults[i].ssid === null || scanResults[i].ssid == "") {
        debug("skip bad scan result");
        continue;
      }

      if (scanResults[i].bssid.includes(currentBssid)) {
        resultsContainCurrentBssid = true;
      }

      var scanId = scanResults[i].ssid + ":" + scanResults[i].bssid;
      debug("scanId = " + scanId);

      // check whether this BSSID is blocked or not
      let status = bssidBlacklist.get(scanResults[i].bssid);
      if (typeof status !== "undefined" && status.isBlacklisted) {
        debug(scanId + " is in blacklist.");
        continue;
      }

      let isWeak24G =
        scanResults[i].is24G &&
        scanResults[i].signalStrength < RSSI_THRESHOLD_BAD_24G;
      let isWeak5G =
        scanResults[i].is5G &&
        scanResults[i].signalStrength < RSSI_THRESHOLD_BAD_5G;
      // skip scan result with too weak signals
      if (isWeak24G || isWeak5G) {
        debug(
          scanId +
            "(" +
            (scanResults[i].is24G ? "2.4GHz" : "5GHz") +
            ")" +
            scanResults[i].signalStrength +
            " / "
        );
        continue;
      }
      // save the result for ongoing network selection
      filteredResults.push(scanResults[i]);
    }

    let isConnected = wifiState == "connected" || wifiState == "associated";
    // If wifi is connected but its bssid is not in scan list,
    // we should assume that the scan is triggered without
    // all channles included. So we just skip these results.
    if (isConnected && !resultsContainCurrentBssid) {
      return [];
    }

    return filteredResults;
  }

  function trackBssid(bssid, enable, reason) {
    debug("trackBssid: " + (enable ? "enable " : "disable ") + bssid);
    if (!bssid.length) {
      return false;
    }

    if (enable) {
      return bssidBlacklist.delete(bssid);
    }

    let status = bssidBlacklist.get(bssid);
    if (typeof status == "undefined") {
      // first time
      status = new BssidBlacklistStatus();
      bssidBlacklist.set(bssid, status);
    }
    status.counter++;
    status.blacklistedTimeStamp = Date.now();
    if (!status.isBlacklisted) {
      if (
        status.counter >= BSSID_BLACKLIST_THRESHOLD ||
        reason == REASON_AP_UNABLE_TO_HANDLE_NEW_STA
      ) {
        status.isBlacklisted = true;
        return true;
      }
    }
    return false;
  }

  function updateBssidBlacklist(callback) {
    let iter = bssidBlacklist[Symbol.iterator]();
    let updated = false;
    for (let [bssid, status] of iter) {
      debug(
        "BSSID black list: BSSID=" +
          bssid +
          " isBlacklisted=" +
          status.isBlacklisted
      );
      if (
        status.isBlacklisted &&
        Date.now() - status.blacklistedTimeStamp >= BSSID_BLACKLIST_EXPIRE_TIME
      ) {
        bssidBlacklist.delete(bssid);
        updated = true;
      }
    }
    callback(updated);
  }

  function updateSavedNetworkSelectionStatus(configuredNetworks) {
    if (Object.keys(configuredNetworks).length == 0) {
      debug("no saved network");
      return;
    }
    WifiConfigManager.tryEnableQualifiedNetwork(configuredNetworks);
  }

  function notify(eventName, eventObject) {
    var handler = wifiNetworkSelector["on" + eventName];
    if (!handler) {
      return;
    }
    if (!eventObject) {
      eventObject = {};
    }
    handler.call(eventObject);
  }

  return wifiNetworkSelector;
})();
