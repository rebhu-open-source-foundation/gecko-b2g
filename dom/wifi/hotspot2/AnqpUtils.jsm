/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

this.EXPORTED_SYMBOLS = ["AnqpCache", "AnqpData"];

const CACHE_SWEEP_INTERVAL_MILLISECONDS = 60 * 1000;
const DATA_LIFETIME_MILLISECONDS = 60 * 60 * 1000;

this.AnqpData = function(anqpElements) {
  if (anqpElements) {
    this._AnqpElements = anqpElements;
  }
  this._ExpiredTime = Date.now() + DATA_LIFETIME_MILLISECONDS;
};

this.AnqpData.prototype = {
  _AnqpElements: null,

  _ExpiredTime: 0,

  getElements() {
    return this._AnqpElements;
  },

  isExpired(time) {
    return this._ExpiredTime <= time;
  },
};

this.AnqpCache = function() {
  this._AnqpCaches = new Map();
  this._LastSweep = Date.now();
};

this.AnqpCache.prototype = {
  _AnqpCaches: null,

  _LastSweep: 0,

  addEntry(anqpNetworkKey, anqpElements) {
    let data = new AnqpData(anqpElements);
    this._AnqpCaches.set(anqpNetworkKey, data);
  },

  getEntry(anqpNetworkKey) {
    return this._AnqpCaches.get(anqpNetworkKey);
  },

  sweep() {
    let now = Date.now();

    // No need perform sweep if it's not reach threshold.
    if (now < this._LastSweep + CACHE_SWEEP_INTERVAL_MILLISECONDS) {
      return;
    }

    // Collect all expired keys.
    let expiredKeys = [];
    this._AnqpCaches.forEach(function(anqpData, anqpNetworkKey, map) {
      if (anqpData.isExpired(now)) {
        expiredKeys.push(anqpNetworkKey);
      }
    });

    // Remove expired caches.
    for (let key of expiredKeys) {
      this._AnqpCaches.delete(key);
    }
    this._LastSweep = now;
  },
};
