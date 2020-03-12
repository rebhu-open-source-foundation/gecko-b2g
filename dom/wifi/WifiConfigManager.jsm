/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { WifiConfigStore } = ChromeUtils.import(
  "resource://gre/modules/WifiConfigStore.jsm"
);
const { WifiConfigUtils } = ChromeUtils.import(
  "resource://gre/modules/WifiConfiguration.jsm"
);

this.EXPORTED_SYMBOLS = ["WifiConfigManager"];

var gDebug = false;

this.WifiConfigManager = (function() {
  var configManager = {};

  const WIFI_CONFIG_PATH = "/data/misc/wifi/wifi_config.json";

  const INVALID_NETWORK_ID = -1;
  const INVALID_TIME_STAMP = -1;

  const NETWORK_SELECTION_ENABLE = 0;
  const DISABLED_BAD_LINK = 1;
  const DISABLED_ASSOCIATION_REJECTION = 2;
  const DISABLED_AUTHENTICATION_FAILURE = 3;
  const DISABLED_DHCP_FAILURE = 4;
  const DISABLED_DNS_FAILURE = 5;
  const DISABLED_TLS_VERSION_MISMATCH = 6;
  const DISABLED_AUTHENTICATION_NO_CREDENTIALS = 7;
  const DISABLED_NO_INTERNET = 8;
  const DISABLED_BY_WIFI_MANAGER = 9;
  const NETWORK_SELECTION_DISABLED_MAX = 10;

  const NETWORK_SELECTION_ENABLED = 0;
  const NETWORK_SELECTION_TEMPORARY_DISABLED = 1;
  const NETWORK_SELECTION_PERMANENTLY_DISABLED = 2;

  var NETWORK_SELECTION_DISABLE_THRESHOLD = [
    -1, //  threshold for NETWORK_SELECTION_ENABLE
    1, //  threshold for DISABLED_BAD_LINK
    5, //  threshold for DISABLED_ASSOCIATION_REJECTION
    5, //  threshold for DISABLED_AUTHENTICATION_FAILURE
    5, //  threshold for DISABLED_DHCP_FAILURE
    5, //  threshold for DISABLED_DNS_FAILURE
    6, //  threshold for DISABLED_TLS_VERSION_MISMATCH
    1, //  threshold for DISABLED_AUTHENTICATION_NO_CREDENTIALS
    1, //  threshold for DISABLED_NO_INTERNET
    1, //  threshold for DISABLED_BY_WIFI_MANAGER
  ];
  var NETWORK_SELECTION_DISABLE_TIMEOUT = [
    Number.MAX_VALUE, // threshold for NETWORK_SELECTION_ENABLE
    15, // threshold for DISABLED_BAD_LINK
    5, // threshold for DISABLED_ASSOCIATION_REJECTION
    5, // threshold for DISABLED_AUTHENTICATION_FAILURE
    5, // threshold for DISABLED_DHCP_FAILURE
    5, // threshold for DISABLED_DNS_FAILURE
    Number.MAX_VALUE, // threshold for DISABLED_TLS_VERSION
    Number.MAX_VALUE, // threshold for DISABLED_AUTHENTICATION_NO_CREDENTIALS
    Number.MAX_VALUE, // threshold for DISABLED_NO_INTERNET
    Number.MAX_VALUE, // threshold for DISABLED_BY_WIFI_MANAGER
  ];
  var QUALITY_NETWORK_SELECTION_DISABLE_REASON = [
    "NETWORK_SELECTION_ENABLE",
    "NETWORK_SELECTION_DISABLED_BAD_LINK",
    "NETWORK_SELECTION_DISABLED_ASSOCIATION_REJECTION ",
    "NETWORK_SELECTION_DISABLED_AUTHENTICATION_FAILURE",
    "NETWORK_SELECTION_DISABLED_DHCP_FAILURE",
    "NETWORK_SELECTION_DISABLED_DNS_FAILURE",
    "NETWORK_SELECTION_DISABLED_TLS_VERSION",
    "NETWORK_SELECTION_DISABLED_AUTHENTICATION_NO_CREDENTIALS",
    "NETWORK_SELECTION_DISABLED_NO_INTERNET",
    "NETWORK_SELECTION_DISABLED_BY_WIFI_MANAGER",
  ];
  var lastUserSelectedNetwork = null;
  var lastUserSelectedNetworkTimeStamp = 0;
  var configuredNetworks = Object.create(null);

  // WifiConfigManager parameters
  configManager.configuredNetworks = configuredNetworks;
  configManager.NETWORK_SELECTION_ENABLE = NETWORK_SELECTION_ENABLE;
  configManager.DISABLED_BAD_LINK = DISABLED_BAD_LINK;
  configManager.DISABLED_ASSOCIATION_REJECTION = DISABLED_ASSOCIATION_REJECTION;
  configManager.DISABLED_AUTHENTICATION_FAILURE = DISABLED_AUTHENTICATION_FAILURE;
  configManager.DISABLED_DHCP_FAILURE = DISABLED_DHCP_FAILURE;
  configManager.DISABLED_DNS_FAILURE = DISABLED_DNS_FAILURE;
  configManager.DISABLED_TLS_VERSION_MISMATCH = DISABLED_TLS_VERSION_MISMATCH;
  configManager.DISABLED_AUTHENTICATION_NO_CREDENTIALS = DISABLED_AUTHENTICATION_NO_CREDENTIALS;
  configManager.DISABLED_NO_INTERNET = DISABLED_NO_INTERNET;
  configManager.DISABLED_BY_WIFI_MANAGER = DISABLED_BY_WIFI_MANAGER;
  configManager.NETWORK_SELECTION_ENABLED = NETWORK_SELECTION_ENABLED;
  configManager.QUALITY_NETWORK_SELECTION_DISABLE_REASON = QUALITY_NETWORK_SELECTION_DISABLE_REASON;

  // WifiConfigManager functions
  configManager.updateNetworkSelectionStatus = updateNetworkSelectionStatus;
  configManager.updateNetworkSelectionStatusForAuthFail = updateNetworkSelectionStatusForAuthFail;
  configManager.tryEnableQualifiedNetwork = tryEnableQualifiedNetwork;
  configManager.isLastSelectedConfiguration = isLastSelectedConfiguration;
  configManager.setAndEnableLastSelectedConfiguration = setAndEnableLastSelectedConfiguration;
  configManager.getLastSelectedConfiguration = getLastSelectedConfiguration;
  configManager.getLastSelectedTimeStamp = getLastSelectedTimeStamp;
  configManager.clearDisableReasonCounter = clearDisableReasonCounter;
  configManager.getNetworkConfiguration = getNetworkConfiguration;
  configManager.getHiddenNetworks = getHiddenNetworks;
  configManager.loadFromStore = loadFromStore;
  configManager.readFromStore = readFromStore;
  configManager.saveToStore = saveToStore;
  configManager.setDebug = setDebug;

  function setDebug(aDebug) {
    gDebug = aDebug;
  }

  function debug(aMsg) {
    if (gDebug) {
      dump("-*- WifiConfigManager: " + aMsg);
    }
  }

  function updateNetworkSelectionStatus(netId, reason, callback) {
    var found = false;
    for (var networkKey in configuredNetworks) {
      if (configuredNetworks[networkKey].netId == netId) {
        found = true;
        continue;
      }
    }
    if (!found) {
      debug("netId:" + netId + " not found in configuredNetworks");
      return callback(false);
    }

    if (reason == NETWORK_SELECTION_ENABLE) {
      updateNetworkStatus(configuredNetworks[networkKey], reason, callback);
      debug("Enable network:" + uneval(configuredNetworks[networkKey]));
      return;
    }
    incrementDisableReasonCounter(configuredNetworks[networkKey], reason);
    debug(
      "Network:" +
        configuredNetworks[networkKey].ssid +
        "disable counter of " +
        QUALITY_NETWORK_SELECTION_DISABLE_REASON[reason] +
        " is: " +
        configuredNetworks[networkKey].networkSeclectionDisableCounter[reason] +
        " and threshold is: " +
        NETWORK_SELECTION_DISABLE_THRESHOLD[reason]
    );
    if (
      configuredNetworks[networkKey].networkSeclectionDisableCounter[reason] >=
      NETWORK_SELECTION_DISABLE_THRESHOLD[reason]
    ) {
      updateNetworkStatus(configuredNetworks[networkKey], reason, callback);
      return;
    }
    return callback(false);
  }

  function updateNetworkSelectionStatusForAuthFail(netId, reason, callback) {
    var found = false;
    for (var networkKey in configuredNetworks) {
      if (configuredNetworks[networkKey].netId == netId) {
        found = true;
        continue;
      }
    }
    if (!found) {
      debug("netId:" + netId + " not found in configuredNetworks");
      return callback(false);
    }

    if (!configuredNetworks[networkKey].networkSeclectionDisableCounter) {
      configuredNetworks[networkKey].networkSeclectionDisableCounter = Array(
        NETWORK_SELECTION_DISABLED_MAX
      ).fill(0);
    }
    configuredNetworks[networkKey].networkSeclectionDisableCounter[reason] =
      NETWORK_SELECTION_DISABLE_THRESHOLD[reason];
    debug(
      "Network:" +
        configuredNetworks[networkKey].ssid +
        "disable counter of " +
        QUALITY_NETWORK_SELECTION_DISABLE_REASON[reason] +
        " is: " +
        configuredNetworks[networkKey].networkSeclectionDisableCounter[reason] +
        " and threshold is: " +
        NETWORK_SELECTION_DISABLE_THRESHOLD[reason]
    );
    updateNetworkStatus(configuredNetworks[networkKey], reason, callback);
  }

  function updateNetworkStatus(config, reason, callback) {
    if (reason == NETWORK_SELECTION_ENABLE) {
      if (
        typeof config.networkSelectionStatus == "undefined" ||
        !config.networkSelectionStatus
      ) {
        debug(
          "Need not change Qualified network Selection status since" +
            " already enabled"
        );
        return callback(false);
      }
      config.networkSelectionStatus = NETWORK_SELECTION_ENABLED;
      config.networkSelectionDisableReason = reason;
      config.disableTime = INVALID_TIME_STAMP;
      config.networkSeclectionDisableCounter = Array(
        NETWORK_SELECTION_DISABLED_MAX
      ).fill(0);
      callback(false);
    } else {
      if (isNetworkPermanentlyDisabled(config)) {
        debug("Do nothing. Alreay permanent disabled!");
        return callback(false);
      } else if (
        isNetworkTemporaryDisabled(config) &&
        reason < DISABLED_TLS_VERSION_MISMATCH
      ) {
        debug("Do nothing. Already temporarily disabled!");
        return callback(false);
      }
      callback(true);
      if (reason < DISABLED_TLS_VERSION_MISMATCH) {
        config.networkSelectionStatus = NETWORK_SELECTION_TEMPORARY_DISABLED;
      } else {
        config.networkSelectionStatus = NETWORK_SELECTION_PERMANENTLY_DISABLED;
      }
      config.disableTime = Date.now();
      config.networkSelectionDisableReason = reason;
      debug(
        "Network:" +
          config.ssid +
          "Configure new status:" +
          config.networkSelectionStatus +
          " with reason:" +
          config.networkSelectionDisableReason +
          " at: " +
          config.disableTime
      );
    }
  }

  function clearDisableReasonCounter(networkKey) {
    if (networkKey in configuredNetworks) {
      configuredNetworks[networkKey].networkSeclectionDisableCounter = Array(
        NETWORK_SELECTION_DISABLED_MAX
      ).fill(0);
    }
  }

  function incrementDisableReasonCounter(config, reason) {
    if (!config.networkSeclectionDisableCounter) {
      config.networkSeclectionDisableCounter = Array(
        NETWORK_SELECTION_DISABLED_MAX
      ).fill(0);
    }
    config.networkSeclectionDisableCounter[reason]++;
  }

  function isNetworkPermanentlyDisabled(config) {
    return config.networkSelectionStatus == null
      ? false
      : config.networkSelectionStatus == NETWORK_SELECTION_PERMANENTLY_DISABLED;
  }

  function isNetworkTemporaryDisabled(config) {
    return config.networkSelectionStatus == null
      ? false
      : config.networkSelectionStatus == NETWORK_SELECTION_TEMPORARY_DISABLED;
  }

  function isNetworkEnabled(config) {
    return config.networkSelectionStatus == null
      ? true
      : config.networkSelectionStatus == NETWORK_SELECTION_ENABLED;
  }

  function getLastSelectedTimeStamp() {
    return lastUserSelectedNetworkTimeStamp;
  }

  function getLastSelectedConfiguration() {
    return lastUserSelectedNetwork;
  }

  function setAndEnableLastSelectedConfiguration(netId, callback) {
    debug("setAndEnableLastSelectedConfiguration " + netId);
    if (netId == INVALID_NETWORK_ID) {
      lastUserSelectedNetwork = null;
      lastUserSelectedNetworkTimeStamp = 0;
      callback(false);
    } else {
      lastUserSelectedNetwork = netId;
      lastUserSelectedNetworkTimeStamp = Date.now();
      updateNetworkSelectionStatus(netId, NETWORK_SELECTION_ENABLE, callback);
    }
  }

  function isLastSelectedConfiguration(netId) {
    return lastUserSelectedNetwork == netId;
  }

  function tryEnableQualifiedNetwork(configuredNetworks) {
    for (let i in configuredNetworks) {
      if (isNetworkTemporaryDisabled(configuredNetworks[i])) {
        let timeDiff =
          (Date.now() - configuredNetworks[i].disableTime) / 1000 / 60;
        if (
          timeDiff < 0 ||
          timeDiff >=
            NETWORK_SELECTION_DISABLE_TIMEOUT[
              configuredNetworks[i].networkSelectionDisableReason
            ]
        ) {
          configuredNetworks[i].networkSelectionStatus = NETWORK_SELECTION_ENABLED;
          configuredNetworks[i].networkSelectionDisableReason = NETWORK_SELECTION_ENABLE;
          configuredNetworks[i].disableTime = INVALID_TIME_STAMP;
          configuredNetworks[i].networkSeclectionDisableCounter = Array(
            NETWORK_SELECTION_DISABLED_MAX
          ).fill(0);
        }
      }
    }
  }

  function getNetworkConfiguration(config, callback) {
    let networkKey = WifiConfigUtils.getNetworkKey(config);
    if (networkKey in configuredNetworks) {
      return callback(configuredNetworks[networkKey]);
    }
    callback(null);
  }

  function getHiddenNetworks() {
    let networks = [];
    for (let net in configuredNetworks) {
      if (net.hidden) {
        networks.push(configuredNetworks[net].ssid);
      }
    }
    return networks;
  }

  function loadFromStore() {
    let wifiConfig = WifiConfigStore.read();
    if (wifiConfig) {
      for (let i in wifiConfig) {
        let config = wifiConfig[i];
        let networkKey = WifiConfigUtils.getNetworkKey(config);
        configuredNetworks[networkKey] = config;
      }
    }
  }

  function saveToStore(networks, callback) {
    WifiConfigStore.write(networks, callback);
  }

  function readFromStore() {
    let wifiConfig = WifiConfigStore.read();
    return wifiConfig;
  }

  return configManager;
})();
