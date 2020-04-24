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
  const DISABLED_BY_WRONG_PASSWORD = 10;
  const NETWORK_SELECTION_DISABLED_MAX = 11;

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
    1, //  threshold for DISABLED_BY_WRONG_PASSWORD
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
    Number.MAX_VALUE, // threshold for DISABLED_BY_WRONG_PASSWORD
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
    "NETWORK_SELECTION_DISABLED_BY_WRONG_PASSWORD",
  ];
  var lastUserSelectedNetworkId = INVALID_NETWORK_ID;
  var lastUserSelectedNetworkTimeStamp = 0;
  var configuredNetworks = Object.create(null);

  // The network ID to be assigned on a new added network
  var useNetworkId = 0;

  // Set in wpa_supplicant "bssid" field if no specific AP restricted
  const SUPPLICANT_BSSID_ANY = "00:00:00:00:00:00";

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
  configManager.DISABLED_BY_WRONG_PASSWORD = DISABLED_BY_WRONG_PASSWORD;
  configManager.NETWORK_SELECTION_ENABLED = NETWORK_SELECTION_ENABLED;
  configManager.QUALITY_NETWORK_SELECTION_DISABLE_REASON = QUALITY_NETWORK_SELECTION_DISABLE_REASON;

  // WifiConfigManager functions
  configManager.updateNetworkSelectionStatus = updateNetworkSelectionStatus;
  configManager.tryEnableQualifiedNetwork = tryEnableQualifiedNetwork;
  configManager.isLastSelectedNetwork = isLastSelectedNetwork;
  configManager.clearLastSelectedNetwork = clearLastSelectedNetwork;
  configManager.updateLastSelectedNetwork = updateLastSelectedNetwork;
  configManager.getLastSelectedNetwork = getLastSelectedNetwork;
  configManager.getLastSelectedTimeStamp = getLastSelectedTimeStamp;
  configManager.clearDisableReasonCounter = clearDisableReasonCounter;
  configManager.setEverConnected = setEverConnected;
  configManager.getNetworkConfiguration = getNetworkConfiguration;
  configManager.fetchNetworkConfiguration = fetchNetworkConfiguration;
  configManager.getHiddenNetworks = getHiddenNetworks;
  configManager.addOrUpdateNetwork = addOrUpdateNetwork;
  configManager.removeNetwork = removeNetwork;
  configManager.clearCurrentConfigBssid = clearCurrentConfigBssid;
  configManager.loadFromStore = loadFromStore;
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
        break;
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

    if (reason == NETWORK_SELECTION_ENABLE) {
      updateNetworkStatus(configuredNetworks[networkKey], reason, callback);
      debug("Enable network:" + uneval(configuredNetworks[networkKey]));
      return callback(false);
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
      updateNetworkStatus(configuredNetworks[networkKey], reason, function(doDisable) {
        return callback(doDisable);
      });
    }
    return callback(false);
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
      return callback(false);
    }

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
    return callback(true);
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

  function getLastSelectedTimeStamp() {
    return lastUserSelectedNetworkTimeStamp;
  }

  function getLastSelectedNetwork() {
    return lastUserSelectedNetworkId;
  }

  function clearLastSelectedNetwork() {
    lastUserSelectedNetworkId = INVALID_NETWORK_ID;
    lastUserSelectedNetworkTimeStamp = INVALID_TIME_STAMP;
  }

  function updateLastSelectedNetwork(netId, callback) {
    debug("updateLastSelectedNetwork " + netId);
    lastUserSelectedNetworkId = netId;
    lastUserSelectedNetworkTimeStamp = Date.now();
    updateNetworkSelectionStatus(netId, NETWORK_SELECTION_ENABLE, callback);
  }

  function isLastSelectedNetwork(netId) {
    return lastUserSelectedNetworkId == netId;
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
          updateNetworkSelectionStatus(
            configuredNetworks[i].netId,
            NETWORK_SELECTION_ENABLE,
            function() {}
          );
        }
      }
    }
  }

  function setEverConnected(config, everConnected) {
    let networkKey = WifiConfigUtils.getNetworkKey(config);
    if (networkKey in configuredNetworks) {
      configuredNetworks[networkKey].hasEverConnected = everConnected;
    }
  }

  function getNetworkConfiguration(netId, callback) {
    if (netId === INVALID_NETWORK_ID) {
      return callback(null);
    }
    // incoming config may not have entire security field
    // also check id in configured networks
    for (let net in configuredNetworks) {
      if (netId == configuredNetworks[net].netId) {
        return callback(configuredNetworks[net]);
      }
    }
    return callback(null);
  }

  function fetchNetworkConfiguration(config, callback) {
    getNetworkConfiguration(config.netId, function(network) {
      // must make sure the netId is in configuredNetworks.
      if (network == null) {
        return callback(null);
      }

      for (let field in network) {
        if (config[field]) {
          continue;
        }
        config[field] = network[field];
      }
      return callback(config);
    });
  }

  function getHiddenNetworks() {
    let networks = [];
    for (let net in configuredNetworks) {
      if (net.scanSsid) {
        networks.push(configuredNetworks[net].ssid);
      }
    }
    return networks;
  }

  function addOrUpdateNetwork(config, callback) {
    if (config == null) {
      return callback(false);
    }

    let networkKey = WifiConfigUtils.getNetworkKey(config);
    if (networkKey in configuredNetworks) {
      // network already exist, try to update
      let existConfig = configuredNetworks[networkKey];

      for (let field in config) {
        if (config[field]) {
          existConfig[field] = config[field];
        }
      }
    } else {
      if (config.netId !== INVALID_NETWORK_ID) {
        debug("Network id is exist");
      }

      // assign an unique id to this network
      config.netId = useNetworkId;
      useNetworkId = useNetworkId + 1;
      configuredNetworks[networkKey] = config;
    }

    saveToStore(configuredNetworks, function() {});
    return callback(true);
  }

  function removeNetwork(networkId, callback) {
    if (networkId === INVALID_NETWORK_ID) {
      debug("Trying to remove network by invalid network ID");
      return callback(false);
    }

    for (let networkKey in configuredNetworks) {
      if (configuredNetworks[networkKey].netId === networkId) {
        delete configuredNetworks[networkKey];
        saveToStore(configuredNetworks, function() {});
        return callback(true);
      }
    }
    debug("Network " + networkId + " is not in the list");
    return callback(false);
  }

  function clearCurrentConfigBssid(netId, callback) {
    getNetworkConfiguration(netId, function(network) {
      if (network == null) {
        return callback(false);
      }
      let networkKey = WifiConfigUtils.getNetworkKey(network);

      if (networkKey in configuredNetworks) {
        configuredNetworks[networkKey].bssid = SUPPLICANT_BSSID_ANY;
        saveToStore(configuredNetworks, function() {
          return callback(true);
        });
      }
      return callback(false);
    });
  }

  function loadFromStore() {
    let wifiConfig = WifiConfigStore.read();
    if (wifiConfig) {
      for (let i in wifiConfig) {
        let config = wifiConfig[i];
        let networkKey = WifiConfigUtils.getNetworkKey(config);
        config.netId = useNetworkId++;
        configuredNetworks[networkKey] = config;
      }
    }
  }

  function saveToStore(networks, callback) {
    WifiConfigStore.write(networks, callback);
  }

  return configManager;
})();
