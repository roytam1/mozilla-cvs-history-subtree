/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.  Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 */

#ifdef IBMBIDI

#ifndef nsBidiPresUtils_h___
#define nsBidiPresUtils_h___

#include "prtypes.h"
#include "nsVoidArray.h"
#include "nsIFrame.h"
#include "nsIBidi.h"
#include "nsIUBidiUtils.h"

class nsBidiPresUtils {
public:
  nsBidiPresUtils();
  ~nsBidiPresUtils();
  PRBool IsSuccessful(void);
  nsresult Resolve(nsIPresContext* aPresContext,
                   nsIFrame*       aBlockFrame,
                   nsIFrame*       aFirstChild);
  void ReorderFrames(nsIPresContext* aPresContext,
                     nsIFrame*       aFirstChild,
                     nsIFrame*       aNextInFlow,
                     PRInt32         aChildCount);

  nsresult FormatUnicodeText(nsIPresContext* aPresContext,
                             PRUnichar*      aText,
                             PRInt32&        aTextLength,
                             UCharDirection  aTextClass,
                             PRBool          aIsOddLevel,
                             PRBool          aIsBidiSystem);
private:
  void CreateBlockBuffer(nsIPresContext* aPresContext);
  void InitLogicalArray(nsIPresContext* aPresContext,
                        nsIFrame*       aCurrentFrame,
                        nsIFrame*       aNextInFlow,
                        PRBool          aAddMarkers = PR_FALSE);
  nsresult Reorder(nsIPresContext* aPresContext,
                   PRBool&         aBidiEnabled);
  void RepositionInlineFrames(nsIPresContext* aPresContext,
                              nsIFrame*       aFirstChild,
                              PRInt32         aChildCount);
  void RepositionContainerFrame(nsIPresContext* aPresContext,
                                nsIFrame*       aContainer,
                                PRInt32&        aMinX,
                                PRInt32&        aMaxX);
  nsresult CreateBidiContinuation(nsIPresContext* aPresContext,
                                  nsIContent*     aContent,
                                  nsIFrame*       aFrame,
                                  nsIFrame**      aNewFrame);
  PRBool RemoveBidiContinuation(nsIPresContext* aPresContext,
                                nsIFrame*       aFrame,
                                nsIFrame*       aNextFrame);
  void AdjustEmbeddingLevel(nsIFrame* aFrame,
                            PRUint8&  aEmbeddingLevel);
  void CalculateTextClass(PRInt32  aLimit,
                          PRInt32& aOffset,
                          PRUint8& aTextClass,
                          PRUint8& aPrevTextClass);
  nsAutoString    mBuffer;
  nsVoidArray     mLogicalFrames;
  nsVoidArray     mVisualFrames;
  PRInt32         mArraySize;
  PRInt32*        mIndexMap;
  PRUint8*        mLevels;
  nsresult        mSuccess;
public:
  nsIBidi*        mBidiEngine;
  nsIUBidiUtils*  mUnicodeUtils;
};

#endif /* nsBidiPresUtils_h___ */

#endif // IBMBIDI