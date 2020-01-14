/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef nsRilWorkerService_H
#define nsRilWorkerService_H

#include <nsISupportsImpl.h>
#include <nsIRilWorkerService.h>
#include <nsTArray.h>
#include <nsRilWorker.h>
#include "nsCOMPtr.h"

class nsRilWorkerService final : public nsIRilWorkerService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRILWORKERSERVICE

  static already_AddRefed<nsRilWorkerService> CreateRilWorkerService();

  nsRilWorkerService();

private:
  ~nsRilWorkerService();
  nsTArray<RefPtr<nsRilWorker>>  mRilWorkers;
  int32_t mNumRilWorkers;
};

#endif
