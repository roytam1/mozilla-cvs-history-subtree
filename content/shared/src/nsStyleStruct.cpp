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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   David Hyatt (hyatt@netscape.com
 * 
 */

#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIPresContext.h"
#include "nsIStyleRule.h"
#include "nsISupportsArray.h"
#include "nsCRT.h"

#include "nsCOMPtr.h"
#include "nsIStyleSet.h"
#include "nsISizeOfHandler.h"
#include "nsIPresShell.h"
#include "nsLayoutAtoms.h"
#include "prenv.h"

inline PRBool IsFixedUnit(nsStyleUnit aUnit, PRBool aEnumOK)
{
  return PRBool((aUnit == eStyleUnit_Null) || 
                (aUnit == eStyleUnit_Coord) || 
                (aEnumOK && (aUnit == eStyleUnit_Enumerated)));
}

// XXX this is here to support deprecated calc spacing methods only
inline nscoord CalcSideFor(const nsIFrame* aFrame, const nsStyleCoord& aCoord, 
                           PRUint8 aSpacing, PRUint8 aSide,
                           const nscoord* aEnumTable, PRInt32 aNumEnums)
{
  nscoord result = 0;

  switch (aCoord.GetUnit()) {
    case eStyleUnit_Auto:
      // Auto margins are handled by layout
      break;

    case eStyleUnit_Inherit:
      nsIFrame* parentFrame;
      aFrame->GetParent(&parentFrame);  // XXX may not be direct parent...
      if (nsnull != parentFrame) {
        nsIStyleContext* parentContext;
        parentFrame->GetStyleContext(&parentContext);
        if (nsnull != parentContext) {
          nsMargin  parentSpacing;
          switch (aSpacing) {
            case NS_SPACING_MARGIN:
              {
                const nsStyleMargin* parentMargin = (const nsStyleMargin*)parentContext->GetStyleData(eStyleStruct_Margin);
                parentMargin->CalcMarginFor(parentFrame, parentSpacing);  
              }

              break;
            case NS_SPACING_PADDING:
              {
                const nsStylePadding* parentPadding = (const nsStylePadding*)parentContext->GetStyleData(eStyleStruct_Padding);
                parentPadding->CalcPaddingFor(parentFrame, parentSpacing);  
              }

              break;
            case NS_SPACING_BORDER:
              {
                const nsStyleBorder* parentBorder = (const nsStyleBorder*)parentContext->GetStyleData(eStyleStruct_Border);
                parentBorder->CalcBorderFor(parentFrame, parentSpacing);  
              }

              break;
          }
          switch (aSide) {
            case NS_SIDE_LEFT:    result = parentSpacing.left;   break;
            case NS_SIDE_TOP:     result = parentSpacing.top;    break;
            case NS_SIDE_RIGHT:   result = parentSpacing.right;  break;
            case NS_SIDE_BOTTOM:  result = parentSpacing.bottom; break;
          }
          NS_RELEASE(parentContext);
        }
      }
      break;

    case eStyleUnit_Percent:
      {
        nscoord baseWidth = 0;
        PRBool  isBase = PR_FALSE;
        nsIFrame* frame;
        aFrame->GetParent(&frame);
        while (nsnull != frame) {
          frame->IsPercentageBase(isBase);
          if (isBase) {
            nsSize  size;
            frame->GetSize(size);
            baseWidth = size.width; // not really width, need to subtract out padding...
            break;
          }
          frame->GetParent(&frame);
        }
        result = (nscoord)((float)baseWidth * aCoord.GetPercentValue());
      }
      break;

    case eStyleUnit_Coord:
      result = aCoord.GetCoordValue();
      break;

    case eStyleUnit_Enumerated:
      if (nsnull != aEnumTable) {
        PRInt32 value = aCoord.GetIntValue();
        if ((0 <= value) && (value < aNumEnums)) {
          return aEnumTable[aCoord.GetIntValue()];
        }
      }
      break;

    case eStyleUnit_Null:
    case eStyleUnit_Normal:
    case eStyleUnit_Integer:
    case eStyleUnit_Proportional:
    default:
      result = 0;
      break;
  }
  if ((NS_SPACING_PADDING == aSpacing) || (NS_SPACING_BORDER == aSpacing)) {
    if (result < 0) {
      result = 0;
    }
  }
  return result;
}

inline void CalcSidesFor(const nsIFrame* aFrame, const nsStyleSides& aSides, 
                         PRUint8 aSpacing, 
                         const nscoord* aEnumTable, PRInt32 aNumEnums,
                         nsMargin& aResult)
{
  nsStyleCoord  coord;

  aResult.left = CalcSideFor(aFrame, aSides.GetLeft(coord), aSpacing, NS_SIDE_LEFT,
                             aEnumTable, aNumEnums);
  aResult.top = CalcSideFor(aFrame, aSides.GetTop(coord), aSpacing, NS_SIDE_TOP,
                            aEnumTable, aNumEnums);
  aResult.right = CalcSideFor(aFrame, aSides.GetRight(coord), aSpacing, NS_SIDE_RIGHT,
                              aEnumTable, aNumEnums);
  aResult.bottom = CalcSideFor(aFrame, aSides.GetBottom(coord), aSpacing, NS_SIDE_BOTTOM,
                               aEnumTable, aNumEnums);
}

// --------------------
// nsStyleFont
//
nsStyleFont::nsStyleFont()
  : mFont(nsnull, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
            NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE, 0),
    mFixedFont(nsnull, NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                 NS_FONT_WEIGHT_NORMAL, NS_FONT_DECORATION_NONE, 0),
    mFlags(NS_STYLE_FONT_DEFAULT)
{ }

nsStyleFont::nsStyleFont(const nsFont& aVariableFont, const nsFont& aFixedFont)
  : mFont(aVariableFont),
    mFixedFont(aFixedFont),
    mFlags(NS_STYLE_FONT_DEFAULT)
{ }

nsStyleFont::nsStyleFont(const nsStyleFont& aSrc)
:mFont(aSrc.mFont), mFixedFont(aSrc.mFixedFont)
{
  mFlags = aSrc.mFlags;
}

void* 
nsStyleFont::operator new(size_t sz, nsIPresContext* aContext) {
  void* result = nsnull;
  aContext->AllocateFromShell(sz, &result);
  if (result)
  nsCRT::zero(result, sz);
  return result;
}
  
void 
nsStyleFont::Destroy(nsIPresContext* aContext) {
  aContext->FreeToShell(sizeof(nsStyleFont), this);
}

PRInt32 nsStyleFont::CalcDifference(const nsStyleFont& aOther) const
{
  if (mFlags == aOther.mFlags) {
    PRInt32 impact = CalcFontDifference(mFont, aOther.mFont);
    if (impact < NS_STYLE_HINT_REFLOW) {
      impact = CalcFontDifference(mFixedFont, aOther.mFixedFont);
    } 
    return impact;
  }
  return NS_STYLE_HINT_REFLOW;
}

PRInt32 nsStyleFont::CalcFontDifference(const nsFont& aFont1, const nsFont& aFont2)
{
  if ((aFont1.size == aFont2.size) && 
      (aFont1.style == aFont2.style) &&
      (aFont1.variant == aFont2.variant) &&
      (aFont1.weight == aFont2.weight) &&
      (aFont1.name == aFont2.name)) {
    if ((aFont1.decorations == aFont2.decorations)) {
      return NS_STYLE_HINT_NONE;
    }
    return NS_STYLE_HINT_VISUAL;
  }
  return NS_STYLE_HINT_REFLOW;
}

static PRBool IsFixedData(const nsStyleSides& aSides, PRBool aEnumOK)
{
  return PRBool(IsFixedUnit(aSides.GetLeftUnit(), aEnumOK) &&
                IsFixedUnit(aSides.GetTopUnit(), aEnumOK) &&
                IsFixedUnit(aSides.GetRightUnit(), aEnumOK) &&
                IsFixedUnit(aSides.GetBottomUnit(), aEnumOK));
}

static nscoord CalcCoord(const nsStyleCoord& aCoord, 
                         const nscoord* aEnumTable, 
                         PRInt32 aNumEnums)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Null:
      return 0;
    case eStyleUnit_Coord:
      return aCoord.GetCoordValue();
    case eStyleUnit_Enumerated:
      if (nsnull != aEnumTable) {
        PRInt32 value = aCoord.GetIntValue();
        if ((0 <= value) && (value < aNumEnums)) {
          return aEnumTable[aCoord.GetIntValue()];
        }
      }
      break;
    default:
      NS_ERROR("bad unit type");
      break;
  }
  return 0;
}

