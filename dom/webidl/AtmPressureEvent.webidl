/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="device.sensors.atmPressure.enabled", Func="nsGlobalWindowInner::DeviceSensorsEnabled",
 Exposed=Window]
interface AtmPressureEvent : Event
{
  constructor(DOMString type, optional AtmPressureEventInit eventInitDict = {});

  readonly attribute double value;
};

dictionary AtmPressureEventInit : EventInit
{
  double value = 0;
};
