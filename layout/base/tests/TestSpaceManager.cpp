/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include <stdio.h>
#include "nscore.h"
#include "nsCRT.h"
#include "nscoord.h"
#include "nsSpaceManager.h"

class MySpaceManager: public SpaceManager {
public:
  MySpaceManager(nsIFrame* aFrame) : SpaceManager(aFrame) {}

  PRBool  TestAddBand();
  PRBool  TestAddBandOverlap();
  PRBool  TestAddRectToBand();

protected:
  struct BandInfo {
    nscoord   yOffset;
    nscoord   height;
    BandRect* firstRect;
    PRIntn    numRects;
  };

  struct BandsInfo {
    PRIntn   numBands;
    BandInfo bands[25];
  };

  void  GetBandsInfo(BandsInfo&);
};

void MySpaceManager::GetBandsInfo(BandsInfo& aBandsInfo)
{
  aBandsInfo.numBands = 0;

  if (!PR_CLIST_IS_EMPTY(&mBandList)) {
    BandRect* band = (BandRect*)PR_LIST_HEAD(&mBandList);
    while (nsnull != band) {
      BandInfo& info = aBandsInfo.bands[aBandsInfo.numBands];

      info.yOffset = band->top;
      info.height = band->bottom - band->top;
      info.firstRect = band;

      aBandsInfo.numBands++;

      // Get the next band, and count the number of rects in this band
      info.numRects = 0;
      while (info.yOffset == band->top) {
        info.numRects++;

        band = (BandRect*)PR_NEXT_LINK(band);
        if (band == &mBandList) {
          // No bands left
          band = nsnull;
          break;
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//

// Test of adding a rect region that causes a new band to be created (no
// overlap with an existing band)
//
// This tests the following:
// 1. when there are no existing bands
// 2. adding a new band above the topmost band
// 3. inserting a new band between two existing bands
// 4. adding a new band below the bottommost band
PRBool MySpaceManager::TestAddBand()
{
  PRBool    status;
  BandsInfo bandsInfo;
  
  // Clear any existing regions
  ClearRegions();
  NS_ASSERTION(PR_CLIST_IS_EMPTY(&mBandList), "clear regions failed");

  /////////////////////////////////////////////////////////////////////////////
  // #1. Add a rect region. Verify the return status, and that a band rect is
  // added
  status = AddRectRegion((nsIFrame*)0x01, nsRect(10, 100, 100, 100));
  if (PR_FALSE == status) {
    printf("TestAddBand: add returned false (#1)\n");
    return PR_FALSE;
  }
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 1) {
    printf("TestAddBand: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 100) || (bandsInfo.bands[0].height != 100)) {
    printf("TestAddBand: wrong band size (#1)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #2. Add another band rect completely above the first band rect
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, 10, 100, 20));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 2) {
    printf("TestAddBand: wrong number of bands (#2): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 100) || (bandsInfo.bands[1].height != 100)) {
    printf("TestAddBand: wrong band sizes (#2)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #3. Now insert a new band between the two existing bands
  status = AddRectRegion((nsIFrame*)0x03, nsRect(10, 40, 100, 30));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBand: wrong number of bands (#3): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 40) || (bandsInfo.bands[1].height != 30) ||
      (bandsInfo.bands[2].yOffset != 100) || (bandsInfo.bands[2].height != 100)) {
    printf("TestAddBand: wrong band sizes (#3)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #4. Append a new bottommost band
  status = AddRectRegion((nsIFrame*)0x04, nsRect(10, 210, 100, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 4) {
    printf("TestAddBand: wrong number of bands (#4): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 10) || (bandsInfo.bands[0].height != 20) ||
      (bandsInfo.bands[1].yOffset != 40) || (bandsInfo.bands[1].height != 30) ||
      (bandsInfo.bands[2].yOffset != 100) || (bandsInfo.bands[2].height != 100) ||
      (bandsInfo.bands[3].yOffset != 210) || (bandsInfo.bands[3].height != 100)) {
    printf("TestAddBand: wrong band sizes (#4)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

// Test of adding a rect region that overlaps an existing band
//
// This tests the following:
// 1. Adding a rect that's above and pertially overlaps an existing band
// 2. Adding a rect that's completely contained by an existing band
// 3. Adding a rect that overlaps and is below an existing band
// 3. Adding a rect that contains an existing band
PRBool MySpaceManager::TestAddBandOverlap()
{
  PRBool    status;
  BandsInfo bandsInfo;
  
  // Clear any existing regions
  ClearRegions();
  NS_ASSERTION(PR_CLIST_IS_EMPTY(&mBandList), "clear regions failed");

  // Add a new band
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");

  /////////////////////////////////////////////////////////////////////////////
  // #1. Add a rect region that's above and partially overlaps an existing band
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, 50, 50, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBandOverlap: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 50) || (bandsInfo.bands[0].height != 50) ||
      (bandsInfo.bands[1].yOffset != 100) || (bandsInfo.bands[1].height != 50) ||
      (bandsInfo.bands[2].yOffset != 150) || (bandsInfo.bands[2].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#1)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#1)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #2. Add a rect region that's contained by the first band
  status = AddRectRegion((nsIFrame*)0x03, nsRect(200, 60, 50, 10));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 5) {
    printf("TestAddBandOverlap: wrong number of bands (#2): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 50) || (bandsInfo.bands[0].height != 10) ||
      (bandsInfo.bands[1].yOffset != 60) || (bandsInfo.bands[1].height != 10) ||
      (bandsInfo.bands[2].yOffset != 70) || (bandsInfo.bands[2].height != 30) ||
      (bandsInfo.bands[3].yOffset != 100) || (bandsInfo.bands[3].height != 50) ||
      (bandsInfo.bands[4].yOffset != 150) || (bandsInfo.bands[4].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#2)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1) ||
      (bandsInfo.bands[3].numRects != 2) ||
      (bandsInfo.bands[4].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#2)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #3. Add a rect that overlaps and is below an existing band
  status = AddRectRegion((nsIFrame*)0x04, nsRect(200, 175, 50, 50));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 7) {
    printf("TestAddBandOverlap: wrong number of bands (#3): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 50) || (bandsInfo.bands[0].height != 10) ||
      (bandsInfo.bands[1].yOffset != 60) || (bandsInfo.bands[1].height != 10) ||
      (bandsInfo.bands[2].yOffset != 70) || (bandsInfo.bands[2].height != 30) ||
      (bandsInfo.bands[3].yOffset != 100) || (bandsInfo.bands[3].height != 50) ||
      (bandsInfo.bands[4].yOffset != 150) || (bandsInfo.bands[4].height != 25) ||
      (bandsInfo.bands[5].yOffset != 175) || (bandsInfo.bands[5].height != 25) ||
      (bandsInfo.bands[6].yOffset != 200) || (bandsInfo.bands[6].height != 25)) {
    printf("TestAddBandOverlap: wrong band sizes (#3)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1) ||
      (bandsInfo.bands[3].numRects != 2) ||
      (bandsInfo.bands[4].numRects != 1) ||
      (bandsInfo.bands[5].numRects != 2) ||
      (bandsInfo.bands[6].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#3)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #4. Now test adding a rect that contains an existing band
  ClearRegions();
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");

  // Now add a rect that contains the existing band vertically
  status = AddRectRegion((nsIFrame*)0x02, nsRect(200, 50, 100, 200));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");

  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 3) {
    printf("TestAddBandOverlap: wrong number of bands (#4): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].yOffset != 50) || (bandsInfo.bands[0].height != 50) ||
      (bandsInfo.bands[1].yOffset != 100) || (bandsInfo.bands[1].height != 100) ||
      (bandsInfo.bands[2].yOffset != 200) || (bandsInfo.bands[2].height != 50)) {
    printf("TestAddBandOverlap: wrong band sizes (#4)\n");
    return PR_FALSE;
  }
  if ((bandsInfo.bands[0].numRects != 1) ||
      (bandsInfo.bands[1].numRects != 2) ||
      (bandsInfo.bands[2].numRects != 1)) {
    printf("TestAddBandOverlap: wrong number of rects (#4)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

// Test of adding rects within an existing band
//
// This tests the following:
// 1. Add a rect to the left of an existing rect
// 2. Add a rect to the right of the rightmost rect
// 3. Add a rect that's to the left of an existing rect and that overlaps it
// 4. Add a rect that's to the right of an existing rect and that overlaps it
// 5. Add a rect over top of an existing rect (existing rect contains new rect)
// 6. Add a new rect that completely contains an existing rect
PRBool MySpaceManager::TestAddRectToBand()
{
  PRBool    status;
  BandsInfo bandsInfo;
  BandRect* bandRect;
  
  // Clear any existing regions
  ClearRegions();
  NS_ASSERTION(PR_CLIST_IS_EMPTY(&mBandList), "clear regions failed");

  // Add a new band
  status = AddRectRegion((nsIFrame*)0x01, nsRect(100, 100, 100, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");

  /////////////////////////////////////////////////////////////////////////////
  // #1. Add a rect region that's to the left of the existing rect
  status = AddRectRegion((nsIFrame*)0x02, nsRect(10, 100, 50, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  if (bandsInfo.numBands != 1) {
    printf("TestAddRectToBand: wrong number of bands (#1): %i\n", bandsInfo.numBands);
    return PR_FALSE;
  }
  if (bandsInfo.bands[0].numRects != 2) {
    printf("TestAddRectToBand: wrong number of rects (#1): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->left != 10) || (bandRect->right != 60)) {
    printf("TestAddRectToBand: wrong first rect (#1)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 100) || (bandRect->right != 200)) {
    printf("TestAddRectToBand: wrong second rect (#1)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #2. Add a rect region that's to the right of the rightmost rect
  status = AddRectRegion((nsIFrame*)0x03, nsRect(250, 100, 100, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 3) {
    printf("TestAddRectToBand: wrong number of rects (#2): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->left != 10) || (bandRect->right != 60)) {
    printf("TestAddRectToBand: wrong first rect (#2)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 100) || (bandRect->right != 200)) {
    printf("TestAddRectToBand: wrong second rect (#2)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 250) || (bandRect->right != 350)) {
    printf("TestAddRectToBand: wrong third rect (#2)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #3. Add a rect region that's to the left of an existing rect and that
  // overlaps the rect
  status = AddRectRegion((nsIFrame*)0x04, nsRect(80, 100, 40, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 5) {
    printf("TestAddRectToBand: wrong number of rects (#3): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  if ((bandRect->left != 10) || (bandRect->right != 60)) {
    printf("TestAddRectToBand: wrong first rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 80) || (bandRect->right != 100)) {
    printf("TestAddRectToBand: wrong second rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 100) || (bandRect->right != 120) ||
      (bandRect->numFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x04)) {
    printf("TestAddRectToBand: wrong third rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 120) || (bandRect->right != 200)) {
    printf("TestAddRectToBand: wrong fourth rect (#3)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 250) || (bandRect->right != 350)) {
    printf("TestAddRectToBand: wrong fifth rect (#3)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #4. Add a rect region that's to the right of an existing rect and that
  // overlaps the rect
  status = AddRectRegion((nsIFrame*)0x05, nsRect(50, 100, 20, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 7) {
    printf("TestAddRectToBand: wrong number of rects (#4): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 10) || (bandRect->right != 50)) {
    printf("TestAddRectToBand: wrong first rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 50) || (bandRect->right != 60) ||
      (bandRect->numFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x05)) {
    printf("TestAddRectToBand: wrong second rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 60) || (bandRect->right != 70)) {
    printf("TestAddRectToBand: wrong third rect (#4)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 80) || (bandRect->right != 100)) {
    printf("TestAddRectToBand: wrong fourth rect (#4)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #5. Add a new rect over top of an existing rect (existing rect contains
  // the new rect)
  status = AddRectRegion((nsIFrame*)0x06, nsRect(20, 100, 20, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 9) {
    printf("TestAddRectToBand: wrong number of rects (#5): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 10) || (bandRect->right != 20)) {
    printf("TestAddRectToBand: wrong first rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 20) || (bandRect->right != 40) ||
      (bandRect->numFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x06)) {
    printf("TestAddRectToBand: wrong second rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 40) || (bandRect->right != 50)) {
    printf("TestAddRectToBand: wrong third rect (#5)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 50) || (bandRect->right != 60) || (bandRect->numFrames != 2)) {
    printf("TestAddRectToBand: wrong fourth rect (#5)\n");
    return PR_FALSE;
  }

  /////////////////////////////////////////////////////////////////////////////
  // #6. Add a new rect that completely contains an existing rect
  status = AddRectRegion((nsIFrame*)0x07, nsRect(0, 100, 30, 100));
  NS_ASSERTION(PR_TRUE == status, "unexpected status");
  GetBandsInfo(bandsInfo);
  NS_ASSERTION(bandsInfo.numBands == 1, "wrong number of bands");
  if (bandsInfo.bands[0].numRects != 11) {
    printf("TestAddRectToBand: wrong number of rects (#6): %i\n", bandsInfo.bands[0].numRects);
    return PR_FALSE;
  }
  bandRect = bandsInfo.bands[0].firstRect;
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 0) || (bandRect->right != 10)) {
    printf("TestAddRectToBand: wrong first rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 10) || (bandRect->right != 20) ||
      (bandRect->numFrames != 2) || !bandRect->IsOccupiedBy((nsIFrame*)0x07)) {
    printf("TestAddRectToBand: wrong second rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 20) || (bandRect->right != 30) ||
      (bandRect->numFrames != 3) || !bandRect->IsOccupiedBy((nsIFrame*)0x07)) {
    printf("TestAddRectToBand: wrong third rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  if ((bandRect->left != 30) || (bandRect->right != 40) || (bandRect->numFrames != 2)) {
    printf("TestAddRectToBand: wrong fourth rect (#6)\n");
    return PR_FALSE;
  }
  bandRect = (BandRect*)PR_NEXT_LINK(bandRect);
  NS_ASSERTION(1 == bandRect->numFrames, "unexpected shared rect");
  if ((bandRect->left != 40) || (bandRect->right != 50)) {
    printf("TestAddRectToBand: wrong fifth rect (#6)\n");
    return PR_FALSE;
  }

  return PR_TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//

int main(int argc, char** argv)
{
  // Create a space manager
  MySpaceManager* spaceMgr = new MySpaceManager(nsnull);
  
  NS_ADDREF(spaceMgr);

  // Test adding rect regions
  if (!spaceMgr->TestAddBand()) {
    NS_RELEASE(spaceMgr);
    return -1;
  }

  // Test adding rect regions that overlap existing bands
  if (!spaceMgr->TestAddBandOverlap()) {
    NS_RELEASE(spaceMgr);
    return -1;
  }

  // Test adding rects within an existing band
  if (!spaceMgr->TestAddRectToBand()) {
    NS_RELEASE(spaceMgr);
    return -1;
  }

#if 0
  if (!spaceMgr->TestGetBandData()) {
    return -1;
  }
#endif
  NS_RELEASE(spaceMgr);
  return 0;
}
