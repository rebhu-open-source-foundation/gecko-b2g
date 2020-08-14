/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { WifiConstants, EAPConstants, PasspointMatch } = ChromeUtils.import(
  "resource://gre/modules/WifiConstants.jsm"
);

this.EXPORTED_SYMBOLS = [
  "PasspointProvider",
  "PasspointConfig",
  "HomeSp",
  "Credential",
];

// PasspointConfig
this.PasspointConfig = function(config) {
  if (config) {
    if (config.getHomeSp()) {
      this.setHomeSp(config.getHomeSp());
    }

    if (config.getCredential()) {
      this.setCredential(config.getCredential());
    }
  }
};

this.PasspointConfig.prototype = {
  _HomeSp: null,

  _Credential: null,

  setHomeSp(homeSp) {
    this._HomeSp = homeSp;
  },

  getHomeSp() {
    return this._HomeSp;
  },

  setCredential(credential) {
    this._Credential = credential;
  },

  getCredential() {
    return this._Credential;
  },
};

// PasspointProvider
this.PasspointProvider = function(passpointConfig) {
  this._passpointConfig = new PasspointConfig(passpointConfig);
  if (this._passpointConfig.getCredential()) {
    this._EAPMethod = this._passpointConfig.getCredential().getEapType();
    this._Imsi = this._passpointConfig.getCredential().getImsi();
  }
};

this.PasspointProvider.prototype = {
  _PasspointConfig: null,

  _Imsi: null,

  _EAPMethod: EAPConstants.INVALID_EAP,

  getConfig() {
    // Return copied config to avoid updated by others
    return new PasspointConfig(this._PasspointConfig);
  },

  getImsi() {
    return this._Imsi;
  },

  match(anqpElements) {
    //TODO: We need anqpElements detail such as 3GPPNetwork,
    //      RoamingConsortium to complete this part.
    //let providerMatch = matchProviderExceptFor3GPP(anqpElements);

    return PasspointMatch.None;
  },
};

// HomeSp
this.HomeSp = function() {};

this.HomeSp.prototype = {
  _Fqdn: null,

  _FriendlyName: null,

  setFqdn(fqdn) {
    this._Fqdn = fqdn;
  },

  getFqdn() {
    return this._Fqdn;
  },

  setFriendlyName(friendlyName) {
    this._FriendlyName = friendlyName;
  },

  getFriendlyName() {
    return this._FriendlyName;
  },
};

// Credential
this.Credential = function() {};

this.Credential.prototype = {
  _Realm: null,

  //TODO: Only implement SIM credential for Passpoint R1.
  _Imsi: null,

  _EapType: EAPConstants.INVALID_EAP,

  setRealm(realm) {
    this._Realm = realm;
  },

  getRealm() {
    return this._Realm;
  },

  setImsi(imsi) {
    this._Imsi = imsi;
  },

  getImsi() {
    return this._Imsi;
  },

  setEapType(eapType) {
    this._EapType = eapType;
  },

  getEapType() {
    return this._EapType;
  },
};
