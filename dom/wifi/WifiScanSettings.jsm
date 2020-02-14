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

this.EXPORTED_SYMBOLS = ["WifiScanSettings"];

/**
 * Describes WiFi scan settings.
 */
this.WifiScanSettings = (function() {
  var wifiScanSettings = {};

  const SCAN_TYPE_LOW_SPAN = Ci.nsIScanSettings.SCAN_TYPE_LOW_SPAN;
  const SCAN_TYPE_LOW_POWER = Ci.nsIScanSettings.SCAN_TYPE_LOW_POWER;
  const SCAN_TYPE_HIGH_ACCURACY = Ci.nsIScanSettings.SCAN_TYPE_HIGH_ACCURACY;
  const BAND_2_4_GHZ = Ci.nsIScanSettings.BAND_2_4_GHZ;
  const BAND_5_GHZ = Ci.nsIScanSettings.BAND_5_GHZ;
  const BAND_5_GHZ_DFS = Ci.nsIScanSettings.BAND_5_GHZ_DFS;

  var bandMask = 0;
  var scanType = 0;
  var channels = [];
  var hiddenNetworks = [];

  var singleScanSettings = Object.create(null);

  wifiScanSettings.SCAN_TYPE_LOW_SPAN = SCAN_TYPE_LOW_SPAN;
  wifiScanSettings.SCAN_TYPE_LOW_POWER = SCAN_TYPE_LOW_POWER;
  wifiScanSettings.SCAN_TYPE_HIGH_ACCURACY = SCAN_TYPE_HIGH_ACCURACY;
  wifiScanSettings.BAND_2_4_GHZ = BAND_2_4_GHZ;
  wifiScanSettings.BAND_5_GHZ = BAND_5_GHZ;
  wifiScanSettings.BAND_5_GHZ_DFS = BAND_5_GHZ_DFS;

  wifiScanSettings.singleScanSettings = singleScanSettings;
  wifiScanSettings.bandMask = bandMask;
  wifiScanSettings.scanType = scanType;
  wifiScanSettings.channels = channels;
  wifiScanSettings.hiddenNetworks = hiddenNetworks;

  return wifiScanSettings;
})();
