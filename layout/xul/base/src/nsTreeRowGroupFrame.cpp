/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsTreeFrame.h"
#include "nsTreeRowGroupFrame.h"
#include "nsIStyleContext.h"
#include "nsIContent.h"
#include "nsCSSRendering.h"
#include "nsTreeCellFrame.h"
#include "nsCellMap.h"

//
// NS_NewTreeFrame
//
// Creates a new tree frame
//
nsresult
NS_NewTreeRowGroupFrame (nsIFrame** aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");
  if (nsnull == aNewFrame) {
    return NS_ERROR_NULL_POINTER;
  }
  nsTreeRowGroupFrame* it = new nsTreeRowGroupFrame;
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
  
} // NS_NewTreeFrame


// Constructor
nsTreeRowGroupFrame::nsTreeRowGroupFrame()
:nsTableRowGroupFrame() { }

// Destructor
nsTreeRowGroupFrame::~nsTreeRowGroupFrame()
{
}

PRBool nsTreeRowGroupFrame::IsFlexible()
{
  // Only row groups are flexible.
  const nsStyleDisplay *display=nsnull;
  GetStyleData(eStyleStruct_Display, ((const nsStyleStruct *&)display));
  return (display->mDisplay == NS_STYLE_DISPLAY_TABLE_ROW_GROUP);
}
