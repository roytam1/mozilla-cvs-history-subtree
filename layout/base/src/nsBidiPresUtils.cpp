/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * IBM Corporation
 */

#ifdef IBMBIDI

#include "nsBidiPresUtils.h"
#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsTextFragment.h"
#include "nsLayoutAtoms.h"
#include "nsIPresContext.h"
#include "nsIStyleContext.h"
#include "nsIServiceManager.h"
#include "nsHTMLContainerFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIUBidiUtils.h"
#include "nsIFrameManager.h"

const PRUnichar kSpace            = 0x0020;
const PRUnichar kLineSeparator    = 0x2028;
const PRUnichar kObjectSubstitute = 0xFFFC;
const PRUnichar kLRE              = 0x202A;
const PRUnichar kRLE              = 0x202B;
const PRUnichar kPDF              = 0x202C;

static NS_DEFINE_CID(kBidiCID, NS_BIDI_CID);
static NS_DEFINE_CID(kUBidiUtilCID, NS_UNICHARBIDIUTIL_CID);

extern nsresult
NS_SetContentLengthAndOffsetForBidi(nsIFrame* aFrame, PRInt32 aStart, PRInt32 aEnd);
extern nsresult
NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsIFrame** aResult);
extern nsresult
NS_NewDirectionalFrame(nsIFrame** aNewFrame, PRUnichar aChar);
extern PRUnichar
NS_GetControlCharacter(nsIFrame* aFrame);

nsBidiPresUtils::nsBidiPresUtils() : mSuccess(NS_ERROR_FAILURE),
                                     mBidiEngine(nsnull),
                                     mUnicodeUtils(nsnull),
                                     mArraySize(8),
                                     mIndexMap(nsnull),
                                     mLevels(nsnull)
{
  nsresult rv;
  NS_WITH_SERVICE(nsIBidi, bidiEngine, kBidiCID, &rv);
  if (NS_SUCCEEDED(rv) && bidiEngine) {
    mBidiEngine = bidiEngine;
    mSuccess = rv;
  }
}

nsBidiPresUtils::~nsBidiPresUtils()
{
  if (mLevels) {
    delete[] mLevels;
  }
  if (mIndexMap) {
    delete[] mIndexMap;
  }
}

PRBool
nsBidiPresUtils::IsSuccessful() const
{ 
  return NS_SUCCEEDED(mSuccess); 
}

/* Some helper methods for Resolve() */

static nsresult
CreateBidiContinuation(nsIPresContext* aPresContext,
                       nsIContent*     aContent,
                       nsIFrame*       aFrame,
                       nsIFrame**      aNewFrame)
{
  NS_PRECONDITION(aNewFrame, "null OUT ptr");

  *aNewFrame = nsnull;

  NS_PRECONDITION(aFrame, "null ptr");

  nsIFrame* parent;

  nsCOMPtr<nsIPresShell>   presShell;
  aPresContext->GetShell(getter_AddRefs(presShell) );

  NS_NewContinuingTextFrame(presShell, aNewFrame);
  if (!(*aNewFrame) ) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCOMPtr<nsIStyleContext> styleContext;
  aFrame->GetStyleContext(getter_AddRefs(styleContext) );

  aFrame->GetParent(&parent);

  (*aNewFrame)->Init(aPresContext, aContent, parent, styleContext, nsnull);

  // XXX: TODO: Instead, create and insert entire frame list
  (*aNewFrame)->SetNextSibling(nsnull);

  // The list name nsLayoutAtoms::nextBidi would indicate we don't want reflow
  parent->InsertFrames(aPresContext, *presShell, nsLayoutAtoms::nextBidi,
                       aFrame, *aNewFrame);

  aFrame->SetBidiProperty(aPresContext, nsLayoutAtoms::nextBidi, *aNewFrame);

  return NS_OK;
}

