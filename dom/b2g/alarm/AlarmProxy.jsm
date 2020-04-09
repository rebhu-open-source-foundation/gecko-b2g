/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const DEBUG = false;
const REQUEST_CPU_LOCK_TIMEOUT = 10 * 1000; // 10 seconds.

function debug(aStr) {
  if (DEBUG) {
    dump("AlarmProxy: " + aStr + "\n");
  }
}

const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

XPCOMUtils.defineLazyServiceGetter(
  this,
  "gPowerManagerService",
  "@mozilla.org/power/powermanagerservice;1",
  "nsIPowerManagerService"
);

/*
 * AlarmProxy is a helper of passing requests from AlarmManager.cpp to
 * AlarmService.jsm, receiving results from AlarmService.jsm and send it
 * back to AlarmManager.cpp.
 */

function AlarmProxy() {}

AlarmProxy.prototype = {
  init: function init(aOwner) {
    debug("init");

    // A <requestId, {messageName, owner, callback}> map.
    this._alarmRequests = new Map();
    // A <requestId, {cpuLock, timer}> map.
    this._cpuLockDict = new Map();

    this._window = aOwner;

    if (this._window) {
      // We don't use this.innerWindowID, but other classes rely on it.
      let util = this._window.windowUtils;
      this.innerWindowID = util.currentInnerWindowID;

      // TODO:
      // We need innerWindowID to distiguish which window is destroyed,
      // Worker cannot pass its own owner here.
      Services.obs.addObserver(
        this,
        "inner-window-destroyed",
        /* weak-ref */ true
      );
    }

    // TODO: Obtain _manifestURL and _pageURL after app service is ready.
    if (this._window) {
      // document.nodePrincipal.origin somehow is "[System Principal]" on homescreen currently.
      // Use URL instead
      this._pageURL = this._window.document.URL;
      this._manifestURL = this._pageURL;
    } else {
      // TODO: identify worker.
      this._pageURL = "worker-pageURL";
      this._manifestURL = "worker-manifestURL";
    }

    this._destroyed = false;

    this._listenedMessages = [
      "Alarm:Add:Return:OK",
      "Alarm:Add:Return:KO",
      "Alarm:GetAll:Return:OK",
      "Alarm:GetAll:Return:KO",
    ];

    this._listenedMessages.forEach(aName => {
      Services.cpmm.addMessageListener(aName, this);
    });
  },

  add: function add(aOwner, aOptions, aCallback) {
    debug("add " + JSON.stringify(aOptions));
    let date = aOptions.date;
    let ignoreTimezone = !!aOptions.ignoreTimezone;
    let data = aOptions.data;

    // TODO: We used to use this._manifestURL to validate app, but there's
    // no way to retrieve app's manifestURL currently.

    if (!date) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let requestId = Cc["@mozilla.org/uuid-generator;1"]
      .getService(Ci.nsIUUIDGenerator)
      .generateUUID()
      .toString();

    if (data && aOwner) {
      // Run eval() in the sand box with the principal of the calling
      // web page to ensure no cross-origin object is involved. A "Permission
      // Denied" error will be thrown in case of privilege violation.
      // We used JSON.stringify in the past, but the toJSON privileged function is denied now.
      // TODO: We used Cu.getWebIDLCallerPrincipal() to get principal, but there's
      // no way to retrieve app's principal currently.
      // let sandbox = new Cu.Sandbox(Cu.getWebIDLCallerPrincipal());
      // TODO:
      // document.nodePrincipal.origin somehow is "[System Principal]" on homescreen currently.
      // Use URL instead
      let sandbox = new Cu.Sandbox(aOwner.document.URL);
      sandbox.data = data;
      data = Cu.evalInSandbox("eval(data)", sandbox);
    }

    this._alarmRequests.set(requestId, {
      messageName: "Alarm:Add",
      owner: aOwner ? aOwner : {},
      callback: aCallback,
    });
    this._lockCpuForRequest(requestId);
    Services.cpmm.sendAsyncMessage("Alarm:Add", {
      requestId,
      date,
      ignoreTimezone,
      data,
      pageURL: this._pageURL,
      manifestURL: this._manifestURL,
    });
  },

  remove: function remove(aId) {
    debug("remove " + aId);

    Services.cpmm.sendAsyncMessage("Alarm:Remove", {
      id: aId,
      manifestURL: this._manifestURL,
    });
  },

  getAll: function getAll(aOwner, aCallback) {
    debug("getAll");

    let requestId = Cc["@mozilla.org/uuid-generator;1"]
      .getService(Ci.nsIUUIDGenerator)
      .generateUUID()
      .toString();

    this._alarmRequests.set(requestId, {
      messageName: "Alarm:GetAll",
      owner: aOwner ? aOwner : {},
      callback: aCallback,
    });

    Services.cpmm.sendAsyncMessage("Alarm:GetAll", {
      requestId,
      manifestURL: this._manifestURL,
    });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage: " + aMessage.name);
    let json = aMessage.json ? aMessage.json : {};
    let alarmRequest = this._alarmRequests.get(json.requestId);

    if (!alarmRequest) {
      debug(
        "Request with given requestId not found. " +
          aMessage.name +
          " " +
          json.requestId
      );
      return;
    }

    if (!aMessage.name.startsWith(alarmRequest.messageName)) {
      debug(
        "Message name mismatch. " +
          aMessage.name +
          " " +
          alarmRequest.msgName +
          " " +
          json.requestId
      );
      return;
    }

    switch (aMessage.name) {
      case "Alarm:Add:Return:OK":
        this._unlockCpuForRequest(json.requestId);
        alarmRequest.callback.onAdd(
          Cr.NS_OK,
          Cu.cloneInto(json.id, alarmRequest.owner)
        );
        break;

      case "Alarm:GetAll:Return:OK":
        // We don't need to expose everything to the web content.
        let alarms = [];
        json.alarms.forEach(function trimAlarmInfo(aAlarm) {
          let alarm = {
            id: aAlarm.id,
            date: aAlarm.date,
            ignoreTimezone: aAlarm.ignoreTimezone,
            data: aAlarm.data,
          };
          alarms.push(alarm);
        });

        alarmRequest.callback.onGetAll(
          Cr.NS_OK,
          Cu.cloneInto(alarms, alarmRequest.owner)
        );
        break;

      case "Alarm:Add:Return:KO":
        this._unlockCpuForRequest(json.requestId);
        alarmRequest.callback.onAdd(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, alarmRequest.owner)
        );
        break;

      case "Alarm:GetAll:Return:KO":
        alarmRequest.callback.onGetAll(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, alarmRequest.owner)
        );
        break;

      default:
        debug("Wrong message: " + aMessage.name);
        break;
    }
  },

  observe(aSubject, aTopic, aData) {
    debug("observe " + aTopic);
    if (aTopic !== "inner-window-destroyed") {
      return;
    }

    let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
    if (wId != this.innerWindowID) {
      return;
    }

    if (!this.this._destroyed) {
      return;
    }

    this._destroyed = true;

    this._listenedMessages.forEach(aName => {
      Services.cpmm.removeMessageListener(aName, this);
    });

    Services.obs.removeObserver(this, "inner-window-destroyed");
  },

  _lockCpuForRequest(aRequestId) {
    if (this._cpuLockDict.has(aRequestId)) {
      debug(
        "Cpu wakelock for request " +
          aRequestId +
          " has been acquired. " +
          "You may call this function repeatedly or requestId is collision."
      );
      return;
    }

    // Acquire a lock for given request and save for lookup lately.
    debug("Acquire cpu lock for request " + aRequestId);
    let cpuLockInfo = {
      cpuLock: gPowerManagerService.newWakeLock("cpu"),
      timer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
    };
    this._cpuLockDict.set(aRequestId, cpuLockInfo);

    // Start a timer to prevent from non-responding request.
    cpuLockInfo.timer.initWithCallback(
      () => {
        debug("Request timeout! Release the cpu lock");
        this._unlockCpuForRequest(aRequestId);
      },
      REQUEST_CPU_LOCK_TIMEOUT,
      Ci.nsITimer.TYPE_ONE_SHOT
    );
  },

  _unlockCpuForRequest(aRequestId) {
    let cpuLockInfo = this._cpuLockDict.get(aRequestId);
    if (!cpuLockInfo) {
      debug(
        "The cpu lock for requestId " +
          aRequestId +
          " is either invalid or has been released."
      );
      return;
    }

    // Release the cpu lock and cancel the timer.
    debug("Release the cpu lock for " + aRequestId);
    cpuLockInfo.cpuLock.unlock();
    cpuLockInfo.timer.cancel();
    this._cpuLockDict.delete(aRequestId);
  },

  contractID: "@mozilla.org/alarm/proxy;1",

  classID: Components.ID("{fea1e884-9b05-11e1-9b64-87a7016c3860}"),

  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIAlarmProxy,
    Ci.nsISupportsWeakReference,
    Ci.nsIObserver,
  ]),
};

var EXPORTED_SYMBOLS = ["AlarmProxy"];
