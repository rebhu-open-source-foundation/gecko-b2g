/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
  * This dictionnary holds the parameters sent to the wifi service.
  */
[GenerateInit]
dictionary WifiCommandOptions
{
  unsigned long  id = 0;       // opaque id.
  unsigned long  cmd = 0;

  boolean        enabled = false;
  DOMString      countryCode = "";
  DOMString      softapCountryCode = "";
  unsigned short btCoexistenceMode = 0;
  unsigned long  bandMask = 0;
  unsigned long  scanType = 0;

  WifiConfiguration    config = {};
  SoftapConfiguration  softapConfig = {};
  SupplicantDebugLevel debugLevel = {};
  ScanSettings         scanSettings = {};
  PnoScanSettings      pnoScanSettings = {};
};

/**
  * This dictionary holds the callback parameter sent back from WifiCertService
  * to WifiWorker, and should only be passed around in chrome process.
  */
[GenerateInit]
dictionary WifiCertServiceResultOptions
{
  long            id = 0;         // request id in WifiWorker.
  long            status = 0;     // error code of the request, 0 indicates success.
  unsigned short  usageFlag = 0;  // usage flag of certificate, the flag is defined
                                  // in nsIWifiCertService.idl
  DOMString       nickname = "";  // nickname of certificate of the request.
  long            duplicated = 0; // duplicated flag of certificate.
};

/**
 * The dictionary holds the parameters for wifi connection.
 */
[GenerateInit]
dictionary WifiConfiguration
{
  DOMString ssid;
  DOMString bssid;
  DOMString keyMgmt;
  DOMString psk;
  DOMString wepKey0;
  DOMString wepKey1;
  DOMString wepKey2;
  DOMString wepKey3;
  long      wepTxKeyIndex;
  boolean   scanSsid;
  boolean   pmf;
  long      proto;
  long      authAlg;
  long      groupCipher;
  long      pairwiseCipher;

  /* EAP */
  long      eap;
  long      eapPhase2;
  DOMString identity;
  DOMString anonymousId;
  DOMString password;
  DOMString clientCert;
  DOMString caCert;
  DOMString caPath;
  DOMString subjectMatch;
  DOMString engineId;
  boolean   engine;
  DOMString privateKeyId;
  DOMString altSubjectMatch;
  DOMString domainSuffixMatch;
  boolean   proactiveKeyCaching;
};

/**
 * The dictionary holds the parameters for softap.
 */
[GenerateInit]
dictionary SoftapConfiguration
{
  DOMString     ssid;
  unsigned long keyMgmt;
  DOMString     key;
  DOMString     countryCode;
  unsigned long band;
  unsigned long channel;
  boolean       hidden;
  boolean       enable11N;
  boolean       enable11AC;
  boolean       enableACS;
  boolean       acsExcludeDfs;
};

/**
 * The dictionary holds the parameters for wifi connection.
 */
[GenerateInit]
dictionary SupplicantDebugLevel
{
  unsigned long logLevel;
  boolean       showTimeStamp;
  boolean       showKeys;
};

/**
 * The dictionary holds the parameters for scan settings.
 */
[GenerateInit]
dictionary ScanSettings
{
  unsigned long       scanType;
  sequence<long>      channels;
  sequence<DOMString> hiddenNetworks;
};

/**
 * The dictionary holds the parameters for pno scan settings.
 */
[GenerateInit]
dictionary PnoScanSettings
{
  long                 interval;
  long                 min2gRssi;
  long                 min5gRssi;
  sequence<PnoNetwork> pnoNetworks;
};

/**
 * The dictionary holds the parameters for pno network.
 */
[GenerateInit]
dictionary PnoNetwork
{
  boolean        isHidden;
  DOMString      ssid;
  sequence<long> frequencies;
};
