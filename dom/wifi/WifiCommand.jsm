/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["WifiCommand"];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { libcutils } = ChromeUtils.import(
  "resource://gre/modules/systemlibs.js"
);

const DEBUG = false;

const WIFI_CMD_INITIALIZE = Ci.nsIWifiCommand.INITIALIZE;
const WIFI_CMD_GET_CAPABILITIES = Ci.nsIWifiCommand.GET_CAPABILITIES;
const WIFI_CMD_GET_MODULE_VERSION = Ci.nsIWifiCommand.GET_MODULE_VERSION;
const WIFI_CMD_GET_DEBUG_LEVEL = Ci.nsIWifiCommand.GET_DEBUG_LEVEL;
const WIFI_CMD_SET_DEBUG_LEVEL = Ci.nsIWifiCommand.SET_DEBUG_LEVEL;
const WIFI_CMD_SET_LOW_LATENCY_MODE = Ci.nsIWifiCommand.SET_LOW_LATENCY_MODE;
const WIFI_CMD_SET_CONCURRENCY_PRIORITY = Ci.nsIWifiCommand.SET_CONCURRENCY_PRIORITY;
const WIFI_CMD_GET_HOST_WAKE_REASON = Ci.nsIWifiCommand.GET_HOST_WAKE_REASON;
const WIFI_CMD_START_WIFI = Ci.nsIWifiCommand.START_WIFI;
const WIFI_CMD_STOP_WIFI = Ci.nsIWifiCommand.STOP_WIFI;
const WIFI_CMD_GET_MAC_ADDRESS = Ci.nsIWifiCommand.GET_MAC_ADDRESS;
const WIFI_CMD_GET_STA_IFACE = Ci.nsIWifiCommand.GET_STA_IFACE;
const WIFI_CMD_GET_STA_CAPABILITIES = Ci.nsIWifiCommand.GET_STA_CAPABILITIES;
const WIFI_CMD_SET_POWER_SAVE = Ci.nsIWifiCommand.SET_POWER_SAVE;
const WIFI_CMD_SET_SUSPEND_MODE = Ci.nsIWifiCommand.SET_SUSPEND_MODE;
const WIFI_CMD_SET_EXTERNAL_SIM = Ci.nsIWifiCommand.SET_EXTERNAL_SIM;
const WIFI_CMD_SET_AUTO_RECONNECT = Ci.nsIWifiCommand.SET_AUTO_RECONNECT;
const WIFI_CMD_SET_COUNTRY_CODE = Ci.nsIWifiCommand.SET_COUNTRY_CODE;
const WIFI_CMD_SET_BT_COEXIST_MODE = Ci.nsIWifiCommand.SET_BT_COEXIST_MODE;
const WIFI_CMD_SET_BT_COEXIST_SCAN_MODE = Ci.nsIWifiCommand.SET_BT_COEXIST_SCAN_MODE;
const WIFI_CMD_START_SINGLE_SCAN = Ci.nsIWifiCommand.START_SINGLE_SCAN;
const WIFI_CMD_STOP_SINGLE_SCAN = Ci.nsIWifiCommand.STOP_SINGLE_SCAN;
const WIFI_CMD_START_PNO_SCAN = Ci.nsIWifiCommand.START_PNO_SCAN;
const WIFI_CMD_STOP_PNO_SCAN = Ci.nsIWifiCommand.STOP_PNO_SCAN;
const WIFI_CMD_GET_SCAN_RESULTS = Ci.nsIWifiCommand.GET_SCAN_RESULTS;
const WIFI_CMD_GET_PNO_SCAN_RESULTS = Ci.nsIWifiCommand.GET_PNO_SCAN_RESULTS;
const WIFI_CMD_SIGNAL_POLL = Ci.nsIWifiCommand.SIGNAL_POLL;
const WIFI_CMD_GET_TX_PACKET_COUNTERS = Ci.nsIWifiCommand.GET_TX_PACKET_COUNTERS;
const WIFI_CMD_GET_CHANNELS_FOR_BAND = Ci.nsIWifiCommand.GET_CHANNELS_FOR_BAND;
const WIFI_CMD_CONNECT = Ci.nsIWifiCommand.CONNECT;
const WIFI_CMD_RECONNECT = Ci.nsIWifiCommand.RECONNECT;
const WIFI_CMD_REASSOCIATE = Ci.nsIWifiCommand.REASSOCIATE;
const WIFI_CMD_DISCONNECT = Ci.nsIWifiCommand.DISCONNECT;
const WIFI_CMD_REMOVE_NETWORKS = Ci.nsIWifiCommand.REMOVE_NETWORKS;
const WIFI_CMD_START_RSSI_MONITORING = Ci.nsIWifiCommand.START_RSSI_MONITORING;
const WIFI_CMD_STOP_RSSI_MONITORING = Ci.nsIWifiCommand.STOP_RSSI_MONITORING;
const WIFI_CMD_START_SOFTAP = Ci.nsIWifiCommand.START_SOFTAP;
const WIFI_CMD_STOP_SOFTAP = Ci.nsIWifiCommand.STOP_SOFTAP;
const WIFI_CMD_GET_AP_IFACE = Ci.nsIWifiCommand.GET_AP_IFACE;

