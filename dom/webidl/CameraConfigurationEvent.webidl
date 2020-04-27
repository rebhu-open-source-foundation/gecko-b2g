/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

[Exposed=(Window, Worker), Func="B2G::HasCameraSupport", LegacyEventInit]
interface CameraConfigurationEvent : Event
{
  constructor(DOMString type, optional CameraConfigurationEventInit eventInitDict = {});
  readonly attribute CameraMode mode;
  readonly attribute DOMString recorderProfile;
  readonly attribute DOMRectReadOnly? previewSize;
  readonly attribute DOMRectReadOnly? pictureSize;
};

dictionary CameraConfigurationEventInit : EventInit
{
  CameraMode mode = "picture";
  DOMString recorderProfile = "cif";
  DOMRectReadOnly? previewSize = null;
  DOMRectReadOnly? pictureSize = null;
};
