/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { AppConstants } = ChromeUtils.import(
  "resource://gre/modules/AppConstants.jsm"
);
const { Log } = ChromeUtils.import("resource://gre/modules/Log.jsm");
const { Services } = ChromeUtils.import("resource://gre/modules/Services.jsm");
const { XPCOMUtils } = ChromeUtils.import(
  "resource://gre/modules/XPCOMUtils.jsm"
);

const REQUEST_CPU_LOCK_TIMEOUT = 10 * 1000; // 10 seconds.

function getLogger() {
  var logger = Log.repository.getLogger("AlarmProxy");
  logger.addAppender(new Log.DumpAppender(new Log.BasicFormatter()));
  logger.level = Log.Level.Debug;
  return logger;
}

const logger = getLogger();

function debug(aStr) {
  AppConstants.DEBUG_ALARM && logger.debug(aStr);
}

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
  init: function init() {
    debug("init");

    // A <requestId, {messageName, url, callback}> map.
    this._alarmRequests = new Map();
    // A <requestId, {cpuLock, timer}> map.
    this._cpuLockDict = new Map();

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

  add: function add(aUrl, aOptions, aCallback) {
    debug("add " + JSON.stringify(aOptions) + " " + aUrl);
    let date = aOptions.date;
    let ignoreTimezone = !!aOptions.ignoreTimezone;
    let data = aOptions.data;

    if (!date) {
      throw Components.Exception("", Cr.NS_ERROR_INVALID_ARG);
    }

    let requestId = Cc["@mozilla.org/uuid-generator;1"]
      .getService(Ci.nsIUUIDGenerator)
      .generateUUID()
      .toString();

    if (data && aUrl) {
      // Run eval() in the sand box with the principal of the calling
      // web page to ensure no cross-origin object is involved. A "Permission
      // Denied" error will be thrown in case of privilege violation.
      // We used JSON.stringify in the past, but the toJSON privileged function is denied now.
      try {
        let sandbox = new Cu.Sandbox(aUrl);
        sandbox.data = data;
        data = Cu.evalInSandbox("eval(data)", sandbox);
      } catch (e) {
        debug(e);
        aCallback.onAdd(
          Cr.NS_ERROR_INVALID_ARG,
          Cu.cloneInto("data examination in sandbox failed", {})
        );
        return;
      }
    }

    debug("_alarmRequests.set Alarm:Add " + requestId + " " + aUrl);
    this._alarmRequests.set(requestId, {
      messageName: "Alarm:Add",
      url: aUrl,
      callback: aCallback,
    });
    this._lockCpuForRequest(requestId);
    Services.cpmm.sendAsyncMessage("Alarm:Add", {
      requestId,
      date,
      ignoreTimezone,
      data,
      pageURL: aUrl,
      manifestURL: aUrl,
    });
  },

  remove: function remove(aUrl, aId) {
    debug("remove " + aId + " " + aUrl);
    Services.cpmm.sendAsyncMessage("Alarm:Remove", {
      id: aId,
      manifestURL: aUrl,
    });
  },

  getAll: function getAll(aUrl, aCallback) {
    debug("getAll " + aUrl);
    let requestId = Cc["@mozilla.org/uuid-generator;1"]
      .getService(Ci.nsIUUIDGenerator)
      .generateUUID()
      .toString();

    debug("_alarmRequests.set Alarm:GetAll " + requestId + " " + aUrl);
    this._alarmRequests.set(requestId, {
      messageName: "Alarm:GetAll",
      url: aUrl,
      callback: aCallback,
    });

    Services.cpmm.sendAsyncMessage("Alarm:GetAll", {
      requestId,
      manifestURL: aUrl,
    });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage: " + aMessage.name);
    let json = aMessage.json ? aMessage.json : {};
    let alarmRequest = this._alarmRequests.get(json.requestId);

    if (!alarmRequest) {
      debug(
        "Request with given requestId not found, skip." +
          aMessage.name +
          " " +
          JSON.stringify(json)
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

    debug("Handle " + aMessage.name + " json: " + JSON.stringify(json));
    switch (aMessage.name) {
      case "Alarm:Add:Return:OK":
        this._unlockCpuForRequest(json.requestId);
        alarmRequest.callback.onAdd(Cr.NS_OK, Cu.cloneInto(json.id, {}));
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

        alarmRequest.callback.onGetAll(Cr.NS_OK, Cu.cloneInto(alarms, {}));
        break;

      case "Alarm:Add:Return:KO":
        this._unlockCpuForRequest(json.requestId);
        alarmRequest.callback.onAdd(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, {})
        );
        break;

      case "Alarm:GetAll:Return:KO":
        alarmRequest.callback.onGetAll(
          Cr.NS_ERROR_FAILURE,
          Cu.cloneInto(json.error, {})
        );
        break;

      default:
        debug("Wrong message: " + aMessage.name);
        break;
    }
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

  classID: Components.ID("{736b46dc-9bea-4f75-a567-0bec41980f0c}"),

  QueryInterface: ChromeUtils.generateQI([
    Ci.nsIAlarmProxy,
    Ci.nsISupportsWeakReference,
  ]),
};

var EXPORTED_SYMBOLS = ["AlarmProxy"];
