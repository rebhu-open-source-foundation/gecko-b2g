/* -*- indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["WifiWorker"];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);
const { Services } = ChromeUtils.import(
  "resource://gre/modules/Services.jsm"
);
const { libcutils, netHelpers } = ChromeUtils.import(
  "resource://gre/modules/systemlibs.js"
);
const { WifiInfo } = ChromeUtils.import(
  "resource://gre/modules/WifiInfo.jsm"
);
const { WifiNetUtil } = ChromeUtils.import(
  "resource://gre/modules/WifiNetUtil.jsm"
);
const { WifiCommand } = ChromeUtils.import(
  "resource://gre/modules/WifiCommand.jsm"
);
const { OpenNetworkNotifier } = ChromeUtils.import(
  "resource://gre/modules/OpenNetworkNotifier.jsm"
);
const { WifiNetworkSelector } = ChromeUtils.import(
  "resource://gre/modules/WifiNetworkSelector.jsm"
);
const { WifiConfigManager } = ChromeUtils.import(
  "resource://gre/modules/WifiConfigManager.jsm"
);
const { WifiScanSettings, WifiPnoSettings } = ChromeUtils.import(
  "resource://gre/modules/WifiScanSettings.jsm"
);
const { FileUtils } = ChromeUtils.import(
  "resource://gre/modules/FileUtils.jsm"
);
const { Timer, clearTimeout, setTimeout } = ChromeUtils.import(
  "resource://gre/modules/Timer.jsm"
);
const { ScanResult, WifiNetwork, WifiConfigUtils } = ChromeUtils.import(
  "resource://gre/modules/WifiConfiguration.jsm"
);

var DEBUG = true; // set to true to show debug messages.

const SUCCESS = Ci.nsIWifiResult.SUCCESS;
const ERROR_COMMAND_FAILED = Ci.nsIWifiResult.ERROR_COMMAND_FAILED;
const ERROR_INVALID_INTERFACE = Ci.nsIWifiResult.ERROR_INVALID_INTERFACE;
const ERROR_INVALID_ARGS = Ci.nsIWifiResult.ERROR_INVALID_ARGS;
const ERROR_NOT_SUPPORTED = Ci.nsIWifiResult.ERROR_NOT_SUPPORTED;
const ERROR_TIMEOUT = Ci.nsIWifiResult.ERROR_TIMEOUT;
const ERROR_UNKNOWN = Ci.nsIWifiResult.ERROR_UNKNOWN;

const SSID_MAX_LEN_IN_BYTES = Ci.nsIWifiConfiguration.SSID_MAX_LEN_IN_BYTES;
const PSK_PASSPHRASE_MIN_LEN_IN_BYTES = Ci.nsIWifiConfiguration.PSK_PASSPHRASE_MIN_LEN_IN_BYTES;
const PSK_PASSPHRASE_MAX_LEN_IN_BYTES = Ci.nsIWifiConfiguration.PSK_PASSPHRASE_MAX_LEN_IN_BYTES;
const WEP_KEYS_MAX_NUM = Ci.nsIWifiConfiguration.WEP_KEYS_MAX_NUM;
const WEP40_KEY_LEN_IN_BYTES = Ci.nsIWifiConfiguration.WEP40_KEY_LEN_IN_BYTES;
const WEP104_KEY_LEN_IN_BYTES = Ci.nsIWifiConfiguration.WEP104_KEY_LEN_IN_BYTES;

const KEYMGMT_WPA_EAP = Ci.nsIWifiConfiguration.KEYMGMT_WPA_EAP;
const KEYMGMT_WPA_PSK = Ci.nsIWifiConfiguration.KEYMGMT_WPA_PSK;
const KEYMGMT_NONE = Ci.nsIWifiConfiguration.KEYMGMT_NONE;
const KEYMGMT_IEEE8021X = Ci.nsIWifiConfiguration.KEYMGMT_IEEE8021X;
const KEYMGMT_FT_EAP = Ci.nsIWifiConfiguration.KEYMGMT_FT_EAP;
const KEYMGMT_FT_PSK = Ci.nsIWifiConfiguration.KEYMGMT_FT_PSK;
const KEYMGMT_OSEN = Ci.nsIWifiConfiguration.KEYMGMT_OSEN;

const PROTO_WPA = Ci.nsIWifiConfiguration.PROTO_WPA;
const PROTO_RSN = Ci.nsIWifiConfiguration.PROTO_RSN;
const PROTO_OSEN = Ci.nsIWifiConfiguration.PROTO_OSEN;

const AUTHALG_OPEN = Ci.nsIWifiConfiguration.AUTHALG_OPEN;
const AUTHALG_SHARED = Ci.nsIWifiConfiguration.AUTHALG_SHARED;
const AUTHALG_LEAP = Ci.nsIWifiConfiguration.AUTHALG_LEAP;

const GROUPCIPHER_WEP40 = Ci.nsIWifiConfiguration.GROUPCIPHER_WEP40;
const GROUPCIPHER_WEP104 = Ci.nsIWifiConfiguration.GROUPCIPHER_WEP104;
const GROUPCIPHER_TKIP = Ci.nsIWifiConfiguration.GROUPCIPHER_TKIP;
const GROUPCIPHER_CCMP = Ci.nsIWifiConfiguration.GROUPCIPHER_CCMP;
const GROUPCIPHER_GTK_NOT_USED = Ci.nsIWifiConfiguration.GROUPCIPHER_GTK_NOT_USED;

const PAIRWISECIPHER_NONE = Ci.nsIWifiConfiguration.PAIRWISECIPHER_NONE;
const PAIRWISECIPHER_TKIP = Ci.nsIWifiConfiguration.PAIRWISECIPHER_TKIP;
const PAIRWISECIPHER_CCMP = Ci.nsIWifiConfiguration.PAIRWISECIPHER_CCMP;

const EAP_PEAP = Ci.nsIWifiConfiguration.EAP_PEAP;
const EAP_TLS = Ci.nsIWifiConfiguration.EAP_TLS;
const EAP_TTLS = Ci.nsIWifiConfiguration.EAP_TTLS;
const EAP_PWD = Ci.nsIWifiConfiguration.EAP_PWD;
const EAP_SIM = Ci.nsIWifiConfiguration.EAP_SIM;
const EAP_AKA = Ci.nsIWifiConfiguration.EAP_AKA;
const EAP_AKA_PRIME = Ci.nsIWifiConfiguration.EAP_AKA_PRIME;
const EAP_WFA_UNAUTH_TLS = Ci.nsIWifiConfiguration.EAP_WFA_UNAUTH_TLS;

const PHASE2_NONE = Ci.nsIWifiConfiguration.PHASE2_NONE;
const PHASE2_PAP = Ci.nsIWifiConfiguration.PHASE2_PAP;
const PHASE2_MSPAP = Ci.nsIWifiConfiguration.PHASE2_MSPAP;
const PHASE2_MSPAPV2 = Ci.nsIWifiConfiguration.PHASE2_MSPAPV2;
const PHASE2_GTC = Ci.nsIWifiConfiguration.PHASE2_GTC;
const PHASE2_SIM = Ci.nsIWifiConfiguration.PHASE2_SIM;
const PHASE2_AKA = Ci.nsIWifiConfiguration.PHASE2_AKA;
const PHASE2_AKA_PRIME = Ci.nsIWifiConfiguration.PHASE2_AKA_PRIME;

const AP_BAND_24GHZ = Ci.nsISoftapConfiguration.AP_BAND_24GHZ;
const AP_BAND_5GHZ = Ci.nsISoftapConfiguration.AP_BAND_5GHZ;
const AP_BAND_ANY = Ci.nsISoftapConfiguration.AP_BAND_ANY;
const AP_SECURITY_NONE = Ci.nsISoftapConfiguration.SECURITY_NONE;
const AP_SECURITY_WPA = Ci.nsISoftapConfiguration.SECURITY_WPA;
const AP_SECURITY_WPA2 = Ci.nsISoftapConfiguration.SECURITY_WPA2;

const LEVEL_EXCESSIVE = Ci.nsISupplicantDebugLevel.EXCESSIVE;
const LEVEL_MSGDUMP = Ci.nsISupplicantDebugLevel.MSGDUMP;
const LEVEL_DEBUG = Ci.nsISupplicantDebugLevel.DEBUG;
const LEVEL_INFO = Ci.nsISupplicantDebugLevel.INOF;
const LEVEL_WARNING = Ci.nsISupplicantDebugLevel.WARNING;
const LEVEL_ERROR = Ci.nsISupplicantDebugLevel.ERROR;

const USE_SINGLE_SCAN = Ci.nsIScanSettings.USE_SINGLE_SCAN;
const USE_PNO_SCAN = Ci.nsIScanSettings.USE_PNO_SCAN;

const WIFIWORKER_CONTRACTID = "@mozilla.org/wifi/worker;1";
const WIFIWORKER_CID = Components.ID("{a14e8977-d259-433a-a88d-58dd44657e5b}");

const WIFIWORKER_WORKER = "resource://gre/modules/wifi_worker.js";

const kMozSettingsChangedObserverTopic = "mozsettings-changed";
const kXpcomShutdownChangedTopic = "xpcom-shutdown";
const kScreenStateChangedTopic = "screen-state-changed";
const kInterfaceAddressChangedTopic = "interface-address-change";
const kInterfaceDnsInfoTopic = "interface-dns-info";
const kRouteChangedTopic = "route-change";
const kPrefDefaultServiceId = "dom.telephony.defaultServiceId";
const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";
const kWifiCaptivePortalResult = "wifi-captive-portal-result";
const kOpenCaptivePortalLoginEvent = "captive-portal-login";
const kCaptivePortalLoginSuccessEvent = "captive-portal-login-success";

const MAX_SUPPLICANT_LOOP_ITERATIONS = 4;

// Settings DB path for wifi
const SETTINGS_WIFI_ENABLED = "wifi.enabled";
const SETTINGS_WIFI_DEBUG_ENABLED = "wifi.debugging.enabled";
// Settings DB path for Wifi tethering.
const SETTINGS_WIFI_TETHERING_ENABLED = "tethering.wifi.enabled";
const SETTINGS_WIFI_SSID = "tethering.wifi.ssid";
const SETTINGS_WIFI_SECURITY_TYPE = "tethering.wifi.security.type";
const SETTINGS_WIFI_SECURITY_PASSWORD = "tethering.wifi.security.password";
const SETTINGS_WIFI_CHANNEL = "tethering.wifi.channel";
const SETTINGS_WIFI_IP = "tethering.wifi.ip";
const SETTINGS_WIFI_PREFIX = "tethering.wifi.prefix";
const SETTINGS_WIFI_DHCPSERVER_STARTIP = "tethering.wifi.dhcpserver.startip";
const SETTINGS_WIFI_DHCPSERVER_ENDIP = "tethering.wifi.dhcpserver.endip";
const SETTINGS_WIFI_DNS1 = "tethering.wifi.dns1";
const SETTINGS_WIFI_DNS2 = "tethering.wifi.dns2";

// Settings DB path for USB tethering.
const SETTINGS_USB_DHCPSERVER_STARTIP = "tethering.usb.dhcpserver.startip";
const SETTINGS_USB_DHCPSERVER_ENDIP = "tethering.usb.dhcpserver.endip";

// Settings DB for airplane mode.
const SETTINGS_AIRPLANE_MODE = "airplaneMode.enabled";
const SETTINGS_AIRPLANE_MODE_STATUS = "airplaneMode.status";

// Settings DB for open network notify
const SETTINGS_WIFI_NOTIFYCATION = "wifi.notification";

// Default value for WIFI tethering.
const DEFAULT_HOTSPOT_IP = "192.168.1.1";
const DEFAULT_HOTSPOT_PREFIX = "24";
const DEFAULT_HOTSPOT_CHANNEL = 1;
const DEFAULT_HOTSPOT_BAND = AP_BAND_24GHZ;
const DEFAULT_HOTSPOT_ENABLE_11N = true;
const DEFAULT_HOTSPOT_ENABLE_11AC = false;
const DEFAULT_HOTSPOT_ENABLE_ACS = false;
const DEFAULT_HOTSPOT_DHCPSERVER_STARTIP = "192.168.1.10";
const DEFAULT_HOTSPOT_DHCPSERVER_ENDIP = "192.168.1.30";
const DEFAULT_HOTSPOT_SSID = "FirefoxHotspot";
const DEFAULT_HOTSPOT_SECURITY_TYPE = "open";
const DEFAULT_HOTSPOT_SECURITY_PASSWORD = "1234567890";
const DEFAULT_HOTSPOT_HIDDEN = false;
const DEFAULT_DNS1 = "8.8.8.8";
const DEFAULT_DNS2 = "8.8.4.4";

// Default value for USB tethering.
const DEFAULT_USB_DHCPSERVER_STARTIP = "192.168.0.10";
const DEFAULT_USB_DHCPSERVER_ENDIP = "192.168.0.30";

const WIFI_FIRMWARE_AP = "AP";
const WIFI_FIRMWARE_STATION = "STA";
const WIFI_SECURITY_TYPE_NONE = "open";
const WIFI_SECURITY_TYPE_WPA_PSK = "wpa-psk";
const WIFI_SECURITY_TYPE_WPA2_PSK = "wpa2-psk";

const NETWORK_INTERFACE_UP = "up";
const NETWORK_INTERFACE_DOWN = "down";

const DEFAULT_WLAN_INTERFACE = "wlan0";

const DHCP_PROP = "init.svc.dhcpcd";

const POWER_MODE_DHCP = 1;
const POWER_MODE_SCREEN_STATE = 1 << 1;
const POWER_MODE_SETTING_CHANGED = 1 << 2;

const PROMPT_UNVALIDATED_DELAY_MS = 8000;

const INVALID_RSSI = -127;
const INVALID_NETWORK_ID = -1;

const WIFI_ASSOCIATED_SCAN_INTERVAL = 20 * 1000;

XPCOMUtils.defineLazyGetter(this, "ppmm", () => {
  return Cc["@mozilla.org/parentprocessmessagemanager;1"].getService();
});

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gNetworkManager",
  "@mozilla.org/network/manager;1",
  "nsINetworkManager"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gNetworkService",
  "@mozilla.org/network/service;1",
  "nsINetworkService"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gTetheringService",
  "@mozilla.org/tethering/service;1",
  "nsITetheringService"
);

// XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
//                                    "@mozilla.org/settingsService;1",
//                                    "nsISettingsService");

// XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
//                                    "@mozilla.org/mobileconnection/mobileconnectionservice;1",
//                                    "nsIMobileConnectionService");

// XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
//                                    "@mozilla.org/icc/iccservice;1",
//                                    "nsIIccService");

// XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
//                                    "@mozilla.org/power/powermanagerservice;1",
//                                    "nsIPowerManagerService");

// XPCOMUtils.defineLazyServiceGetter(this, "gImsRegService",
//                                    "@mozilla.org/mobileconnection/imsregservice;1",
//                                    "nsIImsRegService");

// XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumberUtils",
//                                   "resource://gre/modules/PhoneNumberUtils.jsm");

ChromeUtils.defineModuleGetter(
  this,
  "WifiP2pManager",
  "resource://gre/modules/WifiP2pManager.jsm"
);

ChromeUtils.defineModuleGetter(
  this,
  "WifiP2pWorkerObserver",
  "resource://gre/modules/WifiP2pWorkerObserver.jsm"
);

const WIFI_CONFIG_PATH = "/data/misc/wifi/wifi_config_file.json";

var wifiInfo = new WifiInfo();
var lastNetwork = null;

// A note about errors and error handling in this file:
// The libraries that we use in this file are intended for C code. For
// C code, it is natural to return -1 for errors and 0 for success.
// Therefore, the code that interacts directly with the worker uses this
// convention (note: command functions do get boolean results since the
// command always succeeds and we do a string/boolean check for the
// expected results).
var WifiManager = (function() {
  var manager = {};

  function getStartupPrefs() {
    return {
      // FIXME: sdk NaN
      sdkVersion: 29, //parseInt(libcutils.property_get("ro.build.version.sdk"), 10),
      schedScanRecovery:
        libcutils.property_get("ro.moz.wifi.sched_scan_recover") !== "false",
      p2pSupported: libcutils.property_get("ro.moz.wifi.p2p_supported") === "1",
      eapSimSupported:
        libcutils.property_get("ro.moz.wifi.eapsim_supported", "true") ===
        "true",
      ibssSupported:
        libcutils.property_get("ro.moz.wifi.ibss_supported", "true") === "true",
      ifname: libcutils.property_get("wifi.interface"),
      //FIXME: NS_ERROR_UNEXPECTED: Component returned failure code: 0x8000ffff (NS_ERROR_UNEXPECTED) [nsIPrefBranch.getIntPref]
      numRil: 1, //Services.prefs.getIntPref("ril.numRadioInterfaces"),
      wapi_key_type: libcutils.property_get(
        "ro.wifi.wapi.wapi_key_type",
        "wapi_key_type"
      ),
      wapi_psk: libcutils.property_get("ro.wifi.wapi.wapi_psk", "wapi_psk"),
      as_cert_file: libcutils.property_get(
        "ro.wifi.wapi.as_cert_file",
        "as_cert_file"
      ),
      user_cert_file: libcutils.property_get(
        "ro.wifi.wapi.user_cert_file",
        "user_cert_file"
      ),
    };
  }

  let {
    sdkVersion,
    schedScanRecovery,
    p2pSupported,
    eapSimSupported,
    ibssSupported,
    ifname,
    numRil,
    wapi_key_type,
    wapi_psk,
    as_cert_file,
    user_cert_file,
  } = getStartupPrefs();

  manager.wapi_key_type = wapi_key_type;
  manager.wapi_psk = wapi_psk;
  manager.as_cert_file = as_cert_file;
  manager.user_cert_file = user_cert_file;

  let capabilities = {
    security: ["OPEN", "WEP", "WPA-PSK", "WPA-EAP", "WAPI-PSK", "WAPI-CERT"],
    eapMethod: ["PEAP", "TTLS", "TLS"],
    eapPhase2: ["PAP", "MSCHAP", "MSCHAPV2", "GTC"],
    certificate: ["SERVER", "USER"],
    mode: [WifiConfigUtils.MODE_ESS],
  };
  if (eapSimSupported) {
    capabilities.eapMethod.unshift("SIM");
    capabilities.eapMethod.unshift("AKA");
    capabilities.eapMethod.unshift("AKA'");
  }
  if (ibssSupported) {
    capabilities.mode.push(WifiConfigUtils.MODE_IBSS);
  }

  let wifiListener = {
    onEventCallback(event, iface) {
      if (handleEvent(event)) {
      }
    },

    onCommandResult(result, iface) {
      onmessageresult(result, iface);
    },
  };

  manager.ifname = ifname;
  // Emulator build runs to here.
  // The debug() should only be used after WifiManager.
  if (!ifname) {
    manager.ifname = DEFAULT_WLAN_INTERFACE;
  }
  manager.schedScanRecovery = schedScanRecovery;

  // Regular Wifi stuff.
  var netUtil = WifiNetUtil(controlMessage);
  var wifiCommand = WifiCommand(controlMessage, manager.ifname, sdkVersion);

  // Wifi P2P stuff
  var p2pManager;
  if (p2pSupported) {
    let p2pCommand = WifiCommand(
      controlMessage,
      WifiP2pManager.INTERFACE_NAME,
      sdkVersion
    );
    p2pManager = WifiP2pManager(p2pCommand, netUtil);
  }

  let wifiService = Cc["@mozilla.org/wifi/service;1"];
  if (wifiService) {
    wifiService = wifiService.getService(Ci.nsIWifiProxyService);
    let interfaces = [manager.ifname];
    if (p2pSupported) {
      interfaces.push(WifiP2pManager.INTERFACE_NAME);
    }
    wifiService.start(wifiListener, interfaces, interfaces.length);
  } else {
    debug("No wifi service component available!");
  }

  // Callbacks to invoke when a reply arrives from the wifi service.
  var controlCallbacks = Object.create(null);
  var idgen = 0;

  function controlMessage(obj, callback) {
    var id = idgen++;
    obj.id = id;
    if (callback) {
      controlCallbacks[id] = callback;
    }
    wifiService.sendCommand(obj, obj.iface);
  }

  let onmessageresult = function(data, iface) {
    var id = data.id;
    var callback = controlCallbacks[id];
    if (callback) {
      callback(data);
      delete controlCallbacks[id];
    }
  };

  // initialize wifi hal
  halInitialize(function() {});

  // Commands to the control worker.
  function halInitialize(callback) {
    wifiCommand.initialize(function(result) {
      callback(result.status == SUCCESS);
    });
  }

  var screenOn = true;
  function handleScreenStateChanged(enabled) {
    screenOn = enabled;
    if (screenOn) {
      fullBandConnectedTimeIntervalMilli = WIFI_ASSOCIATED_SCAN_INTERVAL;
      setSuspendOptimizationsMode(POWER_MODE_SCREEN_STATE, false, function() {});
      enableBackgroundScan(false);
      if (manager.state == "COMPLETED") {
        // Scan after 500ms
        setTimeout(handleScanRequest, 500);
      } else if (!manager.isConnectState(manager.state)) {
        // Scan after 500ms
        setTimeout(handleScanRequest, 500);
      }
    } else {
      setSuspendOptimizationsMode(POWER_MODE_SCREEN_STATE, true, function() {});
      if (manager.isConnectState(manager.state)) {
        enableBackgroundScan(false);
      } else {
        enableBackgroundScan(true);
      }
    }
  }

  // PNO (Preferred Network Offload):
  // device will search known networks if device disconnected and screen off.
  // It will only work in wlan FW without waking system up.
  // Once matched network found, it will wakeup system to start reconnect.
  var pnoEnabled = false;
  var schedulePnoFailed = false;
  function enableBackgroundScan(enable, callback) {
    pnoEnabled = enable;
    if (enable) {
      let pnoSettings = WifiPnoSettings.pnoSettings;
      pnoSettings.interval = WifiPnoSettings.interval;
      pnoSettings.min2gRssi = WifiPnoSettings.min2gRssi;
      pnoSettings.min5gRssi = WifiPnoSettings.min5gRssi;

      let network = {};
      let configuredNetworks = WifiConfigManager.configuredNetworks;
      for (let networkKey in configuredNetworks) {
        // TODO: memory size in firmware is limited,
        //       so we should optimize the pno list.
        let config = configuredNetworks[networkKey];
        network.ssid = config.ssid;
        network.isHidden = (config.scanSsid) ? true : false;

        // TODO: temporarily add all 2G channels in list,
        //       should confirm where to update the frequencies
        // network.frequencies = [config.frequency];
        network.frequencies =
          [ 2412, 2417, 2422, 2427, 2432, 2437,
            2442, 2447, 2452, 2457, 2462 ];

        WifiPnoSettings.pnoNetworks.push(network);
      }
      pnoSettings.pnoNetworks = WifiPnoSettings.pnoNetworks;

      if (pnoSettings.pnoNetworks.length == 0) {
        debug("Empty PNO network list");
        return callback(false);
      }

      wifiCommand.startPnoScan(pnoSettings, function(result) {
        callback(result.status == SUCCESS);
      });
    } else {
      wifiCommand.stopPnoScan(function(result) {
        callback(result.status == SUCCESS);
      });
    }
  }

  function updateChannels(callback) {
    wifiCommand.getChannelsForBand(WifiScanSettings.bandMask, function(result) {
      let channels = result.getChannels();
      if (channels.length > 0) {
        WifiScanSettings.channels = channels;
        return callback(true);
      }
      callback(false);
    });
  }

  function handleScanRequest(callback) {
    updateChannels(function(ok) {
      let scanSettings = WifiScanSettings.singleScanSettings;
      scanSettings.scanType = WifiScanSettings.SCAN_TYPE_HIGH_ACCURACY;
      scanSettings.channels =
        manager.currentConfigChannels.length > 0
          ? manager.currentConfigChannels
          : WifiScanSettings.channels;
      scanSettings.hiddenNetworks = WifiConfigManager.getHiddenNetworks();

      if (scanSettings.channels.length == 0) {
        callback(false);
      }
      manager.handlePreWifiScan();
      wifiCommand.startScan(scanSettings, function(result) {
        if (callback) {
          callback(result.status == SUCCESS);
        }
      });
    });
  }

  var delay = 15000;
  var delayScanId = null;
  var fullBandConnectedTimeIntervalMilli = WIFI_ASSOCIATED_SCAN_INTERVAL;
  var maxFullBandConnectedTimeIntervalMilli = 5 * 60 * 1000;
  var lastFullBandConnectedTimeMilli = -1;
  manager.currentConfigChannels = [];
  manager.startDelayScan = function() {
    debug(
      "startDelayScan: manager.state=" + manager.state + " screenOn=" + screenOn
    );

    if (schedulePnoFailed) {
      schedulePnoFailed = false;
      enableBackgroundScan(pnoEnabled);
    }

    if (screenOn) {
      if (manager.state == "COMPLETED") {
        delay = 20000;

        let tryFullBandScan = false;
        var now_ms = Date.now();
        debug(
          "start delay scan with age=" +
            (now_ms - lastFullBandConnectedTimeMilli) +
            " full scan interval=" +
            fullBandConnectedTimeIntervalMilli +
            " maxinterval=" +
            maxFullBandConnectedTimeIntervalMilli
        );
        if (
          now_ms - lastFullBandConnectedTimeMilli >
          fullBandConnectedTimeIntervalMilli
        ) {
          tryFullBandScan = true;
        }
        // TODO: 1. too much traffic, hence no full band scan.
        //       2. Don't scan if lots of packets are being sent.

        if (!tryFullBandScan && manager.currentConfigChannels) {
          handleScanRequest(function() {});
        } else {
          lastFullBandConnectedTimeMilli = now_ms;
          if (
            fullBandConnectedTimeIntervalMilli <
            maxFullBandConnectedTimeIntervalMilli
          ) {
            // Increase the interval
            fullBandConnectedTimeIntervalMilli =
              (fullBandConnectedTimeIntervalMilli * 12) / 8;
          }
          handleScanRequest(function() {});
        }
      } else if (!manager.isConnectState(manager.state)) {
        delay = 15000;
        handleScanRequest(function() {});
      }
    }

    delayScanId = setTimeout(manager.startDelayScan, delay);
  };

  var debugEnabled = false;
  function syncDebug() {
    if (debugEnabled !== DEBUG) {
      let wanted = DEBUG;
      let level = {
        logLevel: wanted ? LEVEL_DEBUG : LEVEL_INFO,
        showTimeStamp: true,
        showKeys: false,
      };
      wifiCommand.setDebugLevel(level, function(result) {
        if (result.status == SUCCESS) {
          debugEnabled = wanted;
        }
      });
      if (p2pSupported && p2pManager) {
        p2pManager.setDebug(DEBUG);
      }
      if (WifiNetworkSelector) {
        WifiNetworkSelector.setDebug(DEBUG);
      }
      if (WifiConfigManager) {
        WifiConfigManager.setDebug(DEBUG);
      }
      if (OpenNetworkNotifier) {
        OpenNetworkNotifier.setDebug(DEBUG);
      }
    }
  }

  function getDebugEnabled(callback) {
    wifiCommand.getDebugLevel(function(result) {
      if (result.status != SUCCESS) {
        debug("Unable to get wpa_supplicant's log level");
        callback(false);
        return;
      }

      let enabled = result.debugLevel <= LEVEL_DEBUG;
      callback(enabled);
    });
  }

  var httpProxyConfig = Object.create(null);

  /**
   * Given a network, configure http proxy when using wifi.
   * @param network A network object to update http proxy
   * @param info Info should have following field:
   *        - httpProxyHost ip address of http proxy.
   *        - httpProxyPort port of http proxy, set 0 to use default port 8080.
   * @param callback callback function.
   */
  function configureHttpProxy(network, info, callback) {
    if (!network) {
      return;
    }

    let networkKey = WifiConfigUtils.getNetworkKey(network);

    if (!info || info.httpProxyHost === "") {
      delete httpProxyConfig[networkKey];
    } else {
      httpProxyConfig[networkKey] = network;
      httpProxyConfig[networkKey].httpProxyHost = info.httpProxyHost;
      httpProxyConfig[networkKey].httpProxyPort = info.httpProxyPort;
    }

    callback(true);
  }

  function getHttpProxyNetwork(network) {
    if (!network) {
      return null;
    }

    let networkKey = WifiConfigUtils.getNetworkKey(network);
    return httpProxyConfig[networkKey];
  }

  function setHttpProxy(network) {
    if (!network) {
      return;
    }

    // If we got here, arg network must be the currentNetwork, so we just update
    // WifiNetworkInterface correspondingly and notify NetworkManager.
    WifiNetworkInterface.httpProxyHost = network.httpProxyHost;
    WifiNetworkInterface.httpProxyPort = network.httpProxyPort;

    if (
      WifiNetworkInterface.info.state ==
      Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED
    ) {
      gNetworkManager.updateNetworkInterface(WifiNetworkInterface);
    }
  }

  var staticIpConfig = Object.create(null);
  function setStaticIpMode(network, info, callback) {
    let setNetworkKey = WifiConfigUtils.getNetworkKey(network);
    let curNetworkKey = null;
    let currentNetwork = Object.create(null);
    currentNetwork.netId = wifiInfo.networkId;

    WifiConfigManager.getNetworkConfiguration(currentNetwork, function() {
      curNetworkKey = WifiConfigUtils.getNetworkKey(currentNetwork);

      // Add additional information to static ip configuration
      // It is used to compatiable with information dhcp callback.
      info.ipaddr = netHelpers.stringToIP(info.ipaddr_str);
      info.gateway = netHelpers.stringToIP(info.gateway_str);
      info.mask_str = netHelpers.ipToString(
        netHelpers.makeMask(info.maskLength)
      );

      // Optional
      info.dns1 = netHelpers.stringToIP(info.dns1_str);
      info.dns2 = netHelpers.stringToIP(info.dns2_str);
      info.proxy = netHelpers.stringToIP(info.proxy_str);

      staticIpConfig[setNetworkKey] = info;

      // If the ssid of current connection is the same as configured ssid
      // It means we need update current connection to use static IP address.
      if (setNetworkKey == curNetworkKey) {
        // Use configureInterface directly doesn't work, the network interface
        // and routing table is changed but still cannot connect to network
        // so the workaround here is disable interface the enable again to
        // trigger network reconnect with static ip.
        gNetworkService.disableInterface(manager.ifname, function(ok) {
          gNetworkService.enableInterface(manager.ifname, function(ok) {
            callback(ok);
          });
        });
        return;
      }

      callback(true);
    });
  }

  var dhcpInfo = null;

  function runStaticIp(ifname, key) {
    debug("Run static ip");

    // Read static ip information from settings.
    let staticIpInfo;

    if (!(key in staticIpConfig)) {
      return;
    }

    staticIpInfo = staticIpConfig[key];

    // Stop dhcpd when use static IP
    if (dhcpInfo != null) {
#ifdef HAS_KOOST_MODULES
      netUtil.stopDhcp(manager.ifname, function(success) {});
#else
      netUtil.stopDhcp(manager.ifname, function() {});
#endif
    }

    // Set ip, mask length, gateway, dns to network interface
    gNetworkService.configureInterface(
      {
        ifname,
        ipaddr: staticIpInfo.ipaddr,
        mask: staticIpInfo.maskLength,
        gateway: staticIpInfo.gateway,
        dns1: staticIpInfo.dns1,
        dns2: staticIpInfo.dns2,
      },
      function(data) {
        netUtil.runIpConfig(ifname, staticIpInfo, function(data) {
          dhcpInfo = data.info;
          wifiInfo.setInetAddress(dhcpInfo.ipaddr_str);
          notify("networkconnected", data);
        });
      }
    );
  }

  var suppressEvents = false;
  function notify(eventName, eventObject) {
    if (suppressEvents) {
      return;
    }
    var handler = manager["on" + eventName];
    if (handler) {
      if (!eventObject) {
        eventObject = {};
      }
      handler.call(eventObject);
    }
  }

  function notifyStateChange(fields) {
    // Enable background scan when device disconnected and screen off.
    // Disable background scan when device connecting and screen off.
    if (!screenOn) {
      if (
        manager.isConnectState(manager.state) &&
        fields.state === "DISCONNECTED"
      ) {
        enableBackgroundScan(true);
      } else if (manager.isConnectState(fields.state)) {
        enableBackgroundScan(false);
      }
    }

    fields.prevState = manager.state;
    // Detect wpa_supplicant's loop iterations.
    manager.supplicantLoopDetection(fields.prevState, fields.state);
    notify("statechange", fields);

    // Don't update state when and after disabling.
    if (manager.state === "DISABLING" || manager.state === "UNINITIALIZED") {
      return;
    }

    manager.state = fields.state;
  }

  let dhcpRequestGen = 0;
  function onconnected() {
    // For now we do our own DHCP. In the future, this should be handed
    // off to the Network Manager.
    let currentNetwork = Object.create(null);
    currentNetwork.netId = wifiInfo.networkId;
    // Clear the bssid in the current config's network block
    manager.clearCurrentConfigBSSID(currentNetwork.netId, function() {});

    WifiConfigManager.getNetworkConfiguration(currentNetwork, function() {
      let key = WifiConfigUtils.getNetworkKey(currentNetwork);
      if (
        staticIpConfig &&
        key in staticIpConfig &&
        staticIpConfig[key].enabled
      ) {
        debug("Run static ip");
        runStaticIp(manager.ifname, key);
        return;
      }

      preDhcpSetup();
      netUtil.runDhcp(manager.ifname, dhcpRequestGen++, function(data, gen) {
        dhcpInfo = data.info;
        debug("dhcpRequestGen: " + dhcpRequestGen + ", gen: " + gen);
        // Only handle dhcp response in COMPLETED state.
        if (manager.state !== "COMPLETED") {
          postDhcpSetup(function(ok) {});
          return;
        }

        if (!dhcpInfo) {
          wifiInfo.setInetAddress(null);
          if (gen + 1 < dhcpRequestGen) {
            debug("Do not bother younger DHCP request.");
            return;
          }
          WifiConfigManager.updateNetworkSelectionStatus(
            lastNetwork.netId,
            WifiConfigManager.DISABLED_DHCP_FAILURE,
            function(doDisable) {
              if (doDisable) {
                notify("networkdisable", {
                  reason: WifiConfigManager.DISABLED_DHCP_FAILURE,
                });
              } else {
                WifiManager.disconnect(function() {
                  WifiManager.reassociate(function() {});
                });
              }
            }
          );
        } else {
          wifiInfo.setInetAddress(dhcpInfo.ipaddr_str);
          notify("networkconnected", data);
        }
      });
    });
  }

  function preDhcpSetup() {
    manager.inObtainingIpState = true;
    // TODO: gPowerManagerService is not defined
    // Hold wakelock during doing DHCP
    // acquireWifiWakeLock();

    // Disable power saving mode when doing dhcp
    setSuspendOptimizationsMode(POWER_MODE_DHCP, false, function() {});
    manager.setPowerSave(false, function() {});
  }

  function postDhcpSetup(callback) {
    if (!manager.inObtainingIpState) {
      callback(true);
      return;
    }
    setSuspendOptimizationsMode(POWER_MODE_DHCP, true, function(ok) {
      manager.setPowerSave(true, function(result) {
        // TODO: gPowerManagerService is not defined
        // Release wakelock during doing DHCP
        // releaseWifiWakeLock();
        manager.inObtainingIpState = false;
        callback(result.status == SUCCESS);
      });
    });
  }

  var wifiWakeLock = null;
  var wifiWakeLockTimer = null;
  function acquireWifiWakeLock() {
    if (!wifiWakeLock) {
      debug("Acquiring Wifi Wakelock");
      wifiWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!wifiWakeLockTimer) {
      debug("Creating Wifi WakeLock Timer");
      wifiWakeLockTimer = Cc["@mozilla.org/timer;1"].createInstance(
        Ci.nsITimer
      );
    }
    function onTimeout() {
      releaseWifiWakeLock();
    }
    debug("Setting Wifi WakeLock Timer");
    wifiWakeLockTimer.initWithCallback(onTimeout, 60000, Ci.nsITimer.ONE_SHOT);
  }

  function releaseWifiWakeLock() {
    debug("Releasing Wifi WakeLock");
    if (wifiWakeLockTimer) {
      wifiWakeLockTimer.cancel();
    }
    if (wifiWakeLock) {
      wifiWakeLock.unlock();
      wifiWakeLock = null;
    }
  }

  var supplicantStatesMap =
    [
      "DISCONNECTED",
      "INTERFACE_DISABLED",
      "INACTIVE",
      "SCANNING",
      "AUTHENTICATING",
      "ASSOCIATING",
      "ASSOCIATED",
      "FOUR_WAY_HANDSHAKE",
      "GROUP_HANDSHAKE",
      "COMPLETED",
      "UNINITIALIZED",
    ];

  var driverEventMap = {
    STOPPED: "driverstopped",
    STARTED: "driverstarted",
    HANGED: "driverhung",
  };

  var linkDebouncingId = null;
  var isDriverRoaming = false;
  // Handle events sent to us by the event worker.
  function handleEvent(event) {
    let eventData = event.name;
    debug("Event coming in: " + eventData);
    //    if (eventData.indexOf("CTRL-EVENT-") !== 0 && eventData.indexOf("WPS") !== 0) {
    //      if (eventData.indexOf("CTRL-REQ-") == 0) {
    //        const REQUEST_PREFIX_STR = "CTRL-REQ-";
    //        let requestName = eventData.substring(REQUEST_PREFIX_STR.length);
    //        if (!requestName) return true;
    //
    //        if (requestName.startsWith("PASSWORD")) {
    //          WifiConfigManager.updateNetworkSelectionStatus(lastNetwork.netId,
    //            WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
    //            function(doDisable) {
    //              if (doDisable) {
    //                notify("networkdisable",
    //                  {reason: WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE});
    //              }
    //            }
    //          );
    //          return true;
    //        } else if (requestName.startsWith("IDENTITY")) {
    //          let networkId = -2;
    //          let match =
    //            /IDENTITY-([0-9]+):Identity needed for SSID (.+)/.exec(requestName);
    //          if (match) {
    //            try {
    //              networkId = parseInt(match[1]);
    //            } catch (e) {
    //              networkId = -1;
    //            }
    //          } else {
    //            debug("didn't find SSID " + requestName);
    //          }
    //          wifiCommand.getNetworkVariable(networkId, "eap", function(eap) {
    //            let eapMethod;
    //            if (eap == "SIM") {
    //              eapMethod = 1;
    //            } else if (eap == "AKA") {
    //              eapMethod = 0;
    //            } else if (eap == "AKA'") {
    //              eapMethod = 6;
    //            } else {
    //              debug("EAP type not sim, aka or aka'");
    //              return;
    //            }
    //            simIdentityRequest(networkId, eapMethod);
    //          });
    //          return true;
    //        } else if (requestName.startsWith("SIM")) {
    //          simAuthRequest(requestName);
    //          return true;
    //        }
    //        debug("couldn't identify request type - " + eventData);
    //        return true;
    //      }
    //
    //      // Wpa_supplicant won't send WRONG_KEY or AUTH_FAIL event before sdk 20,
    //      // hence we check pre-share key incorrect msg here.
    //      if (sdkVersion < 20 &&
    //          eventData.indexOf("pre-shared key may be incorrect") !== -1 ) {
    //        WifiConfigManager.updateNetworkSelectionStatusForAuthFail(lastNetwork.netId,
    //          WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
    //          function(doDisable) {
    //            if (doDisable) {
    //              wifiInfo.reset();
    //              notify("networkdisable",
    //                {reason: WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE});
    //            }
    //          }
    //        );
    //        return true;
    //      }
    //
    //      if (eventData.indexOf("Failed to schedule PNO") === 0) {
    //        schedulePnoFailed = true;
    //        return true;
    //      }
    //
    //      // This is ugly, but we need to grab the SSID here. BSSID is not guaranteed
    //      // to be provided, so don't grab BSSID here.
    //      var match = /Trying to associate with.*SSID[ =]'(.*)'/.exec(eventData);
    //      if (match) {
    //        debug("Matched: " + match[1] + "\n");
    //        wifiInfo.wifiSsid = match[1];
    //      }
    //      match = /Associated with ((?:[0-9a-f]{2}:){5}[0-9a-f]{2}).*/.exec(eventData);
    //      if (match) {
    //        wifiInfo.setBSSID(match[1]);
    //        if (isDriverRoaming) {
    //          manager.lastDriverRoamAttempt = Date.now();
    //        }
    //      }
    //      return true;
    //    }

    if (eventData.indexOf("SUPPLICANT_STATE_CHANGED") === 0) {
      // Parse the event data.
      var fields = {};
      fields.state = supplicantStatesMap[event.stateChanged.state];
      fields.id = event.stateChanged.id;
      fields.ssid = event.stateChanged.ssid;
      fields.bssid = event.stateChanged.bssid;
      wifiInfo.setSupplicantState(fields.state);

      if (manager.isConnectState(fields.state)) {
        if (
          wifiInfo.bssid != fields.BSSID &&
          fields.BSSID !== "00:00:00:00:00:00"
        ) {
          wifiInfo.setBSSID(fields.BSSID);
        }

        if (manager.connectingNetwork[manager.ifname] &&
            manager.connectingNetwork[manager.ifname].netId != INVALID_NETWORK_ID) {
          wifiInfo.setNetworkId(manager.connectingNetwork[manager.ifname].netId);
          wifiInfo.setSecurity(manager.connectingNetwork[manager.ifname].keyMgmt);
        }
      } else if (manager.isConnectState(manager.state)) {
        manager.lastDriverRoamAttempt = 0;
        wifiInfo.reset();
      }

      if (manager.wpsStarted && manager.state !== "COMPLETED") {
        return true;
      }

      // Check device in roaming state or not.
      if (manager.state === "COMPLETED" && fields.state === "ASSOCIATED") {
        debug("Driver Roaming starts.");
        isDriverRoaming = true;
      } else if (!manager.isConnectState(fields.state)) {
        debug("Driver Roaming resets flag.");
        isDriverRoaming = false;
      }

      fields.isDriverRoaming = isDriverRoaming;

      if (manager.linkDebouncing) {
        debug(
          "State Change from " +
            manager.state +
            " to " +
            fields.state +
            ", linkDebouncing."
        );
        if (fields.state !== "COMPLETED") {
          return true;
        }

        if (linkDebouncingId !== null) {
          clearTimeout(linkDebouncingId);
          linkDebouncingId = null;
        }
        manager.linkDebouncing = false;
      }

      notifyStateChange(fields);
      if (fields.state === "COMPLETED") {
        manager.lastDriverRoamAttempt = 0;
        onconnected();
        if (isDriverRoaming) {
          isDriverRoaming = false;
        }
      }
    } else if (eventData.indexOf("CTRL-EVENT-DRIVER-STATE") === 0) {
      var handlerName = driverEventMap[eventData];
      if (handlerName) {
        notify(handlerName);
      }
    } else if (eventData.indexOf("SUPPLICANT_TERMINATING") === 0) {
      wifiInfo.reset();
      notifyStateChange({ state: "DISCONNECTED", BSSID: null, id: -1 });

      // If the supplicant is terminated as commanded, the supplicant lost
      // notification will be sent after driver unloaded. In such case, the
      // manager state will be "DISABLING" or "UNINITIALIZED".
      // So if supplicant terminated with incorrect manager state, implying
      // unexpected condition, we should notify supplicant lost here.
      if (manager.state !== "DISABLING" && manager.state !== "UNINITIALIZED") {
        notify("supplicantlost", { success: true });
      }
    } else if (eventData.indexOf("CTRL-EVENT-SSID-TEMP-DISABLED") === 0) {
      var id = event.split(" ")[1].split("=")[1];
      WifiConfigManager.updateNetworkSelectionStatus(
        id,
        WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
        function(doDisable) {
          if (doDisable) {
            notify("networkdisable", {
              reason: WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
            });
          }
        }
      );
    } else if (eventData.indexOf("SUPPLICANT_NETWORK_DISCONNECTED") === 0) {
      var bssid = event.bssid;
      var reason = event.reason;
      var lastRoam = 0;
      fullBandConnectedTimeIntervalMilli = WIFI_ASSOCIATED_SCAN_INTERVAL;
      if (manager.lastDriverRoamAttempt != 0) {
        lastRoam = Date.now() - manager.lastDriverRoamAttempt;
        manager.lastDriverRoamAttempt = 0;
      }

      debug("CTRL-EVENT-DISCONNECTED wifiInfo: " + JSON.stringify(wifiInfo));

      if (manager.isWifiEnabled(manager.state)) {
        // Clear the bssid in the current config's network block
        manager.clearCurrentConfigBSSID(lastNetwork.netId, function() {});
        let networkKey = escape(wifiInfo.wifiSsid) + wifiInfo.security;
        var configuredNetworks = WifiConfigManager.configuredNetworks;
        if (typeof configuredNetworks[networkKey] !== "undefined") {
          let networkEnabled = !configuredNetworks[networkKey]
            .networkSelectionStatus;
          if (
            screenOn &&
            !manager.linkDebouncing &&
            networkEnabled &&
            !WifiConfigManager.isLastSelectedConfiguration(lastNetwork.netId) &&
            (reason != 3 || (lastRoam > 0 && lastRoam < 2000)) &&
            ((wifiInfo.is24G &&
              wifiInfo.rssi > WifiNetworkSelector.RSSI_THRESHOLD_LOW_24G) ||
              (wifiInfo.is5G &&
                wifiInfo.rssi > WifiNetworkSelector.RSSI_THRESHOLD_LOW_5G))
          ) {
            handleScanRequest(function() {});
            manager.linkDebouncing = true;
            linkDebouncingId = setTimeout(function() {
              isDriverRoaming = false;
              manager.linkDebouncing = false;
              notifyStateChange({ state: "DISCONNECTED", BSSID: null, id: -1 });
            }, 5000);
            debug(
              "Receive CTRL-EVENT-DISCONNECTED:" +
                " BSSID=" +
                wifiInfo.bssid +
                " RSSI=" +
                wifiInfo.rssi +
                " freq=" +
                wifiInfo.frequency +
                " reason=" +
                reason +
                " -> debounce"
            );
          } else {
            handleScanRequest(function() {});
            debug(
              "Receive CTRL-EVENT-DISCONNECTED:" +
                " BSSID=" +
                wifiInfo.bssid +
                " RSSI=" +
                wifiInfo.rssi +
                " freq=" +
                wifiInfo.frequency +
                " was debouncing=" +
                manager.linkDebouncing +
                " reason=" +
                reason +
                " Network Enabled Status=" +
                networkEnabled
            );
          }
        } else {
          debug(networkKey + " is not defined in conifgured networks");
        }
      }
      manager.currentConfigChannels = [];
      wifiInfo.reset();
      // Restore power save and suspend optimizations when dhcp failed.
      postDhcpSetup(function(ok) {});
    } else if (eventData.indexOf("CTRL-EVENT-CONNECTED") === 0) {
      // Format: CTRL-EVENT-CONNECTED - Connection to 00:1e:58:ec:d5:6d completed (reauth) [id=1 id_str=]
      var bssid = event.split(" ")[4];

      var keyword = "id=";
      var id = event
        .substr(event.indexOf(keyword) + keyword.length)
        .split(" ")[0];
      // Read current BSSID here, it will always being provided.
      wifiInfo.setBSSID(bssid);
      wifiInfo.setNetworkId(id);
      WifiNetworkSelector.trackBssid(bssid, true);
      if (manager.wpsStarted) {
        manager.wpsStarted = false;
      }
    } else if (eventData.indexOf("SCAN_RESULT_READY") === 0) {
      debug("Notifying of scan results available");
      manager.handlePostWifiScan();
      if (!screenOn && manager.state === "SCANNING") {
        enableBackgroundScan(true);
      }
      if (manager.linkDebouncing) {
        manager.setAutoRoam(lastNetwork, function() {});
      }
      notify("scanresultsavailable", { type: USE_SINGLE_SCAN });
    } else if (eventData.indexOf("SCAN_RESULT_FAILED") === 0) {
      debug("Receive single scan failure");
    } else if (eventData.indexOf("PNO_SCAN_FOUND") === 0) {
      notify("scanresultsavailable", { type: USE_PNO_SCAN });
    } else if (eventData.indexOf("PNO_SCAN_FAILED") === 0) {
      debug("Receive PNO scan failure");
    } else if (eventData.indexOf("CTRL-EVENT-EAP") === 0) {
      if (event.indexOf("CTRL-EVENT-EAP-STARTED") !== -1) {
          notifyStateChange({ state: "AUTHENTICATING" });
      }
    } else if (
      eventData.indexOf("CTRL-EVENT-ASSOC-REJECT") === 0 &&
      !manager.wpsStarted
    ) {
      var token = event.split(" ")[1];
      var bssid = token.split("=")[1];
      if (bssid !== "00:00:00:00:00:00") {
        // If we have a BSSID, tell configStore to black list it
        WifiNetworkSelector.trackBssid(bssid, false);
        bssidToNetwork(bssid, function(network) {
          manager.getSecurity(network.netId, function(security) {
            if (security == "WEP") {
              WifiConfigManager.updateNetworkSelectionStatusForAuthFail(
                network.netId,
                WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
                function(doDisable) {
                  if (doDisable) {
                    notify("networkdisable", {
                      reason: WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
                    });
                  }
                }
              );
            } else {
              WifiConfigManager.updateNetworkSelectionStatus(
                network.netId,
                WifiConfigManager.DISABLED_ASSOCIATION_REJECTION,
                function(doDisable) {
                  if (doDisable) {
                    notify("networkdisable", {
                      reason: WifiConfigManager.DISABLED_ASSOCIATION_REJECTION,
                    });
                  }
                }
              );
            }
          });
        });
      }
    } else if (eventData.indexOf("HOTSPOT_CLIENT_CHANGED") === 0) {
      notify("stationinfoupdate", { station: event.numStations });
    } else if (eventData.indexOf("WPS-TIMEOUT") === 0) {
      manager.wpsStarted = false;
      notifyStateChange({ state: "WPS_TIMEOUT", BSSID: null, id: -1 });
    } else if (eventData.indexOf("WPS-FAIL") === 0) {
      manager.wpsStarted = false;
      notifyStateChange({ state: "WPS_FAIL", BSSID: null, id: -1 });
    } else if (eventData.indexOf("WPS-OVERLAP-DETECTED") === 0) {
      manager.wpsStarted = false;
      notifyStateChange({ state: "WPS_OVERLAP_DETECTED", BSSID: null, id: -1 });
    }
    // Unknown event.
    return true;
  }

  function bssidToNetwork(bssid, callback) {
    let bssidReject = bssid;
    manager.getScanResults(function(r) {
      if (!r) {
        return callback(false);
      }
      manager.getConfiguredNetworks(function(networks) {
        if (!networks) {
          debug("Unable to get configured networks");
          return callback(false);
        }
        let lines = r.split("\n");
        for (let i = 1; i < lines.length; ++i) {
          // bssid / frequency / signal level / flags / ssid
          var match = /([\S]+)\s+([\S]+)\s+([\S]+)\s+(\[[\S]+\])?\s(.*)/.exec(
            lines[i]
          );
          if (match && match[5]) {
            let ssid = match[5],
              bssid = match[1],
              frequency = match[2],
              signalLevel = match[3],
              flags = match[4];
            if (bssid === bssidReject) {
              let network = new ScanResult(
                ssid,
                bssid,
                frequency,
                flags,
                signalLevel
              );
              let networkKeyReject = WifiConfigUtils.getNetworkKey(network);
              for (let net in networks) {
                let networkKey = WifiConfigUtils.getNetworkKey(networks[net]);
                if (networkKey === networkKeyReject) {
                  return callback(networks[net]);
                }
              }
              return callback(false);
            }
          }
        }
        return callback(false);
      });
    });
  }

  var requestOptimizationMode = 0;
  function setSuspendOptimizationsMode(reason, enable, callback) {
    debug(
      "setSuspendOptimizationsMode reason = " +
        reason +
        ", enable = " +
        enable +
        ", requestOptimizationMode = " +
        requestOptimizationMode
    );
    if (enable) {
      requestOptimizationMode &= ~reason;
      if (!requestOptimizationMode) {
        wifiCommand.setSuspendMode(enable, function (result) {
          callback(result.status == SUCCESS);
        });
      } else {
        callback(true);
      }
    } else {
      requestOptimizationMode |= reason;
      wifiCommand.setSuspendMode(enable, function (result) {
          callback(result.status == SUCCESS);
      });
    }
  }

  function prepareForStartup(callback) {
    let status = libcutils.property_get(DHCP_PROP + "_" + manager.ifname);
    if (status !== "running") {
      callback();
      return;
    }
    manager.connectionDropped(function() {
      callback();
    });
  }

  // Initial state.
  manager.state = "UNINITIALIZED";
  manager.tetheringState = "UNINITIALIZED";
  manager.supplicantStarted = false;
  manager.wpsStarted = false;
  manager.lastKnownCountryCode = null;
  manager.telephonyServiceId = 0;
  manager.inObtainingIpState = false;
  manager.linkDebouncing = false;
  manager.lastDriverRoamAttempt = 0;
  manager.loopDetectionCount = 0;
  manager.numRil = numRil;
  manager.connectingNetwork = Object.create(null);

  manager.__defineGetter__("enabled", function() {
    switch (manager.state) {
      case "UNINITIALIZED":
      case "INITIALIZING":
      case "DISABLING":
        return false;
      default:
        return true;
    }
  });

  // EAP-SIM: convert string from hex to base64.
  function gsmHexToBase64(hex) {
    let octects = String.fromCharCode(hex.length / 2);
    for (let i = 0; i < hex.length; i += 2) {
      octects += String.fromCharCode(parseInt(hex.substr(i, 2), 16));
    }
    return btoa(octects);
  }

  // EAP-AKA/AKA': convert string from hex to base64.
  function umtsHexToBase64(rand, authn) {
    let octects = String.fromCharCode(rand.length / 2);
    for (let i = 0; i < rand.length; i += 2) {
      octects += String.fromCharCode(parseInt(rand.substr(i, 2), 16));
    }
    octects += String.fromCharCode(authn.length / 2);
    for (let i = 0; i < authn.length; i += 2) {
      octects += String.fromCharCode(parseInt(authn.substr(i, 2), 16));
    }
    return btoa(octects);
  }

  // EAP-SIM/AKA/AKA': convert string from base64 to byte.
  function base64Tobytes(str) {
    let octects = atob(str.replace(/-/g, "+").replace(/_/g, "/"));
    return octects;
  }

  // EAP-SIM/AKA/AKA': convert string from byte to hex.
  function bytesToHex(str, from, len) {
    let hexs = "";
    for (let i = 0; i < len; i++) {
      hexs += ("0" + str.charCodeAt(from + i).toString(16)).substr(-2);
    }
    return hexs;
  }

  function simIdentityRequest(networkId, eapMethod) {
    wifiCommand.getNetworkVariable(networkId, "sim_num", function(sim_num) {
      sim_num = sim_num || 1;
      // For SIM & AKA/AKA' EAP method Only, get identity from ICC
      let icc = gIccService.getIccByServiceId(sim_num - 1);
      if (!icc || !icc.iccInfo || !icc.iccInfo.mcc || !icc.iccInfo.mnc) {
        debug("SIM is not ready or iccInfo is invalid");
        WifiConfigManager.updateNetworkSelectionStatus(
          networkId,
          WifiConfigManager.DISABLED_AUTHENTICATION_NO_CREDENTIALS,
          function(doDisable) {
            if (doDisable) {
              manager.disableNetwork(networkId, function() {});
            }
          }
        );
        return;
      }

      // imsi, mcc, mnc
      let imsi = icc.imsi;
      let mcc = icc.iccInfo.mcc;
      let mnc = icc.iccInfo.mnc;
      if (mnc.length === 2) {
        mnc = "0" + mnc;
      }

      let identity =
        eapMethod +
        imsi +
        "@wlan.mnc" +
        mnc +
        ".mcc" +
        mcc +
        ".3gppnetwork.org";
      debug("identity = " + identity);
      wifiCommand.simIdentityResponse(networkId, identity, function() {});
    });
  }

  function simAuthRequest(requestName) {
    let matchGsm = /SIM-([0-9]*):GSM-AUTH((:[0-9a-f]+)+) needed for SSID (.+)/.exec(
      requestName
    );
    let matchUmts = /SIM-([0-9]*):UMTS-AUTH:([0-9a-f]+):([0-9a-f]+) needed for SSID (.+)/.exec(
      requestName
    );
    // EAP-SIM
    if (matchGsm) {
      let networkId = parseInt(matchGsm[1]);
      let data = matchGsm[2].split(":");
      let authResponse = "";
      let count = 0;

      wifiCommand.getNetworkVariable(networkId, "sim_num", function(sim_num) {
        sim_num = sim_num || 1;
        let icc = gIccService.getIccByServiceId(sim_num - 1);
        for (let value in data) {
          let challenge = data[value];
          if (!challenge.length) {
            continue;
          }
          let base64Challenge = gsmHexToBase64(challenge);
          debug("base64Challenge = " + base64Challenge);

          // Try USIM first for authentication.
          icc.getIccAuthentication(
            Ci.nsIIcc.APPTYPE_USIM,
            Ci.nsIIcc.AUTHTYPE_EAP_SIM,
            base64Challenge,
            {
              notifyAuthResponse(iccResponse) {
                debug("Receive USIM iccResponse: " + iccResponse);
                iccResponseReady(iccResponse);
              },
              notifyError(aErrorMsg) {
                debug("Receive USIM iccResponse error: " + aErrorMsg);
                // In case of failure, retry as a simple SIM.
                icc.getIccAuthentication(
                  Ci.nsIIcc.APPTYPE_SIM,
                  Ci.nsIIcc.AUTHTYPE_EAP_SIM,
                  base64Challenge,
                  {
                    notifyAuthResponse(iccResponse) {
                      debug("Receive SIM iccResponse: " + iccResponse);
                      iccResponseReady(iccResponse);
                    },
                    notifyError(aErrorMsg) {
                      debug("Receive SIM iccResponse error: " + aErrorMsg);
                      wifiCommand.simAuthFailedResponse(
                        networkId,
                        function() {}
                      );
                    },
                  }
                );
              },
            }
          );
        }
      });

      function iccResponseReady(iccResponse) {
        if (!iccResponse || iccResponse.length <= 4) {
          debug("bad response - " + iccResponse);
          wifiCommand.simAuthFailedResponse(networkId, function() {});
          return;
        }
        let result = base64Tobytes(iccResponse);
        let sres_len = result.charCodeAt(0);
        let sres = bytesToHex(result, 1, sres_len);
        let kc_offset = 1 + sres_len;
        let kc_len = result.charCodeAt(kc_offset);
        if (
          sres_len >= result.length ||
          kc_offset >= result.length ||
          kc_offset + kc_len > result.length
        ) {
          debug("malfomed response - " + iccResponse);
          wifiCommand.simAuthFailedResponse(networkId, function() {});
          return;
        }
        let kc = bytesToHex(result, 1 + kc_offset, kc_len);
        debug("kc:" + kc + ", sres:" + sres);
        authResponse = authResponse + ":" + kc + ":" + sres;
        count++;
        if (count == 3) {
          debug("Supplicant Response -" + authResponse);
          wifiCommand.simAuthResponse(
            networkId,
            "GSM-AUTH",
            authResponse,
            function() {}
          );
        }
      }

      // EAP-AKA/AKA'
    } else if (matchUmts) {
      let networkId = parseInt(matchUmts[1]);
      let rand = matchUmts[2];
      let authn = matchUmts[3];

      wifiCommand.getNetworkVariable(networkId, "sim_num", function(sim_num) {
        let icc = gIccService.getIccByServiceId(sim_num - 1);
        if (rand == null || authn == null) {
          debug("null rand or authn");
          return;
        }
        let base64Challenge = umtsHexToBase64(rand, authn);
        debug("base64Challenge = " + base64Challenge);

        icc.getIccAuthentication(
          Ci.nsIIcc.APPTYPE_USIM,
          Ci.nsIIcc.AUTHTYPE_EAP_AKA,
          base64Challenge,
          {
            notifyAuthResponse(iccResponse) {
              debug("Receive iccResponse: " + iccResponse);
              iccResponseReady(iccResponse);
            },
            notifyError(aErrorMsg) {
              debug("Receive iccResponse error: " + aErrorMsg);
            },
          }
        );
      });

      function iccResponseReady(iccResponse) {
        if (!iccResponse || iccResponse.length <= 4) {
          debug("bad response - " + iccResponse);
          wifiCommand.simAuthFailedResponse(networkId, function() {});
          return;
        }
        let result = base64Tobytes(iccResponse);
        let tag = result.charCodeAt(0);
        if (tag == 0xdb) {
          debug("successful 3G authentication ");
          let res_len = result.charCodeAt(1);
          let res = bytesToHex(result, 2, res_len);
          let ck_len = result.charCodeAt(res_len + 2);
          let ck = bytesToHex(result, res_len + 3, ck_len);
          let ik_len = result.charCodeAt(res_len + ck_len + 3);
          let ik = bytesToHex(result, res_len + ck_len + 4, ik_len);
          debug("ik:" + ik + " ck:" + ck + " res:" + res);
          let authResponse = ":" + ik + ":" + ck + ":" + res;
          wifiCommand.simAuthResponse(
            networkId,
            "UMTS-AUTH",
            authResponse,
            function() {}
          );
        } else if (tag == 0xdc) {
          debug("synchronisation failure");
          let auts_len = result.charCodeAt(1);
          let auts = bytesToHex(result, 2, auts_len);
          debug("auts:" + auts);
          let authResponse = ":" + auts;
          wifiCommand.simAuthResponse(
            networkId,
            "UMTS-AUTS",
            authResponse,
            function() {}
          );
        } else {
          debug("bad authResponse - unknown tag = " + tag);
          wifiCommand.umtsAuthFailedResponse(networkId, function() {});
        }
      }
    } else {
      debug("couldn't parse SIM auth request - " + requestName);
    }
  }

  var waitForTerminateEventTimer = null;
  function cancelWaitForTerminateEventTimer() {
    if (waitForTerminateEventTimer) {
      waitForTerminateEventTimer.cancel();
      waitForTerminateEventTimer = null;
    }
  }

  manager.getMacAddress = wifiCommand.getMacAddress;
  manager.getScanResults = wifiCommand.getScanResults;
  manager.handleScanRequest = handleScanRequest;
  manager.wpsPbc = wifiCommand.wpsPbc;
  manager.wpsPin = wifiCommand.wpsPin;
  manager.wpsCancel = wifiCommand.wpsCancel;
  manager.setPowerSave = wifiCommand.setPowerSave;
  manager.setExternalSim = wifiCommand.setExternalSim;
  manager.getHttpProxyNetwork = getHttpProxyNetwork;
  manager.setHttpProxy = setHttpProxy;
  manager.configureHttpProxy = configureHttpProxy;
  manager.setSuspendOptimizationsMode = setSuspendOptimizationsMode;
  manager.setStaticIpMode = setStaticIpMode;
  manager.getRssiApprox = wifiCommand.getRssiApprox;
  manager.getLinkSpeed = wifiCommand.getLinkSpeed;
  manager.getDhcpInfo = function() {
    return dhcpInfo;
  };
  manager.getConnectionInfo = wifiCommand.getConnectionInfo;
  manager.setScanInterval = wifiCommand.setScanInterval;
  manager.enableAutoReconnect = wifiCommand.enableAutoReconnect;
  manager.autoScanMode = wifiCommand.autoScanMode;
  manager.handleScreenStateChanged = handleScreenStateChanged;
  manager.setCountryCode = wifiCommand.setCountryCode;
  manager.postDhcpSetup = postDhcpSetup;
  manager.setNetworkVariable = wifiCommand.setNetworkVariable;
  manager.setDebugLevel = wifiCommand.setDebugLevel;
  manager.setAutoReconnect = wifiCommand.setAutoReconnect;
  manager.getStaCapabilities = wifiCommand.getStaCapabilities;
  manager.disconnect = wifiCommand.disconnect;
  manager.reconnect = wifiCommand.reconnect;
  manager.reassociate = wifiCommand.reassociate;
  manager.syncDebug = syncDebug;

  // Public interface of the wifi service.
  manager.setWifiEnabled = function(enabled, callback) {
    if (enabled === manager.isWifiEnabled(manager.state)) {
      callback(true);
      return;
    }

    if (enabled) {
      manager.state = "INITIALIZING";
      prepareForStartup(function() {
        wifiCommand.startWifi(function(result) {
          if (result.status != SUCCESS) {
            callback(false);
            manager.state = "UNINITIALIZED";
            return;
          }
          // wlan module is loaded, update interface name here
          if (result.staInterface.length > 0) {
            manager.ifname = result.staInterface;
          }

          // Register as network interface.
          WifiNetworkInterface.info.name = manager.ifname;
          if (!WifiNetworkInterface.registered) {
            gNetworkManager.registerNetworkInterface(WifiNetworkInterface);
            WifiNetworkInterface.registered = true;
          }
          WifiNetworkInterface.info.state =
            Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED;
          WifiNetworkInterface.info.ips = [];
          WifiNetworkInterface.info.prefixLengths = [];
          WifiNetworkInterface.info.gateways = [];
          WifiNetworkInterface.info.dnses = [];
          gNetworkManager.updateNetworkInterface(WifiNetworkInterface);

          manager.supplicantStarted = true;
          gNetworkService.enableInterface(manager.ifname, function(ok) {
            callback(ok);
          });
        });
      });
    } else {
      let imsCapability;
      try {
        imsCapability = gImsRegService.getHandlerByServiceId(
          manager.telephonyServiceId
        ).capability;
      } catch (e) {
        manager.setWifiDisable(callback);
        return;
      }
      if (
        imsCapability == Ci.nsIImsRegHandler.IMS_CAPABILITY_VOICE_OVER_WIFI ||
        imsCapability == Ci.nsIImsRegHandler.IMS_CAPABILITY_VIDEO_OVER_WIFI
      ) {
        notify("registerimslistener", { register: true });
      } else {
        manager.setWifiDisable(callback);
      }
    }
  };

  manager.setWifiDisable = function(callback) {
    notify("registerimslistener", { register: false });
    manager.state = "DISABLING";
    clearTimeout(delayScanId);
    delayScanId = null;
    wifiInfo.reset();
    // Note these following calls ignore errors. If we fail to kill the
    // supplicant gracefully, then we need to continue telling it to die
    // until it does.
    let doDisableWifi = function() {
      manager.state = "UNINITIALIZED";
      gNetworkService.disableInterface(manager.ifname, function(ok) {
        wifiCommand.stopWifi(function(result) {
          notify("supplicantlost", { success: true });
          callback(result.status == SUCCESS);
        });
      });

      // We are going to terminate the connection between wpa_supplicant.
      // Stop the polling timer immediately to prevent connection info update
      // command blocking in control thread until socket timeout.
      notify("stopconnectioninfotimer");

      postDhcpSetup(function() {
        manager.connectionDropped(function() {});
      });
    };

    if (p2pSupported) {
      p2pManager.setEnabled(false, { onDisabled: doDisableWifi });
    } else {
      doDisableWifi();
    }
  };

  // Get wifi interface and load wifi driver when enable Ap mode.
  manager.setWifiApEnabled = function(enabled, configuration, callback) {
    if (enabled === manager.isWifiTetheringEnabled(manager.tetheringState)) {
      callback(true);
      return;
    }
    if (enabled) {
      debug("softap configuration -> " + uneval(configuration));
      manager.tetheringState = "INITIALIZING";
      wifiCommand.startSoftap(configuration, function(result) {
        if (result.status != SUCCESS) {
          callback(false);
          manager.tetheringState = "UNINITIALIZED";
          // TODO: clean up
          // wifiCommand.closeHostapdConnection(function(result) {});
          return;
        }
        var ifaceName = result.apInterface;
        gNetworkService.enableInterface(ifaceName, function(ok) {});
        WifiNetworkInterface.info.name = ifaceName;

        gTetheringService.setWifiTethering(
          enabled,
          WifiNetworkInterface.info.name,
          configuration,
          function(result) {
            manager.tetheringState = result ? "UNINITIALIZED" : "COMPLETED";
            // Pop out current request.
            callback(!result);
            // Should we fire a dom event if we fail to set wifi tethering  ?
            debug(
              "Enable Wifi tethering result: " +
                (result ? result : "successfully")
            );
          });
      });
    } else {
      gTetheringService.setWifiTethering(
        enabled,
        WifiNetworkInterface.info.name,
        configuration,
        function(result) {
          // Should we fire a dom event if we fail to set wifi tethering  ?
          debug(
            "Disable Wifi tethering result: " +
              (result ? result : "successfully")
          );
          // Unload wifi driver even if we fail to control wifi tethering.
          wifiCommand.stopSoftap(function(result) {
            if (result.status != SUCCESS) {
              debug("Fail to stop softap");
            }
            manager.tetheringState = "UNINITIALIZED";
            callback(result.status == SUCCESS);
          });
        }
      );
    }
  };

  manager.connectionDropped = function(callback) {
    // Reset network interface when connection drop
    gNetworkService.configureInterface(
      {
        ifname: manager.ifname,
        ipaddr: 0,
        mask: 0,
        gateway: 0,
        dns1: 0,
        dns2: 0,
      },
      function(data) {}
    );

    // If we got disconnected, kill the DHCP client in preparation for
    // reconnection.
    gNetworkService.resetConnections(manager.ifname, function() {
#ifdef HAS_KOOST_MODULES
      netUtil.stopDhcp(manager.ifname, function(success) {
        callback();
      });
#else
      netUtil.stopDhcp(manager.ifname, function() {
        callback();
      });
#endif
    });
  };

  manager.supplicantConnected = function(callback) {
    debug("detected SDK version " + sdkVersion);

    // Give it a state other than UNINITIALIZED, INITIALIZING or DISABLING
    // defined in getter function of WifiManager.enabled. It implies that
    // we are ready to accept dom request from applications.
    WifiManager.state = "DISCONNECTED";

    // Load up the supplicant state.
    getDebugEnabled(function(ok) {
      syncDebug();
    });

    notify("supplicantconnection");

    // WPA supplicant already connected.
    manager.enableAutoReconnect(true, function() {});
    manager.setPowerSave(true, function() {});
    // FIXME: window.navigator.mozPower is undefined
    // let window = Services.wm.getMostRecentWindow("navigator:browser");
    // if (window !== null) {
    //   setSuspendOptimizationsMode(POWER_MODE_SCREEN_STATE,
    //     !window.navigator.mozPower.screenEnabled, function() {});
    // }
    if (p2pSupported) {
      manager.enableP2p(function(success) {});
    }
  };

  var networkConfigurationFields = [
    { name: "ssid", type: "string" },
    { name: "bssid", type: "string" },
    { name: "psk", type: "string" },
    { name: "wepKey0", type: "string" },
    { name: "wepKey1", type: "string" },
    { name: "wepKey2", type: "string" },
    { name: "wepKey3", type: "string" },
    { name: "wepTxKeyIndex", type: "integer" },
    { name: "priority", type: "integer" },
    { name: "keyMgmt", type: "string" },
    { name: "scanSsid", type: "integer" },
    { name: "pmf", type: "integer" },
    { name: "disabled", type: "integer" },
    { name: "groupCipher", type: "integer" },
    { name: "pairwiseCipher", type: "integer" },
    { name: "identity", type: "string" },
    { name: "password", type: "string" },
    { name: "authAlg", type: "integer" },
    { name: "phase1", type: "string" },
    { name: "phase2", type: "string" },
    { name: "eap", type: "string" },
    { name: "pin", type: "string" },
    { name: "pcsc", type: "string" },
    { name: "caCert", type: "string" },
    { name: "caPath", type: "string" },
    { name: "subjectMatch", type: "string" },
    { name: "clientCert", type: "string" },
    { name: "private_key", type: "stirng" },
    { name: "engine", type: "integer" },
    { name: "engineId", type: "string" },
    { name: "privateKeyId", type: "string" },
    { name: "frequency", type: "integer" },
    { name: "mode", type: "integer" },
    { name: "simNum", type: "integer" },
    { name: "proto", type: "integer" },
    { name: wapi_key_type, type: "integer" },
    { name: as_cert_file, type: "string" },
    { name: user_cert_file, type: "string" },
  ];

  if (wapi_psk != "psk") {
    networkConfigurationFields.push({ name: wapi_psk, type: "string" });
  }

  // These fields are only handled in IBSS (aka ad-hoc) mode
  var ibssNetworkConfigurationFields = ["frequency", "mode"];

  manager.setAutoRoam = function(candidate, callback) {
    // set target bssid
    // wifiCommand.setNetworkVariable(
    //   candidate.netId,
    //   "bssid",
    //   manager.linkDebouncing ? "any" : candidate.bssid,
    //   function(ok) {
    //     wifiCommand.reassociate(callback);
    //   }
    // );
  };

  manager.setAutoConnect = function(candidate, callback) {
    // TODO: temporary solution, should merging candidate and configured network
    var configuredNetworks = WifiConfigManager.configuredNetworks;
    if (candidate.networkKey in configuredNetworks) {
      configuredNetworks[candidate.networkKey].bssid = candidate.bssid;
    }
    manager.connect(configuredNetworks[candidate.networkKey], function() {});
  };

  manager.clearCurrentConfigBSSID = function(netId, callback) {
    // wifiCommand.setNetworkVariable(netId, "bssid", "any", callback);
  };

  manager.saveNetwork = function(config, callback) {
    WifiConfigManager.addOrUpdateNetwork(config, callback);
  };

  manager.removeNetwork = function(netId, callback) {
    WifiConfigManager.removeNetwork(netId, callback);
  };

  manager.removeNetworks = function(callback) {
    delete manager.connectingNetwork[manager.ifname];
    wifiCommand.removeNetworks(function(result) {
      callback(result.status == SUCCESS);
    });
  };

  manager.connect = function(config, callback) {
    manager.connectingNetwork[manager.ifname] = config;
    wifiCommand.connect(config, function(result) {
      callback(result.status == SUCCESS);
    });
  };

  manager.enableNetwork = function(netId, callback) {
    if (p2pSupported) {
      // We have to stop wifi direct scan before associating to an AP.
      // Otherwise we will get a "REJECT" wpa supplicant event.
      p2pManager.setScanEnabled(false, function(success) {});
    }
    wifiCommand.enableNetwork(netId, callback);
  };

  manager.disableNetwork = function(netId, callback) {
    wifiCommand.disableNetwork(netId, callback);
  };

  manager.ensureSupplicantDetached = aCallback => {
    if (!manager.enabled) {
      aCallback();
      return;
    }
    wifiCommand.closeSupplicantConnection(aCallback);
  };

  manager.isHandShakeState = function(state) {
    switch (state) {
      case "AUTHENTICATING":
      case "ASSOCIATING":
      case "ASSOCIATED":
      case "FOUR_WAY_HANDSHAKE":
      case "GROUP_HANDSHAKE":
        return true;
      case "COMPLETED":
      case "DISCONNECTED":
      case "INTERFACE_DISABLED":
      case "INACTIVE":
      case "SCANNING":
      case "UNINITIALIZED":
      case "INVALID":
      case "CONNECTED":
      default:
        return false;
    }
  };

  manager.isConnectState = function(state) {
    switch (state) {
      case "AUTHENTICATING":
      case "ASSOCIATING":
      case "ASSOCIATED":
      case "FOUR_WAY_HANDSHAKE":
      case "GROUP_HANDSHAKE":
      case "CONNECTED":
      case "COMPLETED":
        return true;
      case "DISCONNECTED":
      case "INTERFACE_DISABLED":
      case "INACTIVE":
      case "SCANNING":
      case "UNINITIALIZED":
      case "INVALID":
      default:
        return false;
    }
  };

  manager.isWifiEnabled = function(state) {
    switch (state) {
      case "UNINITIALIZED":
      case "DISABLING":
        return false;
      default:
        return true;
    }
  };

  manager.isWifiTetheringEnabled = function(state) {
    switch (state) {
      case "UNINITIALIZED":
        return false;
      default:
        return true;
    }
  };

  manager.stateOrdinal = function(state) {
    return supplicantStatesMap.indexOf(state);
  };

  manager.supplicantLoopDetection = function(prevState, state) {
    var isPrevStateInHandShake = manager.isHandShakeState(prevState);
    var isStateInHandShake = manager.isHandShakeState(state);

    if (isPrevStateInHandShake) {
      if (isStateInHandShake) {
        // Increase the count only if we are in the loop.
        if (manager.stateOrdinal(state) > manager.stateOrdinal(prevState)) {
          manager.loopDetectionCount++;
        }
        if (manager.loopDetectionCount > MAX_SUPPLICANT_LOOP_ITERATIONS) {
          WifiConfigManager.updateNetworkSelectionStatus(
            lastNetwork.netId,
            WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
            function(doDisable) {
              manager.loopDetectionCount = 0;
              if (doDisable) {
                notify("networkdisable", {
                  reason: WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE,
                });
              }
            }
          );
        }
      }
    } else {
      // From others state to HandShake state. Reset the count.
      if (isStateInHandShake) {
        manager.loopDetectionCount = 0;
      }
    }
  };

  manager.handlePreWifiScan = function() {
    if (p2pSupported) {
      // Before doing regular wifi scan, we have to disable wifi direct
      // scan first. Otherwise we will never get the scan result.
      p2pManager.blockScan();
    }
  };

  manager.handlePostWifiScan = function() {
    if (p2pSupported) {
      // After regular wifi scanning, we should restore the restricted
      // wifi direct scan.
      p2pManager.unblockScan();
    }
  };

  //
  // Public APIs for P2P.
  //

  manager.p2pSupported = function() {
    return p2pSupported;
  };

  manager.getP2pManager = function() {
    return p2pManager;
  };

  manager.enableP2p = function(callback) {
    p2pManager.setEnabled(true, {
      onSupplicantConnected() {
        waitForEvent(WifiP2pManager.INTERFACE_NAME);
      },

      onEnabled(success) {
        callback(success);
      },
    });
  };

  manager.getCapabilities = function() {
    return capabilities;
  };

  // Cert Services
  let wifiCertService = Cc["@mozilla.org/wifi/certservice;1"];
  if (wifiCertService) {
    wifiCertService = wifiCertService.getService(Ci.nsIWifiCertService);
    wifiCertService.start(wifiListener);
  } else {
    console.log("No wifi CA service component available");
  }

  manager.importCert = function(caInfo, callback) {
    var id = idgen++;
    if (callback) {
      controlCallbacks[id] = callback;
    }

    wifiCertService.importCert(
      id,
      caInfo.certBlob,
      caInfo.certPassword,
      caInfo.certNickname
    );
  };

  manager.deleteCert = function(caInfo, callback) {
    var id = idgen++;
    if (callback) {
      controlCallbacks[id] = callback;
    }

    wifiCertService.deleteCert(id, caInfo.certNickname);
  };

  manager.sdkVersion = function() {
    return sdkVersion;
  };

  return manager;
})();

