/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "OSPreferences.h"
#include "mozilla/Preferences.h"

using namespace mozilla::intl;

OSPreferences::OSPreferences() {}

bool OSPreferences::ReadSystemLocales(nsTArray<nsCString>& aLocaleList) {
  return false;
}

bool OSPreferences::ReadRegionalPrefsLocales(nsTArray<nsCString>& aLocaleList) {
  // For now we're just taking System Locales since we don't know of any better
  // API for regional prefs.
  return ReadSystemLocales(aLocaleList);
}

bool OSPreferences::ReadDateTimePattern(DateTimeFormatStyle aDateFormatStyle,
                                        DateTimeFormatStyle aTimeFormatStyle,
                                        const nsACString& aLocale,
                                        nsACString& aRetVal) {
  return false;
}

void OSPreferences::RemoveObservers() {}
