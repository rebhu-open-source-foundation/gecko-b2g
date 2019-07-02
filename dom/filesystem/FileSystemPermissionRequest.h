/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_FileSystemPermissionRequest_h
#define mozilla_dom_FileSystemPermissionRequest_h

#include "nsContentPermissionHelper.h"
#include "nsIRunnable.h"
#include "nsString.h"

class nsPIDOMWindowInner;

namespace mozilla {
namespace dom {

class FileSystemTaskChildBase;

class FileSystemPermissionRequest final : public ContentPermissionRequestBase,
                                          public nsIRunnable {
 public:
  // Request permission for the given task.
  static void RequestForTask(FileSystemTaskChildBase* aTask);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE

  // nsIContentPermissionRequest
  NS_IMETHOD Cancel(void) override;
  NS_IMETHOD Allow(JS::HandleValue choices) override;
  NS_IMETHOD GetTypes(nsIArray** aTypes) override;

 private:
  explicit FileSystemPermissionRequest(nsIPrincipal* aPrincipal,
                                       nsPIDOMWindowInner* aWindow,
                                       FileSystemTaskChildBase* aTask);

  ~FileSystemPermissionRequest() = default;

  // Once the permission check has been done, we must run the task using IPC and
  // PBackground. This method checks if the PBackground thread is ready to
  // receive the task and in case waits for ActorCreated() to be called using
  // the PBackgroundInitializer class (see FileSystemPermissionRequest.cpp).
  void ScheduleTask();

  nsCString mPermissionType;
  nsCString mPermissionAccess;
  RefPtr<FileSystemTaskChildBase> mTask;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_FileSystemPermissionRequest_h