static PRBool
RemoveBidiContinuation(nsIPresContext* aPresContext,
                       nsIFrame*       aFrame,
                       nsIFrame*       aNextFrame)
{
  nsIFrame* frame;
  PRBool    rv = PR_FALSE;

  if (aFrame && aNextFrame) {
    nsIFrame* prevInFlow;
    nsIFrame* parent;

    nsCOMPtr<nsIPresShell>   presShell;
    aPresContext->GetShell(getter_AddRefs(presShell) );

    nsCOMPtr<nsIFrameManager> frameManager;
    presShell->GetFrameManager(getter_AddRefs(frameManager) );
    if (frameManager) {
      aFrame->GetParent(&parent);
      for (frame = aNextFrame; frame; frame = aNextFrame) {
        // find the first next bidi frame, which is not next in flow for its prev bidi
        do {
          nsIFrame* nextFrame = aNextFrame;
          nextFrame->GetBidiProperty(aPresContext, nsLayoutAtoms::nextBidi,
                                     (void**) &aNextFrame);
          if (!aNextFrame) {
            break;
          }
          aNextFrame->GetPrevInFlow(&prevInFlow);
        } while (prevInFlow);

      // The list name nsLayoutAtoms::nextBidi would indicate we don't want reflow
        parent->RemoveFrame(aPresContext, *presShell, nsLayoutAtoms::nextBidi, frame);
        rv = PR_TRUE;
      } // for
      frame = aFrame;
      // Remove nextBidi property, associated with the current frame
      // and with all of its prev in flow
      do {
        frameManager->RemoveFrameProperty(frame, nsLayoutAtoms::nextBidi);
        frame->GetPrevInFlow(&frame);
      } while (frame);

    } // if (frameManager)
  } // if (frame && aNextFrame)
  return rv;
}

static void
AdjustEmbeddingLevel(nsIFrame* aFrame,
                     PRUint8&  aEmbeddingLevel)
{
  const nsStyleText* text;
  aFrame->GetStyleData(eStyleStruct_Text, (const nsStyleStruct*&) text);
  
  if (NS_STYLE_UNICODE_BIDI_OVERRIDE == text->mUnicodeBidi) {
    const nsStyleDisplay* display;

    aFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
          
    if (NS_STYLE_DIRECTION_RTL == display->mDirection) {
      // ensure embedding level is odd
      aEmbeddingLevel = (aEmbeddingLevel - 1) | 0x01;
    }
    else { // if (NS_STYLE_DIRECTION_LTR == display->mDirection)
      // ensure embedding level is even
      aEmbeddingLevel &= ~0x01;
    }
  }
}
/*************************************/

/**
 *  Make Bidi engine calculate embedding levels of the frames.
 *
 *  @param  pres context, first kid, base embedding level
 *
 *  @lina 06/18/2000
 */