nsStyleMargin::nsStyleMargin() {
  mMargin.Reset();
  mHasCachedMargin = PR_FALSE;
}

nsStyleMargin::nsStyleMargin(const nsStyleMargin& aSrc) {
  mMargin = aSrc.mMargin;
  mHasCachedMargin = PR_FALSE;
}

void* 
nsStyleMargin::operator new(size_t sz, nsIPresContext* aContext) {
  void* result = nsnull;
  aContext->AllocateFromShell(sz, &result);
  if (result)
  nsCRT::zero(result, sz);
  return result;
}
  
void 
nsStyleMargin::Destroy(nsIPresContext* aContext) {
  aContext->FreeToShell(sizeof(nsStyleMargin), this);
}


void nsStyleMargin::RecalcData()
{
  if (IsFixedData(mMargin, PR_FALSE)) {
    nsStyleCoord  coord;
    mCachedMargin.left = CalcCoord(mMargin.GetLeft(coord), nsnull, 0);
    mCachedMargin.top = CalcCoord(mMargin.GetTop(coord), nsnull, 0);
    mCachedMargin.right = CalcCoord(mMargin.GetRight(coord), nsnull, 0);
    mCachedMargin.bottom = CalcCoord(mMargin.GetBottom(coord), nsnull, 0);

    mHasCachedMargin = PR_TRUE;
  }
  else
    mHasCachedMargin = PR_FALSE;
}

