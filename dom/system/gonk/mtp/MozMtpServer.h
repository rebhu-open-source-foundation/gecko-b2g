/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_system_mozmtpserver_h__
#define mozilla_system_mozmtpserver_h__

#include "MozMtpCommon.h"
#include "MozMtpDatabase.h"

#include "mozilla/FileUtils.h"

#include "nsCOMPtr.h"
#include "nsIThread.h"

BEGIN_MTP_NAMESPACE
using namespace android;

class RefCountedMtpServer : public MtpServer {
 public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedMtpServer)

  RefCountedMtpServer(IMtpDatabase* aDatabase, int controlFd, bool aPtp,
                      const char* deviceInfoManufacturer,
                      const char* deviceInfoModel,
                      const char* deviceInfoDeviceVersion,
                      const char* deviceInfoSerialNumber)
      : MtpServer(aDatabase, controlFd, aPtp, deviceInfoManufacturer,
                  deviceInfoModel, deviceInfoDeviceVersion,
                  deviceInfoSerialNumber) {}

 protected:
  virtual ~RefCountedMtpServer() {}
};

class MozMtpServer {
 public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MozMtpServer)

  bool Init();
  void Run();

  already_AddRefed<RefCountedMtpServer> GetMtpServer();
  already_AddRefed<MozMtpDatabase> GetMozMtpDatabase();

 protected:
  virtual ~MozMtpServer() {}

 private:
  RefPtr<RefCountedMtpServer> mMtpServer;
  RefPtr<MozMtpDatabase> mMozMtpDatabase;
  nsCOMPtr<nsIThread> mServerThread;
};
extern StaticRefPtr<MozMtpServer> sMozMtpServer;

END_MTP_NAMESPACE

#endif  // mozilla_system_mozmtpserver_h__
