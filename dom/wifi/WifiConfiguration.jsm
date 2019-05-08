/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

this.EXPORTED_SYMBOLS = ["WifiConfig", "WifiNetwork", "WifiConfigUtils"];

this.WifiConfigUtils = (function() {
  var wifiConfigUtils = {};

  const MODE_ESS = 0;
  const MODE_IBSS = 1;

  // These constants shamelessly ripped from WifiManager.java
  // strength is the value returned by scan_results. It is nominally in dB. We
  // transform it into a percentage for clients looking to simply show a
  // relative indication of the strength of a network.
  const MIN_RSSI = -100;
  const MAX_RSSI = -55;

  // WifiConfigUtils parameters
  wifiConfigUtils.MODE_ESS = MODE_ESS;
  wifiConfigUtils.MODE_IBSS = MODE_IBSS;
  wifiConfigUtils.MIN_RSSI = MIN_RSSI;
  wifiConfigUtils.MAX_RSSI = MAX_RSSI;

  // WifiConfigUtils functions
  wifiConfigUtils.getMode = getMode;
  wifiConfigUtils.getKeyManagement = getKeyManagement;
  wifiConfigUtils.getCapabilities = getCapabilities;
  wifiConfigUtils.calculateSignal = calculateSignal;
  wifiConfigUtils.getNetworkKey = getNetworkKey;

  function getMode(flags) {
    if (/\[IBSS/.test(flags)) {
      return MODE_IBSS;
    }

    return MODE_ESS;
  }

  function getKeyManagement(flags) {
    var types = [];
    if (!flags) {
      return types;
    }

    if (/\[WPA-PSK/.test(flags) && /\[WPA2-PSK/.test(flags)) {
      // Example of flags : [WPA-PSK-CCMP][WPA2-PSK-CCMP]
      types.push("WPA/WPA2-PSK");
    } else if (/\[WPA-PSK/.test(flags)) {
      types.push("WPA-PSK");
    } else if (/\[WPA2-PSK/.test(flags)) {
      types.push("WPA2-PSK");
    }
    if (/\[WPA2?-EAP/.test(flags)) {
      types.push("WPA-EAP");
    }
    if (/\[WEP/.test(flags)) {
      types.push("WEP");
    }
    if (/\[WAPI-KEY/.test(flags)) {
      types.push("WAPI-PSK");
    }
    if (/\[WAPI-CERT/.test(flags)) {
      types.push("WAPI-CERT");
    }
    return types;
  }

  function getCapabilities(flags) {
    var types = [];
    if (!flags) {
      return types;
    }

    if (/\[WPS/.test(flags)) {
      types.push("WPS");
    }
    return types;
  }

  function calculateSignal(strength) {
    if (strength > 0) {
      strength -= 256;
    }

    if (strength <= MIN_RSSI) {
      return 0;
    }
    if (strength >= MAX_RSSI) {
      return 100;
    }
    return Math.floor(((strength - MIN_RSSI) / (MAX_RSSI - MIN_RSSI)) * 100);
  }

  function dequote(s) {
    if (s[0] != '"' || s[s.length - 1] != '"') {
      throw "Invalid argument, not a quoted string: " + s;
    }
    return s.substr(1, s.length - 2);
  }

  // Get unique key for a network, now the key is created by escape(SSID)+Security.
  // So networks of same SSID but different security mode can be identified.
  function getNetworkKey(network) {
    var ssid = "",
      encryption = "OPEN";

    if ("security" in network) {
      // manager network object, represents an AP
      // object structure
      // {
      //   .ssid           : SSID of AP
      //   .security[]     : "WPA-PSK" for WPA-PSK
      //                     "WPA-EAP" for WPA-EAP
      //                     "WEP" for WEP
      //                     "WAPI-PSK" for WAPI-PSK
      //                     "WAPI-CERT" for WAPI-CERT
      //                     "" for OPEN
      //   other keys
      // }

      var security = network.security;
      ssid = network.ssid;

      for (let j = 0; j < security.length; j++) {
        if (
          security[j] === "WPA-PSK" ||
          security[j] === "WPA2-PSK" ||
          security[j] === "WPA/WPA2-PSK"
        ) {
          encryption = "WPA-PSK";
          break;
        } else if (security[j] === "WPA-EAP") {
          encryption = "WPA-EAP";
          break;
        } else if (security[j] === "WEP") {
          encryption = "WEP";
          break;
        } else if (security[j] === "WAPI-PSK") {
          encryption = "WAPI-PSK";
          break;
        } else if (security[j] === "WAPI-CERT") {
          encryption = "WAPI-CERT";
          break;
        }
      }
    } else if ("key_mgmt" in network) {
      // configure network object, represents a network
      // object structure
      // {
      //   .ssid           : SSID of network, quoted
      //   .key_mgmt       : Encryption type
      //                     "WPA-PSK" for WPA-PSK
      //                     "WPA-EAP" for WPA-EAP
      //                     "NONE" for WEP/OPEN
      //                     "WAPI-PSK" for WAPI-PSK
      //                     "WAPI-CERT" for WAPI-CERT
      //   .auth_alg       : Encryption algorithm(WEP mode only)
      //                     "OPEN_SHARED" for WEP
      //   other keys
      // }
      var key_mgmt = network.key_mgmt,
        auth_alg = network.auth_alg;
      ssid = dequote(network.ssid);

      if (key_mgmt == "WPA-PSK") {
        encryption = "WPA-PSK";
      } else if (key_mgmt.indexOf("WPA-EAP") != -1) {
        encryption = "WPA-EAP";
      } else if (key_mgmt == "WAPI-PSK") {
        encryption = "WAPI-PSK";
      } else if (key_mgmt == "WAPI-CERT") {
        encryption = "WAPI-CERT";
      } else if (key_mgmt == "NONE" && auth_alg === "OPEN SHARED") {
        encryption = "WEP";
      }
    }

    // ssid here must be dequoted, and it's safer to esacpe it.
    // encryption won't be empty and always be assigned one of the followings :
    // "OPEN"/"WEP"/"WPA-PSK"/"WPA-EAP"/"WAPI-PSK"/"WAPI-CERT".
    // So for a invalid network object, the returned key will be "OPEN".
    return escape(ssid) + encryption;
  }

  return wifiConfigUtils;
})();

this.WifiNetwork = function WifiNetwork(
  ssid,
  mode,
  frequency,
  security,
  password,
  capabilities
) {
  this.ssid = ssid;
  this.mode = mode;
  this.frequency = frequency;
  this.security = security;

  if (typeof password !== "undefined") {
    this.password = password;
  }
  if (capabilities !== undefined) {
    this.capabilities = capabilities;
  }

  this.__exposedProps__ = WifiNetwork.api;
};

WifiNetwork.api = {
  ssid: "r",
  mode: "r",
  frequency: "r",
  security: "r",
  capabilities: "r",
  known: "r",
  connected: "r",

  password: "rw",
  keyManagement: "rw",
  psk: "rw",
  identity: "rw",
  wep: "rw",
  keyIndex: "rw",
  hidden: "rw",
  eap: "rw",
  pin: "rw",
  phase1: "rw",
  phase2: "rw",
  serverCertificate: "rw",
  userCertificate: "rw",
  sim_num: "rw",
  wapi_psk: "rw",
  pskType: "rw",
  wapiAsCertificate: "rw",
  wapiUserCertificate: "rw",
};

this.WifiConfig = function WifiConfig(ssid, bssid, frequency, flags, signal) {
  WifiNetwork.call(
    this,
    ssid,
    WifiConfigUtils.getMode(flags),
    frequency,
    WifiConfigUtils.getKeyManagement(flags),
    undefined,
    WifiConfigUtils.getCapabilities(flags)
  );
  this.bssid = bssid;
  this.signalStrength = signal;
  this.relSignalStrength = WifiConfigUtils.calculateSignal(Number(signal));
  this.is24G = this.is24GHz(frequency);
  this.is5G = this.is5GHz(frequency);

  this.__exposedProps__ = WifiConfig.api;
};

WifiConfig.api = {
  bssid: "r",
  signalStrength: "r",
  relSignalStrength: "r",
  connected: "r",
  is24G: "r",
  is5G: "r",
};

for (let i in WifiNetwork.api) {
  WifiConfig.api[i] = WifiNetwork.api[i];
}

WifiConfig.prototype = {
  is24GHz(freq) {
    return freq > 2400 && freq < 2500;
  },

  is5GHz(freq) {
    return freq > 4900 && freq < 5900;
  },
};
