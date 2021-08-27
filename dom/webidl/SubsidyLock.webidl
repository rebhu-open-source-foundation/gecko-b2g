/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[GenerateConversionToJS]
dictionary lockTypes
{
  sequence<unsigned long> lockTypes;
};

[GenerateConversionToJS]
dictionary UnlockOptions
{
  required unsigned long lockType;

  DOMString password;
};

[GenerateConversionToJS]
dictionary SubsidyCardLockError
{
  DOMString? error = null;
  long retryCount = -1; // The number of remaining retries. -1 if unkown.
};

[Pref="dom.subsidylock.enabled",
Func="B2G::HasMobileConnectionSupport",
Exposed=Window]
interface SubsidyLock
{
  /**
   * Subsidy Lock Type.
   *
   * @See 3GPP TS 22.022, and RIL_PersoSubstate @ ril.h
   */
  // Unknown Key.
  const long SUBSIDY_LOCK_UNKNOWN                  = 0;
  // NCK (Network Control Key).
  const long SUBSIDY_LOCK_SIM_NETWORK              = 1;
  // NSCK (Network Subset Control Key).
  const long SUBSIDY_LOCK_NETWORK_SUBSET           = 2;
  // CCK (Corporate Control Key).
  const long SUBSIDY_LOCK_SIM_CORPORATE            = 3;
  // SPCK (Service Provider Control Key).
  const long SUBSIDY_LOCK_SIM_SERVICE_PROVIDER     = 4;
  // PCK (Personalisation Control Key).
  const long SUBSIDY_LOCK_SIM_SIM                  = 5;
  // PUK for NCK (Network Control Key).
  const long SUBSIDY_LOCK_SIM_NETWORK_PUK          = 6;
  // PUK for NSCK (Network Subset Control Key).
  const long SUBSIDY_LOCK_NETWORK_SUBSET_PUK       = 7;
  // PUK for CCK (Corporate Control Key).
  const long SUBSIDY_LOCK_SIM_CORPORATE_PUK        = 8;
  // PUK for SPCK (Service Provider Control Key).
  const long SUBSIDY_LOCK_SIM_SERVICE_PROVIDER_PUK = 9;
  // PUK for PCK (Personalisation Control Key).
  const long SUBSIDY_LOCK_SIM_SIM_PUK              = 10;

  [Throws]
  DOMRequest getSubsidyLockStatus();

  /**
   * Unlock a subsidy lock.
   *
   * @param info
   *        An object containing the information necessary to unlock
   *        the given lock.
   *
   * @return a DOMRequest.
   *         The request's error will be an object containing the number of
   *         remaining retries
   *         @see SubsidyCardLockError.
   */
  [Throws]
  DOMRequest unlockSubsidyLock(UnlockOptions info);
};