function shouldBroadcastRSSIForIMS(newRssi, lastRssi) {
  let needBroadcast = false;
  if (newRssi == lastRssi || lastRssi == INVALID_RSSI) {
    return needBroadcast;
  }

  debug(
    "shouldBroadcastRSSIForIMS, newRssi =" + newRssi + " , oldRssi= " + lastRssi
  );
  if (newRssi > -75 && lastRssi <= -75) {
    needBroadcast = true;
  } else if (newRssi <= -85 && lastRssi > -85) {
    needBroadcast = true;
  }
  return needBroadcast;
}

function quote(s) {
  return '"' + s + '"';
}

function dequote(s) {
  if (s[0] != '"' || s[s.length - 1] != '"') {
    throw "Invalid argument, not a quoted string: " + s;
  }
  return s.substr(1, s.length - 2);
}

function isWepHexKey(s) {
  if (s.length != 10 && s.length != 26 && s.length != 58) {
    return false;
  }
  return !/[^a-fA-F0-9]/.test(s);
}

const wepKeyList = ["wepKey0", "wepKey1", "wepKey2", "wepKey3"];
function isInWepKeyList(name) {
  for (let i = 0; i < wepKeyList.length; i++) {
    let wepField = wepKeyList[i];
    if ((wepField in name) && name[wepField]) {
      return true;
    }
  }
  return false;
}

