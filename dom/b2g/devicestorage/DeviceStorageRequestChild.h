/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_devicestorage_DeviceStorageRequestChild_h
#define mozilla_dom_devicestorage_DeviceStorageRequestChild_h

#include "mozilla/dom/devicestorage/PDeviceStorageRequestChild.h"

class DeviceStorageFile;
class DeviceStorageRequest;
struct DeviceStorageFileDescriptor;

namespace mozilla {
namespace dom {

namespace devicestorage {

class DeviceStorageRequestChildCallback {
 public:
  virtual void RequestComplete() = 0;
};

class DeviceStorageRequestChild : public PDeviceStorageRequestChild {
 public:
  DeviceStorageRequestChild();
  explicit DeviceStorageRequestChild(DeviceStorageRequest* aRequest);
  ~DeviceStorageRequestChild();

  virtual mozilla::ipc::IPCResult Recv__delete__(
      const DeviceStorageResponseValue& value);

 private:
  RefPtr<DeviceStorageRequest> mRequest;
};

}  // namespace devicestorage
}  // namespace dom
}  // namespace mozilla

#endif
