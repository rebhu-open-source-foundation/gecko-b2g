/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Exposed=Window]
interface VideoCallCameraCapabilitiesChangeEvent : Event
{
  constructor(DOMString type, optional VideoCallCameraCapabilitiesChangeEventInit eventInitDict={});
  readonly attribute unsigned short width;
  readonly attribute unsigned short height;
  readonly attribute unsigned short maxZoom;
  readonly attribute boolean zoomSupported;
};

dictionary VideoCallCameraCapabilitiesChangeEventInit : EventInit
{
  unsigned short width = 0;
  unsigned short height = 0;
  float maxZoom = 0.0;
  boolean zoomSupported = false;
};
