/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["ScanResult", "WifiNetwork", "WifiConfigUtils"];

this.WifiConfigUtils = (function() {
  var wifiConfigUtils = {};

  const MODE_ESS = 0;
  const MODE_IBSS = 1;

  const EID_SSID = 0;
  const EID_SUPPORTED_RATES = 1;
  const EID_TIM = 5;
  const EID_BSS_LOAD = 11;
  const EID_ERP = 42;
  const EID_HT_CAPABILITIES = 45;
  const EID_RSN = 48;
  const EID_EXTENDED_SUPPORTED_RATES = 50;
  const EID_HT_OPERATION = 61;
  const EID_INTERWORKING = 107;
  const EID_ROAMING_CONSORTIUM = 111;
  const EID_EXTENDED_CAPS = 127;
  const EID_VHT_CAPABILITIES = 191;
  const EID_VHT_OPERATION = 192;
  const EID_VSA = 221;
  const EID_EXTENSION = 255;
  const EID_EXT_HE_CAPABILITIES = 35;

  const PROTOCOL_NONE = 0;
  const PROTOCOL_WPA = 1;
  const PROTOCOL_RSN = 2;
  const PROTOCOL_OSEN = 3;

  const KEY_MGMT_NONE = 0;
  const KEY_MGMT_PSK = 1;
  const KEY_MGMT_EAP = 2;
  const KEY_MGMT_FT_PSK = 3;
  const KEY_MGMT_FT_EAP = 4;
  const KEY_MGMT_PSK_SHA256 = 5;
  const KEY_MGMT_EAP_SHA256 = 6;
  const KEY_MGMT_OSEN = 7;
  const KEY_MGMT_SAE = 8;
  const KEY_MGMT_OWE = 9;
  const KEY_MGMT_EAP_SUITE_B_192 = 10;
  const KEY_MGMT_FT_SAE = 11;
  const KEY_MGMT_OWE_TRANSITION = 12;
  const KEY_MGMT_DPP = 13;
  const KEY_MGMT_FILS_SHA256 = 14;
  const KEY_MGMT_FILS_SHA384 = 15;

  const CIPHER_NONE = 0;
  const CIPHER_NO_GROUP_ADDRESSED = 1;
  const CIPHER_TKIP = 2;
  const CIPHER_CCMP = 3;
  const CIPHER_GCMP_256 = 4;

  const CAP_ESS_OFFSET = 1 << 0;
  const CAP_IBSS_OFFSET = 1 << 1;
  const CAP_PRIVACY_OFFSET = 1 << 4;

  const WPA_VENDOR_OUI_TYPE_ONE = 0x01f25000;
  const WPS_VENDOR_OUI_TYPE = 0x04f25000;
  const WPA_VENDOR_OUI_VERSION = 0x0001;
  const OWE_VENDOR_OUI_TYPE = 0x1c9a6f50;
  const RSNE_VERSION = 0x0001;

  const WPA_AKM_EAP = 0x01f25000;
  const WPA_AKM_PSK = 0x02f25000;

  const RSN_AKM_EAP = 0x01ac0f00;
  const RSN_AKM_PSK = 0x02ac0f00;
  const RSN_AKM_FT_EAP = 0x03ac0f00;
  const RSN_AKM_FT_PSK = 0x04ac0f00;
  const RSN_AKM_EAP_SHA256 = 0x05ac0f00;
  const RSN_AKM_PSK_SHA256 = 0x06ac0f00;
  const RSN_AKM_SAE = 0x08ac0f00;
  const RSN_AKM_FT_SAE = 0x09ac0f00;
  const RSN_AKM_OWE = 0x12ac0f00;
  const RSN_AKM_EAP_SUITE_B_192 = 0x0cac0f00;
  const WPA2_AKM_FILS_SHA256 = 0x0eac0f00;
  const WPA2_AKM_FILS_SHA384 = 0x0fac0f00;
  const WPA2_AKM_DPP = 0x029a6f50;

  const WPA_CIPHER_NONE = 0x00f25000;
  const WPA_CIPHER_TKIP = 0x02f25000;
  const WPA_CIPHER_CCMP = 0x04f25000;

  const RSN_CIPHER_NONE = 0x00ac0f00;
  const RSN_CIPHER_TKIP = 0x02ac0f00;
  const RSN_CIPHER_CCMP = 0x04ac0f00;
  const RSN_CIPHER_NO_GROUP_ADDRESSED = 0x07ac0f00;
  const RSN_CIPHER_GCMP_256 = 0x09ac0f00;

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
  wifiConfigUtils.getSecurity = getSecurity;
  wifiConfigUtils.getKeyMgmt = getKeyMgmt;
  wifiConfigUtils.calculateSignal = calculateSignal;
  wifiConfigUtils.getNetworkKey = getNetworkKey;
  wifiConfigUtils.parseInformationElements = parseInformationElements;
  wifiConfigUtils.parseCapabilities = parseCapabilities;

  function byte2String(buf) {
    var result = "";
    for (let i = 0; i < buf.length; i++) {
      result += String.fromCharCode(buf[i]);
    }
    return result;
  }

  function byte2Int(buf, pos, len) {
    let result = 0;
    let bytes = buf.splice(pos, len);
    for (let i = 0; i < bytes.length; i++) {
      result += bytes[i] << (8 * i);
    }
    pos = pos + len;
    return result;
  }

  function parseInformationElements(infoElement) {
    let pos = 0;
    let index = 0;
    let ies = [];
    while (pos < infoElement.length) {
      let ie = new InfoElement();
      // assign element id
      ie.id = infoElement[pos];
      pos = pos + 1;

      // assign element length
      let elementSize = infoElement[pos];
      pos = pos + 1;

      // assign element data
      let bytes = [];
      for (let i = 0; i < elementSize; i++) {
        bytes.push(infoElement[pos + i]);
      }
      ie.bytes = bytes;
      pos = pos + elementSize;
      ies[index++] = ie;
    }
    return ies;
  }

  function parseCapabilities(ies, beaconCap) {
    if (!ies || !beaconCap) {
      return {};
    }
    let flags = {
      protocol: [],
      keyManagement: [],
      groupCipher: [],
      pairwiseCipher: [],
      isWEP: false,
      isWPS: false,
    };
    flags.ess = beaconCap & CAP_ESS_OFFSET;
    flags.ibss = beaconCap & CAP_IBSS_OFFSET;
    let privacy = beaconCap & CAP_PRIVACY_OFFSET;

    for (let i = 0; i < ies.length; i++) {
      if (ies[i].id == EID_RSN) {
        parseRsnElement(ies[i].bytes, flags);
      }
      if (ies[i].id == EID_VSA) {
        parseVsaElement(ies[i].bytes, flags);
      }
    }
    flags.isWEP = flags.protocol.length == 0 && privacy;
    return flags;
  }

  function parseRsnCipher(cipher) {
    switch (cipher) {
      case RSN_CIPHER_NONE:
        return CIPHER_NONE;
      case RSN_CIPHER_TKIP:
        return CIPHER_TKIP;
      case RSN_CIPHER_CCMP:
        return CIPHER_CCMP;
      case RSN_CIPHER_GCMP_256:
        return CIPHER_GCMP_256;
      case RSN_CIPHER_NO_GROUP_ADDRESSED:
        return CIPHER_NO_GROUP_ADDRESSED;
      default:
        return CIPHER_NONE;
    }
  }

  function parseWpaCipher(cipher) {
    switch (cipher) {
      case WPA_CIPHER_NONE:
        return CIPHER_NONE;
      case WPA_CIPHER_TKIP:
        return CIPHER_TKIP;
      case WPA_CIPHER_CCMP:
        return CIPHER_CCMP;
      default:
        return CIPHER_NONE;
    }
  }

  function parseAKM(akm) {
    switch (akm) {
      case RSN_AKM_EAP:
        return KEY_MGMT_EAP;
      case RSN_AKM_PSK:
        return KEY_MGMT_PSK;
      case RSN_AKM_FT_EAP:
        return KEY_MGMT_FT_EAP;
      case RSN_AKM_FT_PSK:
        return KEY_MGMT_FT_PSK;
      case RSN_AKM_EAP_SHA256:
        return KEY_MGMT_EAP_SHA256;
      case RSN_AKM_PSK_SHA256:
        return KEY_MGMT_PSK_SHA256;
      case WPA2_AKM_FILS_SHA256:
        return KEY_MGMT_FILS_SHA256;
      case WPA2_AKM_FILS_SHA384:
        return KEY_MGMT_FILS_SHA384;
      case WPA2_AKM_DPP:
        return KEY_MGMT_DPP;
      case RSN_AKM_SAE:
        return KEY_MGMT_SAE;
      case RSN_AKM_FT_SAE:
        return KEY_MGMT_FT_SAE;
      case RSN_AKM_OWE:
        return KEY_MGMT_OWE;
      case RSN_AKM_EAP_SUITE_B_192:
        return KEY_MGMT_EAP_SUITE_B_192;
      default:
        return KEY_MGMT_EAP;
    }
  }

  function parseRsnElement(buf, flags) {
    // RSNE format (size unit: byte)
    //
    // | Element ID | Length | Version | Group Data Cipher Suite |
    //      1           1         2                 4
    // | Pairwise Cipher Suite Count | Pairwise Cipher Suite List |
    //              2                            4 * m
    // | AKM Suite Count | AKM Suite List | RSN Capabilities |
    //          2               4 * n               2
    // | PMKID Count | PMKID List | Group Management Cipher Suite |
    //        2          16 * s                 4
    let pos = 0;
    let version = byte2Int(buf, pos, 2);
    if (version != RSNE_VERSION) {
      return;
    }
    let groupCipher = byte2Int(buf, pos, 4);
    let cipherCount = byte2Int(buf, pos, 2);
    let pairwiseCipher = [];
    let cipher = 0;
    for (let i = 0; i < cipherCount; i++) {
      cipher = byte2Int(buf, pos, 4);
      pairwiseCipher.push(parseRsnCipher(cipher));
    }
    let akmCount = byte2Int(buf, pos, 2);
    let keyManagement = [];
    let akm = 0;
    for (let i = 0; i < akmCount; i++) {
      akm = byte2Int(buf, pos, 4);
      keyManagement.push(parseAKM(akm));
    }

    flags.protocol.push(PROTOCOL_RSN);
    flags.groupCipher.push(parseRsnCipher(groupCipher));
    flags.pairwiseCipher.push(pairwiseCipher);
    flags.keyManagement.push(keyManagement);
  }

  function parseWpaOneElement(buf, flags) {
    // WPA type 1 format (size unit: byte)
    //
    // | Element ID | Length | OUI | Type | Version |
    //      1           1       3     1        2
    // | Group Data Cipher Suite |
    //             4
    // | Pairwise Cipher Suite Count | Pairwise Cipher Suite List |
    //              2                            4 * m
    // | AKM Suite Count | AKM Suite List |
    //          2               4 * n
    let pos = 0;
    byte2Int(buf, pos, 4);
    let version = byte2Int(buf, pos, 2);
    if (version != WPA_VENDOR_OUI_VERSION) {
      return;
    }
    let groupCipher = byte2Int(buf, pos, 4);
    let cipherCount = byte2Int(buf, pos, 2);
    let pairwiseCipher = [];
    let cipher = 0;
    for (let i = 0; i < cipherCount; i++) {
      cipher = byte2Int(buf, pos, 4);
      pairwiseCipher.push(parseWpaCipher(cipher));
    }
    let akmCount = byte2Int(buf, pos, 2);
    let keyManagement = [];
    let akm = 0;
    for (let i = 0; i < akmCount; i++) {
      akm = byte2Int(buf, pos, 4);
      if (akm == WPA_AKM_EAP) {
        keyManagement.push(KEY_MGMT_EAP);
      } else if (akm == WPA_AKM_PSK) {
        keyManagement.push(KEY_MGMT_PSK);
      }
    }
    flags.protocol.push(PROTOCOL_WPA);
    flags.groupCipher.push(parseWpaCipher(groupCipher));
    flags.pairwiseCipher.push(pairwiseCipher);
    flags.keyManagement.push(keyManagement);
  }

  function parseVsaElement(buf, flags) {
    let pos = 0;
    let elementType = byte2Int(buf, pos, 4);
    if (elementType == WPA_VENDOR_OUI_TYPE_ONE) {
      // WPA one
      parseWpaOneElement(buf, flags);
    }
    if (elementType == WPS_VENDOR_OUI_TYPE) {
      // WPS
      flags.isWPS = true;
    }
  }

  function parseEncryption(flags) {
    let encryptType = {
      isWPA: false,
      isRSN: false,
      isPSK: false,
      isEAP: false,
    };

    if (!flags) {
      return encryptType;
    }

    for (let i = 0; i < flags.protocol.length; i++) {
      if (flags.protocol[i] == PROTOCOL_WPA) {
        encryptType.isWPA = true;
      } else if (flags.protocol[i] == PROTOCOL_RSN) {
        encryptType.isRSN = true;
      }
      for (let j = 0; j < flags.keyManagement[i].length; j++) {
        if (flags.keyManagement[i][j] == KEY_MGMT_PSK) {
          encryptType.isPSK = true;
        } else if (flags.keyManagement[i][j] == KEY_MGMT_EAP) {
          encryptType.isEAP = true;
        }
      }
    }
    return encryptType;
  }

  function getMode(flags) {
    if (flags.ibss) {
      return MODE_IBSS;
    }
    return MODE_ESS;
  }

  function getSecurity(flags) {
    var types = [];
    if (!flags) {
      return types;
    }

    let encrypt = parseEncryption(flags);
    if (encrypt.isEAP) {
      types.push("WPA-EAP");
    } else if (encrypt.isPSK) {
      if (encrypt.isWPA && encrypt.isRSN) {
        types.push("WPA/WPA2-PSK");
      } else if (encrypt.isWPA) {
        types.push("WPA-PSK");
      } else if (encrypt.isRSN) {
        types.push("WPA2-PSK");
      }
    } else if (flags.isWEP) {
      types.push("WEP");
    }
    return types;
  }

  function getKeyMgmt(flags) {
    let keyMgmt = "NONE";
    if (!flags) {
      return keyMgmt;
    }

    let encrypt = parseEncryption(flags);
    if (encrypt.isEAP) {
      keyMgmt = "WPA-EAP";
    } else if (encrypt.isPSK) {
      if (encrypt.isWPA || encrypt.isRSN) {
        keyMgmt = "WPA-PSK";
      }
    } else if (flags.isWEP) {
      keyMgmt = "WEP";
    }
    return keyMgmt;
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

    if ("security" in network && !network.security) {
      // manager network object, represents an AP
      // object structure
      // {
      //   .ssid           : SSID of AP
      //   .security       : "WPA-PSK" for WPA-PSK
      //                     "WPA-EAP" for WPA-EAP
      //                     "WEP" for WEP
      //                     "WAPI-PSK" for WAPI-PSK
      //                     "WAPI-CERT" for WAPI-CERT
      //                     "" for OPEN
      //   other keys
      // }
      var security = network.security;
      ssid = network.ssid;

      if (
        security === "WPA-PSK" ||
        security === "WPA2-PSK" ||
        security === "WPA/WPA2-PSK"
      ) {
        encryption = "WPA-PSK";
      } else if (security === "WPA-EAP") {
        encryption = "WPA-EAP";
      } else if (security === "WEP") {
        encryption = "WEP";
      } else if (security === "WAPI-PSK") {
        encryption = "WAPI-PSK";
      } else if (security === "WAPI-CERT") {
        encryption = "WAPI-CERT";
      }
    } else if ("keyMgmt" in network || "keyManagement" in network) {
      // configure network object, represents a network
      // object structure
      // {
      //   .ssid           : SSID of network, quoted
      //   .keyMgmt       : Encryption type
      //                     "WPA-PSK" for WPA-PSK
      //                     "WPA-EAP" for WPA-EAP
      //                     "NONE" for WEP/OPEN
      //                     "WAPI-PSK" for WAPI-PSK
      //                     "WAPI-CERT" for WAPI-CERT
      //   .auth_alg       : Encryption algorithm(WEP mode only)
      //                     "OPEN_SHARED" for WEP
      //   other keys
      // }
      var keyMgmt = network.keyMgmt ? network.keyMgmt : network.keyManagement;
      var auth_alg = network.auth_alg;
      ssid = network.keyMgmt ? dequote(network.ssid) : network.ssid;

      if (keyMgmt == "WPA-PSK") {
        encryption = "WPA-PSK";
      } else if (keyMgmt.includes("WPA-EAP")) {
        encryption = "WPA-EAP";
      } else if (keyMgmt == "WAPI-PSK") {
        encryption = "WAPI-PSK";
      } else if (keyMgmt == "WAPI-CERT") {
        encryption = "WAPI-CERT";
      } else if (keyMgmt == "NONE" && auth_alg === "OPEN SHARED") {
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

this.InfoElement = function InfoElement() {
  this.id = 0;
  this.bytes = [];

  this.__exposedProps__ = InfoElement.api;
};

InfoElement.api = {
  id: "rw",
  data: "rw",
};

this.WifiNetwork = function WifiNetwork(
  ssid,
  mode,
  frequency,
  security,
  password
) {
  this.ssid = ssid;
  this.mode = mode;
  this.frequency = frequency;
  this.security = security;

  if (typeof password !== "undefined") {
    this.password = password;
  }

  this.__exposedProps__ = WifiNetwork.api;
};

WifiNetwork.api = {
  ssid: "r",
  mode: "r",
  frequency: "r",
  security: "r",
  known: "r",
  connected: "r",

  password: "rw",
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

this.ScanResult = function ScanResult(ssid, bssid, frequency, flags, signal) {
  WifiNetwork.call(
    this,
    ssid,
    WifiConfigUtils.getMode(flags),
    frequency,
    WifiConfigUtils.getSecurity(flags),
    undefined
  );
  this.bssid = bssid;
  this.keyManagement = WifiConfigUtils.getKeyMgmt(flags);
  this.signalStrength = signal;
  this.relSignalStrength = WifiConfigUtils.calculateSignal(Number(signal));
  this.is24G = this.is24GHz(frequency);
  this.is5G = this.is5GHz(frequency);

  this.__exposedProps__ = ScanResult.api;
};

ScanResult.api = {
  bssid: "r",
  signalStrength: "r",
  relSignalStrength: "r",
  connected: "r",
  is24G: "r",
  is5G: "r",
};

for (let i in WifiNetwork.api) {
  ScanResult.api[i] = WifiNetwork.api[i];
}

ScanResult.prototype = {
  is24GHz(freq) {
    return freq > 2400 && freq < 2500;
  },

  is5GHz(freq) {
    return freq > 4900 && freq < 5900;
  },
};
