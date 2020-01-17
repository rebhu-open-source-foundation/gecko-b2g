/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
* file or any portion thereof may not be reproduced or used in any manner
* whatsoever without the express written permission of KAI OS TECHNOLOGIES
* (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
* LIMITED or its affiliate company and may be registered in some jurisdictions.
* All other trademarks are the property of their respective owners.
*/

[Exposed=(Window)]
interface TetheringStatusChangeEvent : Event
{
  constructor(DOMString type, optional TetheringStatusChangeEventInit eventInitDict = {});
  /**
   * Device tethering state.
   */
  readonly attribute long tetheringState;
};

dictionary TetheringStatusChangeEventInit : EventInit
{
  long tetheringState = 0;
};