this.WifiCommand = function(aControlMessage, aInterface, aSdkVersion) {
  function debug(msg) {
    if (DEBUG) {
      dump("-------------- WifiCommand: " + msg);
    }
  }

  var command = {};

  //-------------------------------------------------
  // Utilities.
  //-------------------------------------------------
  command.getSdkVersion = function() {
    return aSdkVersion;
  };

  //-------------------------------------------------
  // General commands.
  //-------------------------------------------------
  command.initialize = function(callback) {
    voidControlMessage(WIFI_CMD_INITIALIZE, callback);
  };

  command.getSupportFeatures = function(callback) {
    doGetCommand(WIFI_CMD_GET_CAPABILITIES, callback);
  };

  command.getDriverModuleVersion = function(callback) {
    doGetCommand(WIFI_CMD_GET_MODULE_VERSION, callback);
  };

  // FIXME: the function will be removed
  command.setNetworkVariable = function(netId, name, value, callback) {
    callback(true);
  };

  command.getDebugLevel = function(callback) {
    doGetCommand(WIFI_CMD_GET_DEBUG_LEVEL, callback);
  };

  command.setDebugLevel = function(level, callback) {
    let msg = {
      cmd: WIFI_CMD_SET_DEBUG_LEVEL,
      iface: aInterface,
      debugLevel: level,
    };
    aControlMessage(msg, callback);
  };

  command.setLowLatencyMode = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_LOW_LATENCY_MODE, enable, callback);
  };

  command.setStaHigherPriority = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_CONCURRENCY_PRIORITY, enable, callback);
  };

  command.startWifi = function(callback) {
    voidControlMessage(WIFI_CMD_START_WIFI, callback);
  };

  command.stopWifi = function(callback) {
    voidControlMessage(WIFI_CMD_STOP_WIFI, callback);
  };

  command.getMacAddress = function(callback) {
    doGetCommand(WIFI_CMD_GET_MAC_ADDRESS, callback);
  };

  command.getStaInterface = function(callback) {
    doGetCommand(WIFI_CMD_GET_STA_IFACE, callback);
  };

  command.getStaCapabilities = function(callback) {
    doGetCommand(WIFI_CMD_GET_STA_CAPABILITIES, callback);
  };

  command.setPowerSave = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_POWER_SAVE, enable, callback);
  };

  command.setSuspendMode = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_SUSPEND_MODE, enable, callback);
  };

  command.setExternalSim = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_EXTERNAL_SIM, enable, callback);
  };

  command.enableAutoReconnect = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_AUTO_RECONNECT, enable, callback);
  };

  command.setCountryCode = function(countryCode, callback) {
    let msg = {
      cmd: WIFI_CMD_SET_COUNTRY_CODE,
      iface: aInterface,
      countryCode: countryCode,
    };
    aControlMessage(msg, callback);
  };

  command.setBluetoothCoexistenceMode = function(mode, callback) {
    let msg = {
      cmd: WIFI_CMD_SET_BT_COEXIST_MODE,
      iface: aInterface,
      btCoexistenceMode: mode,
    };
    aControlMessage(msg, callback);
  };

  command.setBluetoothCoexistenceScanMode = function(enable, callback) {
    doSetCommand(WIFI_CMD_SET_BT_COEXIST_SCAN_MODE, enable, callback);
  };

  command.startScan = function(settings, callback) {
    let msg = {
      cmd: WIFI_CMD_START_SINGLE_SCAN,
      iface: aInterface,
      scanSettings: settings,
    };
    aControlMessage(msg, callback);
  };

  command.stopScan = function(callback) {
    voidControlMessage(WIFI_CMD_STOP_SINGLE_SCAN, callback);
  };

  command.startPnoScan = function(callback) {
    voidControlMessage(WIFI_CMD_START_PNO_SCAN, callback);
  };

  command.stopPnoScan = function(callback) {
    voidControlMessage(WIFI_CMD_STOP_PNO_SCAN, callback);
  };

  command.getScanResults = function(callback) {
    doGetCommand(WIFI_CMD_GET_SCAN_RESULTS, callback);
  };

  command.getPnoScanResults = function(callback) {
    doGetCommand(WIFI_CMD_GET_PNO_SCAN_RESULTS, callback);
  };

  command.signalPoll = function(callback) {
    doGetCommand(WIFI_CMD_SIGNAL_POLL, callback);
  };

  command.getPacketCounters = function(callback) {
    doGetCommand(WIFI_CMD_GET_TX_PACKET_COUNTERS, callback);
  };

  command.getChannelsForBand = function(band, callback) {
    let msg = {
      cmd: WIFI_CMD_GET_CHANNELS_FOR_BAND,
      iface: aInterface,
      bandMask: band,
    };
    aControlMessage(msg, callback);
  };

  command.connect = function(config, callback) {
    let msg = {
      cmd: WIFI_CMD_CONNECT,
      iface: aInterface,
      config: config,
    };
    aControlMessage(msg, callback);
  };

  command.reconnect = function(callback) {
    voidControlMessage(WIFI_CMD_RECONNECT, callback);
  };

  command.reassociate = function(callback) {
    voidControlMessage(WIFI_CMD_REASSOCIATE, callback);
  };

  command.disconnect = function(callback) {
    voidControlMessage(WIFI_CMD_DISCONNECT, callback);
  };

  command.removeNetworks = function(callback) {
    voidControlMessage(WIFI_CMD_REMOVE_NETWORKS, callback);
  };

  command.startRssiMonitoring = function(callback) {
    voidControlMessage(WIFI_CMD_START_RSSI_MONITORING, callback);
  };

  command.stopRssiMonitoring = function(callback) {
    voidControlMessage(WIFI_CMD_STOP_RSSI_MONITORING, callback);
  };

  command.startSoftap = function(config, callback) {
    aControlMessage(
      {
        cmd: WIFI_CMD_START_SOFTAP,
        iface: aInterface,
        softapConfig: config
      },
      callback);
  };

  command.stopSoftap = function(callback) {
    voidControlMessage(WIFI_CMD_STOP_SOFTAP, callback);
  };

  command.getSoftapInterface = function(callback) {
    doGetCommand(WIFI_CMD_GET_AP_IFACE, callback);
  };

  //----------------------------------------------------------
  // Private stuff.
  //----------------------------------------------------------

  function voidControlMessage(cmd, callback) {
    aControlMessage({ cmd, iface: aInterface }, function(result) {
      callback(result.status);
    });
  }

  function doGetCommand(cmd, callback) {
    aControlMessage({ cmd, iface: aInterface }, function(result) {
      callback(result);
    });
  }

  function doSetCommand(cmd, enable, callback) {
    aControlMessage({ cmd, iface: aInterface, enabled: enable }, function(result) {
      callback(result);
    });
  }

  return command;
};
