/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.mobileconnection.enabled",
 Exposed=Window]
interface OtaStatusEvent : Event
{
  constructor(DOMString type, optional OtaStatusEventInit eventInitDict={});

  /**
   * One of the following values:
   *
   * spl_unlocked
   * spc_retries_exceeded
   * a_key_exchanged
   * ssd_updated
   * nam_downloaded
   * mdn_downloaded
   * imsi_downloaded
   * prl_downloaded
   * committed
   * otapa_started
   * otapa_stopped
   * otapa_aborted
   */
  readonly attribute DOMString status;
};

dictionary OtaStatusEventInit : EventInit
{
  DOMString status = "";
};
