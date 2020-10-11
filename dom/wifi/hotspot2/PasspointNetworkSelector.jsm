/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* eslint-disable no-unused-vars */
const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { WifiConfigManager } = ChromeUtils.import(
  "resource://gre/modules/WifiConfigManager.jsm"
);
const { WifiConstants, EAPConstans, PasspointMatch } = ChromeUtils.import(
  "resource://gre/modules/WifiConstants.jsm"
);
const { PasspointManager } = ChromeUtils.import(
  "resource://gre/modules/PasspointManager.jsm"
);
const { ScanResult, WifiNetwork, WifiConfigUtils } = ChromeUtils.import(
  "resource://gre/modules/WifiConfiguration.jsm"
);
const {
  PasspointProvider,
  PasspointConfig,
  HomeSp,
  Credential,
} = ChromeUtils.import("resource://gre/modules/PasspointConfiguration.jsm");

const HOME_PROVIDER_AWARD = 100;
const INTERNET_ACCESS_AWARD = 50;
const PUBLIC_OR_PRIVATE_NETWORK_AWARDS = 4;
const PERSONAL_OR_EMERGENCY_NETWORK_AWARDS = 2;
const RESTRICTED_OR_UNKNOWN_IP_AWARDS = 1;
const UNRESTRICTED_IP_AWARDS = 2;
const WAN_PORT_DOWN_OR_CAPPED_PENALTY = 50;
const RSSI_SCORE_START = -80;
const RSSI_BUCKETWIDTH = 20;
const RSSI_LEVEL_BUCKETS = [-10, 0, 10, 20, 30, 40];
const ACTIVE_NETWORK_RSSI_BOOST = 20;

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gIccService",
  "@mozilla.org/icc/iccservice;1",
  "nsIIccService"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gDataCallManager",
  "@mozilla.org/datacall/manager;1",
  "nsIDataCallManager"
);

this.EXPORTED_SYMBOLS = ["PasspointNetworkSelector"];

var gDebug = false;

function debug(aMsg) {
  if (gDebug) {
    dump("-*- PasspointNetworkSelector: " + aMsg);
  }
}

this.PasspointNetworkSelector = function PasspointNetworkSelector() {};

PasspointNetworkSelector.prototype = {
  lastUserSelectedNetwork: WifiConfigManager.getLastSelectedNetwork(),
  lastUserSelectedNetworkTimeStamp: WifiConfigManager.getLastSelectedTimeStamp(),

  setDebug(aDebug) {
    gDebug = aDebug;

    if (PasspointManager) {
      PasspointManager.setDebug(gDebug);
    }
  },

  chooseNetwork(results, wifiInfo) {
    var configuredNetworks = WifiConfigManager.configuredNetworks;
    var bestNetwork = null;
    var candidateList = [];
    var highestScore = Number.MIN_VALUE;

    debug("chooseNetwork ...");
    // Renew the ANQP cache.
    PasspointManager.sweepCache();

    let passpointScanResults = this.filterPasspointResult(results);

    this.createEphemeralProfileForMatchingAp(passpointScanResults);

    // Go through each Passpoint scan result for best provider.
    for (let result of passpointScanResults) {
      let bestProvider = PasspointManager.matchProvider(result);
      if (bestProvider != null) {
        //TODO: Skip providers backed when SIM is not present.

        candidateList.push({
          scanResult: result,
          provider: bestProvider.provider,
          matchStatus: bestProvider.matchStatus,
        });
      }
    }

    if (candidateList.length == 0) {
      debug("No suitable Passpoint network found");
      return null;
    }

    for (let candidate of candidateList) {
      let isActiveNetwork = candidate.scanResult.ssid == wifiInfo.ssid;
      let scanResult = candidate.scanResult;
      let score = this.calculateScore(
        candidate.matchStatus == PasspointMatch.HomeProvider,
        PasspointManager.getANQPElements(scanResult),
        scanResult,
        isActiveNetwork
      );

      if (score > highestScore) {
        bestNetwork = candidate.scanResult;
        highestScore = score;
      }
    }

    return bestNetwork;
  },

  filterPasspointResult(scanResults) {
    let filteredScanResults = [];
    for (let result of scanResults) {
      //TODO: need to figure out isInterwork is bool or int.
      // We only care about passpoint AP.
      if (!result.isInterworking) {
        continue;
      }
      filteredScanResults.push(result);
    }
    return filteredScanResults;
  },

  // Create an ephemeral passpoint profule which matching Passpoint AP's MccMnc.
  createEphemeralProfileForMatchingAp(filteredScanResults) {
    let simSlot = gDataCallManager.dataDefaultServiceId;
    let icc = gIccService.getIccByServiceId(simSlot);

    if (!icc || !icc.iccInfo || !icc.iccInfo.mcc || !icc.iccInfo.mnc) {
      debug("SIM is not ready.");
      return;
    }

    let mccMnc = icc.iccInfo.mcc + icc.iccInfo.mnc;
    let mccMncRealm = WifiConfigUtils.getRealmForMccMnc(
      icc.iccInfo.mcc,
      icc.iccInfo.mnc
    );
    if (PasspointManager.hasCarrierProvider(mccMnc, mccMncRealm)) {
      return;
    }

    let eapMethod = PasspointManager.findEapMethodFromCarrier(
      mccMncRealm,
      filteredScanResults
    );

    if (!WifiConfigUtils.isCarrierEapMethod(eapMethod)) {
      return;
    }

    let passpointConfig = PasspointManager.createEphemeralPasspointConfig(
      icc.iccInfo.spn,
      mccMnc,
      mccMncRealm,
      eapMethod
    );
    if (passpointConfig == null) {
      return;
    }

    PasspointManager.installEphemeralPasspointConfig(passpointConfig);
  },

  calculateScore(isHomeProvider, scanResult, anqpElements, isActiveNetwork) {
    let score = 0;

    // Evaluate match status score.
    if (isHomeProvider) {
      score += HOME_PROVIDER_AWARD;
    }

    //TODO: complete ANT & internet access score evaluate.
    /*
    score += (scanResult.isInternet ? 1 : -1 ) * INTERNET_ACCESS_AWARD;
    score += ant type switch?
    */

    // Evaluate rssi score.
    let rssi = scanResult.signalStrength;
    if (isActiveNetwork) {
      rssi += ACTIVE_NETWORK_RSSI_BOOST;
    }

    let index = (rssi - RSSI_SCORE_START) / RSSI_BUCKETWIDTH;

    if (index < 0) {
      index = 0;
    } else if (index > RSSI_LEVEL_BUCKETS.length - 1) {
      index = RSSI_LEVEL_BUCKETS.length - 1;
    }
    score += RSSI_LEVEL_BUCKETS[index];

    return score;
  },
};
/* eslint-enable no-unused-vars */