nsresult
nsBidiPresUtils::Resolve(nsIPresContext* aPresContext,
                         nsIFrame*       aBlockFrame,
                         nsIFrame*       aFirstChild)
{
  mLogicalFrames.Clear();

  InitLogicalArray(aPresContext, aFirstChild, nsnull, PR_TRUE);

  CreateBlockBuffer(aPresContext);

  PRInt32 bufferLength = mBuffer.Length();

  if (bufferLength < 1) {
    mSuccess = NS_OK;
    return mSuccess;
  }
  const nsStyleDisplay* display;
  aBlockFrame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);

  PRInt32 runCount;
  PRUint8 embeddingLevel;

  UBidiLevel paraLevel = embeddingLevel =
    (NS_STYLE_DIRECTION_RTL == display->mDirection)
    ? UBIDI_RTL : UBIDI_LTR;

  mBidiEngine->setPara(mBuffer.GetUnicode(), bufferLength, paraLevel, nsnull);

  PRBool isVisual;
  aPresContext->IsVisualMode(isVisual);

  mSuccess = mBidiEngine->countRuns(&runCount);
  if (NS_FAILED(mSuccess) ) {
    return mSuccess;
  }
  PRInt32                  runLength = 0;
  PRInt32                  fragmentLength = 0;
  PRInt32                  contentOffset;
  PRInt32                  temp;
  PRInt32                  frameIndex = 0;
  PRInt32                  frameCount = mLogicalFrames.Count();
  PRInt32                  lineOffset = 0;
  PRInt32                  logicalLimit = 0;
  PRInt32                  numRun = 0;
  PRUint8                  textClass;
  PRUint8                  prevClass = U_LEFT_TO_RIGHT;
  PRBool                   hasContinuation;
  PRBool                   removedContinuation;
  PRBool                   isTextFrame;
  nsIFrame*                frame;
  nsIFrame*                nextBidi;
  nsIFrame*                nextInFlow;
  nsCOMPtr<nsIAtom>        frameType;
  nsCOMPtr<nsIContent>     content;
  nsCOMPtr<nsITextContent> textContent;
  const nsTextFragment*    fragment;

  while (frameIndex < frameCount) {
    if (fragmentLength <= 0) {
      contentOffset = 0;
      hasContinuation = PR_FALSE;
      removedContinuation = PR_FALSE;

      frame = (nsIFrame*) (mLogicalFrames[frameIndex]);

      frame->GetFrameType(getter_AddRefs(frameType) );
      if (frameType && nsLayoutAtoms::textFrame == frameType.get() ) {
        mSuccess = frame->GetContent(getter_AddRefs(content) ); 
        if (NS_FAILED(mSuccess) || (!content) ) {
          break;
        }
        textContent = do_QueryInterface(content, &mSuccess);
        if (NS_FAILED(mSuccess) || (!textContent) ) {
          break;
        }
        textContent->GetText(&fragment);
        if (!fragment) {
          mSuccess = NS_ERROR_FAILURE;
          break;
        }
        fragmentLength = fragment->GetLength();
        isTextFrame = PR_TRUE;
      } // if text frame
      else {
        isTextFrame = PR_FALSE;
        fragmentLength = 1;
      }
    } // if (fragmentLength <= 0)
    if ( (runLength <= 0) && (numRun < runCount) ) {
      lineOffset = logicalLimit;
      mSuccess = mBidiEngine->getLogicalRun(lineOffset, &logicalLimit, &embeddingLevel);
      if (isVisual || NS_FAILED(mSuccess) ) {
        embeddingLevel = paraLevel;
      }
      runLength = logicalLimit - lineOffset;
    } // if (runLength <= 0)

    if (nsLayoutAtoms::directionalFrame == frameType.get()) {
      delete frame;
    }
    else {
      AdjustEmbeddingLevel(frame, embeddingLevel);
      frame->SetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel,
                             (void *)embeddingLevel);
      frame->SetBidiProperty(aPresContext, nsLayoutAtoms::baseLevel,
                             (void *)paraLevel);
      if (isTextFrame) {
        PRInt32 limit = PR_MIN(logicalLimit, lineOffset + fragmentLength);
        CalculateTextClass(limit, lineOffset, textClass, prevClass);
        // IBMBIDI - Egypt - Start
        frame->SetBidiProperty(aPresContext,nsLayoutAtoms::textClass,(void *)textClass);
        // IBMBIDI - Egypt - End

        frame->GetBidiProperty(aPresContext, nsLayoutAtoms::nextBidi,
                               (void**) &nextBidi);

        if ( (runLength > 0) && (runLength < fragmentLength) ) {
          frame->GetNextInFlow(&nextInFlow);
          if (nextInFlow && nextInFlow != nextBidi) {
            nextBidi = nextInFlow;
            frame->SetBidiProperty(aPresContext, nsLayoutAtoms::nextBidi,
                                   (void*) nextBidi);
          }
          if (nextBidi) {
            ++frameIndex;
          }
          else {
            mSuccess = CreateBidiContinuation(aPresContext, content,
                                              frame, &nextBidi);
            if (NS_FAILED(mSuccess) ) {
              break;
            }
          }
          hasContinuation = PR_TRUE;
          NS_SetContentLengthAndOffsetForBidi(frame, contentOffset,
                                              contentOffset + runLength);
          frame = nextBidi;
          contentOffset += runLength;
        } // if (runLength < fragmentLength)
        else {
          if (hasContinuation) {
            NS_SetContentLengthAndOffsetForBidi(frame, contentOffset,
                                                contentOffset + fragmentLength);
          }
          else {
            if (runLength <= 0) {
              nextBidi = frame;
              frame = (nsIFrame*) mLogicalFrames[frameIndex - 1];
            }
            else if (nextBidi) {
              NS_SetContentLengthAndOffsetForBidi(frame, contentOffset,
                                                  contentOffset + fragmentLength);
            }
            removedContinuation = RemoveBidiContinuation(aPresContext, frame, nextBidi);
            if (removedContinuation) {
              if (runLength <= 0) {
                break;
              }
              frame->GetNextSibling(&frame);
              if (frame) {
                frame->FirstChild(aPresContext, nsnull, &frame);
              }
            }
          }
        }
      } // isTextFrame
    } // not directionalFrame
    temp = runLength;
    runLength -= fragmentLength;
    fragmentLength -= temp;
    if (fragmentLength <= 0) { // not only for text frames
      ++frameIndex;
      if (removedContinuation) {
        while ( (frameIndex < frameCount)
           && (frame != (nsIFrame*)mLogicalFrames[frameIndex]) ) {
          ++frameIndex;
        }
      }
    } // fragmentLength <= runLength
    if (runLength <= 0) {
      ++numRun;
    }
  } // while (frameIndex < frameCount)
  return mSuccess;
}

