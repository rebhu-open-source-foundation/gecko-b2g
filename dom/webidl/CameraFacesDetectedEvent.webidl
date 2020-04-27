/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

//[Pref="camera.control.face_detection.enabled",
[Exposed=(Window, Worker), Func="B2G::HasCameraSupport", LegacyEventInit]
interface CameraFacesDetectedEvent : Event
{
  constructor(DOMString type, optional CameraFacesDetectedEventInit eventInitDict = {});
  [Pure, Cached]
  readonly attribute sequence<CameraDetectedFace>? faces;
};

dictionary CameraFacesDetectedEventInit : EventInit
{
  sequence<CameraDetectedFace>? faces = null;
};
