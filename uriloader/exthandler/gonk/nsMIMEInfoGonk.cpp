/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsMIMEInfoGonk.h"

nsresult nsMIMEInfoGonk::LoadUriInternal(nsIURI *aURI) {
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMIMEInfoGonk::GetHasDefaultHandler(bool *_retval) {

  *_retval = false;

  return NS_OK;
}

nsresult nsMIMEInfoGonk::LaunchDefaultWithFile(nsIFile *aFile) {
  return NS_ERROR_FILE_NOT_FOUND;
}
