/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

dictionary HomeSpParams {
  DOMString fqdn;
  DOMString friendlyName;
  sequence<long>? matchAllOis;
  sequence<long>? matchAnyOis;
  sequence<DOMString>? otherHomePartners;
  sequence<long>? roamingConsortiumOis;
};

dictionary CredentialParams {
  DOMString realm;
  DOMString imsi;
  long eapType;
};

dictionary PasspointConfiguration {
  HomeSpParams homeSp;
  CredentialParams credential;
};