void
nsBidiPresUtils::InitLogicalArray(nsIPresContext* aPresContext,
                                  nsIFrame*       aCurrentFrame,
                                  nsIFrame*       aNextInFlow,
                                  PRBool          aAddMarkers)
{
  nsIFrame*             frame;
  nsIFrame*             directionalFrame;
  nsIFrame*             kid;
  nsIAtom*              frameType = nsnull;
  const nsStyleDisplay* display;
  nsresult              rv;

  for (frame = aCurrentFrame;
       frame && frame != aNextInFlow;
       frame->GetNextSibling(&frame) ) {

    rv = NS_ERROR_FAILURE;
    frame->GetStyleData(eStyleStruct_Display, (const nsStyleStruct*&) display);
    
    if ( (aAddMarkers) && (!display->IsBlockLevel() ) ) {
      if (NS_STYLE_DIRECTION_RTL == display->mExplicitDirection) {
        rv = NS_NewDirectionalFrame(&directionalFrame, kRLE);
      }
      else if (NS_STYLE_DIRECTION_LTR == display->mExplicitDirection) {
        rv = NS_NewDirectionalFrame(&directionalFrame, kLRE);
      }
      if (NS_SUCCEEDED(rv) ) {
        mLogicalFrames.AppendElement(directionalFrame);
      }
    } // if (aAddMarkers)

    frame->GetFrameType(&frameType);

    if ( (!display->IsBlockLevel() )
        && ( (nsLayoutAtoms::inlineFrame == frameType)
          || (nsLayoutAtoms::letterFrame == frameType)
          || (nsLayoutAtoms::blockFrame == frameType) ) ) {
      frame->FirstChild(aPresContext, nsnull, &kid);
      InitLogicalArray(aPresContext, kid, aNextInFlow, aAddMarkers);
    }
    else { // bidi leaf
      mLogicalFrames.AppendElement(frame);
    }
    NS_IF_RELEASE(frameType);

    // If the element is attributed by dir, indicate direction pop (add PDF frame)
    if (NS_SUCCEEDED(rv) ) {
      rv = NS_NewDirectionalFrame(&directionalFrame, kPDF);
      if (NS_SUCCEEDED(rv) ) {
        mLogicalFrames.AppendElement(directionalFrame);
      }
    }
  } // for
}

/**
 *  Create a string containing entire text content of this block.
 *
 *  @param   <code>nsIPresContext*</code>
 *
 *  @lina 05/02/2000
 */
