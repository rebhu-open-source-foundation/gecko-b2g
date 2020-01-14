/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/**
 * The possible values of coverage losing.
 *
 * "ims-over-wifi"  - indicating ims-over-wifi coverage is going to lose.
 */
enum TelephonyBearer { "ims-over-wifi" };

[Pref="dom.telephony.enabled",
 Exposed=Window]
interface TelephonyCoverageLosingEvent : Event
{
  constructor(DOMString type, optional TelephonyCoverageLosingEventInit eventInitDict={});
  readonly attribute TelephonyBearer type;
};

dictionary TelephonyCoverageLosingEventInit : EventInit
{
  /*
   * Default is "ims-over-wifi" because this call back is created for TMO-US specifically.
   * May add more values after getting related request(s).
   */
  TelephonyBearer type = "ims-over-wifi";
};
