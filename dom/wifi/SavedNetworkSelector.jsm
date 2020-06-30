/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { WifiConfigManager } = ChromeUtils.import(
  "resource://gre/modules/WifiConfigManager.jsm"
);
const { WifiConstants } = ChromeUtils.import(
  "resource://gre/modules/WifiConstants.jsm"
);

this.EXPORTED_SYMBOLS = ["SavedNetworkSelector"];

const BAND_AWARD_5GHZ = 40;
const LAST_SELECTION_AWARD = 480;
const CURRENT_NETWORK_BOOST = 16;
const SAME_BSSID_AWARD = 24;
const SECURITY_AWARD = 80;

var gDebug = false;

function debug(aMsg) {
  if (gDebug) {
    dump("-*- SavedNetworkSelector: " + aMsg);
  }
}

this.SavedNetworkSelector = function SavedNetworkSelector() {};

SavedNetworkSelector.prototype = {
  lastUserSelectedNetwork: WifiConfigManager.getLastSelectedNetwork(),
  lastUserSelectedNetworkTimeStamp: WifiConfigManager.getLastSelectedTimeStamp(),

  setDebug(aDebug) {
    gDebug = aDebug;
  },

  chooseNetwork(results, wifiInfo) {
    var configuredNetworks = WifiConfigManager.configuredNetworks;
    var candidate = null;
    var highestScore = 0;

    // Iterate all scan results and find the best candidate with the highest score
    for (let i in results) {
      let result = results[i];

      // If network disabled, it didn't need to calculate bssid score.
      if (configuredNetworks[result.networkKey].networkSelectionStatus) {
        continue;
      }

      let score = this.calculateBssidScore(
        result,
        this.lastUserSelectedNetwork == null
          ? false
          : this.lastUserSelectedNetwork == result.netId,
        wifiInfo.networkId == result.netId,
        wifiInfo.bssid == null ? false : wifiInfo.bssid == result.bssid,
        this.lastUserSelectedNetworkTimeStamp
      );

      if (score > highestScore) {
        // better candidate is found.
        highestScore = score;
        candidate = result;
      }
    }
    return candidate;
  },

  calculateBssidScore(
    scanResult,
    sameSelect,
    sameNetworkId,
    sameBssid,
    lastUserSelectedNetworkTimeStamp
  ) {
    var score = 0;
    // calculate the RSSI score
    var rssi =
      scanResult.signalStrength <= WifiConstants.RSSI_THRESHOLD_GOOD_24G
        ? scanResult.signalStrength
        : WifiConstants.RSSI_THRESHOLD_GOOD_24G;
    score +=
      (parseInt(rssi, 10) + WifiConstants.RSSI_SCORE_OFFSET) *
      WifiConstants.RSSI_SCORE_SLOPE;
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
    // const NO_INTERNET_PENALTY =
    //   (WifiConstants.RSSI_THRESHOLD_GOOD_24G + WifiConstants.RSSI_SCORE_OFFSET) *
    //     WifiConstants.RSSI_SCORE_SLOPE +
    //   BAND_AWARD_5GHZ +
    //   CURRENT_NETWORK_BOOST +
    //   SAME_BSSID_AWARD +
    //   SECURITY_AWARD;
    //
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
  },
};
