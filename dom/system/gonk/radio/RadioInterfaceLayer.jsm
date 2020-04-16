/* Copyright 2012 Mozilla Foundation and Mozilla contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

const { Services } = ChromeUtils.import(
  "resource://gre/modules/Services.jsm"
);

const { PromiseUtils } = ChromeUtils.import(
  "resource://gre/modules/PromiseUtils.jsm"
);

Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "SIM", function () {
  let obj = {};
  Cu.import("resource://gre/modules/simIOHelper.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

// Ril quirk to always turn the radio off for the client without SIM card
// except hw default client.
var RILQUIRKS_RADIO_OFF_WO_CARD =
  libcutils.property_get("ro.moz.ril.radio_off_wo_card", "false") == "true";

const RADIOINTERFACELAYER_CID =
  Components.ID("{2d831c8d-6017-435b-a80c-e5d422810cea}");
const RADIOINTERFACE_CID =
  Components.ID("{6a7c91f0-a2b3-4193-8562-8969296c0b54}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const kNetworkConnStateChangedTopic      = "network-connection-state-changed";
const kMozSettingsChangedObserverTopic   = "mozsettings-changed";
const kSysMsgListenerReadyObserverTopic  = "system-message-listener-ready";
const kSysClockChangeObserverTopic       = "system-clock-change";
const kScreenStateChangedTopic           = "screen-state-changed";

const kSettingsClockAutoUpdateEnabled = "time.clock.automatic-update.enabled";
const kSettingsClockAutoUpdateAvailable = "time.clock.automatic-update.available";
const kSettingsTimezoneAutoUpdateEnabled = "time.timezone.automatic-update.enabled";
const kSettingsTimezoneAutoUpdateAvailable = "time.timezone.automatic-update.available";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefAppCBConfigurationEnabled = "dom.app_cb_configuration";

const RADIO_POWER_OFF_TIMEOUT = 30000;
const HW_DEFAULT_CLIENT_ID = 0;

const NETWORK_TYPE_WIFI        = Ci.nsINetworkInfo.NETWORK_TYPE_WIFI;
const NETWORK_TYPE_MOBILE      = Ci.nsINetworkInfo.NETWORK_TYPE_MOBILE;

const INVALID_UPTIME = undefined;

// set to true in ril_consts.js to see debug messages
//var DEBUG = RIL.DEBUG_RIL;
var DEBUG = true;

function updateDebugFlag() {
  // Read debug setting from pref
  /*let debugPref;
  try {
    debugPref = Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
  } catch (e) {
    debugPref = false;
  }*/
  //DEBUG = RIL.DEBUG_RIL || debugPref;
  DEBUG = true;
}
updateDebugFlag();

function debug(s) {
  //dump("-*- RadioInterfaceLayer: " + s + "\n");
  console.log("-*- RadioInterfaceLayer: " + s + "\n");
}

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/gonkiccservice;1",
                                   "nsIGonkIccService");

// XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
//                                    "@mozilla.org/mobilemessage/mobilemessageservice;1",
//                                    "nsIMobileMessageService");

// XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
//                                    "@mozilla.org/sms/gonksmsservice;1",
//                                    "nsIGonkSmsService");

// XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
//                                    "@mozilla.org/parentprocessmessagemanager;1",
//                                    "nsIMessageBroadcaster");

// XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
//                                    "@mozilla.org/settingsService;1",
//                                    "nsISettingsService");

 XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                    "@mozilla.org/network/manager;1",
                                    "nsINetworkManager");

// XPCOMUtils.defineLazyServiceGetter(this, "gTimeService",
//                                    "@mozilla.org/time/timeservice;1",
//                                    "nsITimeService");

// XPCOMUtils.defineLazyServiceGetter(this, "gSystemWorkerManager",
//                                    "@mozilla.org/telephony/system-worker-manager;1",
//                                    "nsISystemWorkerManager");

XPCOMUtils.defineLazyServiceGetter(this, "gTelephonyService",
                                   "@mozilla.org/telephony/telephonyservice;1",
                                   "nsIGonkTelephonyService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIGonkMobileConnectionService");

// XPCOMUtils.defineLazyServiceGetter(this, "gCellBroadcastService",
//                                    "@mozilla.org/cellbroadcast/cellbroadcastservice;1",
//                                    "nsIGonkCellBroadcastService");

XPCOMUtils.defineLazyServiceGetter(this, "gStkCmdFactory",
                                   "@mozilla.org/icc/stkcmdfactory;1",
                                   "nsIStkCmdFactory");


XPCOMUtils.defineLazyServiceGetter(this, "gDataCallManager",
                                   "@mozilla.org/datacall/manager;1",
                                   "nsIDataCallManager");

XPCOMUtils.defineLazyServiceGetter(this, "gDataCallInterfaceService",
                                   "@mozilla.org/datacall/interfaceservice;1",
                                   "nsIGonkDataCallInterfaceService");

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gRilWorkerService",
  "@mozilla.org/rilworkerservice;1",
  "nsIRilWorkerService"
);

XPCOMUtils.defineLazyGetter(this, "gRadioEnabledController", function() {
  let _ril = null;
  let _pendingMessages = [];  // For queueing "setRadioEnabled" message.
  let _isProcessingPending = false;
  let _timer = null;
  let _request = null;
  let _deactivatingDeferred = {};
  let _initializedCardState = {};
  let _allCardStateInitialized = !RILQUIRKS_RADIO_OFF_WO_CARD;

  return {
    init: function(ril) {
      _ril = ril;
    },

    receiveCardState: function(clientId) {
      if (_allCardStateInitialized) {
        return;
      }

      if (DEBUG) debug("RadioControl: receive cardState from " + clientId);
      _initializedCardState[clientId] = true;
      if (Object.keys(_initializedCardState).length == _ril.numRadioInterfaces) {
        _allCardStateInitialized = true;
        this._startProcessingPending();
      }
    },

    setRadioEnabled: function(clientId, data, callback) {
      if (DEBUG) debug("setRadioEnabled: " + clientId + ": " + JSON.stringify(data));
      let message = {
        clientId: clientId,
        data: data,
        callback: callback
      };
      _pendingMessages.push(message);
      this._startProcessingPending();
    },

    notifyRadioStateChanged: function(clientId, radioState) {
      gMobileConnectionService.notifyRadioStateChanged(clientId, radioState);
    },

    _startProcessingPending: function() {
      if (!_isProcessingPending) {
        if (DEBUG) debug("RadioControl: start dequeue");
        _isProcessingPending = true;
        this._processNextMessage();
      }
    },

    _processNextMessage: function() {
      if (_pendingMessages.length === 0 || !_allCardStateInitialized) {
        if (DEBUG) debug("RadioControl: stop dequeue");
        _isProcessingPending = false;
        return;
      }

      let msg = _pendingMessages.shift();
      this._handleMessage(msg);
    },

    _getNumCards: function() {
      let numCards = 0;
      for (let i = 0, N = _ril.numRadioInterfaces; i < N; ++i) {
        if (_ril.getRadioInterface(i).isCardPresent()) {
          numCards++;
        }
      }
      return numCards;
    },

    _isRadioAbleToEnableAtClient: function(clientId, numCards) {
      if (!RILQUIRKS_RADIO_OFF_WO_CARD) {
        return true;
      }

      // We could only turn on the radio for clientId if
      // 1. a SIM card is presented or
      // 2. it is the default clientId and there is no any SIM card at any client.

      if (_ril.getRadioInterface(clientId).isCardPresent()) {
        return true;
      }

      numCards = numCards == null ? this._getNumCards() : numCards;
      if (clientId === HW_DEFAULT_CLIENT_ID && numCards === 0) {
        return true;
      }

      return false;
    },

    _handleMessage: function(message) {
      if (DEBUG) debug("RadioControl: handleMessage: " + JSON.stringify(message));
      let clientId = message.clientId || 0;
      let connection =
        gMobileConnectionService.getItemByServiceId(clientId);
      let radioState = connection && connection.radioState;

      if (message.data.enabled) {
        if (this._isRadioAbleToEnableAtClient(clientId)) {
          this._setRadioEnabledInternal(message);
        } else {
          // Not really do it but respond success.
          message.callback(message.data);
        }

        this._processNextMessage();
      } else {
        _request = this._setRadioEnabledInternal.bind(this, message);

        // In 2G network, modem takes 35+ seconds to process deactivate data
        // call request if device has active voice call (please see bug 964974
        // for more details). Therefore we should hangup all active voice calls
        // first. And considering some DSDS architecture, toggling one radio may
        // toggle both, so we send hangUpAll to all clients.
        let hangUpCallback = {
          QueryInterface: ChromeUtils.generateQI([Ci.nsITelephonyCallback]),
          notifySuccess: function() {},
          notifyError: function() {}
        };

        gTelephonyService.enumerateCalls({
          QueryInterface: ChromeUtils.generateQI([Ci.nsITelephonyListener]),
          enumerateCallState: function(aInfo) {
            gTelephonyService.hangUpCall(aInfo.clientId, aInfo.callIndex,
                                         hangUpCallback);
          },
          enumerateCallStateComplete: function() {}
        });

        // In some DSDS architecture with only one modem, toggling one radio may
        // toggle both. Therefore, for safely turning off, we should first
        // explicitly deactivate all data calls from all clients.
        this._deactivateDataCalls().then(() => {
          if (DEBUG) debug("RadioControl: deactivation done");
          this._executeRequest();
        });

        this._createTimer();
      }
    },

    _setRadioEnabledInternal: function(message) {
      let clientId = message.clientId || 0;
      let enabled = message.data.enabled || false;
      let radioInterface = _ril.getRadioInterface(clientId);

      radioInterface.sendRilRequest("setRadioEnabled", message.data,(function(response) {
        if (response.errorMsg) {
          // If request fails, set current radio state to unknown, since we will
          // handle it in |mobileConnectionService|.
          this.notifyRadioStateChanged(clientId,
                                       Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN);
        }
        debug("_setRadioEnabledInternal callback.");
        return message.callback(response);
      }).bind(this));
    },

    _deactivateDataCalls: function() {
      if (DEBUG) debug("RadioControl: deactivating data calls...");
      _deactivatingDeferred = {};

      let promise = Promise.resolve();
      for (let i = 0, N = _ril.numRadioInterfaces; i < N; ++i) {
        promise = promise.then(this._deactivateDataCallsForClient(i));
      }

      return promise;
    },

    _deactivateDataCallsForClient: function(clientId) {
      return function() {
        let deferred = _deactivatingDeferred[clientId] = PromiseUtils.defer();
        let dataCallHandler = gDataCallManager.getDataCallHandler(clientId);

        dataCallHandler.deactivateAllDataCalls(RIL.DATACALL_DEACTIVATE_RADIO_SHUTDOWN, function() {
          deferred.resolve();
        });

        return deferred.promise;
      };
    },

    _createTimer: function() {
      if (!_timer) {
        _timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      }
      _timer.initWithCallback(this._executeRequest.bind(this),
                              RADIO_POWER_OFF_TIMEOUT,
                              Ci.nsITimer.TYPE_ONE_SHOT);
    },

    _cancelTimer: function() {
      if (_timer) {
        _timer.cancel();
      }
    },

    _executeRequest: function() {
      if (typeof _request === "function") {
        if (DEBUG) debug("RadioControl: executeRequest");
        this._cancelTimer();
        _request();
        _request = null;
      }
      this._processNextMessage();
    },
  };
});

// Initialize shared preference "ril.numRadioInterfaces" according to system
// property.
try {
  Services.prefs.setIntPref(kPrefRilNumRadioInterfaces, (function() {
    // When Gonk property "ro.moz.ril.numclients" is not set, return 1; if
    // explicitly set to any number larger-equal than 0, return num; else, return
    // 1 for compatibility.
    try {
      let numString = libcutils.property_get("ro.moz.ril.numclients", "1");
      let num = parseInt(numString, 10);
      if (num >= 0) {
        return num;
      }
    } catch (e) {}

    return 1;
  })());
} catch (e) {}

function DataCall(aAttributes) {
  for (let key in aAttributes) {
    if (key === "pdpType") {
      // Convert pdp type into constant int value.
      this[key] = RIL.RIL_DATACALL_PDP_TYPES.indexOf(aAttributes[key]);
      continue;
    }

    this[key] = aAttributes[key];
  }
}
DataCall.prototype = {
  QueryInterface: ChromeUtils.generateQI([Ci.nsIDataCall]),

  failCause: Ci.nsIDataCallInterface.DATACALL_FAIL_NONE,
  suggestedRetryTime: -1,
  cid: -1,
  active: -1,
  pdpType: -1,
  ifname: null,
  addreses: null,
  dnses: null,
  gateways: null,
  pcscf: null,
  mtu: -1
};

function DataProfile(aAttributes) {
  for (let key in aAttributes) {
    this[key] = aAttributes[key];
  }
}
DataProfile.prototype = {
  QueryInterface: ChromeUtils.generateQI([Ci.nsIDataProfile]),

  profileId: -1,
  apn: null,
  protocol: null,
  authType: -1,
  user: null,
  password: null,
  type: -1,
  maxConnsTime: -1,
  maxConns: -1,
  waitTime: -1,
  enabled: false,
  supportedApnTypesBitmap: -1,
  roamingProtocol: null,
  bearerBitmap: -1,
  mtu: -1,
  mvnoType: null,
  mvnoMatchData: null,
  modemCognitive: false
};

function RadioInterfaceLayer() {

  let numIfaces = this.numRadioInterfaces;
  if (DEBUG) debug(numIfaces + " interfaces");

  this.radioInterfaces = [];
  for (let clientId = 0; clientId < numIfaces; clientId++) {
    this.radioInterfaces.push(new RadioInterface(clientId));
  }

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);

  gRadioEnabledController.init(this);
}
RadioInterfaceLayer.prototype = {

  classID:   RADIOINTERFACELAYER_CID,
  QueryInterface: ChromeUtils.generateQI([Ci.nsIRadioInterfaceLayer,
                                         Ci.nsIObserver]),

  /**
   * nsIObserver interface methods.
   */

  observe: function(subject, topic, data) {
    switch (topic) {
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        for (let radioInterface of this.radioInterfaces) {
          radioInterface.shutdown();
        }
        this.radioInterfaces = null;
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;

      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (data === kPrefRilDebuggingEnabled) {
          updateDebugFlag();
          this.setWorkerDebugFlag(DEBUG);
        }
        break;
    }
  },

  /**
   * nsIRadioInterfaceLayer interface methods.
   */

  getRadioInterface: function(clientId) {
    return this.radioInterfaces[clientId];
  },

  setMicrophoneMuted: function(muted) {
    for (let clientId = 0; clientId < this.numRadioInterfaces; clientId++) {
      let radioInterface = this.radioInterfaces[clientId];
      radioInterface.sendRilRequest("setMute", { muted: muted });
    }
  }
};

XPCOMUtils.defineLazyGetter(RadioInterfaceLayer.prototype,
                            "numRadioInterfaces", function() {
  try {
    //Cameron config the default value to 1.
    return Services.prefs.getIntPref(kPrefRilNumRadioInterfaces,1);
  } catch(e) {}

  return 1;
});

function RadioInterface(aClientId) {
  this.token= 1;
  this.clientId = aClientId;
  this.radioState = Ci.nsIRilIndicationResult.RADIOSTATE_UNKNOWN;
  this.radioTech = Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_UNKNOWN;
  this._waitingRadioTech = false;
  this._attachDataRegistration = {};
  this.pendingToReportSmsMemoryStatus = false;
  this._isCdma = false;
  this.deviceIdentities = null;
  this.basebandVersion = null;
  this._processingNetworkInfo = false;
  this._needRepollNetworkInfo = false;
  this._pendingNetworkInfo = {rilMessageType: "networkinfochanged"};
  this.pendingNetworkType = null;
  this.cardState = RIL.GECKO_CARDSTATE_UNINITIALIZED;
  this.operator = {
        rilMessageType: "operatorchange",
        longName: null,
        shortName: null};
  this._initIccInfo();
  this.networkSelectionMode = RIL.GECKO_NETWORK_SELECTION_UNKNOWN;
  this.voiceRegistrationState = {};
  this.dataRegistrationState = {};
  this.aid = null;
  this.appType = null;

  this.cellBroadcastDisabled = false;

  this._receivedSmsCbPagesMap = null;

  /**
   * Cell Broadcast Search Lists.
   */
  let cbmmi = this.cellBroadcastConfigs && this.cellBroadcastConfigs.MMI;
  this.cellBroadcastConfigs = {
    MMI: cbmmi || null
  };
  this.mergedCellBroadcastConfig = null;

  if (gRilWorkerService) {
    this.rilworker = gRilWorkerService.getRilWorker(this.clientId);
    if (this.rilworker) {
      this.rilworker.initRil(this);
    }
  }

  // For sim io context.
  this.simIOcontext = new SIM.Context(this);

  this.tokenCallbackMap = {};

  // For IccIO
  this.tokenOptionsMap = {};

  /**
     * CallForward options
     */
  this._callForwardOptions = {};

  //Cameron mark first.
  /*
  let lock = gSettingsService.createLock();

  // Read the "time.clock.automatic-update.enabled" setting to see if
  // we need to adjust the system clock time by NITZ or SNTP.
  lock.get(kSettingsClockAutoUpdateEnabled, this);

  // Read the "time.timezone.automatic-update.enabled" setting to see if
  // we need to adjust the system timezone by NITZ.
  lock.get(kSettingsTimezoneAutoUpdateEnabled, this);
  */

  // Set "time.clock.automatic-update.available" to false when starting up.
  this.setClockAutoUpdateAvailable(false);

  // Set "time.timezone.automatic-update.available" to false when starting up.
  this.setTimezoneAutoUpdateAvailable(false);

  Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  Services.obs.addObserver(this, kSysClockChangeObserverTopic, false);
  Services.obs.addObserver(this, kScreenStateChangedTopic, false);

  Services.obs.addObserver(this, kNetworkConnStateChangedTopic, false);

  //Cameron mark first.
 /*this._sntp = new Sntp(this.setClockBySntp.bind(this),
                        Services.prefs.getIntPref("network.sntp.maxRetryCount"),
                        Services.prefs.getIntPref("network.sntp.refreshPeriod"),
                        Services.prefs.getIntPref("network.sntp.timeout"),
                        Services.prefs.getCharPref("network.sntp.pools").split(";"),
                        Services.prefs.getIntPref("network.sntp.port"));*/
}