PRInt32 nsStyleMargin::CalcDifference(const nsStyleMargin& aOther) const
{
  if (mMargin == aOther.mMargin) {
    return NS_STYLE_HINT_NONE;
  }
  return NS_STYLE_HINT_REFLOW;
}

void 
nsStyleMargin::CalcMarginFor(const nsIFrame* aFrame, nsMargin& aMargin) const
{
  if (mHasCachedMargin) {
    aMargin = mCachedMargin;
  } else {
    CalcSidesFor(aFrame, mMargin, NS_SPACING_MARGIN, nsnull, 0, aMargin);
  }
}

nsStylePadding::nsStylePadding() {
  mPadding.Reset();
  mHasCachedPadding = PR_FALSE;
}

nsStylePadding::nsStylePadding(const nsStylePadding& aSrc) {
  mPadding = aSrc.mPadding;
  mHasCachedPadding = PR_FALSE;
}

void* 
nsStylePadding::operator new(size_t sz, nsIPresContext* aContext) {
  void* result = nsnull;
  aContext->AllocateFromShell(sz, &result);
  if (result)
  nsCRT::zero(result, sz);
  return result;
}
  
void 
nsStylePadding::Destroy(nsIPresContext* aContext) {
  aContext->FreeToShell(sizeof(nsStylePadding), this);
}

void nsStylePadding::RecalcData()
{
  if (IsFixedData(mPadding, PR_FALSE)) {
    nsStyleCoord  coord;
    mCachedPadding.left = CalcCoord(mPadding.GetLeft(coord), nsnull, 0);
    mCachedPadding.top = CalcCoord(mPadding.GetTop(coord), nsnull, 0);
    mCachedPadding.right = CalcCoord(mPadding.GetRight(coord), nsnull, 0);
    mCachedPadding.bottom = CalcCoord(mPadding.GetBottom(coord), nsnull, 0);

    mHasCachedPadding = PR_TRUE;
  }
  else
    mHasCachedPadding = PR_FALSE;
}

PRInt32 nsStylePadding::CalcDifference(const nsStylePadding& aOther) const
{
  if (mPadding == aOther.mPadding) {
    return NS_STYLE_HINT_NONE;
  }
  return NS_STYLE_HINT_REFLOW;
}

void 
nsStylePadding::CalcPaddingFor(const nsIFrame* aFrame, nsMargin& aPadding) const
{
  if (mHasCachedPadding) {
    aPadding = mCachedPadding;
  } else {
    CalcSidesFor(aFrame, mPadding, NS_SPACING_PADDING, nsnull, 0, aPadding);
  }
}

