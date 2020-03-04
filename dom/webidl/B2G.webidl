/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[Exposed=(Window,Worker)]
interface B2G {
  // objects implementing this interface also implement the interfaces given
  // below
};

partial interface B2G {
  [Throws, Exposed=Window]
  readonly attribute TetheringManager tetheringManager;
};

#ifdef HAS_KOOST_MODULES
[Exposed=(Window,Worker)]
partial interface B2G {
  [Throws]
  readonly attribute ExternalAPI externalapi;
};
#endif

#ifdef MOZ_B2G_BT
[Exposed=Window]
partial interface B2G {
  [Throws]
  readonly attribute BluetoothManager bluetooth;
};
#endif // MOZ_B2G_BT

#ifndef DISABLE_WIFI
partial interface B2G {
  [Throws, Func="B2G::HasWifiManagerSupport", Exposed=Window]
  readonly attribute WifiManager wifiManager;
};
#endif // DISABLE_WIFI
