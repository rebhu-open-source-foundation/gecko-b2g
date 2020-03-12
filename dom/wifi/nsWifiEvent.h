/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsWifiEvent_H
#define nsWifiEvent_H

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIWifiEvent.h"

class nsStateChanged;

class nsWifiEvent final : public nsIWifiEvent {
 public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIWIFIEVENT
  nsWifiEvent();
  nsWifiEvent(const nsAString& aName);

  void updateStateChanged(nsStateChanged* aStateChanged);

  nsString mName;
  nsString mBssid;
  bool mLocallyGenerated;
  uint32_t mReason;
  RefPtr<nsIStateChanged> mStateChanged;

 private:
  ~nsWifiEvent() {}
};

#endif  // nsWifiEvent_H