var WifiNetworkInterface = {
  QueryInterface: ChromeUtils.generateQI([Ci.nsINetworkInterface]),

  registered: false,

  // nsINetworkInterface

  NETWORK_STATE_UNKNOWN: Ci.nsINetworkInfo.NETWORK_STATE_UNKNOWN,
  NETWORK_STATE_CONNECTING: Ci.nsINetworkInfo.CONNECTING,
  NETWORK_STATE_CONNECTED: Ci.nsINetworkInfo.CONNECTED,
  NETWORK_STATE_DISCONNECTING: Ci.nsINetworkInfo.DISCONNECTING,
  NETWORK_STATE_DISCONNECTED: Ci.nsINetworkInfo.DISCONNECTED,

  NETWORK_TYPE_WIFI: Ci.nsINetworkInfo.NETWORK_TYPE_WIFI,
  NETWORK_TYPE_MOBILE: Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE,
  NETWORK_TYPE_MOBILE_MMS: Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_MMS,
  NETWORK_TYPE_MOBILE_SUPL: Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE_SUPL,

  updateConfig(action, config) {
    debug(
      "Interface " +
        this.info.name +
        " updateConfig: " +
        action +
        " " +
        JSON.stringify(config)
    );

    this.info.state =
      config.state != undefined ? config.state : this.info.state;
    let configHasChanged = false;
    if (action == "updated") {
      if (config.ip != undefined && this.info.ips.indexOf(config.ip) === -1) {
        configHasChanged = true;
        this.info.ips.push(config.ip);
        this.info.prefixLengths.push(config.prefixLengths);
      }
      if (
        config.gateway != undefined &&
        this.info.gateways.indexOf(config.gateway) === -1
      ) {
        configHasChanged = true;
        this.info.gateways.push(config.gateway);
      }
      if (config.dnses != undefined) {
        for (let i in config.dnses) {
          if (
            typeof config.dnses[i] == "string" &&
            config.dnses[i].length &&
            this.info.dnses.indexOf(config.dnses[i]) === -1
          ) {
            configHasChanged = true;
            this.info.dnses.push(config.dnses[i]);
          }
        }
      }
    } else {
      if (config.ip != undefined) {
        let found = this.info.ips.indexOf(config.ip);
        if (found !== -1) {
          configHasChanged = true;
          this.info.ips.splice(found, 1);
          if (this.info.prefixlengths != undefined) {
            this.info.prefixlengths.splice(found, 1);
          }
        }
      }
      if (config.gateway != undefined) {
        let found = this.info.gateways.indexOf(config.gateway);
        if (found !== -1) {
          configHasChanged = true;
          this.info.gateways.splice(found, 1);
        }
      }
    }
    if (
      configHasChanged &&
      this.info.state == Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED
    ) {
      debug("WifiNetworkInterface changed, update network interface");
      gNetworkManager.updateNetworkInterface(WifiNetworkInterface);
    }
  },

  info: {
    QueryInterface: ChromeUtils.generateQI([Ci.nsINetworkInfo]),

    state: Ci.nsINetworkInfo.NETWORK_STATE_UNKNOWN,

    type: Ci.nsINetworkInfo.NETWORK_TYPE_WIFI,

    name: null,

    ips: [],

    prefixLengths: [],

    dnses: [],

    gateways: [],

    getAddresses(ips, prefixLengths) {
      ips.value = this.ips.slice();
      prefixLengths.value = this.prefixLengths.slice();

      return this.ips.length;
    },

    getGateways(count) {
      if (count) {
        count.value = this.gateways.length;
      }
      return this.gateways.slice();
    },

    getDnses(count) {
      if (count) {
        count.value = this.dnses.length;
      }
      return this.dnses.slice();
    },
  },

  httpProxyHost: null,

  httpProxyPort: null,
};