nsStyleBorder::nsStyleBorder(nsIPresContext* aPresContext)
{
  // XXX support mBorderWidths until deprecated methods are removed
  float pixelsToTwips = 20.0f;
  if (aPresContext) {
    aPresContext->GetPixelsToTwips(&pixelsToTwips);
  }
  mBorderWidths[NS_STYLE_BORDER_WIDTH_THIN] = NSIntPixelsToTwips(1, pixelsToTwips);
  mBorderWidths[NS_STYLE_BORDER_WIDTH_MEDIUM] = NSIntPixelsToTwips(3, pixelsToTwips);
  mBorderWidths[NS_STYLE_BORDER_WIDTH_THICK] = NSIntPixelsToTwips(5, pixelsToTwips);
   
  // spacing values not inherited
  nsStyleCoord  medium(NS_STYLE_BORDER_WIDTH_MEDIUM, eStyleUnit_Enumerated);
  mBorder.SetLeft(medium);
  mBorder.SetTop(medium);
  mBorder.SetRight(medium);
  mBorder.SetBottom(medium);
  
  mBorderStyle[0] = NS_STYLE_BORDER_STYLE_NONE;  
  mBorderStyle[1] = NS_STYLE_BORDER_STYLE_NONE; 
  mBorderStyle[2] = NS_STYLE_BORDER_STYLE_NONE; 
  mBorderStyle[3] = NS_STYLE_BORDER_STYLE_NONE;  

  mBorderColor[0] = NS_RGB(0, 0, 0);  
  mBorderColor[1] = NS_RGB(0, 0, 0);  
  mBorderColor[2] = NS_RGB(0, 0, 0);  
  mBorderColor[3] = NS_RGB(0, 0, 0); 

  mBorderRadius.Reset();

  mFloatEdge = NS_STYLE_FLOAT_EDGE_CONTENT;
  
  mHasCachedBorder = PR_FALSE;
}

nsStyleBorder::nsStyleBorder(const nsStyleBorder& aSrc)
{
  nsCRT::memcpy((nsStyleBorder*)this, &aSrc, sizeof(nsStyleBorder));
  mHasCachedBorder = PR_FALSE;
}

void* 
nsStyleBorder::operator new(size_t sz, nsIPresContext* aContext) {
  void* result = nsnull;
  aContext->AllocateFromShell(sz, &result);
  if (result)
  nsCRT::zero(result, sz);
  return result;
}
  
void 
nsStyleBorder::Destroy(nsIPresContext* aContext) {
  aContext->FreeToShell(sizeof(nsStyleBorder), this);
}


PRBool nsStyleBorder::IsBorderSideVisible(PRUint8 aSide) const
{
	PRUint8 borderStyle = GetBorderStyle(aSide);
	return ((borderStyle != NS_STYLE_BORDER_STYLE_NONE)
       && (borderStyle != NS_STYLE_BORDER_STYLE_HIDDEN));
}

void nsStyleBorder::RecalcData()
{
  if (((!IsBorderSideVisible(NS_SIDE_LEFT))|| 
       IsFixedUnit(mBorder.GetLeftUnit(), PR_TRUE)) &&
      ((!IsBorderSideVisible(NS_SIDE_TOP)) || 
       IsFixedUnit(mBorder.GetTopUnit(), PR_TRUE)) &&
      ((!IsBorderSideVisible(NS_SIDE_RIGHT)) || 
       IsFixedUnit(mBorder.GetRightUnit(), PR_TRUE)) &&
      ((!IsBorderSideVisible(NS_SIDE_BOTTOM)) || 
       IsFixedUnit(mBorder.GetBottomUnit(), PR_TRUE))) {
    nsStyleCoord  coord;
    if (!IsBorderSideVisible(NS_SIDE_LEFT)) {
      mCachedBorder.left = 0;
    }
    else {
      mCachedBorder.left = CalcCoord(mBorder.GetLeft(coord), mBorderWidths, 3);
    }
    if (!IsBorderSideVisible(NS_SIDE_TOP)) {
      mCachedBorder.top = 0;
    }
    else {
      mCachedBorder.top = CalcCoord(mBorder.GetTop(coord), mBorderWidths, 3);
    }
    if (!IsBorderSideVisible(NS_SIDE_RIGHT)) {
      mCachedBorder.right = 0;
    }
    else {
      mCachedBorder.right = CalcCoord(mBorder.GetRight(coord), mBorderWidths, 3);
    }
    if (!IsBorderSideVisible(NS_SIDE_BOTTOM)) {
      mCachedBorder.bottom = 0;
    }
    else {
      mCachedBorder.bottom = CalcCoord(mBorder.GetBottom(coord), mBorderWidths, 3);
    }
    mHasCachedBorder = PR_TRUE;
  }
  else {
    mHasCachedBorder = PR_FALSE;
  }

  if ((mBorderStyle[NS_SIDE_TOP] & BORDER_COLOR_DEFINED) == 0) {
    mBorderStyle[NS_SIDE_TOP] = BORDER_COLOR_DEFINED | BORDER_COLOR_FOREGROUND;
  }
  if ((mBorderStyle[NS_SIDE_BOTTOM] & BORDER_COLOR_DEFINED) == 0) {
    mBorderStyle[NS_SIDE_BOTTOM] = BORDER_COLOR_DEFINED | BORDER_COLOR_FOREGROUND;
  }
  if ((mBorderStyle[NS_SIDE_LEFT]& BORDER_COLOR_DEFINED) == 0) {
    mBorderStyle[NS_SIDE_LEFT] = BORDER_COLOR_DEFINED | BORDER_COLOR_FOREGROUND;
  }
  if ((mBorderStyle[NS_SIDE_RIGHT] & BORDER_COLOR_DEFINED) == 0) {
    mBorderStyle[NS_SIDE_RIGHT] = BORDER_COLOR_DEFINED | BORDER_COLOR_FOREGROUND;
  }
}

