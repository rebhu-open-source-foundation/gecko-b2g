/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[Exposed=(Window,Worker),
 HeaderFile="KaiOS.h"]
interface KaiOS {
  // objects implementing this interface also implement the interfaces given
  // below
};

#ifdef MOZ_B2G_BT
[Exposed=Window]
partial interface KaiOS {
  [Throws]
  readonly attribute BluetoothManager mozBluetooth;
};
#endif // MOZ_B2G_BT

#ifdef MOZ_B2G_RIL
[Exposed=Window]
partial interface KaiOS {
  [Throws, Pref="dom.mobileconnection.enabled"]
  readonly attribute MozMobileConnectionArray mozMobileConnections;
};

[Exposed=Window]
partial interface KaiOS {
  [Throws, Pref="dom.telephony.enabled"]
  readonly attribute Telephony mozTelephony;
};

[Exposed=Window]
partial interface KaiOS {
  [Throws, Pref="dom.icc.enabled"]
  readonly attribute MozIccManager? mozIccManager;
};

[Exposed=Window]
partial interface KaiOS {
  [Throws, Pref="dom.datacall.enabled"]
  readonly attribute DataCallManager? dataCallManager;
};

[Exposed=Window]
partial interface KaiOS {
  [Throws, Pref="dom.voicemail.enabled"]
  readonly attribute MozVoicemail mozVoicemail;
};
#endif //MOZ_B2G_RIL

#ifndef DISABLE_WIFI
partial interface KaiOS {
  [Throws, Func="KaiOS::HasWifiManagerSupport", Exposed=Window]
  readonly attribute WifiManager wifiManager;
};
#endif // DISABLE_WIFI