function WifiScanResult() {}

// TODO Make the difference between a DOM-based network object and our
// networks objects much clearer.
var netToDOM;
var netFromDOM;

function WifiWorker() {
  var self = this;

  //this._mm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
  //             .getService(Ci.nsIMessageListenerManager);
  const messages = [
    "WifiManager:getNetworks",
    "WifiManager:getKnownNetworks",
    "WifiManager:associate",
    "WifiManager:forget",
    "WifiManager:wps",
    "WifiManager:getState",
    "WifiManager:setPowerSavingMode",
    "WifiManager:setHttpProxy",
    "WifiManager:setStaticIpMode",
    "WifiManager:importCert",
    "WifiManager:getImportedCerts",
    "WifiManager:deleteCert",
    "WifiManager:setWifiEnabled",
    "WifiManager:setWifiTethering",
    "child-process-shutdown",
  ];

  messages.forEach(
    function(msgName) {
      //this._mm.addMessageListener(msgName, this);
      ppmm.addMessageListener(msgName, this);
    }.bind(this)
  );

  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic);
  Services.obs.addObserver(this, kXpcomShutdownChangedTopic);
  Services.obs.addObserver(this, kScreenStateChangedTopic);
  Services.obs.addObserver(this, kInterfaceAddressChangedTopic);
  Services.obs.addObserver(this, kInterfaceDnsInfoTopic);
  Services.obs.addObserver(this, kRouteChangedTopic);
  Services.obs.addObserver(this, kWifiCaptivePortalResult);
  Services.obs.addObserver(this, kOpenCaptivePortalLoginEvent);
  Services.obs.addObserver(this, kCaptivePortalLoginSuccessEvent);
  Services.prefs.addObserver(kPrefDefaultServiceId, this);

  this.wantScanResults = [];

  this._highestPriority = -1;

  this._addingNetworks = Object.create(null);

  this.ipAddress = "";

  this._lastConnectionInfo = null;
  this._connectionInfoTimer = null;
  this._listeners = [];
  this.wifiDisableDelayId = null;
  this.isDriverRoaming = false;

  WifiManager.telephonyServiceId = this._getDefaultServiceId();

  // Create p2pObserver and assign to p2pManager.
  if (WifiManager.p2pSupported()) {
    this._p2pObserver = WifiP2pWorkerObserver(WifiManager.getP2pManager());
    WifiManager.getP2pManager().setObserver(this._p2pObserver);

    // Add DOM message observerd by p2pObserver to the message listener as well.
    this._p2pObserver.getObservedDOMMessages().forEach(
      function(msgName) {
        //this._mm.addMessageListener(msgName, this);
        ppmm.addMessageListener(msgName, this);
      }.bind(this)
    );
  }

  // Users of instances of nsITimer should keep a reference to the timer until
  // it is no longer needed in order to assure the timer is fired.
  this._callbackTimer = null;

  // A list of requests to turn wifi on or off.
  this._stateRequests = [];

  // Given a connection status network, takes a network and
  // prepares it for the DOM.
  netToDOM = function(net) {
    if (!net) {
      return null;
    }
    var ssid = net.ssid ? dequote(net.ssid) : null;
    var mode = net.mode;
    var frequency = net.frequency;
    var security =
      net.keyMgmt === "NONE" && isInWepKeyList(net)
        ? "WEP"
        : net.keyMgmt && net.keyMgmt !== "NONE"
        ? net.keyMgmt.split(" ")[0]
        : "";
    var password;
    if (
      ("psk" in net && net.psk) ||
      ("password" in net && net.password) ||
      (WifiManager.wapi_psk in net && net[WifiManager.wapi_psk]) ||
      isInWepKeyList(net)
    ) {
      password = "*";
    }

    var pub = new WifiNetwork(ssid, mode, frequency, security, password);
    if (net.identity) {
      pub.identity = dequote(net.identity);
    }
    if ("netId" in net) {
      pub.known = true;
      if (net.netId == wifiInfo.networkId && self.ipAddress) {
        pub.connected = true;
        if (lastNetwork.everValidated) {
          pub.hasInternet = true;
          pub.captivePortalDetected = false;
        } else if (lastNetwork.everCaptivePortalDetected) {
          pub.hasInternet = false;
          pub.captivePortalDetected = true;
        } else {
          pub.hasInternet = false;
          pub.captivePortalDetected = false;
        }
      }
    }
    if (net.scanSsid) {
      pub.hidden = true;
    }
    if (
      "ca_cert" in net &&
      net.ca_cert &&
      net.ca_cert.indexOf("keystore://WIFI_SERVERCERT_" === 0)
    ) {
      pub.serverCertificate = net.ca_cert.substr(27);
    }
    if (net.subject_match) {
      pub.subjectMatch = net.subject_match;
    }
    if (
      "client_cert" in net &&
      net.client_cert &&
      net.client_cert.indexOf("keystore://WIFI_USERCERT_" === 0)
    ) {
      pub.userCertificate = net.client_cert.substr(25);
    }
    if (WifiManager.wapi_key_type in net && net[WifiManager.wapi_key_type]) {
      if (net[WifiManager.wapi_key_type] == 1) {
        pub.pskType = "HEX";
      }
    }
    if (WifiManager.as_cert_file in net && net[WifiManager.as_cert_file]) {
      pub.wapiAsCertificate = net[WifiManager.as_cert_file];
    }
    if (WifiManager.user_cert_file in net && net[WifiManager.user_cert_file]) {
      pub.wapiUserCertificate = net[WifiManager.user_cert_file];
    }
    return pub;
  };

  netFromDOM = function(net, configured) {
    // Takes a network from the DOM and makes it suitable for insertion into
    // configuredNetworks (that is calling addNetwork will
    // do the right thing).
    // NB: Modifies net in place: safe since we don't share objects between
    // the dom and the chrome code.

    // Things that are useful for the UI but not to us.
    delete net.bssid;
    delete net.signalStrength;
    delete net.relSignalStrength;
    delete net.security;
    delete net.capabilities;

    if (!configured) {
      configured = {};
    }

    net.ssid = quote(net.ssid);

    let wep = false;
    if ("keyManagement" in net) {
      if (net.keyManagement === "WEP") {
        wep = true;
        net.keyManagement = "NONE";
      } else if (net.keyManagement === "WPA-EAP") {
        net.keyManagement += " IEEE8021X";
      }
      configured.keyMgmt = net.keyMgmt = net.keyManagement; // WPA2-PSK, WPA-PSK, etc.
      delete net.keyManagement;
    } else {
      configured.keyMgmt = net.keyMgmt = "NONE";
    }

    if (net.hidden) {
      configured.scanSsid = net.scanSsid = true;
      delete net.hidden;
    }

    function checkAssign(name, checkStar) {
      if (name in net) {
        let value = net[name];
        if (!value || (checkStar && value === "*")) {
          if (name in configured && configured[name] !== "*") {
            net[name] = configured[name];
          } else {
            delete net[name];
          }
        } else {
          configured[name] = net[name] = quote(value);
        }
      }
    }

    // TODO: do not need to quote
    //checkAssign("psk", true);

    if (net.keyMgmt == "WAPI-PSK") {
      net.proto = configured.proto = "WAPI";
      if (net.pskType == "HEX") {
        net[WifiManager.wapi_key_type] = 1;
      }
      net[WifiManager.wapi_psk] = configured[WifiManager.wapi_psk] = quote(
        net.wapi_psk
      );

      if (WifiManager.wapi_psk != "wapi_psk") {
        delete net.wapi_psk;
      }
    }
    if (net.keyMgmt == "WAPI-CERT") {
      net.proto = configured.proto = "WAPI";
      if (hasValidProperty("wapiAsCertificate")) {
        net[WifiManager.as_cert_file] = quote(net.wapiAsCertificate);
        delete net.wapiAsCertificate;
      }
      if (hasValidProperty("wapiUserCertificate")) {
        net[WifiManager.user_cert_file] = quote(net.wapiUserCertificate);
        delete net.wapiUserCertificate;
      }
    }

    checkAssign("identity", false);
    checkAssign("password", true);
    if (wep && net.wep && net.wep != "*") {
      net.keyIndex = net.keyIndex || 0;
      configured["wepKey" + net.keyIndex] = net[
        "wepKey" + net.keyIndex
      ] = isWepHexKey(net.wep) ? net.wep : quote(net.wep);
      configured.wepTxKeyIndex = net.wepTxKeyIndex = net.keyIndex;
      configured.authAlg = net.authAlg = (AUTHALG_OPEN | AUTHALG_SHARED);
    }

    function hasValidProperty(name) {
      return name in net && net[name] != null;
    }

    if (hasValidProperty("eap") || net.keyMgmt.indexOf("WPA-EAP") !== -1) {
      if (hasValidProperty("pin")) {
        net.pin = quote(net.pin);
      }

      if (net.eap === "SIM" || net.eap === "AKA" || net.eap === "AKA'") {
        configured.sim_num = net.sim_num = net.sim_num || 1;
      }

      if (hasValidProperty("phase1")) {
        net.phase1 = quote(net.phase1);
      }

      if (hasValidProperty("phase2")) {
        if (net.phase2 === "TLS") {
          net.phase2 = quote("autheap=" + net.phase2);
        } else {
          // PAP, MSCHAPV2, etc.
          net.phase2 = quote("auth=" + net.phase2);
        }
      }

      if (hasValidProperty("serverCertificate")) {
        net.ca_cert = quote(
          "keystore://WIFI_SERVERCERT_" + net.serverCertificate
        );
      }

      if (hasValidProperty("subjectMatch")) {
        net.subject_match = quote(net.subjectMatch);
      }

      if (hasValidProperty("userCertificate")) {
        let userCertName = "WIFI_USERCERT_" + net.userCertificate;
        net.client_cert = quote("keystore://" + userCertName);

        let wifiCertService = Cc["@mozilla.org/wifi/certservice;1"].getService(
          Ci.nsIWifiCertService
        );
        if (wifiCertService.hasPrivateKey(userCertName)) {
          if (WifiManager.sdkVersion() >= 19) {
            // Use openssol engine instead of keystore protocol after Kitkat.
            net.engine = 1;
            net.engine_id = quote("keystore");
            net.key_id = quote("WIFI_USERKEY_" + net.userCertificate);
          } else {
            net.private_key = quote(
              "keystore://WIFI_USERKEY_" + net.userCertificate
            );
          }
        }
      }
    }

    return net;
  };

  WifiManager.onsupplicantconnection = function() {
    debug("Connected to supplicant");
    // Register listener for mobileConnectionService
    if (!self.mobileConnectionRegistered) {
      // gMobileConnectionService
      //   .getItemByServiceId(WifiManager.telephonyServiceId).registerListener(self);
      self.mobileConnectionRegistered = true;
    }
    // Set current country code
    self.lastKnownCountryCode = self.pickWifiCountryCode();
    if (self.lastKnownCountryCode !== "") {
      WifiManager.setCountryCode(self.lastKnownCountryCode, function() {});
    }
    // wifi enabled and reset open network notification.
    OpenNetworkNotifier.clearPendingNotification();

    // Start to trigger period scan.
    WifiManager.startDelayScan();

    // Notify everybody, even if they didn't ask us to come up.
    WifiManager.getMacAddress(function(result) {
      self._macAddress = result.macAddress;
      debug("Got mac: " + self._macAddress);
      self._fireEvent("wifiUp", { macAddress: self._macAddress });
      self.requestDone();
    });

    // Use external processing for SIM/USIM operations.
    WifiManager.setExternalSim(true, function() {});
  };

  WifiManager.onsupplicantlost = function() {
    WifiManager.supplicantStarted = false;
    WifiManager.state = "UNINITIALIZED";
    debug("Supplicant died!");

    // wifi disabled and reset open network notification.
    OpenNetworkNotifier.clearPendingNotification();
    WifiConfigManager.setAndEnableLastSelectedConfiguration(
      INVALID_NETWORK_ID,
      function() {}
    );
    // Notify everybody, even if they didn't ask us to come up.
    self._fireEvent("wifiDown", {});
    self.requestDone();
  };

  WifiManager.onnetworkdisable = function() {
    debug(
      "Notify event: " +
        WifiConfigManager.QUALITY_NETWORK_SELECTION_DISABLE_REASON[this.reason]
    );
    self.handleNetworkConnectionFailure(lastNetwork);
    switch (this.reason) {
      case WifiConfigManager.DISABLED_DHCP_FAILURE:
        self._fireEvent("ondhcpfailed", { network: netToDOM(lastNetwork) });
        break;
      case WifiConfigManager.DISABLED_AUTHENTICATION_FAILURE:
        self._fireEvent("onauthenticationfailed", {
          network: netToDOM(lastNetwork),
        });
        break;
      case WifiConfigManager.DISABLED_ASSOCIATION_REJECTION:
        self._fireEvent("onassociationreject", {
          network: netToDOM(lastNetwork),
        });
        break;
    }
  };

  WifiManager.onstatechange = function() {
    debug("State change: " + this.prevState + " -> " + this.state);

    if (
      self._connectionInfoTimer &&
      this.state !== "CONNECTED" &&
      this.state !== "COMPLETED"
    ) {
      self._stopConnectionInfoTimer();
    }

    switch (this.state) {
      case "ASSOCIATING":
        // id has not yet been filled in, so we can only report the ssid and
        // bssid. mode and frequency are simply made up.
        let networkEverValidated = false;
        let networkEverCaptivePortalDetected = false;
        if (lastNetwork) {
          networkEverValidated = lastNetwork.everValidated;
          networkEverCaptivePortalDetected =
            lastNetwork.everCaptivePortalDetected;
        }

        lastNetwork = {
          bssid: wifiInfo.bssid,
          ssid: quote(wifiInfo.wifiSsid),
          mode: WifiConfigUtils.MODE_ESS,
          frequency: 0,
          everValidated: networkEverValidated,
          everCaptivePortalDetected: networkEverCaptivePortalDetected,
        };
        if (wifiInfo.networkId !== INVALID_NETWORK_ID) {
          lastNetwork.netId = wifiInfo.networkId;
        }
        self._fireEvent("onconnecting", { network: netToDOM(lastNetwork) });
        break;
      case "ASSOCIATED":
        if (!lastNetwork) {
          lastNetwork = {};
        }
        lastNetwork.bssid = wifiInfo.bssid;
        lastNetwork.ssid = quote(wifiInfo.wifiSsid);
        lastNetwork.isDriverRoaming = this.isDriverRoaming;
        lastNetwork.netId = wifiInfo.networkId;
        WifiConfigManager.getNetworkConfiguration(lastNetwork, function() {
          if (!lastNetwork.isDriverRoaming) {
            // Notify again because we get complete network information.
            self._fireEvent("onconnecting", { network: netToDOM(lastNetwork) });
          }
        });
        break;
      case "COMPLETED":
        var _oncompleted = function() {
          self._startConnectionInfoTimer();
          if (!lastNetwork.isDriverRoaming) {
            self._fireEvent("onassociate", { network: netToDOM(lastNetwork) });
          }
        };
        lastNetwork.bssid = wifiInfo.bssid;
        lastNetwork.ssid = quote(wifiInfo.wifiSsid);
        lastNetwork.netId = wifiInfo.networkId;
        lastNetwork.isDriverRoaming = this.isDriverRoaming;
        WifiConfigManager.getNetworkConfiguration(lastNetwork, _oncompleted);
        break;
      case "CONNECTED":
        // BSSID is read after connected, update it.
        lastNetwork.bssid = wifiInfo.bssid;
        break;
      case "DISCONNECTED":
        // wpa_supplicant may give us a "DISCONNECTED" event even if
        // we are already in "DISCONNECTED" state.
        if (
          WifiNetworkInterface.info.state ===
            Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED &&
          (this.prevState === "INITIALIZING" ||
            this.prevState === "DISCONNECTED" ||
            this.prevState === "INTERFACE_DISABLED" ||
            this.prevState === "INACTIVE" ||
            this.prevState === "UNINITIALIZED" ||
            this.prevState === "SCANNING")
        ) {
          return;
        }

        if (!lastNetwork) {
          lastNetwork = {};
        }
        self._fireEvent("ondisconnect", { network: netToDOM(lastNetwork) });

        lastNetwork.everValidated = false;
        lastNetwork.everCaptivePortalDetected = false;
        if (self.handlePromptUnvalidatedId !== null) {
          clearTimeout(self.handlePromptUnvalidatedId);
          self.handlePromptUnvalidatedId = null;
        }
        self.ipAddress = "";

        WifiManager.connectionDropped(function() {});

        // wifi disconnected and reset open network notification.
        OpenNetworkNotifier.clearPendingNotification();

        WifiNetworkInterface.info.state =
          Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED;
        WifiNetworkInterface.info.ips = [];
        WifiNetworkInterface.info.prefixLengths = [];
        WifiNetworkInterface.info.gateways = [];
        WifiNetworkInterface.info.dnses = [];
        gNetworkManager.updateNetworkInterface(WifiNetworkInterface);
        break;
      case "WPS_TIMEOUT":
        self._fireEvent("onwpstimeout", {});
        break;
      case "WPS_FAIL":
        self._fireEvent("onwpsfail", {});
        break;
      case "WPS_OVERLAP_DETECTED":
        self._fireEvent("onwpsoverlap", {});
        break;
      case "AUTHENTICATING":
        lastNetwork.bssid = wifiInfo.bssid;
        lastNetwork.ssid = quote(wifiInfo.wifiSsid);
        lastNetwork.netId = wifiInfo.networkId;
        self._fireEvent("onauthenticating", { network: netToDOM(lastNetwork) });
        break;
      case "SCANNING":
        break;
    }
  };

  WifiManager.onnetworkconnected = function() {
    if (!this.info || !this.info.ipaddr_str) {
      debug("Network information is invalid.");
      return;
    }

    for (let i in WifiNetworkInterface.info.ips) {
      if (WifiNetworkInterface.info.ips[i].indexOf(".") == -1) {
        continue;
      }

      if (
        WifiNetworkInterface.info.ips[i].indexOf(this.info.ipaddr_str) === -1
      ) {
        debug("router subnet change, disconnect it.");
        WifiManager.disconnect(function() {
          WifiManager.reassociate(function() {});
        });
        return;
      }
    }

    let maskLength = netHelpers.getMaskLength(
      netHelpers.stringToIP(this.info.mask_str)
    );
    if (!maskLength) {
      maskLength = 32; // max prefix for IPv4.
    }

    let netConnect = WifiManager.getHttpProxyNetwork(lastNetwork);
    if (netConnect) {
      WifiNetworkInterface.httpProxyHost = netConnect.httpProxyHost;
      WifiNetworkInterface.httpProxyPort = netConnect.httpProxyPort;
    }

    self.handlePromptUnvalidatedId = setTimeout(
      self.handlePromptUnvalidated.bind(self),
      PROMPT_UNVALIDATED_DELAY_MS
    );

    // wifi connected and reset open network notification.
    OpenNetworkNotifier.clearPendingNotification();
    // reset disable counter tracking
    WifiManager.loopDetectionCount = 0;
    WifiConfigManager.clearDisableReasonCounter(
      escape(wifiInfo.wifiSsid) + wifiInfo.security
    );

    WifiNetworkInterface.updateConfig("updated", {
      state: Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED,
      ip: this.info.ipaddr_str,
      gateway: this.info.gateway_str,
      prefixLengths: maskLength,
      dnses: [this.info.dns1_str, this.info.dns2_str],
    });

    self.ipAddress = this.info.ipaddr_str;

    // We start the connection information timer when we associate, but
    // don't have our IP address until here. Make sure that we fire a new
    // connectionInformation event with the IP address the next time the
    // timer fires.
    self._lastConnectionInfo = null;
    if (!lastNetwork.isDriverRoaming) {
      self._fireEvent("onconnect", { network: netToDOM(lastNetwork) });
    }
    WifiManager.postDhcpSetup(function() {});
  };

  WifiManager.onscanresultsavailable = function() {
    WifiManager.getScanResults(this.type, function(data) {
      let scanResults = data.getScanResults();
      // Check any open network and notify to user.
      // TODO: uncomplete
      //if (WifiNetworkInterface.info.state == Ci.nsINetworkInfo.NETWORK_STATE_DISCONNECTED) {
      //  OpenNetworkNotifier.handleScanResults(r);
      //}

      if (scanResults.length <= 0 && self.wantScanResults.length !== 0) {
        self.wantScanResults.forEach(function(callback) {
          callback(null);
        });
        self.wantScanResults = [];
        return;
      }

      let capabilities = WifiManager.getCapabilities();

      // Now that we have scan results, there's no more need to continue
      // scanning. Ignore any errors from this command.
      self.networksArray = [];
      var channelSet = new Set();
      for (let i = 0; i < scanResults.length; i++) {
        let result = scanResults[i];
        let ie = result.getInfoElement();
        debug(" * " +
          result.ssid + " " +
          result.bssid + " " +
          result.frequency + " " +
          result.tsf + " " +
          result.capability + " " +
          result.signal + " " +
          result.associated
        );

        let signalLevel = result.signal / 100;
        let infoElement = WifiConfigUtils.parseInformationElements(ie);
        let flags = WifiConfigUtils.parseCapabilities(
          infoElement,
          result.capability
        );

        /* Skip networks with unknown or unsupported modes. */
        if (capabilities.mode.indexOf(WifiConfigUtils.getMode(flags)) == -1)
          continue;

        // If this is the first time that we've seen this SSID in the scan
        // results, add it to the list along with any other information.
        // Also, we use the highest signal strength that we see.
        let network = new ScanResult(
          result.ssid,
          result.bssid,
          result.frequency,
          flags,
          signalLevel,
          result.associated
        );

        let networkKey = WifiConfigUtils.getNetworkKey(network);
        network.networkKey = networkKey;

        var configuredNetworks = WifiConfigManager.configuredNetworks;
        if (networkKey in configuredNetworks) {
          let known = configuredNetworks[networkKey];
          network.known = true;
          network.netId = known.netId;

          if (
            network.netId == wifiInfo.networkId &&
            self.ipAddress &&
            result.associated
          ) {
            network.connected = true;
            channelSet.add(result.frequency);
            if (lastNetwork.everValidated) {
              network.hasInternet = known.hasInternet = true;
              network.captivePortalDetected = false;
            } else if (lastNetwork.everCaptivePortalDetected) {
              network.hasInternet = known.hasInternet = false;
              network.captivePortalDetected = true;
            } else {
              network.hasInternet = known.hasInternet = false;
              network.captivePortalDetected = false;
            }
          }

          if ("identity" in known && known.identity) {
            network.identity = dequote(known.identity);
          }

          // Note: we don't hand out passwords here! The * marks that there
          // is a password that we're hiding.
          if (
            ("psk" in known && known.psk) ||
            ("password" in known && known.password) ||
            isInWepKeyList(known)
          ) {
            network.password = "*";
          }
          if ("hasInternet" in known) {
            network.hasInternet = known.hasInternet;
          }
        }

        let signal = WifiConfigUtils.calculateSignal(Number(signalLevel));
        if (signal > network.relSignalStrength) {
          network.relSignalStrength = signal;
        }
        self.networksArray.push(network);
      }

      WifiManager.currentConfigChannels = [];
      channelSet.forEach(function(channel) {
        WifiManager.currentConfigChannels.push(channel);
      });

      if (!WifiManager.wpsStarted) {
        self.handleScanResults(self.networksArray);
      }
      if (self.wantScanResults.length !== 0) {
        self.wantScanResults.forEach(function(callback) {
          callback(self.networksArray);
        });
        self.wantScanResults = [];
      }
      self._fireEvent("scanresult", { scanResult: self.networksArray });
    });
  }

  WifiManager.onregisterimslistener = function() {
    let imsService;
    try {
      imsService = gImsRegService.getHandlerByServiceId(
        WifiManager.telephonyServiceId
      );
    } catch (e) {
      return;
    }
    if (this.register) {
      imsService.registerListener(self);
      let imsDelayTimeout = 7000;
      try {
        //FIXME: NS_ERROR_UNEXPECTED: Component returned failure code: 0x8000ffff (NS_ERROR_UNEXPECTED) [nsIPrefBranch.getIntPref]
        imsDelayTimeout = 5000; //Services.prefs.getIntPref("vowifi.delay.timer");
      } catch (e) {}
      debug("delay " + imsDelayTimeout / 1000 + " secs for disabling wifi");
      self.wifiDisableDelayId = setTimeout(
        WifiManager.setWifiDisable,
        imsDelayTimeout
      );
    } else {
      if (self.wifiDisableDelayId === null) {
        return;
      }
      clearTimeout(self.wifiDisableDelayId);
      self.wifiDisableDelayId = null;
      imsService.unregisterListener(self);
    }
  };

  WifiManager.onstationinfoupdate = function() {
    self._fireEvent("stationinfoupdate", { station: this.station });
  };

  OpenNetworkNotifier.onopennetworknotification = function() {
    self._fireEvent("opennetwork", { availability: this.enabled });
  };

  WifiManager.onstopconnectioninfotimer = function() {
    self._stopConnectionInfoTimer();
  };

  // Read the 'wifi.enabled' setting in order to start with a known
  // value at boot time. The handle() will be called after reading.
  //
  // nsISettingsServiceCallback implementation.
  var initWifiEnabledCb = {
    handle: function handle(aName, aResult) {
      if (aName !== SETTINGS_WIFI_ENABLED) {
        return;
      }
      if (aResult === null) {
        aResult = true;
      }
      self.handleWifiEnabled(aResult);
    },
    handleError: function handleError(aErrorMessage) {
      debug("Error reading the 'wifi.enabled' setting. Default to wifi on.");
      self.handleWifiEnabled(true);
    },
  };

  var initWifiDebuggingEnabledCb = {
    handle: function handle(aName, aResult) {
      if (aName !== SETTINGS_WIFI_DEBUG_ENABLED) {
        return;
      }
      if (aResult === null) {
        aResult = false;
      }
      DEBUG = aResult;
      updateDebug();
    },
    handleError: function handleError(aErrorMessage) {
      debug(
        "Error reading the 'wifi.debugging.enabled' setting. Default to debugging off."
      );
      DEBUG = false;
      updateDebug();
    },
  };

  var initAirplaneModeCb = {
    handle: function handle(aName, aResult) {
      if (aName !== SETTINGS_AIRPLANE_MODE) {
        return;
      }
      if (aResult === null) {
        aResult = false;
      }
      self._airplaneMode = aResult;
    },
    handleError: function handleError(aErrorMessage) {
      debug("Error reading the 'SETTINGS_AIRPLANE_MODE' setting.");
      self._airplaneMode = false;
    },
  };

  var initWifiNotifycationCb = {
    handle: function handle(aName, aResult) {
      if (aName !== SETTINGS_WIFI_NOTIFYCATION) {
        return;
      }
      if (aResult === null) {
        aResult = false;
      }
      OpenNetworkNotifier.setOpenNetworkNotifyEnabled(aResult);
    },
    handleError: function handleError(aErrorMessage) {
      debug("Error reading the 'SETTINGS_WIFI_NOTIFYCATION' setting.");
      OpenNetworkNotifier.setOpenNetworkNotifyEnabled(true);
    },
  };

  // Initialize configured network from config file.
  WifiConfigManager.loadFromStore();

  this.initTetheringSettings();
  //FIXME: TypeError: gSettingsService is undefined
  // let lock = gSettingsService.createLock();
  // lock.get(SETTINGS_WIFI_ENABLED, initWifiEnabledCb);
  // lock.get(SETTINGS_WIFI_DEBUG_ENABLED, initWifiDebuggingEnabledCb);
  // lock.get(SETTINGS_AIRPLANE_MODE, initAirplaneModeCb);
  // lock.get(SETTINGS_WIFI_NOTIFYCATION, initWifiNotifycationCb);

  // lock.get(SETTINGS_WIFI_SSID, this);
  // lock.get(SETTINGS_WIFI_SECURITY_TYPE, this);
  // lock.get(SETTINGS_WIFI_SECURITY_PASSWORD, this);
  // lock.get(SETTINGS_WIFI_CHANNEL, this);
  // lock.get(SETTINGS_WIFI_IP, this);
  // lock.get(SETTINGS_WIFI_PREFIX, this);
  // lock.get(SETTINGS_WIFI_DHCPSERVER_STARTIP, this);
  // lock.get(SETTINGS_WIFI_DHCPSERVER_ENDIP, this);
  // lock.get(SETTINGS_WIFI_DNS1, this);
  // lock.get(SETTINGS_WIFI_DNS2, this);

  // lock.get(SETTINGS_USB_DHCPSERVER_STARTIP, this);
  // lock.get(SETTINGS_USB_DHCPSERVER_ENDIP, this);
  this._wifiTetheringSettingsToRead = [
    SETTINGS_WIFI_SSID,
    SETTINGS_WIFI_SECURITY_TYPE,
    SETTINGS_WIFI_SECURITY_PASSWORD,
    SETTINGS_WIFI_CHANNEL,
    SETTINGS_WIFI_IP,
    SETTINGS_WIFI_PREFIX,
    SETTINGS_WIFI_DHCPSERVER_STARTIP,
    SETTINGS_WIFI_DHCPSERVER_ENDIP,
    SETTINGS_WIFI_DNS1,
    SETTINGS_WIFI_DNS2,
    SETTINGS_WIFI_TETHERING_ENABLED,
    SETTINGS_USB_DHCPSERVER_STARTIP,
    SETTINGS_USB_DHCPSERVER_ENDIP,
  ];
}