void
nsBidiPresUtils::CreateBlockBuffer(nsIPresContext* aPresContext)
{
  mBuffer.SetLength(0);

  nsIFrame*                 frame;
  nsIContent*               prevContent = nsnull;
  nsCOMPtr<nsIContent>      content;
  nsCOMPtr<nsITextContent>  textContent;
  const nsTextFragment*     frag;
  PRUint32                  i;
  PRUint32                  count = mLogicalFrames.Count();

  for (i = 0; i < count; i++) {
    frame = (nsIFrame*) (mLogicalFrames[i]);

    nsIAtom* frameType = nsnull;
    frame->GetFrameType(&frameType);

    if (nsLayoutAtoms::textFrame == frameType) {
      mSuccess = frame->GetContent(getter_AddRefs(content) );
      if ( (NS_FAILED(mSuccess) ) || (!content) ) {
        break;
      }
      if (content.get() == prevContent) {
        continue;
      }
      prevContent = content;
      textContent = do_QueryInterface(content, &mSuccess);
      if ( (NS_FAILED(mSuccess) ) || (!textContent) ) {
        break;
      }
      textContent->GetText(&frag);
      if (!frag) {
        mSuccess = NS_ERROR_FAILURE;
        break;
      }
      frag->AppendTo(mBuffer);
    }
    else if (nsLayoutAtoms::brFrame == frameType) { // break frame
      // Append line separator
      mBuffer.Append( (PRUnichar) kLineSeparator);
    }
    else if (nsLayoutAtoms::directionalFrame == frameType) {
      mBuffer.Append(NS_GetControlCharacter(frame) );
    }
    else { // not text frame
      // See the Unicode Bidi Algorithm:
      // "...inline objects (such as graphics) are treated as if they are ... U+FFFC"
      mBuffer.Append( (PRUnichar) kObjectSubstitute);
    }
    NS_IF_RELEASE(frameType);
  }
  // XXX: TODO: Handle preformatted text ('\n')
  mBuffer.ReplaceChar("\t\r\n", kSpace);
}

/**
 * Reorder this line using Bidi engine.
 * Update frame array, following the new visual sequence.
 * 
 * @param   pres context, the first frame of the line,
 *          the first frame of the next line, amount of kids
 *
 * @lina 05/02/2000
 */

void
nsBidiPresUtils::ReorderFrames(nsIPresContext* aPresContext,
                               nsIFrame*       aFirstChild,
                               nsIFrame*       aNextInFlow,
                               PRInt32         aChildCount)
{
  mLogicalFrames.Clear();

  InitLogicalArray(aPresContext, aFirstChild, aNextInFlow);

  if (mLogicalFrames.Count() > 1) {
    PRBool bidiEnabled;
    // Set bidiEnabled to true if the line is reordered
    Reorder(aPresContext, bidiEnabled);
    if (bidiEnabled) {
      RepositionInlineFrames(aPresContext, aFirstChild, aChildCount);
    }
  }
}

