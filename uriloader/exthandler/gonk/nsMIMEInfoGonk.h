/* -*- Mode: C++; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsMIMEInfoGonk_h_
#define nsMIMEInfoGonk_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoGonk : public nsMIMEInfoImpl {
 public:
  explicit nsMIMEInfoGonk(const char* aMIMEType = "")
      : nsMIMEInfoImpl(aMIMEType) {}
  explicit nsMIMEInfoGonk(const nsACString& aMIMEType)
      : nsMIMEInfoImpl(aMIMEType) {}
  nsMIMEInfoGonk(const nsACString& aType, HandlerClass aClass)
      : nsMIMEInfoImpl(aType, aClass) {}
  static bool HandlerExists(const char* aProtocolScheme);

 protected:
  NS_IMETHOD GetHasDefaultHandler(bool* _retval) override;

  virtual nsresult LoadUriInternal(nsIURI* aURI) override;

  virtual nsresult LaunchDefaultWithFile(nsIFile* aFile) override;
};

#endif  // nsMIMEInfoGonk_h_