function translateState(state) {
  switch (state) {
    case "INTERFACE_DISABLED":
    case "INACTIVE":
    case "SCANNING":
    case "DISCONNECTED":
    default:
      return "disconnected";

    case "AUTHENTICATING":
    case "ASSOCIATING":
    case "ASSOCIATED":
    case "FOUR_WAY_HANDSHAKE":
    case "GROUP_HANDSHAKE":
      return "connecting";

    case "COMPLETED":
      return WifiManager.getDhcpInfo() ? "connected" : "associated";
  }
}

WifiWorker.prototype = {
  classID: WIFIWORKER_CID,
  contractID: WIFIWORKER_CONTRACTID,
  // classInfo: XPCOMUtils.generateCI({classID: WIFIWORKER_CID,
  //                                   contractID: WIFIWORKER_CONTRACTID,
  //                                   classDescription: "WifiWorker",
  //                                   interfaces: [Ci.nsIWorkerHolder,
  //                                                Ci.nsIWifi,
  //                                                Ci.nsIObserver]}),
  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIWorkerHolder,
    Ci.nsIWifi,
    Ci.nsIObserver,
    Ci.nsISettingsServiceCallback,
    Ci.nsIMobileConnectionListener,
    Ci.nsIImsRegListener,
    Ci.nsIIccListener,
  ]),

  disconnectedByWifi: false,

  disconnectedByWifiTethering: false,

  lastKnownCountryCode: null,
  mobileConnectionRegistered: false,

  _wifiTetheringSettingsToRead: [],

  _oldWifiTetheringEnabledState: null,

  _airplaneMode: false,
  _airplaneMode_status: null,

  _listeners: null,

  tetheringSettings: {},
  wifiNetworkInfo: wifiInfo,

  initTetheringSettings() {
    this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED] = null;
    this.tetheringSettings[SETTINGS_WIFI_SSID] = DEFAULT_HOTSPOT_SSID;
    this.tetheringSettings[
      SETTINGS_WIFI_SECURITY_TYPE
    ] = DEFAULT_HOTSPOT_SECURITY_TYPE;
    this.tetheringSettings[
      SETTINGS_WIFI_SECURITY_PASSWORD
    ] = DEFAULT_HOTSPOT_SECURITY_PASSWORD;
    this.tetheringSettings[SETTINGS_WIFI_CHANNEL] = DEFAULT_HOTSPOT_CHANNEL;
    this.tetheringSettings[SETTINGS_WIFI_IP] = DEFAULT_HOTSPOT_IP;
    this.tetheringSettings[SETTINGS_WIFI_PREFIX] = DEFAULT_HOTSPOT_PREFIX;
    this.tetheringSettings[
      SETTINGS_WIFI_DHCPSERVER_STARTIP
    ] = DEFAULT_HOTSPOT_DHCPSERVER_STARTIP;
    this.tetheringSettings[
      SETTINGS_WIFI_DHCPSERVER_ENDIP
    ] = DEFAULT_HOTSPOT_DHCPSERVER_ENDIP;
    this.tetheringSettings[SETTINGS_WIFI_DNS1] = DEFAULT_DNS1;
    this.tetheringSettings[SETTINGS_WIFI_DNS2] = DEFAULT_DNS2;
    this.tetheringSettings[
      SETTINGS_USB_DHCPSERVER_STARTIP
    ] = DEFAULT_USB_DHCPSERVER_STARTIP;
    this.tetheringSettings[
      SETTINGS_USB_DHCPSERVER_ENDIP
    ] = DEFAULT_USB_DHCPSERVER_ENDIP;
  },

  // nsIImsRegListener
  notifyEnabledStateChanged(aEnabled) {},

  notifyPreferredProfileChanged(aProfile) {},

  notifyCapabilityChanged(aCapability, aUnregisteredReason) {
    debug("notifyCapabilityChanged: aCapability = " + aCapability);
    if (
      aCapability != Ci.nsIImsRegHandler.IMS_CAPABILITY_VOICE_OVER_WIFI &&
      aCapability != Ci.nsIImsRegHandler.IMS_CAPABILITY_VIDEO_OVER_WIFI
    ) {
      WifiManager.setWifiDisable();
    }
  },

  // nsIMobileConnectionListener
  notifyVoiceChanged() {},

  notifyDataChanged() {},

  notifyDataError(aMessage) {},

  notifyCFStateChanged(
    aAction,
    aReason,
    aNumber,
    aTimeSeconds,
    aServiceClass
  ) {},

  notifyEmergencyCbModeChanged(aActive, aTimeoutMs) {},

  notifyOtaStatusChanged(aStatus) {},

  notifyRadioStateChanged() {},

  notifyClirModeChanged(aMode) {},

  notifyLastKnownNetworkChanged() {
    let countryCode = PhoneNumberUtils.getCountryName().toUpperCase();
    if (countryCode != "" && countryCode !== this.lastKnownCountryCode) {
      debug("Set country code = " + countryCode);
      this.lastKnownCountryCode = countryCode;
      if (WifiManager.enabled) {
        WifiManager.setCountryCode(countryCode, function() {});
      }
    }
  },

  notifyLastKnownHomeNetworkChanged() {},

  notifyNetworkSelectionModeChanged() {},

  notifyDeviceIdentitiesChanged() {},

  notifySignalStrengthChanged() {},

  notifyModemRestart(aReason) {},

  pickWifiCountryCode() {
    if (this.lastKnownCountryCode) {
      return this.lastKnownCountryCode;
    }
    //FIXME: NS_ERROR_FILE_NOT_FOUND
    return "US"; //PhoneNumberUtils.getCountryName().toUpperCase();
  },

  // nsIIccListener
  notifyIccInfoChanged() {},

  notifyStkCommand(aStkProactiveCmd) {},

  notifyStkSessionEnd() {},

  notifyCardStateChanged() {},

  notifyIsimInfoChanged() {},

  isAirplaneMode() {
    let airplaneMode = false;

    if (
      this._airplaneMode &&
      (this._airplaneMode_status === "enabling" ||
        this._airplaneMode_status === "enabled")
    ) {
      airplaneMode = true;
    }
    return airplaneMode;
  },

  handleScanResults(scanResults) {
    WifiNetworkSelector.selectNetwork(
      scanResults,
      WifiConfigManager.configuredNetworks,
      WifiManager.linkDebouncing,
      translateState(WifiManager.state),
      wifiInfo,
      function(candidate) {
        if (candidate == null) {
          debug("Can not find any suitable candidates");
        } else {
          debug("candidate = " + uneval(candidate));
          var currentAssociationId =
            wifiInfo.wifiSsid + ":" + wifiInfo.networkId;
          var targetAssociationId = candidate.ssid + ":" + candidate.netId;

          if (
            candidate.bssid == wifiInfo.bssid &&
            WifiManager.isConnectState(WifiManager.state)
          ) {
            debug(currentAssociationId + " is already the best choice!");
          } else if (
            candidate.netId == wifiInfo.networkId ||
            candidate.networkKey ==
              escape(wifiInfo.wifiSsid) + wifiInfo.security
          ) {
            debug(
              "Roaming from " +
                currentAssociationId +
                " to " +
                targetAssociationId
            );
            WifiManager.lastDriverRoamAttempt = 0;
            WifiManager.setAutoRoam(candidate, function() {});
          } else {
            debug(
              "reconnect from " +
                currentAssociationId +
                " to " +
                targetAssociationId
            );
            WifiManager.setAutoConnect(candidate, function() {});
          }
        }
      }
    );
  },

  handleNetworkConnectionFailure(network, callback) {
    let self = this;
    let netId = network.netId;
    let currentKey = WifiConfigUtils.getNetworkKey(network);
    const MIN_PRIORITY = 0;

    if (netId !== INVALID_NETWORK_ID) {
      WifiManager.disableNetwork(netId, function() {
        debug("disable network - id: " + netId);
      });
    }
    if (callback) {
      callback(true);
    }
  },

  handlePromptUnvalidatedId: null,
  handlePromptUnvalidated() {
    this.handlePromptUnvalidatedId = null;
    if (lastNetwork == null) {
      return;
    }
    debug("handlePromptUnvalidated: " + uneval(lastNetwork));
    if (lastNetwork.everValidated || lastNetwork.everCaptivePortalDetected) {
      return;
    }
    this._fireEvent("wifihasinternet", {
      hasInternet: false,
      network: netToDOM(lastNetwork),
    });
  },

  // Internal methods.
  waitForScan(callback) {
    this.wantScanResults.push(callback);
  },

  _startConnectionInfoTimer() {
    if (this._connectionInfoTimer) {
      return;
    }

    var self = this;
    function getConnectionInformation() {
      // WifiManager.getConnectionInfo(function(connInfo) {
      //   // See comments in WifiConfigUtils.calculateSignal for information about this.
      //   if (!connInfo) {
      //     self._lastConnectionInfo = null;
      //     return;
      //   }
      //   let { rssi, linkspeed, frequency } = connInfo;
      //   if (rssi > 0)
      //     rssi -= 256;
      //   if (rssi <= WifiConfigUtils.MIN_RSSI)
      //     rssi = WifiConfigUtils.MIN_RSSI;
      //   else if (rssi >= WifiConfigUtils.MAX_RSSI)
      //     rssi = WifiConfigUtils.MAX_RSSI;
      //   if (shouldBroadcastRSSIForIMS(rssi, wifiInfo.rssi)) {
      //     self.deliverListenerEvent("notifyRssiChanged", [rssi]);
      //   }
      //   wifiInfo.setRssi(rssi);
      //   wifiInfo.setLinkSpeed(linkspeed);
      //   wifiInfo.setFrequency(frequency);
      //   let info = { signalStrength: rssi,
      //                relSignalStrength: WifiConfigUtils.calculateSignal(rssi),
      //                linkSpeed: linkspeed,
      //                ipAddress: self.ipAddress };
      //   let last = self._lastConnectionInfo;
      //   // Only fire the event if the link speed changed or the signal
      //   // strength changed by more than 10%.
      //   function tensPlace(percent) {
      //     return (percent / 10) | 0;
      //   }
      //   if (last && last.linkSpeed === info.linkSpeed &&
      //       last.ipAddress === info.ipAddress &&
      //       tensPlace(last.relSignalStrength) === tensPlace(info.relSignalStrength)) {
      //     return;
      //   }
      //   self._lastConnectionInfo = info;
      //   if (WifiManager.linkDebouncing) {
      //     debug("linkDebouncing, don't fire connection info update");
      //     return;
      //   }
      //   debug("Firing connectioninfoupdate: " + uneval(info));
      //   self._fireEvent("connectioninfoupdate", info);
      // });
    }

    // Prime our _lastConnectionInfo immediately and fire the event at the
    // same time.
    getConnectionInformation();

    // Now, set up the timer for regular updates.
    this._connectionInfoTimer = Cc["@mozilla.org/timer;1"].createInstance(
      Ci.nsITimer
    );
    this._connectionInfoTimer.init(
      getConnectionInformation,
      3000,
      Ci.nsITimer.TYPE_REPEATING_SLACK
    );
  },

  _stopConnectionInfoTimer() {
    if (!this._connectionInfoTimer) {
      return;
    }

    this._connectionInfoTimer.cancel();
    this._connectionInfoTimer = null;
    this._lastConnectionInfo = null;
  },

  // nsIWifi

  _domManagers: [],
  _fireEvent(message, data) {
    this._domManagers.forEach(function(manager) {
      // Note: We should never have a dead message manager here because we
      // observe our child message managers shutting down, below.
      manager.sendAsyncMessage("WifiManager:" + message, data);
    });
  },

  _sendMessage(message, success, data, msg) {
    try {
      msg.manager.sendAsyncMessage(message + (success ? ":OK" : ":NO"), {
        data,
        rid: msg.rid,
        mid: msg.mid,
      });
    } catch (e) {
      debug("sendAsyncMessage error : " + e);
    }
    this._splicePendingRequest(msg);
  },

  _domRequest: [],

  _splicePendingRequest(msg) {
    for (let i = 0; i < this._domRequest.length; i++) {
      if (this._domRequest[i].msg === msg) {
        this._domRequest.splice(i, 1);
        return;
      }
    }
  },

  _clearPendingRequest() {
    if (this._domRequest.length === 0) {
      return;
    }
    this._domRequest.forEach(
      function(req) {
        this._sendMessage(
          req.name + ":Return",
          false,
          "Wifi is disabled",
          req.msg
        );
      }.bind(this)
    );
  },

  receiveMessage: function MessageManager_receiveMessage(aMessage) {
    let msg = aMessage.data || {};
    msg.manager = aMessage.target;

    if (WifiManager.p2pSupported()) {
      // If p2pObserver returns something truthy, return it!
      // Otherwise, continue to do the rest of tasks.
      var p2pRet = this._p2pObserver.onDOMMessage(aMessage);
      if (p2pRet) {
        return p2pRet;
      }
    }

    // Note: By the time we receive child-process-shutdown, the child process
    // has already forgotten its permissions so we do this before the
    // permissions check.
    if (aMessage.name === "child-process-shutdown") {
      let i;
      if ((i = this._domManagers.indexOf(msg.manager)) != -1) {
        this._domManagers.splice(i, 1);
      }
      for (i = this._domRequest.length - 1; i >= 0; i--) {
        if (this._domRequest[i].msg.manager === msg.manager) {
          this._domRequest.splice(i, 1);
        }
      }
      return;
    }

    // FIXME: aMessage.target.assertPermission is not a function
    // if (!aMessage.target.assertPermission("wifi-manage")) {
    //   return;
    // }

    // We are interested in DOMRequests only.
    if (aMessage.name != "WifiManager:getState") {
      this._domRequest.push({ name: aMessage.name, msg });
    }

    switch (aMessage.name) {
      case "WifiManager:setWifiEnabled":
        this.setWifiEnabled(msg);
        break;
      case "WifiManager:getNetworks":
        this.getNetworks(msg);
        break;
      case "WifiManager:getKnownNetworks":
        this.getKnownNetworks(msg);
        break;
      case "WifiManager:associate":
        this.associate(msg);
        break;
      case "WifiManager:forget":
        this.forget(msg);
        break;
      case "WifiManager:wps":
        this.wps(msg);
        break;
      case "WifiManager:setPowerSavingMode":
        this.setPowerSavingMode(msg);
        break;
      case "WifiManager:setHttpProxy":
        this.setHttpProxy(msg);
        break;
      case "WifiManager:setStaticIpMode":
        this.setStaticIpMode(msg);
        break;
      case "WifiManager:importCert":
        this.importCert(msg);
        break;
      case "WifiManager:getImportedCerts":
        this.getImportedCerts(msg);
        break;
      case "WifiManager:deleteCert":
        this.deleteCert(msg);
        break;
      case "WifiManager:setWifiTethering":
        this.setWifiTethering(msg);
        break;
      case "WifiManager:getState": {
        let i;
        if ((i = this._domManagers.indexOf(msg.manager)) === -1) {
          this._domManagers.push(msg.manager);
        }

        let net = lastNetwork ? netToDOM(lastNetwork) : null;
        return {
          network: net,
          connectionInfo: this._lastConnectionInfo,
          enabled: WifiManager.enabled,
          status: translateState(WifiManager.state),
          macAddress: this._macAddress,
          capabilities: WifiManager.getCapabilities(),
        };
      }
    }
  },

  getNetworks(msg) {
    const message = "WifiManager:getNetworks:Return";
    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    let sent = false;
    let callback = function(networks) {
      if (sent) {
        return;
      }
      sent = true;
      this._sendMessage(message, networks !== null, networks, msg);
    }.bind(this);
    this.waitForScan(callback);

    // TODO: set 2.4G only, should check if device support 5G
    WifiScanSettings.bandMask = WifiScanSettings.BAND_2_4_GHZ;
    WifiManager.handleScanRequest(
      function(ok) {
        // If the scan command succeeded, we're done.
        if (ok) {
          return;
        }

        // Avoid sending multiple responses.
        if (sent) {
          return;
        }

        // Otherwise, let the client know that it failed, it's responsible for
        // trying again in a few seconds.
        sent = true;
        this._sendMessage(message, false, "ScanFailed", msg);
      }.bind(this)
    );
  },

  getWifiScanResults(callback) {
    var count = 0;
    var timer = null;
    var self = this;

    if (!WifiManager.enabled) {
      callback.onfailure();
      return;
    }

    self.waitForScan(waitForScanCallback);
    doScan();
    function doScan() {
      WifiManager.handleScanRequest(function(ok) {
        if (!ok) {
          if (!timer) {
            count = 0;
            timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          }

          if (count++ >= 3) {
            timer = null;
            self.wantScanResults.splice(
              self.wantScanResults.indexOf(waitForScanCallback),
              1
            );
            callback.onfailure();
            return;
          }

          // Else it's still running, continue waiting.
          timer.initWithCallback(doScan, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
        }
      });
    }

    function waitForScanCallback(networks) {
      if (networks === null) {
        callback.onfailure();
        return;
      }

      var wifiScanResults = new Array();
      var net;
      for (let net in networks) {
        let value = networks[net];
        wifiScanResults.push(transformResult(value));
      }
      callback.onready(wifiScanResults.length, wifiScanResults);
    }

    function transformResult(element) {
      var result = new WifiScanResult();
      result.connected = false;
      for (let id in element) {
        if (id === "__exposedProps__") {
          continue;
        }
        if (id === "security") {
          result[id] = 0;
          var security = element[id];
          if (
            security === "WPA-PSK" ||
            security === "WPA2-PSK" ||
            security === "WPA/WPA2-PSK"
          ) {
            result[id] = Ci.nsIWifiScanResult.WPA_PSK;
          } else if (security === "WPA-EAP") {
            result[id] = Ci.nsIWifiScanResult.WPA_EAP;
          } else if (security === "WEP") {
            result[id] = Ci.nsIWifiScanResult.WEP;
          } else if (security === "WAPI-PSK") {
            result[id] = Ci.nsIWifiScanResult.WAPI_PSK;
          } else if (security === "WAPI-CERT") {
            result[id] = Ci.nsIWifiScanResult.WAPI_CERT;
          } else {
            result[id] = 0;
          }
        } else {
          result[id] = element[id];
        }
      }
      return result;
    }
  },

  registerListener(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }
    this._listeners.push(aListener);
  },

  unregisterListener(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index >= 0) {
      this._listeners.splice(index, 1);
    }
  },

  deliverListenerEvent(aName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) === -1) {
        continue;
      }
      let handler = listener[aName];
      if (typeof handler != "function") {
        throw new Error("No handler for " + aName);
      }
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        if (DEBUG) {
          this._debug("listener for " + aName + " threw an exception: " + e);
        }
      }
    }
  },

  _macAddress: null,
  get macAddress() {
    return this._macAddress;
  },

  getKnownNetworks(msg) {
    const message = "WifiManager:getKnownNetworks:Return";
    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }
    var networks = [];
    var configuredNetworks = WifiConfigManager.configuredNetworks;
    for (let networkKey in configuredNetworks) {
      networks.push(netToDOM(configuredNetworks[networkKey]));
    }
    this._sendMessage(message, true, networks, msg);
  },

  handleWifiEnabled(enabled, callback) {
    // FIXME: this would also return current request.
    // Reply error to pending requests.
    // if (!enabled) {
    //   this._clearPendingRequest();
    // }
    WifiManager.setWifiEnabled(enabled, callback);
  },

  handleWifiEnabledCallback(message, msg, result) {
    if (!result) {
      this.requestDone();
      this._sendMessage(message, false, "Set wifi enabled failed", msg);
      return;
    }

    this._sendMessage(message, true, true, msg);
    if (WifiManager.supplicantStarted) {
      WifiManager.supplicantConnected();
    } else {
      this._clearPendingRequest();
    }
    this.requestDone();
  },

  /**
   * Compatibility flags for detecting if Gaia is controlling wifi by settings
   * or API, once API is called, gecko will no longer accept wifi enable
   * control from settings.
   * This is used to deal with compatibility issue while Gaia adopted to use
   * API but gecko doesn't remove the settings code in time.
   * TODO: Remove this flag in Bug 1050147
   */
  setWifiEnabled(msg) {
    const message = "WifiManager:setWifiEnabled:Return";
    let self = this;
    let enabled = msg.data;
    // No change.
    if (enabled === WifiManager.enabled) {
      this._sendMessage(message, true, true, msg);
      return;
    }

    // Make sure Wifi hotspot is idle before switching to Wifi mode.
    if (enabled && WifiManager.isWifiTetheringEnabled(WifiManager.tetheringState)) {
      this.queueRequest({ command: "setWifiApEnabled", value: false }, function(data) {
        this.disconnectedByWifi = true;
        this.handleHotspotEnabled(false, this.handleHotspotEnabledCallback.bind(this, message, msg));
      }.bind(this));
    }
    this.queueRequest({ command: "setWifiEnabled", value: enabled }, function(data) {
      this.handleWifiEnabled(enabled, this.handleWifiEnabledCallback.bind(this, message, msg));
    }.bind(this));

    if (!enabled) {
      // TODO: NS_ERROR_UNEXPECTED on nsIPrefBranch.getBoolPref
      //let isWifiAffectTethering = Services.prefs.getBoolPref("wifi.affect.tethering");
      if (/* isWifiAffectTethering && */this.disconnectedByWifi && this.isAirplaneMode() === false) {
        this.queueRequest({ command: "setWifiApEnabled", value: true }, function(data) {
          this.handleHotspotEnabled(true, this.handleHotspotEnabledCallback.bind(this, message, msg));
        }.bind(this));
      }
      this.disconnectedByWifi = false;
    }
  },

  // requestDone() must be called to before callback complete(or error)
  // so next queue in the request quene can be executed.
  // TODO: Remove command queue in Bug 1050147
  queueRequest(data, callback) {
    if (!callback) {
      throw "Try to enqueue a request without callback";
    }

    let optimizeCommandList = ["setWifiEnabled", "setWifiApEnabled"];
    if (optimizeCommandList.indexOf(data.command) != -1) {
      this._stateRequests = this._stateRequests.filter(function(element) {
        return element.data.command !== data.command;
      });
    }

    this._stateRequests.push({
      data,
      callback,
    });

    this.nextRequest();
  },

  getWifiTetheringParameters(enable) {
    if (this.useTetheringAPI) {
      return this.getWifiTetheringConfiguration(enable);
    }
    return this.getWifiTetheringParametersBySetting(enable);
  },

  getWifiTetheringConfiguration(
    enable
  ) {
    let config = {};
    let params = this.tetheringConfig;
    // TODO: need more conditions if we support 5GHz hotspot.
    // Random choose 1, 6, 11 if there's no specify channel.
    let defaultChannel = 11 - Math.floor(Math.random() * 3) * 5;

    let check = function(field, _default) {
      config[field] = field in params ? params[field] : _default;
    };

    check("ssid", DEFAULT_HOTSPOT_SSID);
    check("security", DEFAULT_HOTSPOT_SECURITY_TYPE);
    check("key", DEFAULT_HOTSPOT_SECURITY_PASSWORD);
    check("hidden", DEFAULT_HOTSPOT_HIDDEN);
    check("band", DEFAULT_HOTSPOT_BAND);
    check("channel", DEFAULT_HOTSPOT_CHANNEL);
    check("enable11N", DEFAULT_HOTSPOT_ENABLE_11N);
    check("enable11AC", DEFAULT_HOTSPOT_ENABLE_11AC);
    check("enableACS", DEFAULT_HOTSPOT_ENABLE_ACS);
    check("ip", DEFAULT_HOTSPOT_IP);
    check("prefix", DEFAULT_HOTSPOT_PREFIX);
    check("wifiStartIp", DEFAULT_HOTSPOT_DHCPSERVER_STARTIP);
    check("wifiEndIp", DEFAULT_HOTSPOT_DHCPSERVER_ENDIP);
    check("usbStartIp", DEFAULT_USB_DHCPSERVER_STARTIP);
    check("usbEndIp", DEFAULT_USB_DHCPSERVER_ENDIP);
    check("dns1", DEFAULT_DNS1);
    check("dns2", DEFAULT_DNS2);
    check("channel", defaultChannel);

    config.enable = enable;
    config.mode = enable ? WIFI_FIRMWARE_AP : WIFI_FIRMWARE_STATION;
    config.link = enable ? NETWORK_INTERFACE_UP : NETWORK_INTERFACE_DOWN;

    // Check the format to prevent netd from crash.
    if (enable) {
      if (!config.ssid || config.ssid == "") {
        debug("Invalid SSID value.");
        return null;
      }

      if (config.security == WIFI_SECURITY_TYPE_NONE) {
        config.keyMgmt = AP_SECURITY_NONE;
      } else if (config.security == WIFI_SECURITY_TYPE_WPA_PSK) {
        config.keyMgmt = AP_SECURITY_WPA;
      } else if (config.security == WIFI_SECURITY_TYPE_WPA2_PSK) {
        config.keyMgmt = AP_SECURITY_WPA2;
      } else {
        debug("Invalid softap security type");
        return null;
      }

      if (config.security != WIFI_SECURITY_TYPE_NONE && !config.key) {
        debug("Invalid security password.");
        return null;
      }

      config.countryCode = this.pickWifiCountryCode();
      if (config.countryCode === "" && config.band == AP_BAND_5GHZ) {
        debug("Failed to set 5G hotspot without country code.");
        return null;
      }

      if (config.enableACS) {
        config.acsExcludeDfs = true;
      }
    }

    // Using the default values here until application supports these settings.
    if (
      config.ip == "" ||
      config.prefix == "" ||
      config.wifiStartIp == "" ||
      config.wifiEndIp == "" ||
      config.usbStartIp == "" ||
      config.usbEndIp == ""
    ) {
      debug("Invalid subnet information.");
      return null;
    }

    if (config.channel > 11 || config.channel < 1) {
      debug("Invalid channel value.");
      return null;
    }

    return config;
  },

  getWifiTetheringParametersBySetting(
    enable
  ) {
    let ssid;
    let securityType;
    let securityId;
    let channel;
    let interfaceIp;
    let prefix;
    let wifiDhcpStartIp;
    let wifiDhcpEndIp;
    let usbDhcpStartIp;
    let usbDhcpEndIp;
    let dns1;
    let dns2;

    ssid = this.tetheringSettings[SETTINGS_WIFI_SSID];
    securityType = this.tetheringSettings[SETTINGS_WIFI_SECURITY_TYPE];
    securityId = this.tetheringSettings[SETTINGS_WIFI_SECURITY_PASSWORD];
    channel = this.tetheringSettings[SETTINGS_WIFI_CHANNEL];
    interfaceIp = this.tetheringSettings[SETTINGS_WIFI_IP];
    prefix = this.tetheringSettings[SETTINGS_WIFI_PREFIX];
    wifiDhcpStartIp = this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_STARTIP];
    wifiDhcpEndIp = this.tetheringSettings[SETTINGS_WIFI_DHCPSERVER_ENDIP];
    usbDhcpStartIp = this.tetheringSettings[SETTINGS_USB_DHCPSERVER_STARTIP];
    usbDhcpEndIp = this.tetheringSettings[SETTINGS_USB_DHCPSERVER_ENDIP];
    dns1 = this.tetheringSettings[SETTINGS_WIFI_DNS1];
    dns2 = this.tetheringSettings[SETTINGS_WIFI_DNS2];

    // Check the format to prevent netd from crash.
    if (!ssid || ssid == "") {
      debug("Invalid SSID value.");
      return null;
    }
    // Truncate ssid if its length of encoded to utf8 is longer than 32.
    while (unescape(encodeURIComponent(ssid)).length > 32) {
      ssid = ssid.substring(0, ssid.length - 1);
    }

    if (
      securityType != WIFI_SECURITY_TYPE_NONE &&
      securityType != WIFI_SECURITY_TYPE_WPA_PSK &&
      securityType != WIFI_SECURITY_TYPE_WPA2_PSK
    ) {
      debug("Invalid security type.");
      return null;
    }
    if (securityType != WIFI_SECURITY_TYPE_NONE && !securityId) {
      debug("Invalid security password.");
      return null;
    }
    // Using the default values here until application supports these settings.
    if (
      interfaceIp == "" ||
      prefix == "" ||
      wifiDhcpStartIp == "" ||
      wifiDhcpEndIp == "" ||
      usbDhcpStartIp == "" ||
      usbDhcpEndIp == ""
    ) {
      debug("Invalid subnet information.");
      return null;
    }

    return {
      ssid,
      security: securityType,
      key: securityId,
      channel,
      ip: interfaceIp,
      prefix,
      wifiStartIp: wifiDhcpStartIp,
      wifiEndIp: wifiDhcpEndIp,
      usbStartIp: usbDhcpStartIp,
      usbEndIp: usbDhcpEndIp,
      dns1,
      dns2,
      enable,
      mode: enable ? WIFI_FIRMWARE_AP : WIFI_FIRMWARE_STATION,
      link: enable ? NETWORK_INTERFACE_UP : NETWORK_INTERFACE_DOWN,
    };
  },

  handleHotspotEnabled(enabled, callback) {
    let configuration = this.getWifiTetheringParameters(enabled);

    if (!configuration) {
      debug("Invalid Wifi Tethering configuration.");
      this.requestDone();
      callback(false);
      return;
    }
    WifiManager.setWifiApEnabled(enabled, configuration, callback);
  },

  handleHotspotEnabledCallback(message, msg, result) {
    if (!result) {
      msg.data.reason = msg.data.enabled ?
        "Enable WIFI tethering failed" :
        "Disable WIFI tethering failed";
      this._sendMessage(message, false, msg.data, msg);
    } else {
      this._sendMessage(message, true, msg.data, msg);
    }
    this.requestDone();
  },

  // TODO : These two variables should be removed once GAIA uses tethering API.
  useTetheringAPI: false,
  tetheringConfig: {},

  setWifiTethering(msg) {
    const message = "WifiManager:setWifiTethering:Return";
    let self = this;
    let enabled = msg.data.enabled;

    this.useTetheringAPI = true;
    this.tetheringConfig = msg.data.config;

    if (enabled && WifiManager.enabled == true) {
      this.queueRequest({ command: "setWifiEnabled", value: false }, function(data) {
        this.disconnectedByWifiTethering = true;
        this.handleWifiEnabled(false, this.handleWifiEnabledCallback.bind(this, message, msg));
      }.bind(this));
    }
    this.queueRequest({ command: "setWifiApEnabled", value: enabled }, function(data) {
      this.handleHotspotEnabled(enabled, this.handleHotspotEnabledCallback.bind(this, message, msg));
    }.bind(this));

    if (!enabled) {
      // TODO: NS_ERROR_UNEXPECTED on nsIPrefBranch.getBoolPref
      // let isTetheringAffectWifi = Services.prefs.getBoolPref("tethering.affect.wifi");
      if (/* isTetheringAffectWifi && */this.disconnectedByWifiTethering && this.isAirplaneMode() === false) {
        this.queueRequest({ command: "setWifiEnabled", value: true }, function(data) {
          this.handleWifiEnabled(true, this.handleWifiEnabledCallback.bind(this, message, msg));
        }.bind(this));
      }
      this.disconnectedByWifiTethering = false;
    }
  },

  associate(msg) {
    const MAX_PRIORITY = 9999;
    const message = "WifiManager:associate:Return";
    let network = msg.data;

    let privnet = network;
    let dontConnect = privnet.dontConnect;
    delete privnet.dontConnect;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }
    let self = this;
    function networkReady() {
      function selectAndConnect() {
        WifiManager.loopDetectionCount = 0;
        WifiConfigManager.setAndEnableLastSelectedConfiguration(
          privnet.netId,
          function() {
            WifiManager.removeNetworks(function(ok) {
              if (!ok) {
                this._sendMessage(message, false, "Failed to remove networks", msg);
                return;
              }
              WifiManager.connect(privnet, function (ok) {
                self._sendMessage(message, ok, ok, msg);
              });
            });
          }
        );
      }
      var selectAndConnectOrReturn = dontConnect ?
        function() {
          self._sendMessage(message, true, "Wifi has been recorded", msg);
        } : selectAndConnect;
      if (self._highestPriority >= MAX_PRIORITY) {
        // FIXME: check priority
      } else {
        selectAndConnectOrReturn();
      }
    }

    let networkKey = WifiConfigUtils.getNetworkKey(privnet);
    let configured;

    if (networkKey in this._addingNetworks) {
      this._sendMessage(message, false, "Racing associates");
      return;
    }

    var configuredNetworks = WifiConfigManager.configuredNetworks;
    if (networkKey in configuredNetworks) {
      configured = configuredNetworks[networkKey];
    }

    netFromDOM(privnet, configured);

    privnet.priority = ++this._highestPriority;
    debug("Device associate with ap info: " + uneval(privnet));

    if (configured) {
      // network already saved, should keep previous network id
      privnet.netId = configured.netId;
    }

    this._addingNetworks[networkKey] = privnet;
    WifiManager.saveNetwork(
      privnet,
      function(ok) {
        delete this._addingNetworks[networkKey];

        if (!ok) {
          this._sendMessage(message, false, "Network is misconfigured", msg);
          return;
        }

        // Supplicant will connect to access point directly without disconnect
        // if we are currently associated, hence trigger a disconnect
        if (
          WifiManager.state == "COMPLETED" &&
          lastNetwork &&
          lastNetwork.netId !== INVALID_NETWORK_ID &&
          lastNetwork.netId !== privnet.netId
        ) {
          WifiManager.disconnect(function() {
            networkReady();
          });
        } else {
          networkReady();
        }
      }.bind(this)
    );
  },

  forget(msg) {
    const message = "WifiManager:forget:Return";
    let self = this;
    let network = msg.data;
    debug("Device forget with ap info: " + uneval(network));
    if (!WifiManager.enabled) {
      self._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    let networkKey = WifiConfigUtils.getNetworkKey(network);
    var configuredNetworks = WifiConfigManager.configuredNetworks;
    if (!(networkKey in configuredNetworks)) {
      self._sendMessage(
        message,
        false,
        "Trying to forget an unknown network",
        msg
      );
      return;
    }

    let configured = configuredNetworks[networkKey];
    WifiManager.removeNetwork(configured.netId, function(ok) {
      if (!ok) {
        self._sendMessage(message, false, "Unable to remove the network", msg);
        return;
      }
      WifiManager.disconnect(function() {});
      self._sendMessage(message, true, true, msg);
    });
  },

  wps(msg) {
    const message = "WifiManager:wps:Return";
    let self = this;
    let detail = msg.data;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    if (detail.method === "pbc") {
      WifiManager.wpsPbc(function(ok) {
        if (ok) {
          WifiManager.wpsStarted = true;
          self._sendMessage(message, true, true, msg);
        } else {
          WifiManager.wpsStarted = false;
          self._sendMessage(message, false, "WPS PBC failed", msg);
        }
      });
    } else if (detail.method === "pin") {
      WifiManager.wpsPin(detail, function(pin) {
        if (pin) {
          WifiManager.wpsStarted = true;
          self._sendMessage(message, true, pin, msg);
        } else {
          WifiManager.wpsStarted = false;
          self._sendMessage(message, false, "WPS PIN failed", msg);
        }
      });
    } else if (detail.method === "cancel") {
      WifiManager.wpsCancel(function(ok) {
        if (ok) {
          WifiManager.wpsStarted = false;
          self._sendMessage(message, true, true, msg);
        } else {
          self._sendMessage(message, false, "WPS Cancel failed", msg);
        }
      });
    } else {
      self._sendMessage(
        message,
        false,
        "Invalid WPS method=" + detail.method,
        msg
      );
    }
  },

  setPowerSavingMode(msg) {
    const message = "WifiManager:setPowerSavingMode:Return";
    let self = this;
    let enabled = msg.data;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    // Some wifi drivers may not implement this command. Set power mode
    // even if suspend optimization command failed.
    WifiManager.setSuspendOptimizationsMode(
      POWER_MODE_SETTING_CHANGED,
      enabled,
      function(ok) {
        if (ok) {
          self._sendMessage(message, true, true, msg);
        } else {
          self._sendMessage(
            message,
            false,
            "Set power saving mode failed",
            msg
          );
        }
      }
    );
  },

  setHttpProxy(msg) {
    const message = "WifiManager:setHttpProxy:Return";
    let self = this;
    let network = msg.data.network;
    let info = msg.data.info;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    WifiManager.configureHttpProxy(network, info, function(ok) {
      if (ok) {
        // If configured network is current connected network
        // need update http proxy immediately.
        let setNetworkKey = WifiConfigUtils.getNetworkKey(network);
        let lastNetworkKey = lastNetwork
          ? WifiConfigUtils.getNetworkKey(lastNetwork)
          : null;
        if (setNetworkKey === curNetworkKey) {
          WifiManager.setHttpProxy(network);
        }

        self._sendMessage(message, true, true, msg);
      } else {
        self._sendMessage(message, false, "Set http proxy failed", msg);
      }
    });
  },

  setStaticIpMode(msg) {
    const message = "WifiManager:setStaticIpMode:Return";
    let self = this;
    let network = msg.data.network;
    let info = msg.data.info;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    // To compatiable with DHCP returned info structure, do translation here
    info.ipaddr_str = info.ipaddr;
    info.proxy_str = info.proxy;
    info.gateway_str = info.gateway;
    info.dns1_str = info.dns1;
    info.dns2_str = info.dns2;

    WifiManager.setStaticIpMode(network, info, function(ok) {
      if (ok) {
        self._sendMessage(message, true, true, msg);
      } else {
        self._sendMessage(message, false, "Set static ip mode failed", msg);
      }
    });
  },

  importCert(msg) {
    const message = "WifiManager:importCert:Return";
    let self = this;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    WifiManager.importCert(msg.data, function(data) {
      if (data.status === 0) {
        let usageString = ["ServerCert", "UserCert"];
        let usageArray = [];
        for (let i = 0; i < usageString.length; i++) {
          if (data.usageFlag & (0x01 << i)) {
            usageArray.push(usageString[i]);
          }
        }

        self._sendMessage(
          message,
          true,
          {
            nickname: data.nickname,
            usage: usageArray,
          },
          msg
        );
      } else if (data.duplicated) {
        self._sendMessage(message, false, "Import duplicate certificate", msg);
      } else {
        self._sendMessage(message, false, "Import damaged certificate", msg);
      }
    });
  },

  getImportedCerts(msg) {
    const message = "WifiManager:getImportedCerts:Return";
    let self = this;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    let certDB = Cc["@mozilla.org/security/x509certdb;1"].getService(
      Ci.nsIX509CertDB
    );
    if (!certDB) {
      self._sendMessage(message, false, "Failed to query NSS DB service", msg);
    }

    let certList = certDB.getCerts();
    if (!certList) {
      self._sendMessage(message, false, "Failed to get certificate List", msg);
    }

    let certListEnum = certList.getEnumerator();
    if (!certListEnum) {
      self._sendMessage(
        message,
        false,
        "Failed to get certificate List enumerator",
        msg
      );
    }
    let importedCerts = {
      ServerCert: [],
      UserCert: [],
    };
    let UsageMapping = {
      SERVERCERT: "ServerCert",
      USERCERT: "UserCert",
    };

    while (certListEnum.hasMoreElements()) {
      let certInfo = certListEnum.getNext().QueryInterface(Ci.nsIX509Cert);
      let certNicknameInfo = /WIFI\_([A-Z]*)\_(.*)/.exec(certInfo.nickname);
      if (!certNicknameInfo) {
        continue;
      }
      importedCerts[UsageMapping[certNicknameInfo[1]]].push(
        certNicknameInfo[2]
      );
    }

    self._sendMessage(message, true, importedCerts, msg);
  },

  deleteCert(msg) {
    const message = "WifiManager:deleteCert:Return";
    let self = this;

    if (!WifiManager.enabled) {
      this._sendMessage(message, false, "Wifi is disabled", msg);
      return;
    }

    WifiManager.deleteCert(msg.data, function(data) {
      self._sendMessage(message, data.status === 0, "Delete Cert failed", msg);
    });
  },

  // This is a bit ugly, but works. In particular, this depends on the fact
  // that RadioManager never actually tries to get the worker from us.
  get worker() {
    throw "Not implemented";
  },

  shutdown() {
    debug("shutting down ...");
    this.queueRequest(
      { command: "setWifiEnabled", value: false },
      function(data) {
        this._setWifiEnabled(false, this._setWifiEnabledCallback.bind(this));
      }.bind(this)
    );
    Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
    Services.obs.removeObserver(this, kXpcomShutdownChangedTopic);
    Services.obs.removeObserver(this, kScreenStateChangedTopic);
    Services.obs.removeObserver(this, kInterfaceAddressChangedTopic);
    Services.obs.removeObserver(this, kInterfaceDnsInfoTopic);
    Services.obs.removeObserver(this, kRouteChangedTopic);
    Services.obs.removeObserver(this, kWifiCaptivePortalResult);
    Services.obs.removeObserver(this, kOpenCaptivePortalLoginEvent);
    Services.obs.removeObserver(this, kCaptivePortalLoginSuccessEvent);
    Services.prefs.removeObserver(kPrefDefaultServiceId, this);
  },

  // TODO: Remove command queue in Bug 1050147.
  requestProcessing: false, // Hold while dequeue and execution a request.
  // Released upon the request is fully executed,
  // i.e, mostly after callback is done.
  requestDone() {
    this.requestProcessing = false;
    this.nextRequest();
  },

  nextRequest() {
    // No request to process
    if (this._stateRequests.length === 0) {
      return;
    }

    // Handling request, wait for it.
    if (this.requestProcessing) {
      return;
    }

    // Hold processing lock
    this.requestProcessing = true;

    // Find next valid request
    let request = this._stateRequests.shift();

    request.callback(request.data);
  },

  _getDefaultServiceId() {
    //FIXME: NS_ERROR_UNEXPECTED: Component returned failure code: 0x8000ffff (NS_ERROR_UNEXPECTED) [nsIPrefBranch.getIntPref]
    let id = 0; //Services.prefs.getIntPref(kPrefDefaultServiceId);
    let numRil = 1; //Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);

    if (id >= numRil || id < 0) {
      id = 0;
    }

    return id;
  },

  // nsIObserver implementation
  observe(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        // To avoid WifiWorker setting the wifi again, don't need to deal with
        // the "mozsettings-changed" event fired from internal setting.
        if ("wrappedJSObject" in subject) {
          subject = subject.wrappedJSObject;
        }
        if (subject.isInternalChange) {
          return;
        }

        this.handle(subject.key, subject.value);
        break;

      case kXpcomShutdownChangedTopic:
        // Ensure the supplicant is detached from B2G to avoid XPCOM shutdown
        // blocks forever.
        WifiManager.ensureSupplicantDetached(() => {
          let wifiService = Cc["@mozilla.org/wifi/service;1"].getService(
            Ci.nsIWifiProxyService
          );
          wifiService.shutdown();
          let wifiCertService = Cc[
            "@mozilla.org/wifi/certservice;1"
          ].getService(Ci.nsIWifiCertService);
          wifiCertService.shutdown();
        });
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        Services.obs.removeObserver(this, kXpcomShutdownChangedTopic);
        Services.obs.removeObserver(this, kScreenStateChangedTopic);
        Services.obs.removeObserver(this, kInterfaceAddressChangedTopic);
        Services.obs.removeObserver(this, kInterfaceDnsInfoTopic);
        Services.obs.removeObserver(this, kRouteChangedTopic);
        Services.obs.removeObserver(this, kWifiCaptivePortalResult);
        Services.obs.removeObserver(this, kOpenCaptivePortalLoginEvent);
        Services.obs.removeObserver(this, kCaptivePortalLoginSuccessEvent);
        Services.prefs.removeObserver(kPrefDefaultServiceId, this);
        if (this.mobileConnectionRegistered) {
          // gMobileConnectionService
          //   .getItemByServiceId(WifiManager.telephonyServiceId).unregisterListener(this);
          this.mobileConnectionRegistered = false;
        }
        for (let simId = 0; simId < WifiManager.numRil; simId++) {
          gIccService.getIccByServiceId(simId).unregisterListener(this);
        }
        break;

      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (data === kPrefDefaultServiceId) {
          let defaultServiceId = this._getDefaultServiceId();
          if (defaultServiceId == WifiManager.telephonyServiceId) {
            return;
          }
          if (this.mobileConnectionRegistered) {
            // gMobileConnectionService
            //   .getItemByServiceId(WifiManager.telephonyServiceId).unregisterListener(this);
            this.mobileConnectionRegistered = false;
          }
          WifiManager.telephonyServiceId = defaultServiceId;
          // gMobileConnectionService
          //   .getItemByServiceId(WifiManager.telephonyServiceId).registerListener(this);
          this.mobileConnectionRegistered = true;
        }
        break;

      case kScreenStateChangedTopic:
        let enabled = data === "on";
        debug("Receive ScreenStateChanged=" + enabled);
        WifiManager.handleScreenStateChanged(enabled);
        break;

      case kInterfaceAddressChangedTopic:
        // Format: "Address updated <addr> <iface> <flags> <scope>"
        //         "Address removed <addr> <iface> <flags> <scope>"
        var token = data.split(" ");
        if (token.length < 6) {
          return;
        }
        var iface = token[3];
        if (iface.indexOf("wlan") === -1) {
          return;
        }
        var action = token[1];
        var addr = token[2].split("/")[0];
        var prefix = token[2].split("/")[1];

        // We only handle IPv6 route advertisement address here.
        if (addr.indexOf(":") == -1) {
          return;
        }

        WifiNetworkInterface.updateConfig(action, {
          ip: addr,
          prefixLengths: prefix,
        });
        break;

      case kInterfaceDnsInfoTopic:
        // Format: "DnsInfo servers <interface> <lifetime> <servers>"
        var token = data.split(" ");
        if (token.length !== 5) {
          return;
        }
        var iface = token[2];
        if (iface.indexOf("wlan") === -1) {
          return;
        }
        var dnses = token[4].split(",");
        WifiNetworkInterface.updateConfig("updated", { dnses });
        break;

      case kRouteChangedTopic:
        // Format: "Route <updated|removed> <dst> [via <gateway] [dev <iface>]"
        var token = data.split(" ");
        if (token.length < 7) {
          return;
        }
        var iface = null;
        var gateway = null;
        var action = token[1];
        var valid = true;
        for (let i = 3; i + 1 < token.length; i += 2) {
          if (token[i] == "dev") {
            if (iface == null) {
              iface = token[i + 1];
            } else {
              valid = false; // Duplicate interface.
            }
          } else if (token[i] == "via") {
            if (gateway == null) {
              gateway = token[i + 1];
            } else {
              valid = false; // Duplicate gateway.
            }
          } else {
            valid = false; // Unknown syntax.
          }
        }

        if (!valid || iface.indexOf("wlan") === -1) {
          return;
        }

        WifiNetworkInterface.updateConfig(action, { gateway });
        break;

      case kWifiCaptivePortalResult:
        if (!(subject instanceof Ci.nsIPropertyBag2)) {
          return;
        }
        if (
          WifiNetworkInterface.info.state !==
          Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED
        ) {
          debug(
            "ignore captive portal result event when network not connected"
          );
          return;
        }
        let props = subject.QueryInterface(Ci.nsIPropertyBag2);
        let landing = props.get("landing");
        if (lastNetwork == null) {
          debug("lastNetwork is null");
          return;
        }
        debug(
          "Receive lastNetwork : " +
            uneval(lastNetwork) +
            " landing = " +
            landing
        );
        lastNetwork.everValidated |= landing;
        this._fireEvent("wifihasinternet", {
          hasInternet: !!lastNetwork.everValidated,
          network: netToDOM(lastNetwork),
        });
        break;

      case kOpenCaptivePortalLoginEvent:
        if (
          WifiNetworkInterface.info.state !==
          Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED
        ) {
          debug("ignore captive portal login event when network not connected");
          return;
        }
        if (lastNetwork == null) {
          debug("lastNetwork is null");
          return;
        }
        debug(
          "Receive captive-portal-login, set lastNetwork : " +
            uneval(lastNetwork)
        );
        lastNetwork.everCaptivePortalDetected = true;
        this._fireEvent("captiveportallogin", {
          loginSuccess: false,
          network: netToDOM(lastNetwork),
        });
        break;
      case kCaptivePortalLoginSuccessEvent:
        if (
          WifiNetworkInterface.info.state !==
          Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED
        ) {
          debug("ignore captive portal login event when network not connected");
          return;
        }
        if (lastNetwork == null) {
          debug("lastNetwork is null");
          return;
        }
        debug("Receive captive-portal-login-success");
        this._fireEvent("captiveportallogin", {
          loginSuccess: true,
          network: netToDOM(lastNetwork),
        });
        break;
    }
  },

  handle(aName, aResult) {
    switch (aName) {
      // TODO: Remove function call in Bug 1050147.
      case SETTINGS_WIFI_ENABLED:
        this.handleWifiEnabled(aResult);
        break;
      case SETTINGS_WIFI_DEBUG_ENABLED:
        if (aResult === null) {
          aResult = false;
        }
        DEBUG = aResult;
        updateDebug();
        break;
      case SETTINGS_AIRPLANE_MODE:
        this._airplaneMode = aResult;
        break;
      case SETTINGS_AIRPLANE_MODE_STATUS:
        this._airplaneMode_status = aResult;
        break;
      case SETTINGS_WIFI_NOTIFYCATION:
        OpenNetworkNotifier.setOpenNetworkNotifyEnabled(aResult);
        break;
      case SETTINGS_WIFI_TETHERING_ENABLED:
        this._oldWifiTetheringEnabledState = this.tetheringSettings[
          SETTINGS_WIFI_TETHERING_ENABLED
        ];
      // Fall through!
      case SETTINGS_WIFI_SSID:
      case SETTINGS_WIFI_SECURITY_TYPE:
      case SETTINGS_WIFI_SECURITY_PASSWORD:
      case SETTINGS_WIFI_CHANNEL:
      case SETTINGS_WIFI_IP:
      case SETTINGS_WIFI_PREFIX:
      case SETTINGS_WIFI_DHCPSERVER_STARTIP:
      case SETTINGS_WIFI_DHCPSERVER_ENDIP:
      case SETTINGS_WIFI_DNS1:
      case SETTINGS_WIFI_DNS2:
      case SETTINGS_USB_DHCPSERVER_STARTIP:
      case SETTINGS_USB_DHCPSERVER_ENDIP:
        // TODO: code related to wifi-tethering setting should be removed after GAIA
        //       use tethering API
        if (this.useTetheringAPI) {
          break;
        }

        if (aResult !== null) {
          this.tetheringSettings[aName] = aResult;
        }
        debug("'" + aName + "'" + " is now " + this.tetheringSettings[aName]);
        let index = this._wifiTetheringSettingsToRead.indexOf(aName);

        if (index != -1) {
          this._wifiTetheringSettingsToRead.splice(index, 1);
        }

        if (this._wifiTetheringSettingsToRead.length) {
          debug(
            "We haven't read completely the wifi Tethering data from settings db."
          );
          break;
        }

        if (
          this._oldWifiTetheringEnabledState ===
          this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED]
        ) {
          debug(
            "No changes for SETTINGS_WIFI_TETHERING_ENABLED flag. Nothing to do."
          );
          break;
        }

        if (
          this._oldWifiTetheringEnabledState === null &&
          !this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED]
        ) {
          debug(
            "Do nothing when initial settings for SETTINGS_WIFI_TETHERING_ENABLED flag is false."
          );
          break;
        }

        this._oldWifiTetheringEnabledState = this.tetheringSettings[
          SETTINGS_WIFI_TETHERING_ENABLED
        ];
        this.handleWifiTetheringEnabled(aResult);
        break;
    }
  },

  handleError(aErrorMessage) {
    debug("There was an error while reading Tethering settings.");
    this.tetheringSettings = {};
    this.tetheringSettings[SETTINGS_WIFI_TETHERING_ENABLED] = false;
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([WifiWorker]);

var debug;
function updateDebug() {
  if (DEBUG) {
    debug = function(s) {
      console.log("-*- WifiWorker component: ", s, "\n");
    };
  } else {
    debug = function(s) {};
  }
  //WifiManager.syncDebug();
}
updateDebug();
