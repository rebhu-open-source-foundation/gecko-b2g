/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PowerManagerDelegateService_h
#define PowerManagerDelegateService_h

#include "nsIGeckoBridge.h"
#include "nsCOMPtr.h"

namespace mozilla {

class PowerManagerDelegateService final : public nsIPowerManagerDelegate {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPOWERMANAGERDELEGATE

  static already_AddRefed<PowerManagerDelegateService>
  ConstructPowerManagerDelegate();

 private:
  PowerManagerDelegateService();
  ~PowerManagerDelegateService();
};

}  // namespace mozilla

#endif  // PowerManagerDelegateService_h
