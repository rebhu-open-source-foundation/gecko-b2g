/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

/* eslint-disable no-unused-vars */
const { AnqpCache, AnqpData } = ChromeUtils.import(
  "resource://gre/modules/AnqpUtils.jsm"
);

const { ScanResult, WifiNetwork, WifiConfigUtils } = ChromeUtils.import(
  "resource://gre/modules/WifiConfiguration.jsm"
);
const { WifiConstants, EAPConstans, PasspointMatch } = ChromeUtils.import(
  "resource://gre/modules/WifiConstants.jsm"
);
const {
  PasspointProvider,
  PasspointConfig,
  HomeSp,
  Credential,
} = ChromeUtils.import("resource://gre/modules/PasspointConfiguration.jsm");

this.EXPORTED_SYMBOLS = ["PasspointManager"];

var gDebug = false;

this.PasspointManager = (function() {
  var passpointManager = {};
  var mAnqpCache = new AnqpCache();
  var mProvidersMap = new Map();

  passpointManager.setDebug = setDebug;
  passpointManager.onANQPResponse = onANQPResponse;
  passpointManager.sweepCache = sweepCache;
  passpointManager.hasCarrierProvider = hasCarrierProvider;
  passpointManager.findEapMethodFromCarrier = findEapMethodFromCarrier;
  passpointManager.getANQPElements = getANQPElements;

  function debug(aMsg) {
    if (gDebug) {
      console.log("-*- PasspointManager: ", aMsg);
    }
  }

  function setDebug(aDebug) {
    gDebug = aDebug;
  }

  function onANQPResponse(anqpNetworkKey, anqpElements) {
    if (!anqpElements || !anqpNetworkKey) {
      // Failed to acquire result.
      return;
    }

    // Add new entry to the cache.
    mAnqpCache.addEntry(anqpNetworkKey, anqpElements);
  }

  function sweepCache() {
    mAnqpCache.sweep();
  }

  function matchProvider(scanResult) {
    let bestMatch = null;
    let allMatches = getAllMatchedProviders(scanResult);

    if (allMatches.length() == 0) {
      return null;
    }

    for (let match of allMatches) {
      if (match.matchStatus == PasspointMatch.HomeProvider) {
        //TODO: shall we return scanresult ?
        bestMatch = match;
        break;
      }
      if (
        match.matchStatus == PasspointMatch.RoamingProvider &&
        bestMatch == null
      ) {
        bestMatch = match;
      }
    }

    if (bestMatch != null) {
      debug("Matched " + scanResult.ssid + " by " + JSON.stringify(bestMatch));
    } else {
      debug("No servier provider found for " + scanResult.ssid);
    }

    return bestMatch;
  }

  function getAllMatchedProviders(scanResult) {
    let allMatches = [];
    let anqpNetworkKey = WifiConfigUtils.getAnqpNetworkKey(scanResult);
    let anqpData = mAnqpCache.getEntry(anqpNetworkKey);
    if (anqpData == null) {
      //TODO: trigger AnqpRequestManager.request.
      return allMatches;
    }

    for (let provider of mProvidersMap.values()) {
      //TODO: roamingConsortium needed.
      let matchStatus = provider.match(anqpData.getElements());
      if (
        matchStatus == PasspointMatch.HomeProvider ||
        matchStatus == PasspointMatch.RoamingProvider
      ) {
        allMatches.push({ provider, matchStatus });
      }
    }

    return allMatches;
  }

  function hasCarrierProvider(mccMnc, mccMncRealm) {
    if (mccMncRealm == null) {
      return false;
    }

    for (let [key, provider] of mProvidersMap) {
      let config = provider.getConfig();
      if (config.getCredential() == null) {
        continue;
      }
      if (mccMncRealm == key) {
        // We already got same FQDN in providers.
        return true;
      }

      let imsiMccMnc = provider.getImsi();
      if (imsiMccMnc) {
        imsiMccMnc.replace("*", "");
      }

      if (mccMnc == imsiMccMnc) {
        // We already got same IMSI in providers.
        return true;
      }
    }

    return false;
  }

  function findEapMethodFromCarrier(mccMncRealm, filteredScanResults) {
    if (filteredScanResults.length == 0) {
      return -1;
    }

    if (!mccMncRealm) {
      return -1;
    }

    for (let result of filteredScanResults) {
      let anqpNetworkKey = WifiConfigUtils.getAnqpNetworkKey(result);
      let anqpData = mAnqpCache.getEntry(anqpNetworkKey);

      if (anqpData == null) {
        //TODO: trigger AnqpRequestManager.request.
        continue;
      }

      //TODO: anqpData.getElements().get(Constants.ANQPElementType.ANQPNAIRealm)
      let naiRealmElement = anqpData.getElements();
      /*
      let eapMethod =
        ANQPMatcher.getCarrierEapMethodFromMatchingNAIRealm(mccMncRealm, naiRealmElement);
      if (eapMethod != -1) {
        return eapMethod;
      }
      */
    }
    return -1;
  }

  function createEphemeralPasspointConfig(
    friendlyName,
    mccMnc,
    mccMncRealm,
    eapMethod
  ) {
    if (mccMncRealm == null) {
      debug("invalid mccMncRealm");
      return null;
    }

    if (friendlyName == null) {
      debug("invalid friendlyName");
      return null;
    }

    if (!WifiConfigUtils.isCarrierEapMethod(eapMethod)) {
      debug("invalid eapMethod type");
      return null;
    }

    let config = new PasspointConfig();
    let homeSp = new HomeSp();
    homeSp.setFqdn(mccMncRealm);
    homeSp.setFriendlyName(friendlyName);
    config.setHomeSp(homeSp);

    let credential = new Credential();
    credential.setRealm(mccMncRealm);
    credential.setImsi(mccMnc + "*");
    credential.setEapType(eapMethod);
    config.setCredential(credential);

    return config;
  }

  function installEphemeralPasspointConfig(config) {
    if (config == null) {
      debug("PasspointConfig is null");
      return false;
    }

    let newProvider = new PasspointProvider(config);
    mProvidersMap.set(config.getHomeSp().getFqdn(), newProvider);
    return true;
  }

  function getANQPElements(scanResult) {
    let anqpNetworkKey = WifiConfigUtils.getAnqpNetworkKey(scanResult);
    let anqpData = mAnqpCache.getEntry(anqpNetworkKey);
    if (anqpData) {
      return anqpData.getElements();
    }
    return null;
  }

  return passpointManager;
})();
/* eslint-enable no-unused-vars */