RadioInterface.prototype = {

  classID:   RADIOINTERFACE_CID,
  QueryInterface: ChromeUtils.generateQI([Ci.nsIRadioInterface,
                                         Ci.nsIObserver,
                                         Ci.nsIRilCallback,
                                         Ci.nsISettingsServiceCallback]),

  rilworker: null,

  // This gets incremented each time we send out a message.
  token: 1,

  // Maps tokens we send out with messages to the message callback.
  tokenCallbackMap: null,

  // Maps tokens we stroe the command type for mapping the handler function.
  tokenOptionsMap: null,

  /**
   * One of the RADIO_STATE_* constants.
   */
  radioState: null,

  /**
   * Set when radio is ready but radio tech is unknown. That is, we are
   * waiting for REQUEST_VOICE_RADIO_TECH
   */

  radioTech: null,

  _waitingRadioTech: false,

  _attachDataRegistration: null,

  /**
   * True when the request to report SMS Memory Status is pending.
   */
  pendingToReportSmsMemoryStatus: false,

  /**
   * True if we are on a CDMA phone.
   */
  _isCdma: false,

  /**
   * Device Identities including IMEI, IMEISV, ESN and MEID.
   */
  deviceIdentities: null,

  /**
   * String containing the baseband version.
   */
  basebandVersion: null,

  /**
   * Whether or not the multiple requests in requestNetworkInfo() are currently
   * being processed
   */
  _processingNetworkInfo: false,

  /**
   * Multiple requestNetworkInfo() in a row before finishing the first
   * request, hence we need to fire requestNetworkInfo() again after
   * gathering all necessary stuffs. This is to make sure that ril_worker
   * gets precise network information.
   */
   _needRepollNetworkInfo: false,

  /**
   * Pending messages to be send in batch from requestNetworkInfo()
   */
  _pendingNetworkInfo: {},

   /**
   * Marker object.
   */
  pendingNetworkType: null,

  /**
   * Card state
   */
  cardState: null,

  /**
   * List of strings identifying the network operator.
   */
  operator: null,

  // Icc card
  iccInfoPrivate: null,

  iccInfo: null,

  /**
  * Application identification for apps in ICC.
  */
  aid: null,

  /**
  * Application type for apps in ICC.
  */
  appType: null,

  networkSelectionMode: null,

  voiceRegistrationState: null,
  dataRegistrationState: null,

  // For SIM IO
  simIOcontext: null,

  /**
   * Global Cell Broadcast switch.
   */
  cellBroadcastDisabled: false,

  /**
   * Parsed Cell Broadcast search lists.
   * cellBroadcastConfigs.MMI should be preserved over rild reset.
   */
  cellBroadcastConfigs: null,
  mergedCellBroadcastConfig: null,

  _receivedSmsCbPagesMap: null,

  /**
     * CallForward options
     */
  _callForwardOptions: null,



  debug: function(s) {
    //dump("-*- RadioInterface[" + this.clientId + "]: " + s + "\n");
    console.log("-*- RadioInterface[" + this.clientId + "]: " + s + "\n");
  },

  shutdown: function() {
    Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
    Services.obs.removeObserver(this, kSysClockChangeObserverTopic);
    Services.obs.removeObserver(this, kScreenStateChangedTopic);
    Services.obs.removeObserver(this, kNetworkConnStateChangedTopic);
  },

  _initIccInfo: function() {
    /**
     * ICC information that is not exposed to Gaia.
     */
    this.iccInfoPrivate = {};

    /**
     * ICC information, such as MSISDN, MCC, MNC, SPN...etc.
     */
    let iccInfoReseted = false;
    if (this.iccInfo && Object.keys(this.iccInfo).length > 0) {
      iccInfoReseted = true;
    }

    this.iccInfo = {};
    if (iccInfoReseted) {
      this.simIOcontext.ICCUtilsHelper.handleICCInfoChange();
    }
  },

  isCardPresent: function() {
    let icc = gIccService.getIccByServiceId(this.clientId);
    let cardState = icc ? icc.cardState : Ci.nsIIcc.CARD_STATE_UNKNOWN;
    return cardState !== Ci.nsIIcc.CARD_STATE_UNDETECTED &&
      cardState !== Ci.nsIIcc.CARD_STATE_UNKNOWN;
  },

  handleUnsolicitedMessage: function(message) {
    if (DEBUG) this.debug("message.rilMessageType:" + message.rilMessageType);
    switch (message.rilMessageType) {
      case "rilconnected":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RIL_CONNECTED");
        this.handleRilConnected(message);
        break;
      case "callRing":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_CALL_RING");
        gTelephonyService.notifyCallRing();
        break;
      case "callStateChanged":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED");
        this.handleCallStateChanged();
        break;
      case "cdmaCallWaiting":
        gTelephonyService.notifyCdmaCallWaiting(this.clientId,
                                                message.waitingCall);
        break;
      case "suppSvcNotification":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_SUPP_SVC_NOTIFICATION");
        gTelephonyService.notifySupplementaryService(this.clientId,
                                                     message.notificationType,
                                                     message.code,
                                                     message.index,
                                                     message.type,
                                                     message.number);
        break;
      case "ussdreceived":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_ON_USSD typeCode = " + message.typeCode + " , message = " + message.message);
        // Per ril.h the USSD session is assumed to persist if
        // the type code is "1", otherwise the current session
        // (if any) is assumed to have terminated.
        let sessionEnded = (message.typeCode !== 1);
        gTelephonyService.notifyUssdReceived(this.clientId, message.message,
                                             sessionEnded);
        break;
      case "datacalllistchanged":
        let dataCalls = message.getDataCallLists().map(dataCall => new DataCall(dataCall));
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_DATA_CALL_LIST_CHANGED :" + JSON.stringify(dataCalls));

        gDataCallInterfaceService.notifyDataCallListChanged(this.clientId,
                                                            dataCalls.length,
                                                            dataCalls);
        break;
      case "ringbackTone":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RINGBACK_TONE : playRingbackTone = " + message.playRingbackTone);
        gTelephonyService.notifyRingbackTone(this.clientId, message.playRingbackTone);
        break;
      // We don't need this let enterEmCb/exitEmCb to do the job.
      case "emergencyCbModeChange":
        gMobileConnectionService.notifyEmergencyCallbackModeChanged(this.clientId,
                                                                    message.active,
                                                                    message.timeoutMs);
        break;
      //====== For networkinfochange not a unsol command=====
      case "networkinfochanged":
        gMobileConnectionService.notifyNetworkInfoChanged(this.clientId,
                                                          message);
        break;
      case "networkselectionmodechange":
        gMobileConnectionService.notifyNetworkSelectModeChanged(this.clientId,
                                                                message.mode);
        break;
      case "voiceregistrationstatechange":
        gMobileConnectionService.notifyVoiceInfoChanged(this.clientId, message);
        break;
      case "dataregistrationstatechange":
        gMobileConnectionService.notifyDataInfoChanged(this.clientId, message);
        break;
      case "operatorchange":
        gMobileConnectionService.notifyOperatorChanged(this.clientId, message);
        break;
      //==================================
      case "signalstrengthchange":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_SIGNAL_STRENGTH,  signalStrength = " + JSON.stringify(message.signalStrength));
        this.handleSignalStrength(message);
        break;
      case "otastatuschange":
        gMobileConnectionService.notifyOtaStatusChanged(this.clientId, message.status);
        break;
      case "radiostatechange":
        // gRadioEnabledController should know the radio state for each client,
        // so notify gRadioEnabledController here.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, radioStateChanged: " + message.radioState);
        this.handleRadioStateChange(message.radioState);
        gRadioEnabledController.notifyRadioStateChanged(this.clientId,
                                                        message.radioState);
        // After the modem assert, the data calls should be deactivated.
        if (message.radioState != RIL.RADIOSTATE_ENABLED) {
          gRadioEnabledController._deactivateDataCalls();
        }
        break;
      case "sms-received":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_NEW_SMS");
        this.handleSmsReceived(message);
        break;
      case "cellbroadcast-received":
        this.handleCellbroadcastMessageReceived(message);
        break;
      case "nitzTimeReceived":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_NITZ_TIME_RECEIVED dateString = " + message.dateString + " ,receiveTimeInMS = " + message.receiveTimeInMS);
        this.handleNitzTime(message);
        break;
      //==== For icc info not a Unsol command ===
      case "iccinfochange":
        if (DEBUG) this.debug("iccinfochange message=" + JSON.stringify(message));
        gIccService.notifyIccInfoChanged(this.clientId,
                                         message.iccid ? message : null);
        break;
      case "iccmbdn":
        if (DEBUG) this.debug("iccmbdn message=" + JSON.stringify(message));
        this.handleIccMbdn(message);
        break;
      case "iccmwis":
        if (DEBUG) this.debug("iccmwis message=" + JSON.stringify(message));
        this.handleIccMwis(message.mwi);
        break;
      case "isiminfochange":
        gIccService.notifyIsimInfoChanged(this.clientId,
                                          message.impi ? message : null);
        break;
      //================================================
      case "stkcommand":
        gIccService.notifyStkCommand(this.clientId,
                                     gStkCmdFactory.createCommand(message));
        break;
      case "stksessionend":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_STK_SESSION_END");
        gIccService.notifyStkSessionEnd(this.clientId);
        break;
      case "stkProactiveCommand":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_STK_PROACTIVE_COMMAND");
        this.handleStkProactiveCommand(message);
        break;
      case "stkEventNotify":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_STK_EVENT_NOTIFY");
        this.handleStkProactiveCommand(message);
        break;
      case "cdma-info-rec-received":
        this.handleCdmaInformationRecords(message.records);
        break;
      case "pcochange":
        let connHandler = gDataCallManager.getDataCallHandler(this.clientId);
        connHandler.updatePcoData(message.pco.cid,
                                  message.pco.bearerProto,
                                  message.pco.pcoId,
                                  message.pco.contents,
                                  message.pco.contents.length);
        break;
      // Cameron TODO complete the modem reset.
      case "modemrestart":
        gMobileConnectionService.notifyModemRestart(this.clientId, message.reason);
        break;
      case "networkStateChanged":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED");
        this.handleNetworkStateChanged();
        break;
      case "voiceRadioTechChanged":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_VOICE_RADIO_TECH_CHANGED radioTech = " + message.radioTech);
        this.handleVoiceRadioTechnology(message);
        break;
      case "simSmsStorageFull":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_SIM_SMS_STORAGE_FULL");
        break;
      case "simStatusChanged":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED");
        break;
      case "resendIncallMute":
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESEND_INCALL_MUTE");
        break;
      case "cellInfoList":
        let cellInfoLists = message.getCellInfo();
        // Gecko do not handle this UNSL command.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_CELL_INFO_LIST cellInfoLists = " + JSON.stringify(cellInfoLists));
        break;
      case "simRefresh":
        let refreshResult = message.refreshResult;
        // Gecko do not handle this UNSL command.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_SIM_REFRESH refreshResult = " + JSON.stringify(refreshResult));
        break;
      case "restrictedStateChanged":
        let restrictedState = message.restrictedState;
        // Gecko do not handle this UNSL command.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESTRICTED_STATE_CHANGED restrictedState = " + JSON.stringify(restrictedState));
        break;
      case "enterEmergencyCbMode":
        //Cameron TODO complete the emgCBmode
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE");
        break;
      case "exitEmergencyCbMode":
        //Cameron TODO complete the emgCBmode
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE");
        break;
      case "subscriptionStatusChanged":
        let activate = message.activate;
        // Gecko do not handle this UNSL command.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED activate = " + JSON.stringify(activate));
        break;
      case "srvccStateNotify":
        // Gecko do not handle this UNSL command.
        let srvccState = message.srvccState;
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_SRVCC_STATE_NOTIFY srvccState = " + JSON.stringify(srvccState));
        break;
      case "hardwareConfigChanged":
        // Gecko do not handle this UNSL command.
        let HWConfigs = message.getHardwardConfig();
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_HARDWARE_CONFIG_CHANGED HWConfigs = " + JSON.stringify(HWConfigs));
        break;
      case "radioCapabilityIndication":
        // Gecko do not handle this UNSL command.
        let rc = message.rc;
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RADIO_CAPABILITY rc = " + JSON.stringify(rc));
        break;
      case "lceData":
        // Gecko do not handle this UNSL command.
        let lce = message.lce;
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_LCEDATA_RECV lce = " + JSON.stringify(lce));
        break;
      case "pcoData":
        // Gecko do not handle this UNSL command.
        let pco = message.pco;
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_PCO_DATA pco = " + JSON.stringify(pco));
        break;
      case "imsNetworkStateChanged":
        // Gecko do not handle this UNSL command.
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED ");
        break;
      case "modemReset":
        // Cameon TODO complete the modem reset feature.
        let reason = message.reason;
        if (DEBUG) this.debug("RILJ: [UNSL]< RIL_UNSOL_MODEM_RESTART reason = " + JSON.stringify(reason));
        break;
      default:
        throw new Error("Don't know about this message type: " +
                        message.rilMessageType);
    }
  },

  handleRilConnected: function (newState) {
    this.sendRilRequest("setRadioEnabled", {enabled: false});
    this.sendRilRequest("setCellInfoListRate", null);
    //Cameron handle the CDMA later.
    //mRil->setCdmaSubscriptionSource(mRil.mCdmaSubscription, null);
  },

  handleRadioStateChange: function (newState) {
    if (DEBUG) {
      this.debug("Radio state changed from '" + this.radioState +
                         "' to '" + newState + "'");
    }

    if (this.radioState == newState) {
      return;
    }

    if (newState !== Ci.nsIRilIndicationResult.RADIOSTATE_UNKNOWN) {
      // Retrieve device identities once radio is available.
      this.sendRilRequest("getDeviceIdentity", null);
    }

    if (newState == Ci.nsIRilIndicationResult.RADIOSTATE_ENABLED) {
      // This value is defined in RIL v7, we will retrieve radio tech by another
      // request. We leave _isCdma untouched, and it will be set once we get the
      // radio technology.
      this._waitingRadioTech = true;
      this.sendRilRequest("getVoiceRadioTechnology", null);
    }

    if (newState != Ci.nsIRilIndicationResult.RADIOSTATE_ENABLED) {
      this._attachDataRegistration.result = false;
    }

    if ((this.radioState == Ci.nsIRilIndicationResult.RADIOSTATE_UNKNOWN ||
       this.radioState == Ci.nsIRilIndicationResult.RADIOSTATE_DISABLED) &&
       newState == Ci.nsIRilIndicationResult.RADIOSTATE_ENABLED) {
      // The radio became available, let's get its info.
      if (!this._waitingRadioTech) {
        this.sendRilRequest("getDeviceIdentity", null);
      }
      this.sendRilRequest("getBasebandVersion", null);

      this.updateCellBroadcastConfig();

      if (libcutils.property_get("ro.moz.ril.data_reg_on_demand", "false") === "true" &&
        !this._attachDataRegistration.result) {
        this.setDataRegistration({attach: this._attachDataRegistration.attach});
      } else if (libcutils.property_get("ro.moz.ril.subscription_control", "false") === "true" &&
        this._attachDataRegistration.attach) {
        this.setDataRegistration({attach: true});
      }

      if (this.pendingToReportSmsMemoryStatus) {
        this.sendRilRequest("reportSmsMemoryStatus", {isAvailable: this.smsStorageAvailable});
      }
    }

    this.radioState = newState;

    // If the radio is up and on, so let's query the card state.
    // On older RILs only if the card is actually ready, though.
    // If _waitingRadioTech is set, we don't need to get icc status now.
    if (newState == Ci.nsIRilIndicationResult.RADIOSTATE_UNKNOWN ||
        newState == Ci.nsIRilIndicationResult.RADIOSTATE_DISABLED ||
        this._waitingRadioTech) {
      return;
    }

    this.sendRilRequest("getICCStatus", null);
  },

  _mergeCellBroadcastConfigs: function(list, from, to) {
    if (!list) {
      return [from, to];
    }

    for (let i = 0, f1, t1; i < list.length;) {
      f1 = list[i++];
      t1 = list[i++];
      if (to == f1) {
        // ...[from]...[to|f1]...(t1)
        list[i - 2] = from;
        return list;
      }

      if (to < f1) {
        // ...[from]...(to)...[f1] or ...[from]...(to)[f1]
        if (i > 2) {
          // Not the first range pair, merge three arrays.
          return list.slice(0, i - 2).concat([from, to]).concat(list.slice(i - 2));
        } else {
          return [from, to].concat(list);
        }
      }

      if (from > t1) {
        // ...[f1]...(t1)[from] or ...[f1]...(t1)...[from]
        continue;
      }

      // Have overlap or merge-able adjacency with [f1]...(t1). Replace it
      // with [min(from, f1)]...(max(to, t1)).

      let changed = false;
      if (from < f1) {
        // [from]...[f1]...(t1) or [from][f1]...(t1)
        // Save minimum from value.
        list[i - 2] = from;
        changed = true;
      }

      if (to <= t1) {
        // [from]...[to](t1) or [from]...(to|t1)
        // Can't have further merge-able adjacency. Return.
        return list;
      }

      // Try merging possible next adjacent range.
      let j = i;
      for (let f2, t2; j < list.length;) {
        f2 = list[j++];
        t2 = list[j++];
        if (to > t2) {
          // [from]...[f2]...[t2]...(to) or [from]...[f2]...[t2](to)
          // Merge next adjacent range again.
          continue;
        }

        if (to < t2) {
          if (to < f2) {
            // [from]...(to)[f2] or [from]...(to)...[f2]
            // Roll back and give up.
            j -= 2;
          } else if (to < t2) {
            // [from]...[to|f2]...(t2), or [from]...[f2]...[to](t2)
            // Merge to [from]...(t2) and give up.
            to = t2;
          }
        }

        break;
      }

      // Save maximum to value.
      list[i - 1] = to;

      if (j != i) {
        // Remove merged adjacent ranges.
        let ret = list.slice(0, i);
        if (j < list.length) {
          ret = ret.concat(list.slice(j));
        }
        return ret;
      }

      return list;
    }

    // Append to the end.
    list.push(from);
    list.push(to);

    return list;
  },

  _isCellBroadcastConfigReady: function() {
    if (!("MMI" in this.cellBroadcastConfigs)) {
      return false;
    }

    // CBMI should be ready in GSM.
    if (!this._isCdma &&
        (!("CBMI" in this.cellBroadcastConfigs) ||
         !("CBMID" in this.cellBroadcastConfigs) ||
         !("CBMIR" in this.cellBroadcastConfigs))) {
      return false;
    }

    return true;
  },

  /**
   * Merge all members of cellBroadcastConfigs into mergedCellBroadcastConfig.
   */
  _mergeAllCellBroadcastConfigs: function() {
    if ((Services.prefs.getBoolPref(kPrefAppCBConfigurationEnabled) || false) &&
        !this._isCellBroadcastConfigReady()) {
      if (DEBUG) {
        this.debug("cell broadcast configs not ready, waiting ...");
      }
      return;
    }

    // Prepare cell broadcast config. CBMI* are only used in GSM.
    let usedCellBroadcastConfigs = {MMI: this.cellBroadcastConfigs.MMI};
    if ((Services.prefs.getBoolPref(kPrefAppCBConfigurationEnabled) || false)  && !this._isCdma) {
      usedCellBroadcastConfigs.CBMI = this.cellBroadcastConfigs.CBMI;
      usedCellBroadcastConfigs.CBMID = this.cellBroadcastConfigs.CBMID;
      usedCellBroadcastConfigs.CBMIR = this.cellBroadcastConfigs.CBMIR;
    }

    if (DEBUG) {
      this.debug("Cell Broadcast search lists: " +
                         JSON.stringify(usedCellBroadcastConfigs));
    }

    let list = null;
    for (let key in usedCellBroadcastConfigs) {
      let ll = usedCellBroadcastConfigs[key];
      if (ll == null) {
        continue;
      }

      for (let i = 0; i < ll.length; i += 2) {
        list = this._mergeCellBroadcastConfigs(list, ll[i], ll[i + 1]);
      }
    }

    if (DEBUG) {
      this.debug("Cell Broadcast search lists(merged): " +
                         JSON.stringify(list));
    }
    this.mergedCellBroadcastConfig = list;
    this.updateCellBroadcastConfig();
  },

  setCellBroadcastDisabled: function(options) {
    this.cellBroadcastDisabled = options.disabled;

    // Return the response and let ril handle the rest thing.
    options.errorMsg = RIL.ERROR_SUCCESS;
    this.handleRilResponse(options);

    // If |this.mergedCellBroadcastConfig| is null, either we haven't finished
    // reading required SIM files, or no any channel is ever configured.  In
    // the former case, we'll call |this.updateCellBroadcastConfig()| later
    // with correct configs; in the latter case, we don't bother resetting CB
    // to disabled again.
    if (this.mergedCellBroadcastConfig) {
      this.updateCellBroadcastConfig();
    }
  },

  setCellBroadcastSearchList: function(options) {
    let getSearchListStr = function(aSearchList) {
      if (typeof aSearchList === "string" || aSearchList instanceof String) {
        return aSearchList;
      }

      // TODO: Set search list for CDMA/GSM individually. Bug 990926
      let prop = this._isCdma ? "cdma" : "gsm";

      return aSearchList && aSearchList[prop];
    }.bind(this);

    try {
      let str = getSearchListStr(options.searchList);
      this.debug("setCellBroadcastSearchList  str: " + str);
      this.cellBroadcastConfigs.MMI = this._convertCellBroadcastSearchList(str);
      options.errorMsg = RIL.ERROR_SUCCESS;
    } catch (e) {
      if (DEBUG) {
        this.debug("Invalid Cell Broadcast search list: " + e);
      }
      options.errorMsg = RIL.GECKO_ERROR_UNSPECIFIED_ERROR;
    }

    this.handleRilResponse(options);
    if (options.errorMsg) {
      return;
    }

    this._mergeAllCellBroadcastConfigs();
  },

    /**
   * Convert Cell Broadcast settings string into search list.
   */
  _convertCellBroadcastSearchList: function(searchListStr) {
    let parts = searchListStr && searchListStr.split(",");
    if (!parts) {
      return null;
    }

    let list = null;
    let result, from, to;
    for (let range of parts) {
      // Match "12" or "12-34". The result will be ["12", "12", null] or
      // ["12-34", "12", "34"].
      result = range.match(/^(\d+)(?:-(\d+))?$/);
      if (!result) {
        throw "Invalid format";
      }

      from = parseInt(result[1], 10);
      to = (result[2]) ? parseInt(result[2], 10) + 1 : from + 1;
      if (!RIL.CB_NON_MMI_SETTABLE_RANGES &&
          !this._checkCellBroadcastMMISettable(from, to)) {
        throw "Invalid range";
      }

      if (list == null) {
        list = [];
      }
      list.push(from);
      list.push(to);
    }

    return list;
  },

    /**
   * Check whether search list from settings is settable by MMI, that is,
   * whether the range is bounded in any entries of CB_NON_MMI_SETTABLE_RANGES.
   */
  _checkCellBroadcastMMISettable: function(from, to) {
    if ((to <= from) || (from >= 65536) || (from < 0)) {
      return false;
    }

    if (!this._isCdma) {
      // GSM not settable ranges.
      for (let i = 0, f, t; i < RIL.CB_NON_MMI_SETTABLE_RANGES.length;) {
        f = RIL.CB_NON_MMI_SETTABLE_RANGES[i++];
        t = RIL.CB_NON_MMI_SETTABLE_RANGES[i++];
        if ((from < t) && (to > f)) {
          // Have overlap.
          return false;
        }
      }
    }

    return true;
  },

  updateCellBroadcastConfig: function() {
    let activate = !this.cellBroadcastDisabled &&
                   (this.mergedCellBroadcastConfig != null) &&
                   (this.mergedCellBroadcastConfig.length > 0);
    if (activate) {
      this.setSmsBroadcastConfig(this.mergedCellBroadcastConfig);
    } else {
      // It's unnecessary to set config first if we're deactivating.
      this.setSmsBroadcastActivation(false);
    }
  },

  setSmsBroadcastActivation: function(activate) {
    let parcelType = this._isCdma ? RIL.REQUEST_CDMA_SMS_BROADCAST_ACTIVATION :
                                    RIL.REQUEST_GSM_SMS_BROADCAST_ACTIVATION;

    if (this._isCdma) {
      // TODO complete the CDMA part.
    } else {
      this.sendRilRequest("setGsmBroadcastActivation"
                          , {activate: activate}
                          , null);
    }
  },

  setSmsBroadcastConfig: function(config) {
    if (this._isCdma) {
      // TODO mark the CDMA feature.
      /*this.sendRilRequest("setCdmaSmsBroadcastConfig"
                          , {config: config}
                          , null);*/
    } else {
      this.sendRilRequest("setGsmBroadcastConfig"
                          , {config: config}
                          , null);
    }
  },

  handleStkProactiveCommand: function(message) {
    this.debug("handleStkProactiveCommand cmd: " + JSON.stringify(message.cmd));
    let length = message.cmd.length;
    let berTlv;
    try {
      berTlv = this.simIOcontext.BerTlvHelper.decode(message.cmd);
      this.debug("handleStkProactiveCommand berTlv: " + JSON.stringify(berTlv));
    } catch (e) {
      if (DEBUG) this.debug("handleStkProactiveCommand : " + e);
      this.sendStkTerminalResponse({
        resultCode: RIL.STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      return;
    }

    let ctlvs = berTlv.value;
    this.debug("handleStkProactiveCommand ctlvs: " + JSON.stringify(ctlvs));
    let ctlv = this.simIOcontext.StkProactiveCmdHelper.searchForTag(
        RIL.COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
    this.debug("handleStkProactiveCommand ctlv: " + JSON.stringify(ctlv));
    if (!ctlv) {
      this.sendStkTerminalResponse({
        resultCode: RIL.STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("Can't find COMMAND_DETAILS ComprehensionTlv");
    }

    let cmdDetails = ctlv.value;
    this.debug("handleStkProactiveCommand cmdDetails: " + JSON.stringify(cmdDetails));
    if (DEBUG) {
      this.debug("commandNumber = " + cmdDetails.commandNumber +
                         " typeOfCommand = " + cmdDetails.typeOfCommand.toString(16) +
                         " commandQualifier = " + cmdDetails.commandQualifier);
    }

    // STK_CMD_MORE_TIME need not to propagate event to chrome.
    if (cmdDetails.typeOfCommand == RIL.STK_CMD_MORE_TIME) {
      this.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: RIL.STK_RESULT_OK});
      return;
    }

    this.simIOcontext.StkCommandParamsFactory.createParam(cmdDetails,
                                                     ctlvs,
                                                     (aResult) => {
      cmdDetails.options = aResult;
      cmdDetails.rilMessageType = "stkcommand";
      this.handleUnsolicitedMessage(cmdDetails);
      //this.sendChromeMessage(cmdDetails);
    });
  },

  handleNetworkStateChanged: function () {
    this.sendRilRequest("getICCStatus", null);
    //this.getICCStatus();
    this.requestNetworkInfo();
  },

  handleCallStateChanged: function () {
    this.sendRilRequest("getCurrentCalls" , null);
  },

    /**
   * Request various states about the network.
   */
  requestNetworkInfo: function() {
    if (this._processingNetworkInfo) {
      if (DEBUG) {
        /*this.debug("Network info requested, but we're already " +
                           "requesting network info.");*/
      }
      this._needRepollNetworkInfo = true;
      return;
    }

    //if (DEBUG) console.log("Requesting network info");

    this._processingNetworkInfo = true;
    this.sendRilRequest("getVoiceRegistrationState", null);
    //this.getVoiceRegistrationState();
    this.sendRilRequest("getDataRegistrationState", null);
    //this.getDataRegistrationState(); //TODO only GSM
    this.sendRilRequest("getOperator", null);
    //this.getOperator();
    this.sendRilRequest("getNetworkSelectionMode", null);
    //this.getNetworkSelectionMode();
    this.sendRilRequest("getSignalStrength", null);
    //this.getSignalStrength();
  },

  setDataRegistration: function(attach) {
    let deferred = PromiseUtils.defer();
    radioInterface.sendWorkerMessage("setDataRegistration",
                              {attach: attach},
                              (function(response) {
      // Always resolve to proceed with the following steps.
      deferred.resolve(response.errorMsg ? response.errorMsg : null);
    }).bind(this));
    /*
    this.workerMessenger.send("setDataRegistration",
                              {attach: attach},
                              (function(response) {
      // Always resolve to proceed with the following steps.
      deferred.resolve(response.errorMsg ? response.errorMsg : null);
    }).bind(this));*/

    return deferred.promise;
  },

  /**
   * TODO: Bug 911713 - B2G NetworkManager: Move policy control logic to
   *                    NetworkManager
   */
  updateRILNetworkInterface: function() {
    let connHandler = gDataCallManager.getDataCallHandler(this.clientId);
    connHandler.updateRILNetworkInterface();
  },

  /**
   * handle received SMS.
   */
  handleSmsReceived: function(aMessage) {
    let header = aMessage.header;
    // Concatenation Info:
    // - segmentRef: a modulo 256 counter indicating the reference number for a
    //               particular concatenated short message. '0' is a valid number.
    // - The concatenation info will not be available in |header| if
    //   segmentSeq or segmentMaxSeq is 0.
    // See 3GPP TS 23.040, 9.2.3.24.1 Concatenated Short Messages.
    let segmentRef = (header && header.segmentRef !== undefined)
      ? header.segmentRef : 1;
    let segmentSeq = header && header.segmentSeq || 1;
    let segmentMaxSeq = header && header.segmentMaxSeq || 1;
    // Application Ports:
    // The port number ranges from 0 to 49151.
    // see 3GPP TS 23.040, 9.2.3.24.3/4 Application Port Addressing.
    let originatorPort = (header && header.originatorPort !== undefined)
      ? header.originatorPort
      : Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID;
    let destinationPort = (header && header.destinationPort !== undefined)
      ? header.destinationPort
      : Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID;
    // MWI info:
    let mwiPresent = (aMessage.mwi)? true : false;
    let mwiDiscard = (mwiPresent)? aMessage.mwi.discard: false;
    let mwiMsgCount = (mwiPresent)? aMessage.mwi.msgCount: 0;
    let mwiActive = (mwiPresent)? aMessage.mwi.active: false;
    // CDMA related attributes:
    let cdmaMessageType = aMessage.messageType || 0;
    let cdmaTeleservice = aMessage.teleservice || 0;
    let cdmaServiceCategory = aMessage.serviceCategory || 0;

    gSmsService
      .notifyMessageReceived(this.clientId,
                             aMessage.SMSC || null,
                             aMessage.sentTimestamp,
                             aMessage.sender,
                             aMessage.pid,
                             aMessage.encoding,
                             RIL.GECKO_SMS_MESSAGE_CLASSES
                               .indexOf(aMessage.messageClass),
                             aMessage.language || null,
                             segmentRef,
                             segmentSeq,
                             segmentMaxSeq,
                             originatorPort,
                             destinationPort,
                             mwiPresent,
                             mwiDiscard,
                             mwiMsgCount,
                             mwiActive,
                             cdmaMessageType,
                             cdmaTeleservice,
                             cdmaServiceCategory,
                             aMessage.body || null,
                             aMessage.data || [],
                             (aMessage.data) ? aMessage.data.length : 0);
  },

  //Cameron mark first.

  /**
   * Set the setting value of "time.clock.automatic-update.available".
   */
  setClockAutoUpdateAvailable: function(value) {
    //gSettingsService.createLock().set(kSettingsClockAutoUpdateAvailable, value, null);
  },

  /**
   * Set the setting value of "time.timezone.automatic-update.available".
   */
  setTimezoneAutoUpdateAvailable: function(value) {
    //gSettingsService.createLock().set(kSettingsTimezoneAutoUpdateAvailable, value, null);
  },

  /**
   * Set the system clock by NITZ.
   */
  setClockByNitz: function(message) {
    // To set the system clock time. Note that there could be a time diff
    // between when the NITZ was received and when the time is actually set.
    let systemUpTime = this._getSystemUpTime();
    let upTimeDiff = systemUpTime ?
                     systemUpTime - message.receiveTimeInMS :
                     -1;
    if (upTimeDiff < 0) {
      this.debug("setClockByNitz upTimeDiff is invalid, skip!");
      return;
    }
    gTimeService.set(
      message.networkTimeInMS + upTimeDiff);
  },

  /**
   * Set the system time zone by NITZ.
   */
  setTimezoneByNitz: function(message) {
    // To set the sytem timezone. Note that we need to convert the time zone
    // value to a UTC repesentation string in the format of "UTC(+/-)hh:mm".
    // Ex, time zone -480 is "UTC+08:00"; time zone 630 is "UTC-10:30".

    // DST can only be 0|1|2 hr according to 3GPP TS 22.042, besides, there is
    // no need to unapply the DST correction because FE should be able to get
    // the correct time zone offset by using "time.timezone.dst" value.
    // Cameron mark fisrt.
    /*gSettingsService.createLock().set("time.timezone.dst",
                                      message.networkDSTInHr,
                                      null);

    if (message.networkTimeZoneInMinutes != (new Date()).getTimezoneOffset()) {
      let absTimeZoneInMinutes = Math.abs(message.networkTimeZoneInMinutes);
      let timeZoneStr = "UTC";
      timeZoneStr += (message.networkTimeZoneInMinutes > 0 ? "-" : "+");
      timeZoneStr += ("0" + Math.floor(absTimeZoneInMinutes / 60)).slice(-2);
      timeZoneStr += ":";
      timeZoneStr += ("0" + absTimeZoneInMinutes % 60).slice(-2);
      gSettingsService.createLock().set("time.timezone", timeZoneStr, null);
    }*/
  },

  /**
   * Handle the NITZ message.
   */
  handleNitzTime: function(message) {
    //Cameron mark first
    /*
    // Got the NITZ info received from the ril_worker.
    this.setClockAutoUpdateAvailable(true);
    this.setTimezoneAutoUpdateAvailable(true);

    // Cache the latest NITZ message whenever receiving it.
    this._lastNitzMessage = message;

    // Set the received NITZ clock if the setting is enabled.
    if (this._clockAutoUpdateEnabled) {
      this.setClockByNitz(message);
    }
    // Set the received NITZ timezone if the setting is enabled.
    if (this._timezoneAutoUpdateEnabled) {
      this.setTimezoneByNitz(message);
    }*/
  },

  /**
   * Set the system clock by SNTP.
   */
  setClockBySntp: function(offset) {
    // Got the SNTP info.
    this.setClockAutoUpdateAvailable(true);
    if (!this._clockAutoUpdateEnabled) {
      return;
    }
    if (this._lastNitzMessage) {
      if (DEBUG) debug("SNTP: NITZ available, discard SNTP");
      return;
    }
    gTimeService.set(Date.now() + offset);
  },

  handleIccMbdn: function(message) {
    // Cameron mark first.
    /*let service = Cc["@mozilla.org/voicemail/voicemailservice;1"]
                  .getService(Ci.nsIGonkVoicemailService);
    service.notifyInfoChanged(this.clientId, message.number, message.alphaId);*/
  },

  handleIccMwis: function(mwi) {
    /*let service = Cc["@mozilla.org/voicemail/voicemailservice;1"]
                  .getService(Ci.nsIGonkVoicemailService);
    // Note: returnNumber and returnMessage is not available from UICC.
    service.notifyStatusChanged(this.clientId, mwi.active, mwi.msgCount,
                                null, null);*/
  },

  _convertCbGsmGeographicalScope: function(aGeographicalScope) {
    return (aGeographicalScope != null)
      ? aGeographicalScope
      : Ci.nsICellBroadcastService.GSM_GEOGRAPHICAL_SCOPE_INVALID;
  },

  _convertCbMessageClass: function(aMessageClass) {
    let index = RIL.GECKO_SMS_MESSAGE_CLASSES.indexOf(aMessageClass);
    return (index != -1)
      ? index
      : Ci.nsICellBroadcastService.GSM_MESSAGE_CLASS_NORMAL;
  },

  _convertCbEtwsWarningType: function(aWarningType) {
    return (aWarningType != null)
      ? aWarningType
      : Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID;
  },

  handleCellbroadcastMessageReceived: function(aMessage) {
    let etwsInfo = aMessage.etws;
    let hasEtwsInfo = etwsInfo != null;
    let serviceCategory = (aMessage.serviceCategory)
      ? aMessage.serviceCategory
      : Ci.nsICellBroadcastService.CDMA_SERVICE_CATEGORY_INVALID;

    gCellBroadcastService
      .notifyMessageReceived(this.clientId,
                             this._convertCbGsmGeographicalScope(aMessage.geographicalScope),
                             aMessage.messageCode,
                             aMessage.messageId,
                             aMessage.language,
                             aMessage.fullBody,
                             this._convertCbMessageClass(aMessage.messageClass),
                             Date.now(),
                             serviceCategory,
                             hasEtwsInfo,
                             (hasEtwsInfo)
                               ? this._convertCbEtwsWarningType(etwsInfo.warningType)
                               : Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID,
                             hasEtwsInfo ? etwsInfo.emergencyUserAlert : false,
                             hasEtwsInfo ? etwsInfo.popup : false);
  },

  handleCdmaInformationRecords: function(aRecords) {
    if (DEBUG) this.debug("cdma-info-rec-received: " + JSON.stringify(aRecords));

    let clientId = this.clientId;

    aRecords.forEach(function(aRecord) {
      if (aRecord.display) {
        gMobileConnectionService
          .notifyCdmaInfoRecDisplay(clientId, aRecord.display);
        return;
      }

      if (aRecord.calledNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecCalledPartyNumber(clientId,
                                              aRecord.calledNumber.type,
                                              aRecord.calledNumber.plan,
                                              aRecord.calledNumber.number,
                                              aRecord.calledNumber.pi,
                                              aRecord.calledNumber.si);
        return;
      }

      if (aRecord.callingNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecCallingPartyNumber(clientId,
                                               aRecord.callingNumber.type,
                                               aRecord.callingNumber.plan,
                                               aRecord.callingNumber.number,
                                               aRecord.callingNumber.pi,
                                               aRecord.callingNumber.si);
        return;
      }

      if (aRecord.connectedNumber) {
        gMobileConnectionService
          .notifyCdmaInfoRecConnectedPartyNumber(clientId,
                                                 aRecord.connectedNumber.type,
                                                 aRecord.connectedNumber.plan,
                                                 aRecord.connectedNumber.number,
                                                 aRecord.connectedNumber.pi,
                                                 aRecord.connectedNumber.si);
        return;
      }

      if (aRecord.signal) {
        gMobileConnectionService
          .notifyCdmaInfoRecSignal(clientId,
                                   aRecord.signal.type,
                                   aRecord.signal.alertPitch,
                                   aRecord.signal.signal);
        return;
      }

      if (aRecord.redirect) {
        gMobileConnectionService
          .notifyCdmaInfoRecRedirectingNumber(clientId,
                                              aRecord.redirect.type,
                                              aRecord.redirect.plan,
                                              aRecord.redirect.number,
                                              aRecord.redirect.pi,
                                              aRecord.redirect.si,
                                              aRecord.redirect.reason);
        return;
      }

      if (aRecord.lineControl) {
        gMobileConnectionService
          .notifyCdmaInfoRecLineControl(clientId,
                                        aRecord.lineControl.polarityIncluded,
                                        aRecord.lineControl.toggle,
                                        aRecord.lineControl.reverse,
                                        aRecord.lineControl.powerDenial);
        return;
      }

      if (aRecord.clirCause) {
        gMobileConnectionService
          .notifyCdmaInfoRecClir(clientId,
                                 aRecord.clirCause);
        return;
      }

      if (aRecord.audioControl) {
        gMobileConnectionService
          .notifyCdmaInfoRecAudioControl(clientId,
                                         aRecord.audioControl.upLink,
                                         aRecord.audioControl.downLink);
        return;
      }
    });
  },

  // nsIObserver

  observe: function(subject, topic, data) {
    switch (topic) {
      case kMozSettingsChangedObserverTopic:
        if ("wrappedJSObject" in subject) {
          subject = subject.wrappedJSObject;
        }
        this.handleSettingsChange(subject.key, subject.value, subject.isInternalChange);
        break;
      case kSysClockChangeObserverTopic:
        let offset = parseInt(data, 10);
        //Cameron mark fist.
        //this._sntp.updateOffset(offset);
        break;
      case kNetworkConnStateChangedTopic:
        let networkInfo = subject.QueryInterface(Ci.nsINetworkInfo);
        if (networkInfo.state != Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED) {
          return;
        }

        // SNTP can only update when we have mobile or Wifi connections.
        if (networkInfo.type != NETWORK_TYPE_WIFI &&
            networkInfo.type != NETWORK_TYPE_MOBILE) {
          return;
        }

        // If the network comes from RIL, make sure the RIL service is matched.
        if (subject instanceof Ci.nsIRilNetworkInfo) {
          networkInfo = subject.QueryInterface(Ci.nsIRilNetworkInfo);
          if (networkInfo.serviceId != this.clientId) {
            return;
          }
        }

        //Cameron mark first.
        // SNTP won't update unless the SNTP is already expired.
        /*if (this._sntp.isExpired()) {
          this._sntp.request();
        }*/
        break;
      case kScreenStateChangedTopic:
        //Q do not support this command.
        //this.workerMessenger.send("setScreenState", { on: (data === "on") });
        break;
    }
  },

  // Flag to determine whether to update system clock automatically. It
  // corresponds to the "time.clock.automatic-update.enabled" setting.
  _clockAutoUpdateEnabled: null,

  // Flag to determine whether to update system timezone automatically. It
  // corresponds to the "time.clock.automatic-update.enabled" setting.
  _timezoneAutoUpdateEnabled: null,

  // Remember the last NITZ message so that we can set the time based on
  // the network immediately when users enable network-based time.
  _lastNitzMessage: null,

  // Object that handles SNTP.
  _sntp: null,

  // Cell Broadcast settings values.
  _cellBroadcastSearchList: null,

  handleSettingsChange: function(aName, aResult, aIsInternalSetting) {
    // Don't allow any content processes to modify the setting
    // "time.clock.automatic-update.available" except for the chrome process.
    if (aName === kSettingsClockAutoUpdateAvailable &&
        !aIsInternalSetting) {
      let isClockAutoUpdateAvailable = this._lastNitzMessage !== null ||
                                       this._sntp.isAvailable();
      if (aResult !== isClockAutoUpdateAvailable) {
        if (DEBUG) {
          debug("Content processes cannot modify 'time.clock.automatic-update.available'. Restore!");
        }
        // Restore the setting to the current value.
        this.setClockAutoUpdateAvailable(isClockAutoUpdateAvailable);
      }
    }

    // Don't allow any content processes to modify the setting
    // "time.timezone.automatic-update.available" except for the chrome
    // process.
    if (aName === kSettingsTimezoneAutoUpdateAvailable &&
        !aIsInternalSetting) {
      let isTimezoneAutoUpdateAvailable = this._lastNitzMessage !== null;
      if (aResult !== isTimezoneAutoUpdateAvailable) {
        if (DEBUG) {
          this.debug("Content processes cannot modify 'time.timezone.automatic-update.available'. Restore!");
        }
        // Restore the setting to the current value.
        this.setTimezoneAutoUpdateAvailable(isTimezoneAutoUpdateAvailable);
      }
    }

    this.handle(aName, aResult);
  },

  // nsISettingsServiceCallback
  handle: function(aName, aResult) {
    switch(aName) {
      case kSettingsClockAutoUpdateEnabled:
        this._clockAutoUpdateEnabled = aResult;
        if (!this._clockAutoUpdateEnabled) {
          break;
        }

        // Set the latest cached NITZ time if it's available.
        if (this._lastNitzMessage) {
          this.setClockByNitz(this._lastNitzMessage);
        } else if (gNetworkManager.activeNetworkInfo &&
                   gNetworkManager.activeNetworkInfo.state ==
                   Ci.nsINetworkInfo.NETWORK_STATE_CONNECTED) {

          //Cameron mark first.
          // Set the latest cached SNTP time if it's available.
          /*if (!this._sntp.isExpired()) {
            this.setClockBySntp(this._sntp.getOffset());
          } else {
            // Or refresh the SNTP.
            this._sntp.request();
          }*/
        } else {
          // Set a sane minimum time.
          let buildTime = libcutils.property_get("ro.build.date.utc", "0") * 1000;
          let file = FileUtils.File("/system/b2g/b2g");
          if (file.lastModifiedTime > buildTime) {
            buildTime = file.lastModifiedTime;
          }
          if (buildTime > Date.now()) {
            gTimeService.set(buildTime);
          }
        }
        break;
      case kSettingsTimezoneAutoUpdateEnabled:
        this._timezoneAutoUpdateEnabled = aResult;

        if (this._timezoneAutoUpdateEnabled) {
          // Apply the latest cached NITZ for timezone if it's available.
          if (this._timezoneAutoUpdateEnabled && this._lastNitzMessage) {
            this.setTimezoneByNitz(this._lastNitzMessage);
          }
        }
        break;
    }
  },

  handleError: function(aErrorMessage) {
    if (DEBUG) {
      this.debug("There was an error while reading RIL settings.");
    }
  },

  // nsIRadioInterface

  // TODO: Bug 928861 - B2G NetworkManager: Provide a more generic function
  //                    for connecting
  setupDataCallByType: function(networkType) {
    let connHandler = gDataCallManager.getDataCallHandler(this.clientId);
    connHandler.setupDataCallByType(networkType);
  },

  // TODO: Bug 928861 - B2G NetworkManager: Provide a more generic function
  //                    for connecting
  deactivateDataCallByType: function(networkType) {
    let connHandler = gDataCallManager.getDataCallHandler(this.clientId);
    connHandler.deactivateDataCallByType(networkType);
  },

  // TODO: Bug 904514 - [meta] NetworkManager enhancement
  getDataCallStateByType: function(networkType) {
    let connHandler = gDataCallManager.getDataCallHandler(this.clientId);
    return connHandler.getDataCallStateByType(networkType);
  },

  // nsIRilCallback

  handleRilIndication: function(response) {
    if (DEBUG) {
      this.debug(this.clientId, "Received message from ril: " + JSON.stringify(response));
    }

    if (!response) {
      return;
    }

    this.handleUnsolicitedMessage(response);
    return;
  },

  handleRilResponse: function(response) {
    if (DEBUG) this.debug("Received message from worker handleRilResponse:" + JSON.stringify(response));

    if (!response) {
      return;
    }

    // For backward avaliable. Intention for not change the origianl parameter naming for the upperlayer.
    let result = {};
    result.rilMessageType = response.rilMessageType;
    result.errorMsg = response.errorMsg;

    // Handle solic reponse by function then call the call back.
    switch(response.rilMessageType){
      case "getICCStatus":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_SIM_STATUS cardStatus = "
                      + JSON.stringify(response.cardStatus));
          this.handleIccStatus(response);
          result.cardState = this.cardState;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_SIM_STATUS error = " + response.errorMsg);
        }
        break;
      case "iccUnlockCardLock":
        if (DEBUG) this.debug("iccUnlockCardLock command error.");
        break;
      case "iccSetCardLockEnabled":
        if (DEBUG) this.debug("iccSetCardLockEnabled command error.");
        break;
      case "iccGetCardLockEnabled":
        if (DEBUG) this.debug("iccGetCardLockEnabled command error.");
        break;
      case "iccChangeCardLockPassword":
        if (DEBUG) this.debug("iccChangeCardLockPassword command error.");
        break;
      case "iccGetCardLockRetryCount":
        if (DEBUG) this.debug("iccGetCardLockRetryCount command error.");
        break;
      case "queryICCFacilityLock":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_FACILITY_LOCK response = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_FACILITY_LOCK error = " + response.errorMsg);
        }
        break;
      case "setICCFacilityLock":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_FACILITY_LOCK remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_FACILITY_LOCK error = " + response.errorMsg);
        }
        break;
      case "enterICCPIN":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PIN remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PIN error = " + response.errorMsg);
        }
        break;
      case "enterICCPUK":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PUK remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PUK error = " + response.errorMsg);
        }
        break;
      case "enterICCPIN2":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PIN2 remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PIN2 error = " + response.errorMsg);
        }
        break;
      case "enterICCPUK2":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PUK2 remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ENTER_SIM_PUK2 error = " + response.errorMsg);
        }
        break;
      case "changeICCPIN":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_SIM_PIN remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_SIM_PIN error = " + response.errorMsg);
        }
        break;
      case "changeICCPIN2":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_SIM_PIN2 remainingRetries = " + JSON.stringify(response.remainingRetries));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_SIM_PIN2 error = " + response.errorMsg);
        }
        break;
      case "enterDepersonalization":
        break;
      case "getCurrentCalls":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_CURRENT_CALLS");
          result.calls = this.handleCurrentCalls(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_CURRENT_CALLS error = " + response.errorMsg);
        }
        break;
      case "dial":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DIAL");
          result = response;
          //this.handleRequestDial();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DIAL error = " + response.errorMsg);
        }
        break;
      case "getIMSI":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_IMSI imsi = " + response.imsi);
          this.iccInfoPrivate.imsi = response.imsi;
          result = response;
          gIccService.notifyImsiChanged(this.clientId, this.iccInfoPrivate.imsi);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_IMSI error = " + response.errorMsg);
        }
        break;
      case "hangUpCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP");
          result = response;
          //this.handleHangUpCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP error = " + response.errorMsg);
        }
        break;
      case "hangUpBackground":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND");
          result = response;
          //this.handleHangUpBackgroundCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND error = " + response.errorMsg);
        }
        break;
      case "hangUpForeground":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND");
          result = response;
          //this.handleHangUpForegroundCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND error = " + response.errorMsg);
        }
        break;
      case "switchActiveCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE");
          result = response;
          //this.handleSwitchActiveCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE error = " + response.errorMsg);
        }
        break;
      case "conferenceCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CONFERENCE");
          result = response;
          //this.handleConferenceCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CONFERENCE error = " + response.errorMsg);
        }
        break;
      case "udub":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_UDUB");
          result = response;
          //this.handleRejectCall();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_UDUB error = " + response.errorMsg);
        }
        break;
      case "getFailCause":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_LAST_CALL_FAIL_CAUSE Cause = "
                      + response.causeCode);
          //this.handleLastCallFailCause(response);
          // Treat it as CALL_FAIL_ERROR_UNSPECIFIED if the request failed.
          let failCause = RIL.RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[RIL.CALL_FAIL_ERROR_UNSPECIFIED];
          failCause = RIL.RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[response.causeCode] || failCause;
          if (DEBUG) this.debug("Last call fail cause: " + failCause);
          result.failCause = failCause;
          result.vendorCause = response.vendorCause;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_LAST_CALL_FAIL_CAUSE error = " + response.errorMsg);
        }
        break;
      case "explicitCallTransfer":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_EXPLICIT_CALL_TRANSFER");
          result = response;
          //this.handleExplicitCallTransfer();
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_EXPLICIT_CALL_TRANSFER error = " + response.errorMsg);
        }
        break;
      case "getSignalStrength":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SIGNAL_STRENGTH signalStrength = "
                      + JSON.stringify(response.signalStrength));
          this.handleSignalStrength(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SIGNAL_STRENGTH error = " + response.errorMsg);
        }
        break;
      case "getVoiceRegistrationState":
        this._receivedNetworkInfo(RIL.NETWORK_INFO_VOICE_REGISTRATION_STATE);
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_REGISTRATION_STATE voiceRegStatus = "
                      + JSON.stringify(response.voiceRegStatus));

          this.handleVoiceRegistrationState(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_REGISTRATION_STATE error = " + response.errorMsg);
        }
        break;
      case "getDataRegistrationState":
        this._receivedNetworkInfo(RIL.NETWORK_INFO_DATA_REGISTRATION_STATE);
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DATA_REGISTRATION_STATE dataRegStatus = "
                      + JSON.stringify(response.dataRegStatus));
          this.handleDataRegistrationState(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_REGISTRATION_STATE error = " + response.errorMsg);
        }
        break;
      case "getOperator":
        this._receivedNetworkInfo(RIL.NETWORK_INFO_OPERATOR);
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_OPERATOR operator = "
                      + JSON.stringify(response.operator));

          this.handleOperator(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_OPERATOR error = " + response.errorMsg);
        }
        break;
      case "getNetworkSelectionMode":
        this._receivedNetworkInfo(RIL.NETWORK_INFO_NETWORK_SELECTION_MODE);
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE nwModeManual = "
                    + response.nwModeManual);
          this.handleNetworkSelectionMode(response.nwModeManual);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE error = " + response.errorMsg);
        }
        break;
      case "setRadioEnabled":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_RADIO_POWER");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_RADIO_POWER error = " + response.errorMsg);
        }
        break;
      case "sendSMS":
        break;
      case "setupDataCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SETUP_DATA_CALL dcResponse = " + JSON.stringify(response.dcResponse));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SETUP_DATA_CALL error = " + response.errorMsg);

        }
        break;
      case "iccIO":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SIM_IO IccIoResult = " + JSON.stringify(response.iccIo));
          this.handleIccIoResult(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SIM_IO error = " + response.errorMsg + " , IccIoResult =" + JSON.stringify(response.iccIo));
          this.handleIccIoErrorResult(response);
        }
        break;
      case "sendUSSD":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SEND_USSD");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SEND_USSD error = " + response.errorMsg);
        }
        break;
      case "cancelUSSD":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CANCEL_USSD");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CANCEL_USSD error = " + response.errorMsg);
        }
        break;
      case "getCLIR":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_CLIR n = " + JSON.stringify(response.n) + " , m = " + JSON.stringify(response.m));
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_CLIR error = " + response.errorMsg);
        }
        break;
      case "setCLIR":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CLIR");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CLIR error = " + response.errorMsg);
        }
        break;
      case "queryCallForwardStatus":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CALL_FORWARD_STATUS");
          result = handleQueryCallForwardStatus(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CALL_FORWARD_STATUS error = " + response.errorMsg);
        }
        break;
      case "setCallForward":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CALL_FORWARD");
          this._setCallForwardToSim(this._callForwardOptions);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CALL_FORWARD error = " + response.errorMsg);
          this._callForwardOptions = {};
        }
        break;
      case "queryCallWaiting":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CALL_WAITING");
          result.serviceClass = response.enable? response.serviceClass : RIL.ICC_SERVICE_CLASS_NONE;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CALL_WAITING error = " + response.errorMsg);
        }
        break;
      case "setCallWaiting":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CALL_WAITING");
          result.serviceClass = response.enable? response.serviceClass : RIL.ICC_SERVICE_CLASS_NONE;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CALL_WAITING error = " + response.errorMsg);
        }
        break;
      case "acknowledgeGsmSms":
        break;
      case "answerCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ANSWER");
          //this.handleAnswerCall();
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ANSWER error = " + response.errorMsg);
        }
        break;
      case "deactivateDataCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DEACTIVATE_DATA_CALL");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DEACTIVATE_DATA_CALL error = " + response.errorMsg);
        }
        break;
      case "changeCallBarringPassword":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_BARRING_PASSWORD");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_CHANGE_BARRING_PASSWORD error = " + response.errorMsg);
        }
        break;
      case "selectNetworkAuto":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC");
          this.handleNetworkSelectionMode(false);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC error = " + response.errorMsg);
        }
        break;
      case "selectNetwork":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL");
          this.handleNetworkSelectionMode(true);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL error = " + response.errorMsg);
        }
        break;
      case "getAvailableNetworks":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_AVAILABLE_NETWORKS");
          result.networks = this.handleAvailableNetworks(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_AVAILABLE_NETWORKS error = " + response.errorMsg);
        }
        break;
      case "sendTone":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF error = " + response.errorMsg);
        }
        break;
      case "startTone":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF_START");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF_START error = " + response.errorMsg);
        }
        break;
      case "stopTone":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF_STOP");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DTMF_STOP error = " + response.errorMsg);
        }
        break;
      case "getBasebandVersion":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_BASEBAND_VERSION basebandVersion = "
                      + response.basebandVersion);
          this.handleBasebandVersion(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_BASEBAND_VERSION error = " + response.errorMsg);
        }
        break;
      case "separateCall":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SEPARATE_CONNECTION");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SEPARATE_CONNECTION error = " + response.errorMsg);
        }
        break;
      case "setMute":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_MUTE");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_MUTE error = " + response.errorMsg);
        }
        break;
      case "queryCLIP":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CLIP provisioned = " + response.provisioned);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_CLIP error = " + response.errorMsg);
        }
        break;
      case "getDataCallList":
        if (response.errorMsg == 0) {
          let dataCallLists = response.getDataCallLists();
          if (DEBUG) this.debug("RILJ: [UNSL]< RIL_REQUEST_DATA_CALL_LIST :" + JSON.stringify(dataCallLists));
          result.datacalls = dataCallLists;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE error = " + response.errorMsg);
        }
        break;
      case "writeSmsToSIM":
        break;
      case "setPreferredNetworkType":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE error = " + response.errorMsg);
        }
        break;
      case "getPreferredNetworkType":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE type = " + response.type);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE error = " + response.errorMsg);
        }
        break;
      case "getNeighboringCellIds":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_NEIGHBORING_CELL_IDS");
          result = this.handleNeighboringCellIds(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_NEIGHBORING_CELL_IDS error = " + response.errorMsg);
        }
        break;
      case "queryTtyMode":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_QUERY_TTY_MODE ttymode = " + response.ttyMode);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_NEIGHBORING_CELL_IDS error = " + response.errorMsg);
        }
        break;
      case "setRoamingPreference":
        break;
      case "queryRoamingPreference":
        break;
      case "setTtyMode":
        break;
      case "setVoicePrivacyMode":
        break;
      case "queryVoicePrivacyMode":
        break;
      case "cdmaFlash":
        break;
      case "sendSMS":
        break;
      case "acknowledgeCdmaSms":
        break;
      case "setCellBroadcastDisabled":
        // This is not a ril reponse.
        result = response;
        break;
      case "setCellBroadcastSearchList":
        // This is not a ril reponse.
        result = response;
        break;
      case "setGsmBroadcastActivation":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GSM_BROADCAST_ACTIVATION");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GSM_BROADCAST_ACTIVATION error = " + response.errorMsg);
        }
        break;
      case "setGsmBroadcastConfig":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GSM_SET_BROADCAST_CONFIG");
          this.setSmsBroadcastActivation(true);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GSM_SET_BROADCAST_CONFIG error = " + response.errorMsg);
        }
        break;
      case "setCdmaSmsBroadcastConfig":
        break;
      case "getCdmaSubscription":
        break;
      case "getDeviceIdentity":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DEVICE_IDENTITY imei = "
                      + response.imei + " , imeisv = " + response.imeisv + " ,esn = " + response.esn + " , meid = " + response.meid);
          result.deviceIdentities = this.handleDeviceIdentity(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_DEVICE_IDENTITY error = " + response.errorMsg);
        }
        break;
      case "sendExitEmergencyCbModeRequest":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE error = " + response.errorMsg);
        }
        break;
      case "getSmscAddress":
      if (response.errorMsg == 0) {
        if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_SMSC_ADDRESS smsc= " + response.smsc);

          result = this.handleSmscAddress(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_SMSC_ADDRESS error = " + response.errorMsg);
        }
        break;
      case "setSmscAddress":
        break;
      case "reportSmsMemoryStatus":
        break;
      case "reportStkServiceIsRunning":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING error = " + response.errorMsg);
        }
        break;
      case "acknowledgeIncomingGsmSmsWithPDU":
        break;
      case "dataDownloadViaSMSPP":
        break;
      case "getVoiceRadioTechnology":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_RADIO_TECH radioTech = "
                      + response.radioTech);
          this.handleVoiceRadioTechnology(response);
          result = this.radioTech;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_RADIO_TECH error = " + response.errorMsg);
        }
        break;
      case "getCellInfoList":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_GET_CELL_INFO_LIST cellInfoLists = " + JSON.stringify(response.getCellInfoList()));
          result.result = this.handleCellInfoList(response);
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_VOICE_RADIO_TECH error = " + response.errorMsg);
        }
        break;
      case "setInitialAttachApn":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_INITIAL_ATTACH_APN");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_INITIAL_ATTACH_APN error = " + response.errorMsg);
        }
        break;
      case "iccOpenChannel":
        break;
      case "iccCloseChannel":
        break;
      case "iccExchangeAPDU":
        break;
      case "setUiccSubscription":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_UICC_SUBSCRIPTION");
          this.handleUiccSubscription(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_UICC_SUBSCRIPTION error = " + response.errorMsg);
        }
        break;
      case "setDataRegistration":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ALLOW_DATA");
          this.handleDataRegistration(response);
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_ALLOW_DATA error = " + response.errorMsg);
        }
        break;
      case "getIccAuthentication":
        break;
      case "setCellInfoListRate":
        if (response.errorMsg == 0) {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CELL_INFO_LIST_RATE");
          result = response;
        } else {
          if (DEBUG) this.debug("RILJ: ["+ response.rilMessageToken +"] < RIL_REQUEST_SET_CELL_INFO_LIST_RATE error = " + response.errorMsg);
        }
        break;
      default:
    }

    let token = response.rilMessageToken;

    let callback = this.tokenCallbackMap[token];
    if (!callback) {
      if (DEBUG) this.debug(this.clientId, "Ignore orphan token: " + token);
      return;
    }

    let keep = false;
    try {
      keep = callback(result);
    } catch(e) {
      if (DEBUG) this.debug(this.clientId, "callback throws an exception: " + e);
    }

    if (!keep) {
      delete this.tokenCallbackMap[token];
    }
  },

  // We combine all of the NETWORK_INFO_MESSAGE_TYPES into one "networkinfochange"
  // message to the RadioInterfaceLayer, so we can avoid sending multiple
  // VoiceInfoChanged events for both operator / voice_data_registration
  //
  // State management here is a little tricky. We need to know both:
  // 1. Whether or not a response was received for each of the
  //    NETWORK_INFO_MESSAGE_TYPES
  // 2. The outbound message that corresponds with that response -- but this
  //    only happens when internal state changes (i.e. it isn't guaranteed)
  //
  // To collect this state, each message response function first calls
  // _receivedNetworkInfo, to mark the response as received. When the
  // final response is received, a call to _sendPendingNetworkInfo is placed
  // on the next tick of the worker thread.
  //
  // Since the original call to _receivedNetworkInfo happens at the top
  // of the response handler, this gives the final handler a chance to
  // queue up it's "changed" message by calling _sendNetworkInfoMessage if/when
  // the internal state has actually changed.
  _sendNetworkInfoMessage: function(type, message) {
    if (!this._processingNetworkInfo) {
      // We only combine these messages in the case of the combined request
      // in requestNetworkInfo()
      this.handleUnsolicitedMessage(message);
      return;
    }

    if (DEBUG) {
      this.debug("Queuing " + type + " network info message: " +
                         JSON.stringify(message));
    }
    this._pendingNetworkInfo[type] = message;
  },

  _receivedNetworkInfo: function(type) {
    if (DEBUG) this.debug("Received " + type + " network info.");
    if (!this._processingNetworkInfo) {
      return;
    }

    let pending = this._pendingNetworkInfo;

    // We still need to track states for events that aren't fired.
    if (!(type in pending)) {
      pending[type] = this.pendingNetworkType;
    }

    // Pending network info is ready to be sent when no more messages
    // are waiting for responses, but the combined payload hasn't been sent.
    for (let i = 0; i < RIL.NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      let msgType = RIL.NETWORK_INFO_MESSAGE_TYPES[i];
      if (!(msgType in pending)) {
        if (DEBUG) {
          this.debug("Still missing some more network info, not " +
                             "notifying main thread.");
        }
        return;
      }
    }

    // Do a pass to clean up the processed messages that didn't create
    // a response message, so we don't have unused keys in the outbound
    // networkinfochanged message.
    for (let key in pending) {
      if (pending[key] == this.pendingNetworkType) {
        delete pending[key];
      }
    }

    if (DEBUG) {
      this.debug("All pending network info has been received: " +
                         JSON.stringify(pending));
    }

    // Send the message on the next tick of the worker's loop, so we give the
    // last message a chance to call _sendNetworkInfoMessage first.
    //setTimeout(this._sendPendingNetworkInfo.bind(this), 0);
    this._sendPendingNetworkInfo();
  },

  _sendPendingNetworkInfo: function() {
    this.handleUnsolicitedMessage(this._pendingNetworkInfo);

    this._processingNetworkInfo = false;
    for (let i = 0; i < RIL.NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      delete this._pendingNetworkInfo[RIL.NETWORK_INFO_MESSAGE_TYPES[i]];
    }

    if (this._needRepollNetworkInfo) {
      this._needRepollNetworkInfo = false;
      this.requestNetworkInfo();
    }
  },
  /**
   * Process LTE signal strength to the signal info object.
   *
   * @param signal
   *        The signal object reported from RIL/modem.
   *
   * @return The object of signal strength info.
   *         Or null if invalid signal input.
   *
   * TODO: Bug 982013: reconsider the format of signal strength APIs for
   *       GSM/CDMA/LTE to expose details, such as rsrp and rsnnr,
   *       individually.
   */

  /**
   * Process the network registration flags.
   *
   * @return true if the state changed, false otherwise.
   */
  _processCREG: function(curState, newState) {
    let changed = false;

    let regState = newState.regState;
    if (curState.regState === undefined || curState.regState !== regState) {
      changed = true;
      curState.regState = regState;

      curState.state = RIL.NETWORK_CREG_TO_GECKO_MOBILE_CONNECTION_STATE[regState];
      curState.connected = regState == Ci.nsIRilResponseResult.RADIO_REG_STATE_REG_HOME ||
                           regState == Ci.nsIRilResponseResult.RADIO_REG_STATE_REG_ROAMING;
      curState.roaming = regState == Ci.nsIRilResponseResult.RADIO_REG_STATE_REG_ROAMING;
      curState.emergencyCallsOnly = !curState.connected;
    }

    if (!curState.cell) {
      curState.cell = {};
    }
    //Cameron TODO need to separate it to GSM/WCDMA/LTE/TDSCDMA
    // Current MobileCellInfo only support GSM or CDMA.
    if (newState.cellIdentity) {
      switch (newState.cellIdentity.cellInfoType) {
        case Ci.nsIRilResponseResult.RADIO_CELL_INFO_TYPE_GSM: {
          let lac = newState.cellIdentity.cellIdentityGsm.lac;
          if (curState.cell.gsmLocationAreaCode === undefined ||
              curState.cell.gsmLocationAreaCode !== lac) {
            curState.cell.gsmLocationAreaCode = lac;
            changed = true;
          }

          let cid = newState.cellIdentity.cellIdentityGsm.cid;
          if (curState.cell.gsmCellId === undefined ||
              curState.cell.gsmCellId !== cid) {
            curState.cell.gsmCellId = cid;
            changed = true;
          }
          break;
        }
        case Ci.nsIRilResponseResult.RADIO_CELL_INFO_TYPE_WCDMA: {
          let lac = newState.cellIdentity.cellIdentityWcdma.lac;
          if (curState.cell.gsmLocationAreaCode === undefined ||
              curState.cell.gsmLocationAreaCode !== lac) {
            curState.cell.gsmLocationAreaCode = lac;
            changed = true;
          }

          let cid = newState.cellIdentity.cellIdentityWcdma.cid;
          if (curState.cell.gsmCellId === undefined ||
              curState.cell.gsmCellId !== cid) {
            curState.cell.gsmCellId = cid;
            changed = true;
          }
          break;
        }
        case Ci.nsIRilResponseResult.RADIO_CELL_INFO_TYPE_TD_SCDMA: {
          let lac = newState.cellIdentity.cellIdentityTdScdma.lac;
          if (curState.cell.gsmLocationAreaCode === undefined ||
              curState.cell.gsmLocationAreaCode !== lac) {
            curState.cell.gsmLocationAreaCode = lac;
            changed = true;
          }

          let cid = newState.cellIdentity.cellIdentityTdScdma.cid;
          if (curState.cell.gsmCellId === undefined ||
              curState.cell.gsmCellId !== cid) {
            curState.cell.gsmCellId = cid;
            changed = true;
          }
          break;
        }
        default:
          break;
      }
    } else {
      if (DEBUG) this.debug("cellIdentity null.");
    }

    let radioTech = newState.rat;
    if (curState.radioTech === undefined || curState.radioTech !== radioTech) {
      changed = true;
      curState.radioTech = radioTech;
      curState.type = RIL.GECKO_RADIO_TECH[radioTech] || null;
    }
    // From TS 23.003, 0000 and 0xfffe are indicated that no valid LAI exists
    // in MS. So we still need to report the '0000' as well.
    /*let lac = this.parseInt(newState[1], -1, 16);
    if (curState.cell.gsmLocationAreaCode === undefined ||
        curState.cell.gsmLocationAreaCode !== lac) {
      curState.cell.gsmLocationAreaCode = lac;
      changed = true;
    }

    let cid = this.parseInt(newState[2], -1, 16);
    if (curState.cell.gsmCellId === undefined ||
        curState.cell.gsmCellId !== cid) {
      curState.cell.gsmCellId = cid;
      changed = true;
    }

    let radioTech = (newState[3] === undefined ?
                     NETWORK_CREG_TECH_UNKNOWN :
                     this.parseInt(newState[3], NETWORK_CREG_TECH_UNKNOWN));
    if (curState.radioTech === undefined || curState.radioTech !== radioTech) {
      changed = true;
      curState.radioTech = radioTech;
      curState.type = GECKO_RADIO_TECH[radioTech] || null;
    }*/
    return changed;
  },

  handleVoiceRegistrationState: function(response){
    if (DEBUG) this.debug("handleVoiceRegistrationState " + JSON.stringify(response.voiceRegStatus));
    let rs = this.voiceRegistrationState;
    let state = response.voiceRegStatus;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged && rs.connected) {
      this.sendRilRequest("getSmscAddress", null);
    }

    let cell = rs.cell;
    if (this._isCdma) {
      if (newState.cellIdentity
          && newState.cellIdentity.cellInfoType == Ci.nsIRilResponseResult.RADIO_CELL_INFO_TYPE_CDMA) {
        // Some variables below are not used. Comment them instead of removing to
        // keep the information about state[x].
        let cdmaBaseStationId = newState.cellIdentity.cellIdentityCdma.baseStationId;
        let cdmaBaseStationLatitude = newState.cellIdentity.cellIdentityCdma.latitude;
        let cdmaBaseStationLongitude = newState.cellIdentity.cellIdentityCdma.longitude;
        // let cssIndicator = this.parseInt(state[7]);
        let cdmaSystemId = newState.cellIdentity.cellIdentityCdma.systemId;
        let cdmaNetworkId = newState.cellIdentity.cellIdentityCdma.networkId;
        let cdmaRoamingIndicator = newState.roamingIndicator;
        let cdmaSystemIsInPRL = !!newState.systemIsInPrl;
        let cdmaDefaultRoamingIndicator = newState.defaultRoamingIndicator;
        // let reasonForDenial = this.parseInt(state[13]);

        if (cell.cdmaBaseStationId !== cdmaBaseStationId ||
            cell.cdmaBaseStationLatitude !== cdmaBaseStationLatitude ||
            cell.cdmaBaseStationLongitude !== cdmaBaseStationLongitude ||
            cell.cdmaSystemId !== cdmaSystemId ||
            cell.cdmaNetworkId !== cdmaNetworkId ||
            cell.cdmaRoamingIndicator !== cdmaRoamingIndicator ||
            cell.cdmaSystemIsInPRL !== cdmaSystemIsInPRL ||
            cell.cdmaDefaultRoamingIndicator !== cdmaDefaultRoamingIndicator) {
          stateChanged = true;
          cell.cdmaBaseStationId = cdmaBaseStationId;
          cell.cdmaBaseStationLatitude = cdmaBaseStationLatitude;
          cell.cdmaBaseStationLongitude = cdmaBaseStationLongitude;
          cell.cdmaSystemId = cdmaSystemId;
          cell.cdmaNetworkId = cdmaNetworkId;
          cell.cdmaRoamingIndicator = cdmaRoamingIndicator;
          cell.cdmaSystemIsInPRL = cdmaSystemIsInPRL;
          cell.cdmaDefaultRoamingIndicator = cdmaDefaultRoamingIndicator;
        }
      }
    }

    if (stateChanged) {
      rs.rilMessageType = "voiceregistrationstatechange";
      this._sendNetworkInfoMessage(RIL.NETWORK_INFO_VOICE_REGISTRATION_STATE, rs);
    }
  },

  handleDataRegistrationState: function(response) {
    if (DEBUG) this.debug("handleDataRegistrationState " + JSON.stringify(response.dataRegStatus));
    let rs = this.dataRegistrationState;
    let state = response.dataRegStatus;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged) {
      rs.rilMessageType = "dataregistrationstatechange";
      this._sendNetworkInfoMessage(RIL.NETWORK_INFO_DATA_REGISTRATION_STATE, rs);
    }
  },

  /**
   * The "numeric" portion of the operator info is a tuple
   * containing MCC (country code) and MNC (network code).
   * AFAICT, MCC should always be 3 digits, making the remaining
   * portion the MNC.
   */
  _processNetworkTuple: function(networkTuple, network) {
    let tupleLen = networkTuple.length;

    if (tupleLen == 5 || tupleLen == 6) {
      network.mcc = networkTuple.substr(0, 3);
      network.mnc = networkTuple.substr(3);
    } else {
      network.mcc = null;
      network.mnc = null;

      throw new Error("Invalid network tuple (should be 5 or 6 digits): " + networkTuple);
    }
  },

  handleOperator: function(response) {
    if (DEBUG) this.debug("handleOperator " + JSON.stringify(response.operator));
    let operatorData = response.operator;

    if (!this.operator) {
      this.operator = {
        rilMessageType: "operatorchange",
        longName: null,
        shortName: null
      };
    }

    let longName = operatorData.alphaLong;
    let shortName = operatorData.alphaShort;
    let networkTuple = operatorData.operatorNumeric;
    let thisTuple = (this.operator.mcc || "") + (this.operator.mnc || "");

    if (this.operator.longName !== longName ||
        this.operator.shortName !== shortName ||
        thisTuple !== networkTuple) {

      this.operator.mcc = null;
      this.operator.mnc = null;

      if (networkTuple) {
        try {
          this._processNetworkTuple(networkTuple, this.operator);
        } catch (e) {
          if (DEBUG) this.debug("Error processing operator tuple: " + e);
        }
      } else {
        // According to ril.h, the operator fields will be NULL when the operator
        // is not currently registered. We can avoid trying to parse the numeric
        // tuple in that case.
        if (DEBUG) {
          this.debug("Operator is currently unregistered");
        }
      }

      this.operator.longName = longName;
      this.operator.shortName = shortName;

      let ICCUtilsHelper = this.simIOcontext.ICCUtilsHelper;
      if (ICCUtilsHelper.updateDisplayCondition() && this.iccInfo.iccid) {
        ICCUtilsHelper.handleICCInfoChange();
      }

      // NETWORK_INFO_OPERATOR message will be sent out by overrideNetworkName
      // itself if operator name is overridden after checking, or we have to
      // do it by ourself.
      if (!this.overrideNetworkName()) {
        this._sendNetworkInfoMessage(RIL.NETWORK_INFO_OPERATOR, this.operator);
      }
    }
  },

  /**
   * PLMN name display priority: EONS > ONS > NITZ (done by rild) > PLMN.
   *
   * @return true if PLMN name is overridden, false otherwise.
   */
  overrideNetworkName: function() {
    if (!this.operator) {
      return false;
    }

    if (this.overrideEonsNetworkName()) {
      if (DEBUG) {
        this.debug("Network name is overridden by EONS");
      }
      return true;
    } else if (this.overrideOnsNetworkName()) {
      if (DEBUG) {
        this.debug("Network name is overridden by ONS");
      }
      return true;
    }

    return false;
  },

  /**
   * Check if operator name needs to be overriden by current voiceRegistrationState
   * , EFOPL and EFPNN. See 3GPP TS 31.102 clause 4.2.58 EFPNN and 4.2.59 EFOPL
   *  for detail.
   *
   * @return true if operator name is overridden, false otherwise.
   */
  overrideEonsNetworkName: function() {
    // We won't get network name using PNN and OPL if voice registration isn't
    // ready.
    this.debug("overrideEonsNetworkName");
    if (!this.voiceRegistrationState.cell ||
        this.voiceRegistrationState.cell.gsmLocationAreaCode == -1) {
      return false;
    }

    this.debug("overrideEonsNetworkName gsmLocationAreaCode=" + this.voiceRegistrationState.cell.gsmLocationAreaCode);

    let ICCUtilsHelper = this.simIOcontext.ICCUtilsHelper;
    let networkName = ICCUtilsHelper.getNetworkNameFromICC(
      this.operator.mcc,
      this.operator.mnc,
      this.voiceRegistrationState.cell.gsmLocationAreaCode);

    if (!networkName) {
      return false;
    }

    if (DEBUG) {
      this.debug("Operator names will be overriden: " +
                         "longName = " + networkName.fullName + ", " +
                         "shortName = " + networkName.shortName);
    }

    this.operator.longName = networkName.fullName;
    this.operator.shortName = networkName.shortName;

    this._sendNetworkInfoMessage(RIL.NETWORK_INFO_OPERATOR, this.operator);
    return true;
  },

  /**
   * Check if operator name needs to be overriden by current voiceRegistrationState
   * , CPHS Operator Name String and Operator Name Shortform. See B.4.1.2
   * Network Operator Namefor detail.
   *
   * @return true if operator name is overridden, false otherwise.
   */
  overrideOnsNetworkName: function() {
    if (!this.iccInfoPrivate.ons) {
      return false;
    }
    this.debug("overrideOnsNetworkName this.iccInfoPrivate.ons=" + this.iccInfoPrivate.ons);

    // If ONS exists, only replace alpha long/short while under HPLMN.
    if (!this.voiceRegistrationState.roaming) {
      this.operator.longName = this.iccInfoPrivate.ons;
      this.operator.shortName = this.iccInfoPrivate.ons_short_form || "";

      this._sendNetworkInfoMessage(RIL.NETWORK_INFO_OPERATOR, this.operator);
      return true;
    }

    return false;
  },

  handleNeighboringCellIds: function (response) {
    let result = {};
    let rilNeighboringCellIds = response.getNeighboringCids();
    if (DEBUG) this.debug("handleNeighboringCellIds " + JSON.stringify(rilNeighboringCellIds));

    let radioTech = this.voiceRegistrationState.radioTech;
      if (radioTech == undefined || radioTech == RIL.NETWORK_CREG_TECH_UNKNOWN) {
      result.errorMsg = "RadioTechUnavailable";
      return result;
    }

    // Reference :
    // http://androidxref.com/7.1.1_r6/xref/frameworks/base/telephony/java/android/telephony/NeighboringCellInfo.java#113
    if (!this._isGsmTechGroup(radioTech) || radioTech === RIL.NETWORK_CREG_TECH_GSM ||
        radioTech === RIL.NETWORK_CREG_TECH_LTE) {
      result.errorMsg = "UnsupportedRadioTech";
      return result;
    }

    let neighboringCellIds = [];
    for (let i = 0; i < num; i++) {
      let cellId = {};
      cellId.networkType = RIL.GECKO_RADIO_TECH[radioTech];
      cellId.signalStrength = rilNeighboringCellIds[i].rssi;

      let cid = rilNeighboringCellIds[i].cid;
      // pad cid string with leading "0"
      let length = cid.length;
      if (length > 8) {
        continue;
      }
      if (length < 8) {
        for (let j = 0; j < (8-length); j++) {
          cid = "0" + cid;
        }
      }

      switch (radioTech) {
        case RIL.NETWORK_CREG_TECH_GPRS:
        case RIL.NETWORK_CREG_TECH_EDGE:
          cellId.gsmCellId = this.parseInt(cid.substring(4), -1, 16);
          cellId.gsmLocationAreaCode = this.parseInt(cid.substring(0, 4), -1, 16);
          break;
        case RIL.NETWORK_CREG_TECH_UMTS:
        case RIL.NETWORK_CREG_TECH_HSDPA:
        case RIL.NETWORK_CREG_TECH_HSUPA:
        case RIL.NETWORK_CREG_TECH_HSPA:
        case RIL.NETWORK_CREG_TECH_HSPAP:
          cellId.wcdmaPsc = this.parseInt(cid, -1, 16);
          break;
        default:
          continue;
      }

      neighboringCellIds.push(cellId);
    }

    result.result = neighboringCellIds;
    return result;
  },

  /**
   * Parse an integer from a string, falling back to a default value
   * if the the provided value is not a string or does not contain a valid
   * number.
   *
   * @param string
   *        String to be parsed.
   * @param defaultValue [optional]
   *        Default value to be used.
   * @param radix [optional]
   *        A number that represents the numeral system to be used. Default 10.
   */
  parseInt: function(string, defaultValue, radix) {
    let number = parseInt(string, radix || 10);
    if (!isNaN(number)) {
      return number;
    }
    if (defaultValue === undefined) {
      defaultValue = null;
    }
    return defaultValue;
  },

  handleNetworkSelectionMode: function (mode) {
    if (DEBUG) this.debug("handleNetworkSelectionMode " + JSON.stringify(mode));

    let selectionMode = mode? RIL.GECKO_NETWORK_SELECTION_MANUAL : RIL.GECKO_NETWORK_SELECTION_AUTOMATIC;
    if (this.networkSelectionMode === selectionMode) {
      return;
    }

    let options = {
      rilMessageType: "networkselectionmodechange",
      mode: selectionMode
    };
    this.networkSelectionMode = selectionMode;
    this._sendNetworkInfoMessage(RIL.NETWORK_INFO_NETWORK_SELECTION_MODE, options);
  },

  handleSignalStrength: function (response) {
    if (DEBUG) this.debug("handleSignalStrength " + JSON.stringify(response.signalStrength));
    let signalStrength = response.signalStrength;

    let signal = {};

    signal.gsmSignalStrength = signalStrength.gsmSignalStrength.signalStrength;
    signal.gsmBitErrorRate = signalStrength.gsmSignalStrength.bitErrorRate;

    signal.cdmaDbm = signalStrength.cdmaSignalStrength.dbm;
    signal.cdmaEcio = signalStrength.cdmaSignalStrength.ecio;
    signal.cdmaEvdoDbm = signalStrength.evdoSignalStrength.dbm;
    signal.cdmaEvdoEcio = signalStrength.evdoSignalStrength.ecio;
    signal.cdmaEvdoSNR = signalStrength.evdoSignalStrength.signalNoiseRatio;

    signal.lteSignalStrength = signalStrength.lteSignalStrength.signalStrength;
    signal.lteRsrp =           signalStrength.lteSignalStrength.rsrp;
    signal.lteRsrq =           signalStrength.lteSignalStrength.rsrq;
    signal.lteRssnr =          signalStrength.lteSignalStrength.rssnr;
    signal.lteCqi =            signalStrength.lteSignalStrength.cqi;
    signal.lteTimingAdvance =  signalStrength.lteSignalStrength.timingAdvance;

    signal.tdscdmaRscp =       signalStrength.tdscdmaSignalStrength.rscp;

    if (DEBUG) this.debug("signal strength: " + JSON.stringify(signal));

    gMobileConnectionService.notifySignalStrengthChanged(this.clientId, signal);
  },

  handleUiccSubscription: function (response) {
    // Resend data subscription after uicc subscription.
    if (this._attachDataRegistration.attach) {
      this.setDataRegistration({attach: true});
    }
  },

  handleIccStatus: function (response) {
    if (DEBUG) this.debug("handleIccStatus " + JSON.stringify(response.cardStatus));
       // If |_waitingRadioTech| is true, we should not get app information because
    // the |_isCdma| flag is not ready yet. Otherwise we may use wrong index to
    // get app information, especially for the case that icc card has both cdma
    // and gsm subscription.
    if (this._waitingRadioTech) {
      return;
    }

    let iccStatus = response.cardStatus;
    // When |iccStatus.cardState| is not CARD_STATE_PRESENT, set cardState to
    // undetected.
    if (iccStatus.cardState !== Ci.nsIRilResponseResult.CARD_STATE_PRESENT) {
      if (this.cardState !== Ci.nsIRilResponseResult.CARD_STATE_UNDETECTED) {
        this.operator = null;
        // We should send |cardstatechange| before |iccinfochange|, otherwise we
        // may lost cardstatechange event when icc card becomes undetected.
        this.cardState = Ci.nsIRilResponseResult.CARD_STATE_UNDETECTED;

        gIccService.notifyCardStateChanged(this.clientId, this.cardState);
        gRadioEnabledController.receiveCardState(this.clientId);
        this.iccInfo = {iccType: null};
        gIccService.notifyIccInfoChanged(this.clientId, this.iccInfo);
      }
      return;
    }

    if (libcutils.property_get("ro.moz.ril.subscription_control", "false") === "true") {
      // All appIndex is -1 means the subscription is not activated yet.
      // Note that we don't support "ims" for now, so we don't take it into
      // account.
      let neetToActivate = iccStatus.cdmaSubscriptionAppIndex === -1 &&
                           iccStatus.gsmUmtsSubscriptionAppIndex === -1;
      if (neetToActivate &&
          // Note: setUiccSubscription works abnormally when RADIO is OFF,
          // which causes SMS function broken in Flame.
          // See bug 1008557 for detailed info.
          this.radioState === Ci.nsIRilIndicationResult.GECKO_RADIOSTATE_ENABLED) {
        for (let i = 0; i < iccStatus.apps.length; i++) {
          this.sendRilRequest("setUiccSubscription", {slotId: this.clientId
                                                      , appIndex: i
                                                      , subId: this.clientId
                                                      , subStatus: true});
        }
      }
    }

    let newCardState;
    let index = this._isCdma ? iccStatus.cdmaSubscriptionAppIndex
                             : iccStatus.gsmUmtsSubscriptionAppIndex;
    let apps = iccStatus.getAppStatus();
    let app = apps[index];

    if (app) {
      // fetchICCRecords will need to read aid, so read aid here.
      this.aid = app.aidPtr;
      this.appType = app.appType;
      this.iccInfo.iccType = RIL.GECKO_CARD_TYPE[this.appType];

      switch (app.appState) {
        case RIL.CARD_APPSTATE_ILLEGAL:
          newCardState = RIL.GECKO_CARDSTATE_ILLEGAL;
          break;
        case RIL.CARD_APPSTATE_PIN:
          newCardState = RIL.GECKO_CARDSTATE_PIN_REQUIRED;
          break;
        case RIL.CARD_APPSTATE_PUK:
          newCardState = RIL.GECKO_CARDSTATE_PUK_REQUIRED;
          break;
        case RIL.CARD_APPSTATE_READY:
          newCardState = RIL.GECKO_CARDSTATE_READY;
          break;
        case RIL.CARD_APPSTATE_SUBSCRIPTION_PERSO:
          newCardState = PERSONSUBSTATE[app.persoSubstate];
          break;
        case RIL.CARD_APPSTATE_UNKNOWN:
        case RIL.CARD_APPSTATE_DETECTED:
          // Fall through.
        default:
          newCardState = RIL.GECKO_CARDSTATE_UNKNOWN;
      }

      let pin1State = app.pin1Replaced ? iccStatus.universalPinState :
                                          app.pin1;
      if (pin1State === RIL.CARD_PINSTATE_ENABLED_PERM_BLOCKED) {
        newCardState = RIL.GECKO_CARDSTATE_PERMANENT_BLOCKED;
      }
    } else {
      // Having incorrect app information, set card state to unknown.
      newCardState = RIL.GECKO_CARDSTATE_UNKNOWN;
    }

    //Read the iccid only when card app status change from
    //uninitialized,undetected,unknown to ready or locked
    if ((this.cardState === RIL.GECKO_CARDSTATE_UNINITIALIZED ||
         this.cardState === RIL.GECKO_CARDSTATE_UNDETECTED ||
         this.cardState === RIL.GECKO_CARDSTATE_UNKNOWN) &&
        (newCardState != RIL.GECKO_CARDSTATE_UNINITIALIZED &&
         newCardState != RIL.GECKO_CARDSTATE_UNDETECTED &&
         newCardState != RIL.GECKO_CARDSTATE_UNKNOWN)) {
      this.simIOcontext.ICCRecordHelper.readICCID();
    }

    if (this.cardState == newCardState) {
      return;
    }

    // This was moved down from CARD_APPSTATE_READY
    this.requestNetworkInfo();
    if (newCardState == RIL.GECKO_CARDSTATE_READY) {
      // Cache SIM/RUIM/ISIM aid(s) for later use.
      let usimApp = apps[iccStatus.gsmUmtsSubscriptionAppIndex];
      if (usimApp) {
        this.simIOcontext.SimRecordHelper.setAid(usimApp.aidPtr);
      }

      let ruimApp = apps[iccStatus.cdmaSubscriptionAppIndex];
      if (ruimApp) {
        this.simIOcontext.RuimRecordHelper.setAid(ruimApp.aidPtr);
      }

      let isimApp = apps[iccStatus.imsSubscriptionAppIndex];
      if (isimApp) {
        this.simIOcontext.ISimRecordHelper.setAid(isimApp.aidPtr);
      }

      // For type SIM, we need to check EF_phase first.
      // Other types of ICC we can send Terminal_Profile immediately.
      if (this.appType == RIL.CARD_APPTYPE_SIM) {
        this.simIOcontext.SimRecordHelper.readSimPhase();
      } else if (libcutils.property_get("ro.moz.ril.send_stk_profile_dl", "false") === "true") {
        //Cameron mark first.
        //this.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
      }
      this.simIOcontext.ICCRecordHelper.fetchICCRecords();
    }

    this.cardState = newCardState;
    gIccService.notifyCardStateChanged(this.clientId,
                                           this.cardState);
    gRadioEnabledController.receiveCardState(this.clientId);
    /*this.sendChromeMessage({rilMessageType: "cardstatechange",
                            cardState: this.cardState});*/
  },

  handleDeviceIdentity: function (response) {
    if (DEBUG) this.debug("handleDeviceIdentity imei=" + response.imei + " , imeisv=" + response.imeisv + " ,esn =" + response.esn + " , meid=" + response.meid);
    if (response.errorMsg) {
      if (DEBUG) this.debug("Failed to get device identities:" + response.errorMsg);

      //Cameron mark first.
      // Fallback to getIMEI/getIMEISV in case some RILD did not handle this API
      // properly.
      /*if (!this._isCdma) {
        this.getIMEI();
        this.getIMEISV();
      }*/
      return;
    }

    let newIMEI, newIMEISV, newESN, newMEID;
    newIMEI = response.imei;
    newIMEISV = response.imeisv;
    newESN = response.esn;
    newMEID = response.meid;

    if (!this.deviceIdentities ||
        (newIMEI != this.deviceIdentities.imei) ||
        (newIMEISV != this.deviceIdentities.imeisv) ||
        (newESN != this.deviceIdentities.esn) ||
        (newMEID != this.deviceIdentities.meid)) {

      this.deviceIdentities = {
        imei: newIMEI || null,
        imeisv: newIMEISV || null,
        esn: newESN || null,
        meid: newMEID || null,
      };

      gMobileConnectionService.notifyDeviceIdentities(this.clientId,
                                                      this.deviceIdentities);

      gMobileConnectionService.notifyDeviceIdentitiesChanged(this.clientId,
                                                             this.deviceIdentities.imei,
                                                             this.deviceIdentities.imeisv,
                                                             this.deviceIdentities.esn,
                                                             this.deviceIdentities.meid);
      return this.deviceIdentities;
    }
  },

  handleVoiceRadioTechnology: function(response) {
    if (DEBUG) this.debug("Received message from worker handleVoiceRadioTechnology:" + JSON.stringify(response.radioTech));

    let isCdma = !this._isGsmTechGroup(response.radioTech);
    this.radioTech = response.radioTech;

    if (DEBUG) {
      this.debug("Radio tech is set to: " + RIL.GECKO_RADIO_TECH[response.radioTech] +
                         ", it is a " + (isCdma?"cdma":"gsm") + " technology");
    }

    // We should request SIM information when
    //  1. Radio state has been changed, so we are waiting for radioTech or
    //  2. isCdma is different from this._isCdma.
    if (this._waitingRadioTech || isCdma != this._isCdma) {
      this._isCdma = isCdma;
      this._waitingRadioTech = false;

      this.sendRilRequest("getDeviceIdentity", null);
      //this.getDeviceIdentity();
      this.sendRilRequest("getICCStatus", null);
      //this.getICCStatus();
    }
  },

    /**
   * Check if GSM radio access technology group.
   */
  _isGsmTechGroup: function(radioTech) {
    if (!radioTech) {
      return true;
    }

    switch(radioTech) {
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_GPRS:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_EDGE:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_UMTS:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_HSDPA:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_HSUPA:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_HSPA:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_LTE:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_HSPAP:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_GSM:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_TD_SCDMA:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_IWLAN:
      case Ci.nsIRadioTechnologyState.RADIO_CREG_TECH_LTE_CA:
        return true;
    }

    return false;
  },

  handleDataRegistration: function(response) {
    if (response.errorMsg) {
      this._attachDataRegistration.result = false;
    } else {
      this._attachDataRegistration.result = true;
    }
  },

  handleBasebandVersion: function(response) {
    if (response.errorMsg) {
      return;
    }
    this.basebandVersion = response.basebandVersion;
  },

  /**
   * Set CallForward Options to EFCfis
   * TODO: Only support SERVICE_CLASS_VOICE currently
   */
  _setCallForwardToSim: function(options) {
    if (DEBUG) this.debug("setCallForward to sim, options:" + JSON.stringify(options));

    if (options.serviceClass & ICC_SERVICE_CLASS_VOICE == 0) {
      if (DEBUG) this.debug("setCallForwardToSim only support ICC_SERVICE_CLASS_VOICE");
      return;
    }

    // Only unconditional CF needs to set flag.
    if (options.reason != RIL.CALL_FORWARD_REASON_UNCONDITIONAL) {
      return;
    }

    let iccInfo = this.iccInfoPrivate;

    if (iccInfo.cfis) {
      // Cameron mark first.
      //this.context.SimRecordHelper.updateCFIS(options);
    }
    if (iccInfo.cff) {
      // Cameron mark first.
      //this.context.SimRecordHelper.updateCphsCFF(options.action);
    }
  },

  handleCurrentCalls: function(response) {
    let rilCallResponse = response.getCurrentCalls();
    if (DEBUG) this.debug("handleCurrentCalls " + JSON.stringify(rilCallResponse));
    let calls = {};
    for (let i = 0; i < rilCallResponse.length; i++) {
      let call = {};

      call.state          = rilCallResponse[i].state; // CALL_STATE_*
      call.callIndex      = rilCallResponse[i].index; // GSM index (1-based)
      call.toa            = rilCallResponse[i].toa;
      call.isMpty         = rilCallResponse[i].isMpty;
      call.isMT           = rilCallResponse[i].isMT;
      call.als            = rilCallResponse[i].als;
      call.isVoice        = rilCallResponse[i].isVoice;
      call.isVoicePrivacy = rilCallResponse[i].isVoicePrivacy;
      call.number             = rilCallResponse[i].number;
      call.numberPresentation = rilCallResponse[i].numberPresentation; // CALL_PRESENTATION_*
      call.name               = rilCallResponse[i].name;
      call.namePresentation   = rilCallResponse[i].namePresentation;

      let rilUusInfos = rilCallResponse[i].getUusInfo();
      let uusInfos = {};
      for (let j = 0; j < rilUusInfos.length; j++) {
        let uusInfo = {};

        uusInfo.type = rilUusInfos[j].uusType;
        uusInfo.dcs = rilUusInfos[j].uusDcs;
        uusInfo.userData = rilUusInfos[j].uusData;
        uusInfos[j] = uusInfo;
      }

      call.uusinfo = uusInfos;

      //Add fake rttMode for upper layer
      call.rttMode = RIL.RTT_MODE_OFF;

      if (call.isVoice) {
        calls[call.callIndex] = call;
      }
    }

    gTelephonyService.notifyCurrentCalls(this.clientId, calls);
    return calls;
  },

  handleQueryCallForwardStatus: function(response) {
    let rilCallForwardStatus = response.getCallForwardStatus();
    if (DEBUG) this.debug("handleQueryCallForwardStatus " + JSON.stringify(rilCallForwardStatus));

    let result = {};

    let rulesLength = rilCallForwardStatus.length;
    if (!rulesLength) {
      result.errorMsg = RIL.GECKO_ERROR_GENERIC_FAILURE;
      return result;
    }

    let rules = new Array(rulesLength);
    for (let i = 0; i < rulesLength; i++) {
      let rule = {};
      rule.active       = rilCallForwardStatus[i].status == 1; // CALL_FORWARD_STATUS_*
      rule.reason       = rilCallForwardStatus[i].reason; // CALL_FORWARD_REASON_*
      rule.serviceClass = rilCallForwardStatus[i].serviceClass;
      rule.toa          = rilCallForwardStatus[i].toa;
      rule.number       = rilCallForwardStatus[i].number;
      rule.timeSeconds  = rilCallForwardStatus[i].timeSeconds;
      rules[i] = rule;
    }
    result.rules = rules;
    return result;
  },

  handleSmscAddress: function(response) {
    let result = {};
    let tosca = RIL.TOA_UNKNOWN;
    let smsc = response.smsc;
    // Convert the NPI value to the corresponding index of CALLED_PARTY_BCD_NPI
    // array. If the value does not present in the array, use
    // CALLED_PARTY_BCD_NPI_ISDN.
    let npi = RIL.CALLED_PARTY_BCD_NPI.indexOf(tosca & 0xf);
    if (npi === -1) {
      npi = RIL.CALLED_PARTY_BCD_NPI.indexOf(RIL.CALLED_PARTY_BCD_NPI_ISDN);
    }

    // Extract TON.
    let ton = (tosca & 0x70) >> 4;

    // Ensure + sign if TON is international, and vice versa.
    const TON_INTERNATIONAL = (RIL.TOA_INTERNATIONAL & 0x70) >> 4;
    if (ton ===  TON_INTERNATIONAL && smsc.charAt(0) !== "+") {
      smsc = "+" + smsc;
    } else if (smsc.charAt(0) === "+" && ton !== TON_INTERNATIONAL) {
      if (DEBUG) {
        this.debug("SMSC address number begins with '+' while the TON is not international. Change TON to international.");
      }
      ton = TON_INTERNATIONAL;
    }

    result.smscAddress = smsc;
    result.typeOfNumber = ton;
    result.numberPlanIdentification = npi;
    return result;
  },

  handleAvailableNetworks: function (response) {
    let rilAvailableNetworks = response.getAvailableNetworks();
    let networks = {};
    for (let i = 0; i < rilAvailableNetworks.length; i++) {
      let network = {
        longName: rilAvailableNetworks[i].alphaLong,
        shortName: rilAvailableNetworks[i].alphaShort,
        mcc: null,
        mnc: null,
        state: RIL.RIL_QAN_STATE_TO_GECKO_STATE[rilAvailableNetworks[i].status]
      };

      let networkTuple = ilAvailableNetworks[i].operatorNumeric;
      try {
        this._processNetworkTuple(networkTuple, network);
      } catch (e) {
        if (DEBUG) this.debug("Error processing operator tuple: " + e);
      }

      networks.push(network);
    }
    return networks;
  },

  handleCellInfoList: function (response) {
    let rilCellInfoLists = response.getCellInfoList();
    let cellInfoLists = [];
    for (let i = 0; i < rilCellInfoLists.length ; i++) {
      let cellInfo = {};
      switch(rilCellInfoLists[i].cellInfoType) {
        case RIL.CELL_INFO_TYPE_GSM:
          //nsICellIdentityGsm
          cellInfo.mcc = rilCellInfoLists[i].gsm.cellIdentityGsm.mcc;
          cellInfo.mnc = rilCellInfoLists[i].gsm.cellIdentityGsm.mnc;
          cellInfo.lac = rilCellInfoLists[i].gsm.cellIdentityGsm.lac;
          cellInfo.cid = rilCellInfoLists[i].gsm.cellIdentityGsm.cid;
          cellInfo.arfcn = rilCellInfoLists[i].gsm.cellIdentityGsm.arfcn;
          cellInfo.bsic = rilCellInfoLists[i].gsm.cellIdentityGsm.bsic;
          //nsIGsmSignalStrength
          cellInfo.signalStrength = rilCellInfoLists[i].gsm.signalStrengthGsm.signalStrength;
          cellInfo.bitErrorRate = rilCellInfoLists[i].gsm.signalStrengthGsm.bitErrorRate;
          cellInfo.timingAdvance = rilCellInfoLists[i].gsm.signalStrengthGsm.timingAdvance;
          break;

        case RIL.CELL_INFO_TYPE_WCDMA:
          //nsICellIdentityWcdma
          cellInfo.mcc = rilCellInfoLists[i].wcdma.cellIdentityWcdma.mcc;
          cellInfo.mnc = rilCellInfoLists[i].wcdma.cellIdentityWcdma.mnc;
          cellInfo.lac = rilCellInfoLists[i].wcdma.cellIdentityWcdma.lac;
          cellInfo.cid = rilCellInfoLists[i].wcdma.cellIdentityWcdma.cid;
          cellInfo.psc = rilCellInfoLists[i].wcdma.cellIdentityWcdma.psc;
          cellInfo.uarfcn = rilCellInfoLists[i].wcdma.cellIdentityWcdma.uarfcn;
          //nsIWcdmaSignalStrength
          cellInfo.signalStrength = rilCellInfoLists[i].wcdma.signalStrengthWcdma.signalStrength;
          cellInfo.bitErrorRate = rilCellInfoLists[i].wcdma.signalStrengthWcdma.bitErrorRate;
          break;

        case RIL.CELL_INFO_TYPE_LTE:
          //nsICellIdentityLte
          cellInfo.mcc = rilCellInfoLists[i].lte.cellIdentityLte.mcc;
          cellInfo.mnc = rilCellInfoLists[i].lte.cellIdentityLte.mnc;
          // Note: naming different between ril and mobileconnectionservice.
          cellInfo.cid = rilCellInfoLists[i].lte.cellIdentityLte.ci;
          cellInfo.pci = rilCellInfoLists[i].lte.cellIdentityLte.pci;
          cellInfo.tac = rilCellInfoLists[i].lte.cellIdentityLte.tac;
          cellInfo.earfcn = rilCellInfoLists[i].lte.cellIdentityLte.earfcn;
          //nsILteSignalStrength
          cellInfo.signalStrength = rilCellInfoLists[i].lte.signalStrengthLte.signalStrength;
          cellInfo.rsrp = rilCellInfoLists[i].lte.signalStrengthLte.rsrp;
          cellInfo.rsrq = rilCellInfoLists[i].lte.signalStrengthLte.rsrq;
          cellInfo.rssnr = rilCellInfoLists[i].lte.signalStrengthLte.rssnr;
          cellInfo.cqi = rilCellInfoLists[i].lte.signalStrengthLte.cqi;
          cellInfo.timingAdvance = rilCellInfoLists[i].lte.signalStrengthLte.timingAdvance;
          break;

        case RIL.CELL_INFO_TYPE_TDSCDMA:
          //nsICellIdentityTdScdma
          cellInfo.mcc = rilCellInfoLists[i].tdscdma.nsICellIdentityTdScdma.mcc;
          cellInfo.mnc = rilCellInfoLists[i].tdscdma.nsICellIdentityTdScdma.mnc;
          cellInfo.cid = rilCellInfoLists[i].tdscdma.nsICellIdentityTdScdma.ci;
          cellInfo.pci = rilCellInfoLists[i].tdscdma.nsICellIdentityTdScdma.pci;
          cellInfo.cpid = rilCellInfoLists[i].tdscdma.nsICellIdentityTdScdma.cpid;
          //nsITdScdmaSignalStrength
          cellInfo.rscp = rilCellInfoLists[i].tdscdma.signalStrengthTdScdma.rscp;
          break;

        case RIL.CELL_INFO_TYPE_CDMA:
          //nsICellIdentityCdma
          cellInfo.networkId = rilCellInfoLists[i].cdma.cellIdentityCdma.networkId;
          cellInfo.systemId = rilCellInfoLists[i].cdma.cellIdentityCdma.systemId;
          cellInfo.baseStationId = rilCellInfoLists[i].cdma.cellIdentityCdma.baseStationId;
          cellInfo.longitude = rilCellInfoLists[i].cdma.cellIdentityCdma.longitude;
          cellInfo.latitude = rilCellInfoLists[i].cdma.cellIdentityCdma.latitude;
          //nsICdmaSignalStrength
          // Note: naming different between ril and mobileconnectionservice.
          cellInfo.cdmaDbm = rilCellInfoLists[i].cdma.signalStrengthCdma.dbm;
          cellInfo.cdmaEcio = rilCellInfoLists[i].cdma.signalStrengthCdma.ecio;
          //nsIEvdoSignalStrength
          cellInfo.evdoDbm = rilCellInfoLists[i].cdma.signalStrengthEvdo.dbm;
          cellInfo.evdoEcio = rilCellInfoLists[i].cdma.signalStrengthEvdo.ecio;
          cellInfo.evdoSnr = rilCellInfoLists[i].cdma.signalStrengthEvdo.signalNoiseRatio;
          break;
      }
      cellInfoLists.push(cellInfo);
    }
    return cellInfoLists;
  },

  handleIccIoResult: function (response) {

    // Get the requset options.
    let token = response.rilMessageToken;
    let options = this.tokenOptionsMap[token];
    if (!options) {
      if (DEBUG) this.debug(clientId, "Ignore orphan token: " + token);
      return;
    }

    // Handle the icc io response
    let iccIoResult = response.iccIo;

    options.sw1 = iccIoResult.sw1;
    options.sw2 = iccIoResult.sw2;
    options.simResponse = iccIoResult.simResponse;
    // See 3GPP TS 11.11, clause 9.4.1 for operation success results.
    if (options.sw1 !== RIL.ICC_STATUS_NORMAL_ENDING &&
        options.sw1 !== RIL.ICC_STATUS_NORMAL_ENDING_WITH_EXTRA &&
        options.sw1 !== RIL.ICC_STATUS_WITH_SIM_DATA &&
        options.sw1 !== RIL.ICC_STATUS_WITH_RESPONSE_DATA) {
      if (DEBUG) {
        this.debug("ICC I/O Error EF id = 0x" + options.fileId.toString(16) +
                           ", command = 0x" + options.command.toString(16) +
                           ", sw1 = 0x" + options.sw1.toString(16) +
                           ", sw2 = 0x" + options.sw2.toString(16));
      }
      if (options.onerror) {
        // We can get fail cause from sw1/sw2 (See TS 11.11 clause 9.4.1 and
        // ISO 7816-4 clause 6). But currently no one needs this information,
        // so simply reports "GenericFailure" for now.
        options.onerror(RIL.GECKO_ERROR_GENERIC_FAILURE);
      }
    }
    this.simIOcontext.ICCIOHelper.processICCIO(options);

    // All finish delete option.
    delete this.tokenOptionsMap[token];
  },

  handleIccIoErrorResult: function (response) {
    // Get the requset options.
    let token = response.rilMessageToken;
    let options = this.tokenOptionsMap[token];
    if (!options) {
      if (DEBUG) this.debug(clientId, "Ignore orphan token: " + token);
      return;
    }

    if (options.onerror) {
      options.onerror(response.errorMsg);
    }

    // All finish delete option.
    delete this.tokenOptionsMap[token];
  },

  /**
   * Send arbitrary message to worker.
   *
   * @param rilMessageType
   *        A text message type.
   * @param message [optional]
   *        An optional message object to send.
   * @param callback [optional]
   *        An optional callback function which is called when worker replies
   *        with an message containing a "rilMessageToken" attribute of the
   *        same value we passed.  This callback function accepts only one
   *        parameter -- the reply from worker.  It also returns a boolean
   *        value true to keep current token-callback mapping and wait for
   *        another worker reply, or false to remove the mapping.
   */
  sendRilRequest: function(rilMessageType, message, callback) {
    message = message || {};

    message.rilMessageToken = this.token;
    // TODO what if meet the maximum value?
    this.token++;

    if (callback) {
      // Only create the map if callback is provided.  For sending a request
      // and intentionally leaving the callback undefined, that reply will
      // be dropped in |this.onmessage| because of that orphan token.
      //
      // For sending a request that never replied at all, we're fine with this
      // because no callback shall be passed and we leave nothing to be cleaned
      // up later.
      this.tokenCallbackMap[message.rilMessageToken] = callback;
    }
    message.rilMessageType = rilMessageType;

    // Handle the solic request by function then call the ril_worker.
    switch(message.rilMessageType){
      case "getICCStatus":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_SIM_STATUS");
        this.rilworker.getIccCardStatus(message.rilMessageToken);
        break;
      case "iccUnlockCardLock":
        this.processIccUnlockCardLock(message);
        break;
      case "iccSetCardLockEnabled":
        this.processIccSetCardLockEnabled(message);
        break;
      case "iccGetCardLockEnabled":
        this.processIccGetCardLockEnabled(message);
        break;
      case "iccChangeCardLockPassword":
        this.processIccChangeCardLockPassword(message);
        break;
      case "iccGetCardLockRetryCount":
        this.processIccGetCardLockRetryCount(message);
        break;
      case "enterDepersonalization":
        break;
      case "getCurrentCalls":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_CURRENT_CALLS");
        this.rilworker.getCurrentCalls(message.rilMessageToken);
        break;
      case "dial":
        this.processRequestDial(message);
        break;
      case "getIMSI":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_IMSI aid = " + message.aid);
        this.rilworker.getIMSI(message.rilMessageToken, message.aid || this.aid);
        break;
      case "hangUpCall":
        this.processHangUpCall(message);
        break;
      case "hangUpBackground":
        this.processHangUpBackgroundCall(message);
        break;
      case "hangUpForeground":
        this.processHangUpForegroundCall(message);
        break;
      case "switchActiveCall":
        this.processSwitchActiveCall(message);
        break;
      case "conferenceCall":
        this.processConferenceCall(message);
        break;
      case "udub":
        this.processRejectCall(message);
        break;
      case "explicitCallTransfer":
        this.processExplicitCallTransfer(message);
        break;
      case "getFailCause":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_LAST_CALL_FAIL_CAUSE");
        this.rilworker.getLastCallFailCause(message.rilMessageToken);
        break;
      case "getSignalStrength":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SIGNAL_STRENGTH");
        this.rilworker.getSignalStrength(message.rilMessageToken);
        break;
      case "getVoiceRegistrationState":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_VOICE_REGISTRATION_STATE");
        this.rilworker.getVoiceRegistrationState(message.rilMessageToken);
        break;
      case "getDataRegistrationState":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DATA_REGISTRATION_STATE");
        this.rilworker.getDataRegistrationState(message.rilMessageToken);
        break;
      case "getOperator":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_OPERATOR");
        this.rilworker.getOperator(message.rilMessageToken);
        break;
      case "setRadioEnabled":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_RADIO_POWER on = " + message.enabled);
        this.rilworker.setRadioPower(message.rilMessageToken, message.enabled);
        break;
      case "sendSMS":
        break;
      case "setupDataCall":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SETUP_DATA_CALL radioTechnology = " + message.radioTechnology + ", isRoaming = "
                    + message.isRoaming + ", allowRoaming = " + message.allowRoaming);
        this.rilworker.setupDataCall(message.rilMessageToken, message.radioTechnology,  new DataProfile(message.profile), message.isRoaming, message.allowRoaming);
        break;
      case "iccIO":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SIM_IO command = " + message.command + " , fileId = "
                    + message.fileId + " , path = " + message.pathId + " , p1 = " + message.p1 + " , p2 = " + message.p2 + " , p3 = " + message.p3
                    + " , data = " + message.dataWriter + " , pin2 = " + message.pin2 + " , aid = " + message.aid);
        // Store the command options(message).
        this.tokenOptionsMap[message.rilMessageToken] = message;
        this.rilworker.iccIOForApp(message.rilMessageToken, message.command, message.fileId, message.pathId, message.p1, message.p2, message.p3, message.data, message.pin2, (message.aid || this.aid));
        break;
      case "sendUSSD":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SEND_USSD ussd = " + message.ussd);
        this.rilworker.sendUssd(message.rilMessageToken, message.ussd);
        break;
      case "cancelUSSD":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_CANCEL_USSD ussd = " + message.ussd);
        this.rilworker.cancelPendingUssd(message.rilMessageToken);
        break;
      case "setCallBarring":
        this.processSetCallBarring(message);
        break;
      case "queryCallBarringStatus":
        this.processQueryCallBarringStatus(message);
        break;
      case "getCLIR":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_CLIR");
        this.rilworker.getClir(message.rilMessageToken);
        break;
      case "setCLIR":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_CLIR clirMode = " + message.clirMode);
        this.rilworker.setClir(message.rilMessageToken, message.clirMode);
        break;
      case "queryCallForwardStatus":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_CALL_FORWARD_STATUS reason = " + message.reason + " , serviceClass = " + message.serviceClass + " , number = " + message.number || "");
        let toaNumber = this._toaFromString(message.number);
        this.rilworker.getCallForwardStatus(message.rilMessageToken, message.reason, message.serviceClass, message.number || "", toaNumber || "");
        break;
      case "setCallForward":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_CALL_FORWARD action = " + message.action + " , reason = " + message.reason + " , serviceClass = " + message.serviceClass + " , number = " + message.number || "");
        let number = this._toaFromString(message.number);
        this._callForwardOptions.action = message.action;
        this._callForwardOptions.reason = message.reason;
        this._callForwardOptions.serviceClass = message.serviceClass;
        this._callForwardOptions.number = message.number || "";
        this._callForwardOptions.toaNumner = number || "";
        this.rilworker.setCallForwardStatus(message.rilMessageToken, message.action, message.reason, message.serviceClass, message.number || "", number || "");
        break;
      case "queryCallWaiting":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_CALL_WAITING serviceClass = " + message.serviceClass);
        this.rilworker.getCallWaiting(message.rilMessageToken, message.serviceClass);
        break;
      case "setCallWaiting":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_CALL_WAITING enable = " + message.enable + " , serviceClass = " + message.serviceClass);
        this.rilworker.setCallWaiting(message.rilMessageToken, message.enable, message.serviceClass);
        break;
      case "acknowledgeGsmSms":
        break;
      case "answerCall":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ANSWER");
        this.processAnswerCall(message);
        break;
      case "deactivateDataCall":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DEACTIVATE_DATA_CALL");
        this.rilworker.deactivateDataCall(message.rilMessageToken, message.cid, message.reason);
        break;
      case "changeCallBarringPassword":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_CHANGE_BARRING_PASSWORD");
        this.rilworker.setBarringPassword(message.rilMessageToken, RIL.ICC_CB_FACILITY_BA_ALL, message.pin || "", message.newPin || "");
        break;
      case "getNetworkSelectionMode":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE");
        this.rilworker.getNetworkSelectionMode(message.rilMessageToken);
        break;
      case "selectNetworkAuto":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC");
        this.rilworker.setNetworkSelectionModeAutomatic(message.rilMessageToken);
        break;
      case "selectNetwork":
        let numeric = (message.mcc && message.mnc) ? message.mcc + message.mnc : null;
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL numeric = " + numeric);
        this.rilworker.setNetworkSelectionModeManual(message.rilMessageToken, numeric);
        break;
      case "getAvailableNetworks":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_AVAILABLE_NETWORKS numeric = " + numeric);
        this.rilworker.getAvailableNetworks(message.rilMessageToken);
        break;
      case "sendTone":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DTMF dtmfChar = " + message.dtmfChar);
        this.rilworker.sendDtmf(message.rilMessageToken,  message.dtmfChar+"");
        break;
      case "startTone":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DTMF_START dtmfChar = " + message.dtmfChar);
        this.rilworker.startDtmf(message.rilMessageToken, message.dtmfChar+"");
        break;
      case "stopTone":
        console.log("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DTMF_STOP");
        this.rilworker.stopDtmf(message.rilMessageToken);
        break;
      case "getBasebandVersion":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_BASEBAND_VERSION");
        this.rilworker.getBasebandVersion(message.rilMessageToken);
        break;
      case "separateCall":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SEPARATE_CONNECTION");
        this.rilworker.separateConnection(message.rilMessageToken, message.callIndex);
        break;
      case "setMute":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_MUTE enableMute = " + message.muted);
        this.rilworker.setMute(message.rilMessageToken, message.muted);
        break;
      case "getMute":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_MUTE");
        this.rilworker.getMute(message.rilMessageToken);
        break;
      case "queryCLIP":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_CLIP ");
        this.rilworker.getClip(message.rilMessageToken);
        break;
      case "getDataCallList":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DATA_CALL_LIST");
        this.rilworker.getDataCallList(message.rilMessageToken);
        break;
      case "writeSmsToSIM":
        break;
      case "setPreferredNetworkType":
        let networkType = message.type;
        if (networkType < 0 || networkType >= RIL.RIL_PREFERRED_NETWORK_TYPE_TO_GECKO.length) {
          message.errorMsg = GECKO_ERROR_INVALID_PARAMETER;
          this.handleRilResponse(message);
          return;
        }
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE networkType = " + networkType);
        this.rilworker.setPreferredNetworkType(message.rilMessageToken, networkType);
        break;
      case "getPreferredNetworkType":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE");
        this.rilworker.getPreferredNetworkType(message.rilMessageToken);
        break;
      case "getNeighboringCellIds":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_NEIGHBORING_CELL_IDS");
        this.rilworker.getNeighboringCids(message.rilMessageToken);
        break;
      case "setRoamingPreference":
        break;
      case "queryRoamingPreference":
        break;
      case "setTtyMode":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_TTY_MODE ttyMode = " + message.ttyMode);
        this.rilworker.setTTYMode(message.rilMessageToken, message.ttyMode);
        break;
      case "queryTtyMode":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_TTY_MODE");
        this.rilworker.queryTTYMode(message.rilMessageToken);
        break;
      case "setVoicePrivacyMode":
        break;
      case "queryVoicePrivacyMode":
        break;
      case "cdmaFlash":
        break;
      case "sendSMS":
        break;
      case "acknowledgeCdmaSms":
        break;
      case "setGsmBroadcastConfig":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GSM_SET_BROADCAST_CONFIG config = " + message.config);
        this.rilworker.setGsmBroadcastConfig(message.rilMessageToken, message.config);
        break;
      case "setGsmBroadcastActivation":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GSM_BROADCAST_ACTIVATION activate = " + message.activate);
        this.rilworker.setGsmBroadcastActivation(message.rilMessageToken, message.activate);
        break;
      case "setCdmaSmsBroadcastConfig":
        break;
      case "getCdmaSubscription":
        break;
      case "getDeviceIdentity":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DEVICE_IDENTITY");
        this.rilworker.getDeviceIdentity(message.rilMessageToken);
        break;
      case "sendExitEmergencyCbModeRequest":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE");
        this.rilworker.exitEmergencyCallbackMode(message.rilMessageToken);
        break;
      case "getSmscAddress":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_SMSC_ADDRESS");
        this.rilworker.getSmscAddress(message.rilMessageToken);
        break;
      case "setSmscAddress":
        break;
      case "reportSmsMemoryStatus":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_REPORT_SMS_MEMORY_STATUS");
        this.rilworker.reportSmsMemoryStatus(message.rilMessageToken, message.isAvailable);
        break;
      case "reportStkServiceIsRunning":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING");
        this.rilworker.reportStkServiceIsRunning(message.rilMessageToken);
        break;
      case "acknowledgeIncomingGsmSmsWithPDU":
        break;
      case "dataDownloadViaSMSPP":
        break;
      case "getVoiceRadioTechnology":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_VOICE_RADIO_TECH");
        this.rilworker.getVoiceRadioTechnology(message.rilMessageToken);
        break;
      case "getCellInfoList":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_GET_CELL_INFO_LIST");
        this.rilworker.getCellInfoList(message.rilMessageToken);
        break;
      case "setInitialAttachApn":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_INITIAL_ATTACH_APN profile = " + JSON.stringify(message.profile));
        this.rilworker.setInitialAttachApn(message.rilMessageToken, new DataProfile(message.profile), message.isRoaming);
        break;
      case "iccOpenChannel":
        break;
      case "iccCloseChannel":
        break;
      case "iccExchangeAPDU":
        break;
      case "setUiccSubscription":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_UICC_SUBSCRIPTION slotId = "
                    + message.slotId + " appIndex = " + message.appIndex
                    + " subId = " + message.subId + " subStatus = " + message.subStatus? 1: 0);
        this.rilworker.setUiccSubscription(message.rilMessageToken
                                           , message.serial
                                           , message.slotId
                                           , message.appIndex
                                           , message.subId
                                           , message.subStatus? 1: 0);
        break;
      case "setDataRegistration":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ALLOW_DATA allowed = " + message.allowed);
        this.rilworker.setDataAllowed(message.rilMessageToken, message.allowed);
        break;
      case "getIccAuthentication":
        break;
      case "setCellInfoListRate":
        if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_CELL_INFO_LIST_RATE rateInMillis = " + message.rateInMillis);
        if (message.rateInMillis) {
          this.rilworker.setCellInfoListRate(message.rilMessageToken, message.rateInMillis);
        } else {
          this.rilworker.setCellInfoListRate(message.rilMessageToken);
        }
        break;
      case "setCellBroadcastDisabled":
        // This is not a ril request.
        this.setCellBroadcastDisabled(message);
        break;
      case "setCellBroadcastSearchList":
        // This is not a ril request.
        this.setCellBroadcastSearchList(message);
        break;
      default:
    }
  },

    /**
   * Helper for returning the TOA for the given dial string.
   */
  _toaFromString: function(number) {
    let toa = RIL.TOA_UNKNOWN;
    if (number && number.length > 0 && number[0] == '+') {
      toa = RIL.TOA_INTERNATIONAL;
    }
    return toa;
  },

  processRequestDial: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_DIAL address = " + message.number + " , clirMode = " + message.clirMode + " , uusInfo = " + message.uusInfo);
    this.rilworker.requestDial(message.rilMessageToken, message.number || "", message.clirMode
      , (message.uusInfo ? message.uusInfo.uusType || 0 : 0)
      , (message.uusInfo ? message.uusInfo.uusDcs || 0 : 0)
      , (message.uusInfo ? message.uusInfo.uusData || "" : ""));
  },

  processHangUpCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_HANGUP callIndex = " + message.callIndex);
    this.rilworker.hangupConnection(message.rilMessageToken, message.callIndex);
  },

  processAnswerCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ANSWER");
    this.rilworker.acceptCall(message.rilMessageToken, message.callIndex);
  },

  processHangUpBackgroundCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND");
    this.rilworker.hangupWaitingOrBackground(message.rilMessageToken);
  },

  processHangUpForegroundCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND");
    this.rilworker.hangupForegroundResumeBackground(message.rilMessageToken);
  },

  processSwitchActiveCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE");
    this.rilworker.hangupForegroundResumeBackground(message.rilMessageToken);
  },

  processConferenceCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_CONFERENCE");
    this.rilworker.hangupForegroundResumeBackground(message.rilMessageToken);
  },

  processRejectCall: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_UDUB");
    this.rilworker.rejectCall(message.rilMessageToken);
  },

  processExplicitCallTransfer: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_EXPLICIT_CALL_TRANSFER");
    this.rilworker.explicitCallTransfer(message.rilMessageToken);
  },

    /**
   * Queries current call barring rules.
   *
   * @param program
   *        One of CALL_BARRING_PROGRAM_* constants.
   * @param serviceClass
   *        One of ICC_SERVICE_CLASS_* constants.
   */
  processQueryCallBarringStatus: function(message) {
    message.facility = RIL.CALL_BARRING_PROGRAM_TO_FACILITY[message.program];
    message.password = ""; // For query no need to provide it.

    // For some operators, querying specific serviceClass doesn't work. We use
    // serviceClass 0 instead, and then process the response to extract the
    // answer for queryServiceClass.
    message.queryServiceClass = message.serviceClass;
    message.serviceClass = 0;

    this.queryICCFacilityLock(message);
  },

  /**
   * Configures call barring rule.
   *
   * @param program
   *        One of CALL_BARRING_PROGRAM_* constants.
   * @param enabled
   *        Enable or disable the call barring.
   * @param password
   *        Barring password.
   * @param serviceClass
   *        One of ICC_SERVICE_CLASS_* constants.
   */
  processSetCallBarring: function(message) {
    message.facility = RIL.CALL_BARRING_PROGRAM_TO_FACILITY[message.program];
    this.setICCFacilityLock(message);
  },

  /**
   * Helper function for unlocking ICC locks.
   */
  processIccUnlockCardLock: function(message) {
    switch (message.lockType) {
      case RIL.GECKO_CARDLOCK_PIN:
        this.enterICCPIN(message);
        break;
      case RIL.GECKO_CARDLOCK_PIN2:
        this.enterICCPIN2(message);
        break;
      case RIL.GECKO_CARDLOCK_PUK:
        this.enterICCPUK(message);
        break;
      case RIL.GECKO_CARDLOCK_PUK2:
        this.enterICCPUK2(message);
        break;
      case RIL.GECKO_CARDLOCK_NCK:
      case RIL.GECKO_CARDLOCK_NSCK:
      case RIL.GECKO_CARDLOCK_NCK1:
      case RIL.GECKO_CARDLOCK_NCK2:
      case RIL.GECKO_CARDLOCK_HNCK:
      case RIL.GECKO_CARDLOCK_CCK:
      case RIL.GECKO_CARDLOCK_SPCK:
      case RIL.GECKO_CARDLOCK_PCK:
      case RIL.GECKO_CARDLOCK_RCCK:
      case RIL.GECKO_CARDLOCK_RSPCK:
      case RIL.GECKO_CARDLOCK_NCK_PUK:
      case RIL.GECKO_CARDLOCK_NSCK_PUK:
      case RIL.GECKO_CARDLOCK_NCK1_PUK:
      case RIL.GECKO_CARDLOCK_NCK2_PUK:
      case RIL.GECKO_CARDLOCK_HNCK_PUK:
      case RIL.GECKO_CARDLOCK_CCK_PUK:
      case RIL.GECKO_CARDLOCK_SPCK_PUK:
      case RIL.GECKO_CARDLOCK_PCK_PUK:
      case RIL.GECKO_CARDLOCK_RCCK_PUK: // Fall through.
      case RIL.GECKO_CARDLOCK_RSPCK_PUK:
        message.facility = RIL.GECKO_CARDLOCK_TO_FACILITY_CODE[message.lockType];
        message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE |
                               RIL.ICC_SERVICE_CLASS_DATA  |
                               RIL.ICC_SERVICE_CLASS_FAX;
        message.enabled = false;
        this.setICCFacilityLock(message);
        break;
      default:
        message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.handleRilResponse(message);
    }
  },

  enterICCPIN: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ENTER_SIM_PIN pin = " + message.password + " , aid = " + this.aid);
    this.rilworker.supplyIccPinForApp(message.rilMessageToken, message.password || "", this.aid);
  },

  enterICCPIN2: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ENTER_SIM_PIN2 pin = " + message.password + " , aid = " + this.aid);
    this.rilworker.supplyIccPin2ForApp(message.rilMessageToken, message.password || "", this.aid);
  },

  enterICCPUK: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ENTER_SIM_PUK puk = " + message.password + " , newPin = " + message.newPin + " , aid = " + this.aid);
    this.rilworker.supplyIccPukForApp(message.rilMessageToken, message.password || "", message.newPin || "", this.aid);
  },

  enterICCPUK2: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_ENTER_SIM_PUK2 pin = " + message.password + " , newPin = " + message.newPin || "" + " , aid = " + this.aid);
    this.rilworker.supplyIccPuk2ForApp(message.rilMessageToken, message.password || "", message.newPin || "", this.aid);
  },

  setICCFacilityLock: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_SET_FACILITY_LOCK facility = " + message.facility + " , enabled = " + message.enabled + " , password = " + message.password + " , serviceClass = " + message.serviceClass + " , aid = " + this.aid);
    this.rilworker.setFacilityLockForApp(message.rilMessageToken, message.facility || "", message.enabled, message.password || "", message.serviceClass, this.aid);
  },

  /**
   * Helper function for setting the state of ICC locks.
   */
  processIccSetCardLockEnabled: function(message) {
    switch (message.lockType) {
      case RIL.GECKO_CARDLOCK_PIN: // Fall through.
      case RIL.GECKO_CARDLOCK_FDN:
        message.facility = RIL.GECKO_CARDLOCK_TO_FACILITY[message.lockType];
        break;
      default:
        message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.handleRilResponse(message);
        return;
    }

    message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE |
                           RIL.ICC_SERVICE_CLASS_DATA  |
                           RIL.ICC_SERVICE_CLASS_FAX;
    this.setICCFacilityLock(message);
  },

  /**
   * Helper function for fetching the state of ICC locks.
   */
  processIccGetCardLockEnabled: function(message) {
    switch (message.lockType) {
      case RIL.GECKO_CARDLOCK_PIN: // Fall through.
      case RIL.GECKO_CARDLOCK_FDN:
        message.facility = RIL.GECKO_CARDLOCK_TO_FACILITY[message.lockType];
        break;
      default:
        message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.handleRilResponse(message);
        return;
    }

    message.password = ""; // For query no need to provide pin.
    message.serviceClass = RIL.ICC_SERVICE_CLASS_VOICE |
                           RIL.ICC_SERVICE_CLASS_DATA  |
                           RIL.ICC_SERVICE_CLASS_FAX;
    this.queryICCFacilityLock(message);
  },

  queryICCFacilityLock: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_QUERY_FACILITY_LOCK facility = " + message.facility
      + " , password = " + message.password + " , serviceClass = " + message.serviceClass + " , aid = " + this.aid);
    this.rilworker.getFacilityLockForApp(message.rilMessageToken, message.facility || "", message.password || "", message.serviceClass, this.aid);
  },

  /**
   * Helper function for changing ICC locks.
   */
  processIccChangeCardLockPassword: function(message) {
    switch (message.lockType) {
      case RIL.GECKO_CARDLOCK_PIN:
        this.changeICCPIN(message);
        break;
      case RIL.GECKO_CARDLOCK_PIN2:
        this.changeICCPIN2(message);
        break;
      default:
        message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.handleRilResponse(message);
    }
  },

  changeICCPIN: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_CHANGE_SIM_PIN old = " + message.password + " , new = " + message.newPassword + " , aid = " + this.aid);
    this.rilworker.changeIccPinForApp(message.rilMessageToken, message.password || "", message.newPassword || "", this.aid);
  },

  changeICCPIN2: function(message) {
    if (DEBUG) this.debug("RILJ: ["+ message.rilMessageToken +"] > RIL_REQUEST_CHANGE_SIM_PIN2 old = " + message.password + " , new = " + message.newPassword + " , aid = " + this.aid);
    this.rilworker.changeIccPin2ForApp(message.rilMessageToken, message.password || "", message.newPassword || "", this.aid);
  },

  /**
   * Helper function for fetching the number of unlock retries of ICC locks.
   *
   * We only query the retry count when we're on the emulator. The phones do
   * not support the request id and their rild doesn't return an error.
   */
  processIccGetCardLockRetryCount: function(message) {
    this.debug("Current ril do not support processIccGetCardLockRetryCount api.");
    message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
    this.handleRilResponse(message);
    return;
    /*if (!RIL.RILQUIRKS_HAVE_QUERY_ICC_LOCK_RETRY_COUNT) {
      // Only the emulator supports this request.
      message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
      this.handleRilResponse(message);
      return;
    }

    switch (message.lockType) {
      case RIL.GECKO_CARDLOCK_PIN:
      case RIL.GECKO_CARDLOCK_PIN2:
      case RIL.GECKO_CARDLOCK_PUK:
      case RIL.GECKO_CARDLOCK_PUK2:
      case RIL.GECKO_CARDLOCK_NCK:
      case RIL.GECKO_CARDLOCK_NSCK:
      case RIL.GECKO_CARDLOCK_CCK: // Fall through.
      case RIL.GECKO_CARDLOCK_SPCK:
      // TODO: Bug 1116072: identify the mapping between RIL_PERSOSUBSTATE_SIM_SIM
      //       @ ril.h and TS 27.007, clause 8.65 for GECKO_CARDLOCK_SPCK.
        message.selCode = RIL.GECKO_CARDLOCK_TO_SEL_CODE[message.lockType];
        break;
      default:
        message.errorMsg = RIL.GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.handleRilResponse(message);
        return;
    }*/
    // New ril do not support this REQUEST_GET_UNLOCK_RETRY_COUNT command.
    //this.queryICCLockRetryCount(message);
  },

  sendWorkerMessage: function(rilMessageType, message, callback) {
    if (DEBUG) this.debug("sendWorkerMessage: rilMessageType: " + rilMessageType + ": " + JSON.stringify(message));
    // Special handler for setRadioEnabled.
    if (rilMessageType === "setRadioEnabled") {
      // Forward it to gRadioEnabledController.
      gRadioEnabledController.setRadioEnabled(this.clientId, message,
                                              callback.handleResponse);
      return;
    }

    if (callback) {
      this.sendRilRequest(rilMessageType, message, function(response) {
        return callback.handleResponse(response);
      });
    } else {
      this.sendRilRequest(rilMessageType, message);
    }
  },

  _getSystemUpTime: function() {
    let upTime;
    try {
      upTime = gTimeService.elapsedRealtime;
    } catch (e) {
      this.debug("_getSystemUpTime error: " + e);
      upTime = INVALID_UPTIME;
    }

    return upTime;
  },
};

var EXPORTED_SYMBOLS = ["RadioInterfaceLayer"];
