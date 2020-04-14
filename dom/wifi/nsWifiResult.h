/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsWifiResult_H
#define nsWifiResult_H

#include "nsWifiElement.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIWifiResult.h"

class nsScanResult;

class nsWifiResult final : public nsIWifiResult {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFIRESULT

  nsWifiResult();

  void updateScanResults(nsTArray<RefPtr<nsScanResult>>& aScanResults);
  void updateChannels(nsTArray<int32_t>& aChannels);

  uint32_t mId;
  uint32_t mStatus;

  nsString mDriverVersion;
  nsString mFirmwareVersion;
  nsString mMacAddress;
  nsString mStaInterface;
  nsString mApInterface;
  uint32_t mCapabilities;
  uint32_t mStaCapabilities;
  uint32_t mDebugLevel;
  uint32_t mNumStations;
  nsTArray<int32_t> mChannels;
  nsTArray<RefPtr<nsScanResult>> mScanResults;

 private:
  ~nsWifiResult(){};
};

#endif  // nsWifiResult_H