PRInt32 nsStyleBorder::CalcDifference(const nsStyleBorder& aOther) const
{
  if ((mBorder == aOther.mBorder) && 
      (mFloatEdge == aOther.mFloatEdge)) {
    PRInt32 ix;
    for (ix = 0; ix < 4; ix++) {
      if ((mBorderStyle[ix] != aOther.mBorderStyle[ix]) || 
          (mBorderColor[ix] != aOther.mBorderColor[ix])) {
        if ((mBorderStyle[ix] != aOther.mBorderStyle[ix]) &&
            ((NS_STYLE_BORDER_STYLE_NONE == mBorderStyle[ix]) ||
             (NS_STYLE_BORDER_STYLE_NONE == aOther.mBorderStyle[ix]) ||
             (NS_STYLE_BORDER_STYLE_HIDDEN == mBorderStyle[ix]) ||          // bug 45754
             (NS_STYLE_BORDER_STYLE_HIDDEN == aOther.mBorderStyle[ix]))) {
          return NS_STYLE_HINT_REFLOW;  // border on or off
        }
        return NS_STYLE_HINT_VISUAL;
      }
    }
    if (mBorderRadius != aOther.mBorderRadius) {
      return NS_STYLE_HINT_VISUAL;
    }
    return NS_STYLE_HINT_NONE;
  }
  return NS_STYLE_HINT_REFLOW;
}

void 
nsStyleBorder::CalcBorderFor(const nsIFrame* aFrame, nsMargin& aBorder) const
{
  if (mHasCachedBorder) {
    aBorder = mCachedBorder;
  } else {
    CalcSidesFor(aFrame, mBorder, NS_SPACING_BORDER, mBorderWidths, 3, aBorder);
  }
}

nsStyleOutline::nsStyleOutline(nsIPresContext* aPresContext)
{
  // XXX support mBorderWidths until deprecated methods are removed
  float pixelsToTwips = 20.0f;
  if (aPresContext)
    aPresContext->GetPixelsToTwips(&pixelsToTwips);
  mBorderWidths[NS_STYLE_BORDER_WIDTH_THIN] = NSIntPixelsToTwips(1, pixelsToTwips);
  mBorderWidths[NS_STYLE_BORDER_WIDTH_MEDIUM] = NSIntPixelsToTwips(3, pixelsToTwips);
  mBorderWidths[NS_STYLE_BORDER_WIDTH_THICK] = NSIntPixelsToTwips(5, pixelsToTwips);
 
  // spacing values not inherited
  mOutlineRadius.Reset();

  nsStyleCoord  medium(NS_STYLE_BORDER_WIDTH_MEDIUM, eStyleUnit_Enumerated);
  mOutlineWidth = medium;
  mOutlineStyle = NS_STYLE_BORDER_STYLE_NONE;
  mOutlineColor = NS_RGB(0, 0, 0);

  mHasCachedOutline = PR_FALSE;
}

nsStyleOutline::nsStyleOutline(const nsStyleOutline& aSrc) {
  nsCRT::memcpy((nsStyleOutline*)this, &aSrc, sizeof(nsStyleOutline));
}

void 
nsStyleOutline::RecalcData(void)
{
  if ((NS_STYLE_BORDER_STYLE_NONE == GetOutlineStyle()) || 
     IsFixedUnit(mOutlineWidth.GetUnit(), PR_TRUE)) {
    if (NS_STYLE_BORDER_STYLE_NONE == GetOutlineStyle())
      mCachedOutlineWidth = 0;
    else
      mCachedOutlineWidth = CalcCoord(mOutlineWidth, mBorderWidths, 3);
    mHasCachedOutline = PR_TRUE;
  }
  else
    mHasCachedOutline = PR_FALSE;
}

