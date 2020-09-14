/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[Pref="dom.icc.enabled",
 Func="B2G::HasMobileConnectionSupport",
 Exposed=Window]
interface IccCardLockError {
  constructor(DOMString errorName, short retryCount);
  readonly attribute DOMString errorName;
  readonly attribute short retryCount;
};