nsresult
nsBidiPresUtils::Reorder(nsIPresContext* aPresContext,
                         PRBool&         aBidiEnabled)
{
  aBidiEnabled = PR_FALSE;
  PRInt32 count = mLogicalFrames.Count();

  if (mArraySize < count) {
    mArraySize = count << 1;
    if (mLevels) {
      delete[] mLevels;
      mLevels = nsnull;
    }
    if (mIndexMap) {
      delete[] mIndexMap;
      mIndexMap = nsnull;
    }
  }
  if (!mLevels) {
    mLevels = new PRUint8[mArraySize];
    if (!mLevels) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  nsCRT::zero(mLevels, sizeof(PRUint8) * mArraySize);

  nsIFrame* frame;
  PRInt32   i;
  void*     level;

  for (i = 0; i < count; i++) {
    frame = (nsIFrame*) (mLogicalFrames[i]);
    frame->GetBidiProperty(aPresContext, nsLayoutAtoms::embeddingLevel, &level);
    mLevels[i] = (PRUint8)level;
  }
  if (!mIndexMap) {
    mIndexMap = new PRInt32[mArraySize];
  }
  if (!mIndexMap) {
    mSuccess = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    nsCRT::zero(mIndexMap, sizeof(PRUint32) * mArraySize);

    mSuccess = mBidiEngine->reorderVisual(mLevels, count, mIndexMap);

    if (NS_SUCCEEDED(mSuccess) ) {
      mVisualFrames.Clear();

      for (i = 0; i < count; i++) {
        mVisualFrames.InsertElementAt(mLogicalFrames[mIndexMap[i]], i);
        if (i != mIndexMap[i]) {
          aBidiEnabled = PR_TRUE;
        }
      }
    } // NS_SUCCEEDED(mSuccess)
  } // indexMap

  if (NS_FAILED(mSuccess) ) {
    aBidiEnabled = PR_FALSE;
  }
  return mSuccess;
}

/**
 *  Adjust frame positions following their visual order
 *
 *  @param  <code>nsIPresContext*</code>, the first kid
 *
 *  @lina 04/11/2000
 */
void
nsBidiPresUtils::RepositionInlineFrames(nsIPresContext* aPresContext,
                                        nsIFrame*       aFirstChild,
                                        PRInt32         aChildCount) const
{
  PRInt32 count = mVisualFrames.Count();
  if (count < 2) {
    return;
  }
  nsIFrame* frame = (nsIFrame*) (mVisualFrames[0]);
  nsPoint origin;
  nsRect rect;
  nsRect nextRect;
  PRInt32 i;

  frame->GetRect(rect);

  if (frame != aFirstChild) {
    aFirstChild->GetOrigin(origin);
    rect.x = origin.x;
    frame->SetRect(aPresContext, rect);
  }

  for (i = 1; i < count; i++) {
    frame = (nsIFrame*) (mVisualFrames[i]);
    frame->GetRect(nextRect);
    nextRect.x = rect.x + rect.width;
    frame->SetRect(aPresContext, nextRect);
    rect = nextRect;
  } // for
  // Now adjust inline container frames.
  // Example: LTR paragraph 
  //                <p><b>english HEBREW</b> 123</p>
  // should be displayed as
  //                <p><b>english </b>123 <b>WERBEH</b></p>

  // We assume that <b></b> rectangle takes all the room from "english" left edge to
  // "WERBEH" right edge.

  nsCOMPtr<nsIAtom> frameType;

  frame = aFirstChild;
  for (i = 0; i < aChildCount; i++) {
    frame->GetFrameType(getter_AddRefs(frameType) );
    if ( frameType
        && ( (nsLayoutAtoms::inlineFrame == frameType.get() )
          || (nsLayoutAtoms::letterFrame == frameType.get() )
          || (nsLayoutAtoms::blockFrame == frameType.get() ) ) ) {
      PRInt32 minX = 0x7FFFFFFF;
      PRInt32 maxX = 0;
      RepositionContainerFrame(aPresContext, frame, minX, maxX);
    }
    frame->GetNextSibling(&frame);
  } // for
}

void
nsBidiPresUtils::RepositionContainerFrame(nsIPresContext* aPresContext,
                                          nsIFrame* aContainer,
                                          PRInt32& aMinX,
                                          PRInt32& aMaxX) const
{
  nsIFrame* frame;
  nsIFrame* firstChild;
  nsCOMPtr<nsIAtom> frameType;
  nsRect rect;
  PRInt32 minX = 0x7FFFFFFF;
  PRInt32 maxX = 0;

  aContainer->FirstChild(aPresContext, nsnull, &firstChild);

  for (frame = firstChild; frame; frame->GetNextSibling(&frame) ) {
    frame->GetFrameType(getter_AddRefs(frameType) );
    if ( (frameType)
      && ( (nsLayoutAtoms::inlineFrame == frameType.get() )
        || (nsLayoutAtoms::letterFrame == frameType.get() )
        || (nsLayoutAtoms::blockFrame == frameType.get() ) ) ) {
      RepositionContainerFrame(aPresContext, frame, minX, maxX);
    }
    else {
      frame->GetRect(rect);
      minX = PR_MIN(minX, rect.x);
      maxX = PR_MAX(maxX, rect.x + rect.width);
    }
  }

  aMinX = PR_MIN(minX, aMinX);
  aMaxX = PR_MAX(maxX, aMaxX);

  if (minX < maxX) {
    aContainer->GetRect(rect);
    rect.x = minX;
    rect.width = maxX - minX;
    aContainer->SetRect(aPresContext, rect);
  }

  // Now adjust all the kids (kid's coordinates are relative to the parent's)
  nsPoint origin;

  for (frame = firstChild; frame; frame->GetNextSibling(&frame) ) {
    frame->GetOrigin(origin);
    frame->MoveTo(aPresContext, origin.x - minX, origin.y);
  }
}

nsresult
nsBidiPresUtils::FormatUnicodeText(nsIPresContext* aPresContext,
                                   PRUnichar*      aText,
                                   PRInt32&        aTextLength,
                                   UCharDirection  aTextClass,
                                   PRBool          aIsOddLevel,
                                   PRBool          aIsBidiSystem)
{
  nsresult rv;
  if (!mUnicodeUtils) {
    NS_WITH_SERVICE(nsIUBidiUtils, bidiUtils, kUBidiUtilCID, &rv);
    if (NS_FAILED(rv) ) {
      return rv;
    }
    mUnicodeUtils = bidiUtils;
  }
// ahmed 
      //adjusted for correct numeral shaping	
    nsBidiOptions mBidioptions;
  	aPresContext->GetBidi(&mBidioptions);
		if (IBMBIDI_NUMERAL_HINDI == mBidioptions.mnumeral)
			mUnicodeUtils->HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_HINDI);
  	else if (IBMBIDI_NUMERAL_ARABIC == mBidioptions.mnumeral)
   	  mUnicodeUtils->HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
  	else if ( (IBMBIDI_NUMERAL_REGULAR == mBidioptions.mnumeral) || (IBMBIDI_NUMERAL_HINDICONTEXT == mBidioptions.mnumeral) ) {
	  	if (U_EUROPEAN_NUMBER == aTextClass)
				mUnicodeUtils->HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_ARABIC);
		 	else if (U_ARABIC_NUMBER == aTextClass)
				mUnicodeUtils->HandleNumbers(aText,aTextLength,IBMBIDI_NUMERAL_HINDI);
     }
	
  PRInt32 newLen;
  PRUnichar* buffer = (PRUnichar*) mBuffer.GetUnicode();
  // buffer can't be shorter than aText, since it was created from entire block

  PRBool doReverse = PR_FALSE;

  if (aIsBidiSystem) {
    if (CLASS_IS_RTL(aTextClass) ^ aIsOddLevel) {
      doReverse = PR_TRUE;
    }
  }
  else if (aIsOddLevel) {
    doReverse = PR_TRUE;
  }
  if (doReverse) {
    rv = mBidiEngine->writeReverse(aText, aTextLength, buffer,
                                   UBIDI_REMOVE_BIDI_CONTROLS | UBIDI_DO_MIRRORING,
                                   &newLen);
  }
  else {
    rv = NS_ERROR_FAILURE;
    // XXX: todo
#if 0
    // Still need to remove bidi controls
    rv = mBidiEngine->writeForward(aText, aTextLength, buffer,
                                   UBIDI_REMOVE_BIDI_CONTROLS, &newLen);
#endif
  }
  if (NS_SUCCEEDED(rv) ) {
    aTextLength = newLen;
    for (PRInt32 i = 0; i < aTextLength; i++) {
      aText[i] = buffer[i];
    }
  }
  return rv;
}

void
nsBidiPresUtils::CalculateTextClass(PRInt32  aLimit,
                                    PRInt32& aOffset,
                                    PRUint8& aTextClass,
                                    PRUint8& aPrevTextClass) const
{
  for (; ;) {
    // IBMBIDI - Egypt - Start
    mBidiEngine->getClassAt(aOffset, &aTextClass);
    // IBMBIDI - Egypt - End
    if ( (aOffset >= aLimit) || (!CLASS_IS_WEAK(aTextClass) ) ) {
      break;
    }
    ++aOffset;
  }

  if ( (U_EUROPEAN_NUMBER == aTextClass)
    && ( (U_RIGHT_TO_LEFT_ARABIC == aPrevTextClass) 
      || (U_ARABIC_NUMBER == aPrevTextClass) ) ) {
    aTextClass = U_ARABIC_NUMBER;
  }
  if (!CLASS_IS_WEAK(aTextClass) ) {
    aPrevTextClass = aTextClass;
  }
}
#endif // IBMBIDI