PRInt32 
nsStyleOutline::CalcDifference(const nsStyleOutline& aOther) const
{
  if ((mOutlineWidth != aOther.mOutlineWidth) ||
      (mOutlineStyle != aOther.mOutlineStyle) ||
      (mOutlineColor != aOther.mOutlineColor) ||
      (mOutlineRadius != aOther.mOutlineRadius)) {
    return NS_STYLE_HINT_VISUAL;	// XXX: should be VISUAL: see bugs 9809 and 9816
  }
  return NS_STYLE_HINT_NONE;
}

// --------------------
// nsStyleList
//
nsStyleList::nsStyleList() 
{
  mListStyleType = NS_STYLE_LIST_STYLE_BASIC;
  mListStylePosition = NS_STYLE_LIST_STYLE_POSITION_OUTSIDE;
  mListStyleImage.Truncate();  
}

nsStyleList::~nsStyleList() 
{
}

nsStyleList::nsStyleList(const nsStyleList& aSource)
{
  mListStyleType = aSource.mListStyleType;
  mListStylePosition = aSource.mListStylePosition;
  mListStyleImage = aSource.mListStyleImage;
}

PRInt32 nsStyleList::CalcDifference(const nsStyleList& aOther) const
{
  if (mListStylePosition == aOther.mListStylePosition)
    if (mListStyleImage == aOther.mListStyleImage)
      if (mListStyleType == aOther.mListStyleType)
        return NS_STYLE_HINT_NONE;
      return NS_STYLE_HINT_REFLOW;
    return NS_STYLE_HINT_REFLOW;
  return NS_STYLE_HINT_REFLOW;
}

#ifdef INCLUDE_XUL
// --------------------
// nsStyleXUL
//
nsStyleXUL::nsStyleXUL() 
{ 
  mBoxOrient = NS_STYLE_BOX_ORIENT_HORIZONTAL;
}

nsStyleXUL::~nsStyleXUL() 
{
}

nsStyleXUL::nsStyleXUL(const nsStyleXUL& aSource)
{
  nsCRT::memcpy((nsStyleXUL*)this, &aSource, sizeof(nsStyleXUL));
}

PRInt32 
nsStyleXUL::CalcDifference(const nsStyleXUL& aOther) const
{
  if (mBoxOrient == aOther.mBoxOrient)
    return NS_STYLE_HINT_NONE;
  return NS_STYLE_HINT_REFLOW;
}

#endif // INCLUDE_XUL

// --------------------
// nsStylePosition
//
nsStylePosition::nsStylePosition(void) 
{ 
  // positioning values not inherited
  mPosition = NS_STYLE_POSITION_NORMAL;
  nsStyleCoord  autoCoord(eStyleUnit_Auto);
  mOffset.SetLeft(autoCoord);
  mOffset.SetTop(autoCoord);
  mOffset.SetRight(autoCoord);
  mOffset.SetBottom(autoCoord);
  mWidth.SetAutoValue();
  mMinWidth.SetCoordValue(0);
  mMaxWidth.Reset();
  mHeight.SetAutoValue();
  mMinHeight.SetCoordValue(0);
  mMaxHeight.Reset();
  mBoxSizing = NS_STYLE_BOX_SIZING_CONTENT;
  mZIndex.SetAutoValue();
}

nsStylePosition::~nsStylePosition(void) 
{ 
}

nsStylePosition::nsStylePosition(const nsStylePosition& aSource)
{
  nsCRT::memcpy((nsStylePosition*)this, &aSource, sizeof(nsStylePosition));
}

PRInt32 nsStylePosition::CalcDifference(const nsStylePosition& aOther) const
{
  if (mPosition == aOther.mPosition) {
    if ((mOffset == aOther.mOffset) &&
        (mWidth == aOther.mWidth) &&
        (mMinWidth == aOther.mMinWidth) &&
        (mMaxWidth == aOther.mMaxWidth) &&
        (mHeight == aOther.mHeight) &&
        (mMinHeight == aOther.mMinHeight) &&
        (mMaxHeight == aOther.mMaxHeight) &&
        (mBoxSizing == aOther.mBoxSizing) &&
        (mZIndex == aOther.mZIndex)) {
      return NS_STYLE_HINT_NONE;
    }
    return NS_STYLE_HINT_REFLOW;
  }
  return NS_STYLE_HINT_FRAMECHANGE;
}
