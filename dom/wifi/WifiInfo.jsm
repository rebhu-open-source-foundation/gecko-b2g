/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

this.EXPORTED_SYMBOLS = ["WifiInfo"];

const INVALID_RSSI = -127;
const INVALID_NETWORK_ID = -1;

/**
 * Describes the state of any Wifi connection that is active or
 * is in the process of being set up.
 */
this.WifiInfo = function WifiInfo() {};
WifiInfo.prototype = {
  QueryInterface: ChromeUtils.generateQI([Ci.nsIWifiInfo]),
  supplicantState: "UNINITIALIZED",
  bssid: null,
  wifiSsid: null,
  security: null,
  networkId: INVALID_NETWORK_ID,
  rssi: INVALID_RSSI,
  linkSpeed: null,
  frequency: -1,
  ipAddress: -1,
  is24G: false,
  is5G: false,

  setSupplicantState(state) {
    this.supplicantState = state;
  },

  setInetAddress(address) {
    this.ipAddress = address;
  },

  setBSSID(bssid) {
    this.bssid = bssid;
  },

  setSSID(ssid) {
    this.wifiSsid = ssid;
  },

  setSecurity(security) {
    this.security = security;
  },

  setNetworkId(netId) {
    this.networkId = netId;
  },

  setRssi(rssi) {
    this.rssi = rssi;
  },

  setLinkSpeed(linkSpeed) {
    this.linkSpeed = linkSpeed;
  },

  setFrequency(frequency) {
    this.frequency = frequency;
    this.is24G = this.is24GHz();
    this.is5G = this.is5GHz();
  },

  reset() {
    this.supplicantState = "UNINITIALIZED";
    this.bssid = null;
    this.wifiSsid = null;
    this.security = null;
    this.networkId = INVALID_NETWORK_ID;
    this.rssi = INVALID_RSSI;
    this.linkSpeed = -1;
    this.frequency = -1;
    this.ipAddress = null;
  },

  is24GHz() {
    return this.frequency > 2400 && this.frequency < 2500;
  },

  is5GHz() {
    return this.frequency > 4900 && this.frequency < 5900;
  },
};
