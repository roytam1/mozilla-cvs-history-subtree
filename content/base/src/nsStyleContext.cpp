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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   IBM Corporation 
 * 
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 03/20/2000   IBM Corp.       Bidi - ability to change the default direction of the browser
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

#include "nsRuleNode.h"

#ifdef IBMBIDI
#include "nsIUBidiUtils.h"
#endif

#ifdef DEBUG
// #define NOISY_DEBUG
#endif

// --------------------------------------
// Macros for getting style data structs
// - if using external data, get from
//   the member style data instance
// - if internal, get the data member
#define GETSCDATA(data) m##data

// EnsureBlockDisplay:
//  - if the display value (argument) is not a block-type
//    then we set it to a valid block display value
//  - For enforcing the floated/positioned element CSS2 rules
static void EnsureBlockDisplay(/*in out*/PRUint8 &display);


// --------------------
// nsStyleColor
//

struct StyleColorImpl: public nsStyleColor {
  StyleColorImpl(void)  { }

  void ResetFrom(const nsStyleColor* aParent, nsIPresContext* aPresContext);
  void SetFrom(const nsStyleColor& aSource);
  void CopyTo(nsStyleColor& aDest) const;
  PRInt32 CalcDifference(const StyleColorImpl& aOther) const;
  
private:  // These are not allowed
  StyleColorImpl(const StyleColorImpl& aOther);
  StyleColorImpl& operator=(const StyleColorImpl& aOther);
};

void StyleColorImpl::ResetFrom(const nsStyleColor* aParent, nsIPresContext* aPresContext)
{
  if (nsnull != aParent) {
    mColor = aParent->mColor;
    mOpacity = aParent->mOpacity;
    mCursor = aParent->mCursor; // fix for bugzilla bug 51113
  }
  else {
    if (nsnull != aPresContext) {
      aPresContext->GetDefaultColor(&mColor);
    }
    else {
      mColor = NS_RGB(0x00, 0x00, 0x00);
    }
    mOpacity = 1.0f;
    mCursor = NS_STYLE_CURSOR_AUTO; // fix for bugzilla bug 51113
  }

  mBackgroundFlags = NS_STYLE_BG_COLOR_TRANSPARENT | NS_STYLE_BG_IMAGE_NONE;
  if (nsnull != aPresContext) {
    aPresContext->GetDefaultBackgroundColor(&mBackgroundColor);
    aPresContext->GetDefaultBackgroundImageAttachment(&mBackgroundAttachment);
    aPresContext->GetDefaultBackgroundImageRepeat(&mBackgroundRepeat);
    aPresContext->GetDefaultBackgroundImageOffset(&mBackgroundXPosition, &mBackgroundYPosition);
    aPresContext->GetDefaultBackgroundImage(mBackgroundImage);
  }
  else {
    mBackgroundColor = NS_RGB(192,192,192);
    mBackgroundAttachment = NS_STYLE_BG_ATTACHMENT_SCROLL;
    mBackgroundRepeat = NS_STYLE_BG_REPEAT_XY;
    mBackgroundXPosition = 0;
    mBackgroundYPosition = 0;
  }
}

void StyleColorImpl::SetFrom(const nsStyleColor& aSource)
{
  mColor = aSource.mColor;
 
  mBackgroundAttachment = aSource.mBackgroundAttachment;
  mBackgroundFlags = aSource.mBackgroundFlags;
  mBackgroundRepeat = aSource.mBackgroundRepeat;

  mBackgroundColor = aSource.mBackgroundColor;
  mBackgroundXPosition = aSource.mBackgroundXPosition;
  mBackgroundYPosition = aSource.mBackgroundYPosition;
  mBackgroundImage = aSource.mBackgroundImage;

  mCursor = aSource.mCursor;
  mCursorImage = aSource.mCursorImage;
  mOpacity = aSource.mOpacity;
}

void StyleColorImpl::CopyTo(nsStyleColor& aDest) const
{
  aDest.mColor = mColor;
 
  aDest.mBackgroundAttachment = mBackgroundAttachment;
  aDest.mBackgroundFlags = mBackgroundFlags;
  aDest.mBackgroundRepeat = mBackgroundRepeat;

  aDest.mBackgroundColor = mBackgroundColor;
  aDest.mBackgroundXPosition = mBackgroundXPosition;
  aDest.mBackgroundYPosition = mBackgroundYPosition;
  aDest.mBackgroundImage = mBackgroundImage;

  aDest.mCursor = mCursor;
  aDest.mCursorImage = mCursorImage;
  aDest.mOpacity = mOpacity;
}

PRInt32 StyleColorImpl::CalcDifference(const StyleColorImpl& aOther) const
{
  if ((mColor == aOther.mColor) && 
      (mBackgroundAttachment == aOther.mBackgroundAttachment) &&
      (mBackgroundFlags == aOther.mBackgroundFlags) &&
      (mBackgroundRepeat == aOther.mBackgroundRepeat) &&
      (mBackgroundColor == aOther.mBackgroundColor) &&
      (mBackgroundXPosition == aOther.mBackgroundXPosition) &&
      (mBackgroundYPosition == aOther.mBackgroundYPosition) &&
      (mBackgroundImage == aOther.mBackgroundImage) &&
      (mCursor == aOther.mCursor) &&
      (mCursorImage == aOther.mCursorImage) &&
      (mOpacity == aOther.mOpacity)) {
    return NS_STYLE_HINT_NONE;
  }
  return NS_STYLE_HINT_VISUAL;
}

// --------------------
// nsStyleText
//

nsStyleText::nsStyleText(void) { }
nsStyleText::~nsStyleText(void) { }

struct StyleTextImpl: public nsStyleText {
  StyleTextImpl(void) { }

  void ResetFrom(const nsStyleText* aParent, nsIPresContext* aPresContext);
  void SetFrom(const nsStyleText& aSource);
  void CopyTo(nsStyleText& aDest) const;
  PRInt32 CalcDifference(const StyleTextImpl& aOther) const;
  
private:  // These are not allowed
  StyleTextImpl(const StyleTextImpl& aOther);
  StyleTextImpl& operator=(const StyleTextImpl& aOther);
};

void StyleTextImpl::ResetFrom(const nsStyleText* aParent, nsIPresContext* aPresContext)
{
  // These properties not inherited
  mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_BASELINE, eStyleUnit_Enumerated);
//  mVerticalAlign.Reset(); TBI

  if (nsnull != aParent) {
    mTextAlign = aParent->mTextAlign;
    mTextTransform = aParent->mTextTransform;
    mWhiteSpace = aParent->mWhiteSpace;
    mLetterSpacing = aParent->mLetterSpacing;

    // Inherit everything except percentage line-height values
    nsStyleUnit unit = aParent->mLineHeight.GetUnit();
    if ((eStyleUnit_Normal == unit) || (eStyleUnit_Factor == unit) ||
        (eStyleUnit_Coord == unit)) {
      mLineHeight = aParent->mLineHeight;
    }
    else {
      mLineHeight.SetInheritValue();
    }
    mTextIndent = aParent->mTextIndent;
    mTextDecorations = aParent->mTextDecorations;
    mTextDecoration = NS_STYLE_TEXT_DECORATION_NONE;
    mWordSpacing = aParent->mWordSpacing;
#ifdef IBMBIDI
    mUnicodeBidi = aParent->mUnicodeBidi;
#endif // IBMBIDI
  }
  else {
    mTextAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
    mTextTransform = NS_STYLE_TEXT_TRANSFORM_NONE;
    mWhiteSpace = NS_STYLE_WHITESPACE_NORMAL;

    mLetterSpacing.SetNormalValue();
    mLineHeight.SetNormalValue();
    mTextIndent.SetCoordValue(0);
    mTextDecorations = mTextDecorations = NS_STYLE_TEXT_DECORATION_NONE;
    mWordSpacing.SetNormalValue();
#ifdef IBMBIDI
    mUnicodeBidi = NS_STYLE_UNICODE_BIDI_INHERIT;
#endif // IBMBIDI
  }
}

void StyleTextImpl::SetFrom(const nsStyleText& aSource)
{
  nsCRT::memcpy((nsStyleText*)this, &aSource, sizeof(nsStyleText));
}

void StyleTextImpl::CopyTo(nsStyleText& aDest) const
{
  nsCRT::memcpy(&aDest, (const nsStyleText*)this, sizeof(nsStyleText));
}

PRInt32 StyleTextImpl::CalcDifference(const StyleTextImpl& aOther) const
{
  if ((mTextAlign == aOther.mTextAlign) &&
      (mTextTransform == aOther.mTextTransform) &&
      (mWhiteSpace == aOther.mWhiteSpace) &&
      (mLetterSpacing == aOther.mLetterSpacing) &&
      (mLineHeight == aOther.mLineHeight) &&
      (mTextIndent == aOther.mTextIndent) &&
      (mWordSpacing == aOther.mWordSpacing) &&
#ifdef IBMBIDI
      (mUnicodeBidi == aOther.mUnicodeBidi) &&
#endif // IBMBIDI
      (mVerticalAlign == aOther.mVerticalAlign)) {
    if (mTextDecorations != aOther.mTextDecorations)
      return NS_STYLE_HINT_VISUAL;
    return NS_STYLE_HINT_NONE;
  }
  return NS_STYLE_HINT_REFLOW;
}


// --------------------
// nsStyleDisplay
//

struct StyleDisplayImpl: public nsStyleDisplay {
  StyleDisplayImpl(void) { }

  void ResetFrom(const nsStyleDisplay* aParent, nsIPresContext* aPresContext);
  void SetFrom(const nsStyleDisplay& aSource);
  void CopyTo(nsStyleDisplay& aDest) const;
  PRInt32 CalcDifference(const StyleDisplayImpl& aOther) const;
 
private:  // These are not allowed
  StyleDisplayImpl(const StyleDisplayImpl& aOther);
  StyleDisplayImpl& operator=(const StyleDisplayImpl& aOther);
};

void StyleDisplayImpl::ResetFrom(const nsStyleDisplay* aParent, nsIPresContext* aPresContext)
{
  if (nsnull != aParent) {
    mDirection = aParent->mDirection;
    mLanguage = aParent->mLanguage;
    mVisible = aParent->mVisible;
  }
  else {
#ifdef IBMBIDI
    PRUint32 mBidioptions;
    aPresContext->GetBidi(&mBidioptions);
    if (GET_BIDI_OPTION_DIRECTION(mBidioptions) == IBMBIDI_TEXTDIRECTION_RTL)
      mDirection = NS_STYLE_DIRECTION_RTL;
    else
      mDirection = NS_STYLE_DIRECTION_LTR;
#else // ifdef IBMBIDI
    aPresContext->GetDefaultDirection(&mDirection);
#endif // IBMBIDI
    aPresContext->GetLanguage(getter_AddRefs(mLanguage));
    mVisible = NS_STYLE_VISIBILITY_VISIBLE;
  }
  mDisplay = NS_STYLE_DISPLAY_INLINE;
  mFloats = NS_STYLE_FLOAT_NONE;
  mBreakType = NS_STYLE_CLEAR_NONE;
  mBreakBefore = PR_FALSE;
  mBreakAfter = PR_FALSE;
  mOverflow = NS_STYLE_OVERFLOW_VISIBLE;
  mClipFlags = NS_STYLE_CLIP_AUTO;
  mClip.SetRect(0,0,0,0);
#ifdef IBMBIDI
  mExplicitDirection = NS_STYLE_DIRECTION_INHERIT;
#endif // IBMBIDI
}

void StyleDisplayImpl::SetFrom(const nsStyleDisplay& aSource)
{
  mDirection = aSource.mDirection;
#ifdef IBMBIDI
  mExplicitDirection = aSource.mExplicitDirection;
#endif // IBMBIDI
  mDisplay = aSource.mDisplay;
  mFloats = aSource.mFloats;
  mBreakType = aSource.mBreakType;
  mBreakBefore = aSource.mBreakBefore;
  mBreakAfter = aSource.mBreakAfter;
  mVisible = aSource.mVisible;
  mOverflow = aSource.mOverflow;
  mClipFlags = aSource.mClipFlags;
  mClip = aSource.mClip;
  mLanguage = aSource.mLanguage;
}

void StyleDisplayImpl::CopyTo(nsStyleDisplay& aDest) const
{
  aDest.mDirection = mDirection;
#ifdef IBMBIDI
  aDest.mExplicitDirection = mExplicitDirection;
#endif // IBMBIDI
  aDest.mDisplay = mDisplay;
  aDest.mFloats = mFloats;
  aDest.mBreakType = mBreakType;
  aDest.mBreakBefore = mBreakBefore;
  aDest.mBreakAfter = mBreakAfter;
  aDest.mVisible = mVisible;
  aDest.mOverflow = mOverflow;
  aDest.mClipFlags = mClipFlags;
  aDest.mClip = mClip;
  aDest.mLanguage = mLanguage;
}

PRInt32 StyleDisplayImpl::CalcDifference(const StyleDisplayImpl& aOther) const
{
  if ((mDisplay == aOther.mDisplay) &&
      (mFloats == aOther.mFloats) &&
      (mOverflow == aOther.mOverflow)) {
    if ((mDirection == aOther.mDirection) &&
        (mLanguage == aOther.mLanguage) &&
        (mBreakType == aOther.mBreakType) &&
        (mBreakBefore == aOther.mBreakBefore) &&
        (mBreakAfter == aOther.mBreakAfter)) {
      if ((mVisible == aOther.mVisible) &&
          (mClipFlags == aOther.mClipFlags) &&
          (mClip == aOther.mClip)) {
        return NS_STYLE_HINT_NONE;
      }
      if ((mVisible != aOther.mVisible) && 
          ((NS_STYLE_VISIBILITY_COLLAPSE == mVisible) || 
           (NS_STYLE_VISIBILITY_COLLAPSE == aOther.mVisible))) {
        return NS_STYLE_HINT_REFLOW;
      }
      return NS_STYLE_HINT_VISUAL;
    }
    return NS_STYLE_HINT_REFLOW;
  }
  return NS_STYLE_HINT_FRAMECHANGE;
}

//-----------------------
// nsStyleContent
//

nsStyleContent::nsStyleContent(void)
  : mMarkerOffset(),
    mContentCount(0),
    mContents(nsnull),
    mIncrementCount(0),
    mIncrements(nsnull),
    mResetCount(0),
    mResets(nsnull),
    mQuotesCount(0),
    mQuotes(nsnull)
{
}

nsStyleContent::~nsStyleContent(void)
{
  DELETE_ARRAY_IF(mContents);
  DELETE_ARRAY_IF(mIncrements);
  DELETE_ARRAY_IF(mResets);
  DELETE_ARRAY_IF(mQuotes);
}


struct StyleContentImpl: public nsStyleContent {
  StyleContentImpl(void) : nsStyleContent() { };

  void ResetFrom(const StyleContentImpl* aParent, nsIPresContext* aPresContext);
  void SetFrom(const nsStyleContent& aSource);
  void CopyTo(nsStyleContent& aDest) const;
  PRInt32 CalcDifference(const StyleContentImpl& aOther) const;
  
private:  // These are not allowed
  StyleContentImpl(const StyleContentImpl& aOther);
  StyleContentImpl& operator=(const StyleContentImpl& aOther);
};

void
StyleContentImpl::ResetFrom(const StyleContentImpl* aParent, nsIPresContext* aPresContext)
{
  // reset data
  mMarkerOffset.Reset();
  mContentCount = 0;
  DELETE_ARRAY_IF(mContents);
  mIncrementCount = 0;
  DELETE_ARRAY_IF(mIncrements);
  mResetCount = 0;
  DELETE_ARRAY_IF(mResets);

  // inherited data
  if (aParent) {
    if (NS_SUCCEEDED(AllocateQuotes(aParent->mQuotesCount))) {
      PRUint32 ix = (mQuotesCount * 2);
      while (0 < ix--) {
        mQuotes[ix] = aParent->mQuotes[ix];
      }
    }
  }
  else {
    mQuotesCount = 0;
    DELETE_ARRAY_IF(mQuotes);
  }
}

void StyleContentImpl::SetFrom(const nsStyleContent& aSource)
{
  mMarkerOffset = aSource.mMarkerOffset;

  PRUint32 index;
  if (NS_SUCCEEDED(AllocateContents(aSource.ContentCount()))) {
    for (index = 0; index < mContentCount; index++) {
      aSource.GetContentAt(index, mContents[index].mType, mContents[index].mContent);
    }
  }

  if (NS_SUCCEEDED(AllocateCounterIncrements(aSource.CounterIncrementCount()))) {
    for (index = 0; index < mIncrementCount; index++) {
      aSource.GetCounterIncrementAt(index, mIncrements[index].mCounter,
                                           mIncrements[index].mValue);
    }
  }

  if (NS_SUCCEEDED(AllocateCounterResets(aSource.CounterResetCount()))) {
    for (index = 0; index < mResetCount; index++) {
      aSource.GetCounterResetAt(index, mResets[index].mCounter,
                                       mResets[index].mValue);
    }
  }

  if (NS_SUCCEEDED(AllocateQuotes(aSource.QuotesCount()))) {
    PRUint32 count = (mQuotesCount * 2);
    for (index = 0; index < count; index += 2) {
      aSource.GetQuotesAt(index, mQuotes[index], mQuotes[index + 1]);
    }
  }
}

void StyleContentImpl::CopyTo(nsStyleContent& aDest) const
{
  aDest.mMarkerOffset = mMarkerOffset;

  PRUint32 index;
  if (NS_SUCCEEDED(aDest.AllocateContents(mContentCount))) {
    for (index = 0; index < mContentCount; index++) {
      aDest.SetContentAt(index, mContents[index].mType,
                                mContents[index].mContent);
    }
  }

  if (NS_SUCCEEDED(aDest.AllocateCounterIncrements(mIncrementCount))) {
    for (index = 0; index < mIncrementCount; index++) {
      aDest.SetCounterIncrementAt(index, mIncrements[index].mCounter,
                                         mIncrements[index].mValue);
    }
  }

  if (NS_SUCCEEDED(aDest.AllocateCounterResets(mResetCount))) {
    for (index = 0; index < mResetCount; index++) {
      aDest.SetCounterResetAt(index, mResets[index].mCounter,
                                     mResets[index].mValue);
    }
  }

  if (NS_SUCCEEDED(aDest.AllocateQuotes(mQuotesCount))) {
    PRUint32 count = (mQuotesCount * 2);
    for (index = 0; index < count; index += 2) {
      aDest.SetQuotesAt(index, mQuotes[index], mQuotes[index + 1]);
    }
  }
}

PRInt32 
StyleContentImpl::CalcDifference(const StyleContentImpl& aOther) const
{
  if (mContentCount == aOther.mContentCount) {
    if ((mMarkerOffset == aOther.mMarkerOffset) &&
        (mIncrementCount == aOther.mIncrementCount) && 
        (mResetCount == aOther.mResetCount) &&
        (mQuotesCount == aOther.mQuotesCount)) {
      PRUint32 ix = mContentCount;
      while (0 < ix--) {
        if ((mContents[ix].mType != aOther.mContents[ix].mType) || 
            (mContents[ix].mContent != aOther.mContents[ix].mContent)) {
          return NS_STYLE_HINT_REFLOW;
        }
      }
      ix = mIncrementCount;
      while (0 < ix--) {
        if ((mIncrements[ix].mValue != aOther.mIncrements[ix].mValue) || 
            (mIncrements[ix].mCounter != aOther.mIncrements[ix].mCounter)) {
          return NS_STYLE_HINT_REFLOW;
        }
      }
      ix = mResetCount;
      while (0 < ix--) {
        if ((mResets[ix].mValue != aOther.mResets[ix].mValue) || 
            (mResets[ix].mCounter != aOther.mResets[ix].mCounter)) {
          return NS_STYLE_HINT_REFLOW;
        }
      }
      ix = (mQuotesCount * 2);
      while (0 < ix--) {
        if (mQuotes[ix] != aOther.mQuotes[ix]) {
          return NS_STYLE_HINT_REFLOW;
        }
      }
      return NS_STYLE_HINT_NONE;
    }
    return NS_STYLE_HINT_REFLOW;
  }
  return NS_STYLE_HINT_FRAMECHANGE;
}

//-----------------------
// nsStyleUserInterface
//

nsStyleUserInterface::nsStyleUserInterface(void) { }
nsStyleUserInterface::~nsStyleUserInterface(void) { }

struct StyleUserInterfaceImpl: public nsStyleUserInterface {
  StyleUserInterfaceImpl(void)  { }

  void ResetFrom(const nsStyleUserInterface* aParent, nsIPresContext* aPresContext);
  void SetFrom(const nsStyleUserInterface& aSource);
  void CopyTo(nsStyleUserInterface& aDest) const;
  PRInt32 CalcDifference(const StyleUserInterfaceImpl& aOther) const;
  
private:  // These are not allowed
  StyleUserInterfaceImpl(const StyleUserInterfaceImpl& aOther);
  StyleUserInterfaceImpl& operator=(const StyleUserInterfaceImpl& aOther);
};

void StyleUserInterfaceImpl::ResetFrom(const nsStyleUserInterface* aParent, nsIPresContext* aPresContext)
{
  if (aParent) {
    mUserInput = aParent->mUserInput;
    mUserModify = aParent->mUserModify;
    mUserFocus = aParent->mUserFocus;
  }
  else {
    mUserInput = NS_STYLE_USER_INPUT_AUTO;
    mUserModify = NS_STYLE_USER_MODIFY_READ_ONLY;
    mUserFocus = NS_STYLE_USER_FOCUS_NONE;
  }

  mUserSelect = NS_STYLE_USER_SELECT_AUTO;
  mKeyEquivalent = PRUnichar(0); // XXX what type should this be?
  mResizer = NS_STYLE_RESIZER_AUTO;
  mBehavior.SetLength(0);
}

void StyleUserInterfaceImpl::SetFrom(const nsStyleUserInterface& aSource)
{
  mUserInput = aSource.mUserInput;
  mUserModify = aSource.mUserModify;
  mUserFocus = aSource.mUserFocus;

  mUserSelect = aSource.mUserSelect;
  mKeyEquivalent = aSource.mKeyEquivalent;
  mResizer = aSource.mResizer;
  mBehavior = aSource.mBehavior;
}

void StyleUserInterfaceImpl::CopyTo(nsStyleUserInterface& aDest) const
{
  aDest.mUserInput = mUserInput;
  aDest.mUserModify = mUserModify;
  aDest.mUserFocus = mUserFocus;

  aDest.mUserSelect = mUserSelect;
  aDest.mKeyEquivalent = mKeyEquivalent;
  aDest.mResizer = mResizer;
  aDest.mBehavior = mBehavior;
}

PRInt32 StyleUserInterfaceImpl::CalcDifference(const StyleUserInterfaceImpl& aOther) const
{
  if (mBehavior != aOther.mBehavior)
    return NS_STYLE_HINT_FRAMECHANGE;

  if ((mUserInput == aOther.mUserInput) && 
      (mResizer == aOther.mResizer)) {
    if ((mUserModify == aOther.mUserModify) && 
        (mUserSelect == aOther.mUserSelect)) {
      if ((mKeyEquivalent == aOther.mKeyEquivalent) &&
          (mUserFocus == aOther.mUserFocus) &&
          (mResizer == aOther.mResizer)) {
        return NS_STYLE_HINT_NONE;
      }
      return NS_STYLE_HINT_CONTENT;
    }
    return NS_STYLE_HINT_VISUAL;
  }
  if ((mUserInput != aOther.mUserInput) &&
      ((NS_STYLE_USER_INPUT_NONE == mUserInput) || 
       (NS_STYLE_USER_INPUT_NONE == aOther.mUserInput))) {
    return NS_STYLE_HINT_FRAMECHANGE;
  }
  return NS_STYLE_HINT_VISUAL;
}

//----------------------------------------------------------------------

class StyleContextImpl : public nsIStyleContext,
                         protected nsIMutableStyleContext { // you can't QI to nsIMutableStyleContext
public:
  StyleContextImpl(nsIStyleContext* aParent, nsIAtom* aPseudoTag, 
                   nsIRuleNode* aRuleNode, 
                   nsIPresContext* aPresContext);
  virtual ~StyleContextImpl();

  void* operator new(size_t sz, nsIPresContext* aPresContext);
  void Destroy();

  NS_DECL_ISUPPORTS

  virtual nsIStyleContext*  GetParent(void) const;
  NS_IMETHOD GetPseudoType(nsIAtom*& aPseudoTag) const;

  NS_IMETHOD FindChildWithRules(const nsIAtom* aPseudoTag, nsIRuleNode* aRules,
                                nsIStyleContext*& aResult);

  virtual PRBool    Equals(const nsIStyleContext* aOther) const;
  
  NS_IMETHOD RemapStyle(nsIPresContext* aPresContext, PRBool aRecurse = PR_TRUE);

  NS_IMETHOD GetBorderPaddingFor(nsStyleBorderPadding& aBorderPadding);

  NS_IMETHOD GetStyle(nsStyleStructID aSID, nsStyleStruct** aStruct);
  NS_IMETHOD SetStyle(nsStyleStructID aSID, const nsStyleStruct& aStruct);

  NS_IMETHOD GetRuleNode(nsIRuleNode** aResult) { *aResult = mRuleNode; NS_IF_ADDREF(*aResult); return NS_OK; };
  NS_IMETHOD AddInheritBit(const PRUint32& aInheritBit) { mInheritBits |= aInheritBit; return NS_OK; };

  virtual const nsStyleStruct* GetStyleData(nsStyleStructID aSID);
  virtual nsStyleStruct* GetMutableStyleData(nsStyleStructID aSID);

  virtual void ForceUnique(void);
  NS_IMETHOD  CalcStyleDifference(nsIStyleContext* aOther, PRInt32& aHint,PRBool aStopAtFirstDifference = PR_FALSE);

  virtual void  List(FILE* out, PRInt32 aIndent);

  virtual void SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize);

#ifdef DEBUG
  virtual void DumpRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent);
#endif

protected:
  void AppendChild(StyleContextImpl* aChild);
  void RemoveChild(StyleContextImpl* aChild);

  StyleContextImpl* mParent;
  StyleContextImpl* mChild;
  StyleContextImpl* mEmptyChild;
  StyleContextImpl* mPrevSibling;
  StyleContextImpl* mNextSibling;

  nsIAtom*          mPseudoTag;

  PRUint32                mInheritBits; // Which structs are inherited from the parent context.
  nsIRuleNode*            mRuleNode; // Weak. Rules can't go away without us going away.
  nsCachedStyleData       mCachedStyleData; // Our cached style data.

  PRInt16           mDataCode;

  // the style data...
  StyleColorImpl          mColor;
  StyleTextImpl           mText;
  StyleDisplayImpl        mDisplay;
  StyleContentImpl        mContent;
  StyleUserInterfaceImpl  mUserInterface;
	
  nsCOMPtr<nsIStyleSet>   mStyleSet;
};

static PRInt32 gLastDataCode;

StyleContextImpl::StyleContextImpl(nsIStyleContext* aParent,
                                   nsIAtom* aPseudoTag,
                                   nsIRuleNode* aRuleNode,
                                   nsIPresContext* aPresContext)
  : mParent((StyleContextImpl*)aParent),
    mChild(nsnull),
    mEmptyChild(nsnull),
    mPseudoTag(aPseudoTag),
    mInheritBits(0),
    mRuleNode(aRuleNode),
    mDataCode(-1),
    mColor(),
    mText(),
    mDisplay(),
    mContent(),
    mUserInterface()
{
  NS_INIT_REFCNT();
  NS_IF_ADDREF(mPseudoTag);
  
  mNextSibling = this;
  mPrevSibling = this;
  if (nsnull != mParent) {
    NS_ADDREF(mParent);
    mParent->AppendChild(this);
  }
}

StyleContextImpl::~StyleContextImpl()
{
  NS_ASSERTION((nsnull == mChild) && (nsnull == mEmptyChild), "destructing context with children");

  if (mParent) {
    mParent->RemoveChild(this);
    NS_RELEASE(mParent);
  }

  NS_IF_RELEASE(mPseudoTag);
  
  // Free up our data structs.
  nsCOMPtr<nsIPresContext> presContext;
  mRuleNode->GetPresContext(getter_AddRefs(presContext));
}

NS_IMPL_ADDREF(StyleContextImpl)
NS_IMPL_RELEASE_WITH_DESTROY(StyleContextImpl, Destroy())

NS_IMETHODIMP
StyleContextImpl::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(nsnull != aInstancePtr, "null pointer");
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
  if (aIID.Equals(NS_GET_IID(nsIStyleContext))) {
    *aInstancePtr = (void*)(nsIStyleContext*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*) (nsISupports*)(nsIStyleContext*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

nsIStyleContext* StyleContextImpl::GetParent(void) const
{
  NS_IF_ADDREF(mParent);
  return mParent;
}

void StyleContextImpl::AppendChild(StyleContextImpl* aChild)
{
  PRBool isRoot = PR_FALSE;
  nsCOMPtr<nsIRuleNode> ruleNode;
  aChild->GetRuleNode(getter_AddRefs(ruleNode));
  ruleNode->IsRoot(&isRoot);

  if (isRoot) {
    // We matched no rules.
    if (nsnull == mEmptyChild) {
      mEmptyChild = aChild;
    }
    else {
      aChild->mNextSibling = mEmptyChild;
      aChild->mPrevSibling = mEmptyChild->mPrevSibling;
      mEmptyChild->mPrevSibling->mNextSibling = aChild;
      mEmptyChild->mPrevSibling = aChild;
    }
  }
  else {
    if (nsnull == mChild) {
      mChild = aChild;
    }
    else {
      aChild->mNextSibling = mChild;
      aChild->mPrevSibling = mChild->mPrevSibling;
      mChild->mPrevSibling->mNextSibling = aChild;
      mChild->mPrevSibling = aChild;
    }
  }
}

void StyleContextImpl::RemoveChild(StyleContextImpl* aChild)
{
  NS_ASSERTION((nsnull != aChild) && (this == aChild->mParent), "bad argument");

  if ((nsnull == aChild) || (this != aChild->mParent)) {
    return;
  }

  PRBool isRoot = PR_FALSE;
  nsCOMPtr<nsIRuleNode> ruleNode;
  aChild->GetRuleNode(getter_AddRefs(ruleNode));
  ruleNode->IsRoot(&isRoot);

  if (isRoot) { // is empty 
    if (aChild->mPrevSibling != aChild) { // has siblings
      if (mEmptyChild == aChild) {
        mEmptyChild = mEmptyChild->mNextSibling;
      }
    } 
    else {
      NS_ASSERTION(mEmptyChild == aChild, "bad sibling pointers");
      mEmptyChild = nsnull;
    }
  }
  else {  // isn't empty
    if (aChild->mPrevSibling != aChild) { // has siblings
      if (mChild == aChild) {
        mChild = mChild->mNextSibling;
      }
    }
    else {
      NS_ASSERTION(mChild == aChild, "bad sibling pointers");
      if (mChild == aChild) {
        mChild = nsnull;
      }
    }
  }
  aChild->mPrevSibling->mNextSibling = aChild->mNextSibling;
  aChild->mNextSibling->mPrevSibling = aChild->mPrevSibling;
  aChild->mNextSibling = aChild;
  aChild->mPrevSibling = aChild;
}

NS_IMETHODIMP
StyleContextImpl::GetPseudoType(nsIAtom*& aPseudoTag) const
{
  aPseudoTag = mPseudoTag;
  NS_IF_ADDREF(aPseudoTag);
  return NS_OK;
}

NS_IMETHODIMP
StyleContextImpl::FindChildWithRules(const nsIAtom* aPseudoTag, 
                                     nsIRuleNode* aRuleNode,
                                     nsIStyleContext*& aResult)
{
  PRUint32 threshold = 10; // The # of siblings we're willing to examine
                           // before just giving this whole thing up.

  aResult = nsnull;

  if ((nsnull != mChild) || (nsnull != mEmptyChild)) {
    StyleContextImpl* child;
    PRBool isRoot = PR_TRUE;
    aRuleNode->IsRoot(&isRoot);
    if (isRoot) {
      if (nsnull != mEmptyChild) {
        child = mEmptyChild;
        do {
          if ((0 == child->mDataCode) &&  // only look at children with un-twiddled data
              (aPseudoTag == child->mPseudoTag)) {
            aResult = child;
            break;
          }
          child = child->mNextSibling;
          threshold--;
          if (threshold == 0)
            break;
        } while (child != mEmptyChild);
      }
    }
    else if (nsnull != mChild) {
      child = mChild;
      
      do {
        if ((0 == child->mDataCode) &&  // only look at children with un-twiddled data
            (child->mRuleNode == aRuleNode) &&
            (child->mPseudoTag == aPseudoTag)) {
          aResult = child;
          break;
        }
        child = child->mNextSibling;
        threshold--;
        if (threshold == 0)
          break;
      } while (child != mChild);
    }
  }
  NS_IF_ADDREF(aResult);
  return NS_OK;
}


PRBool StyleContextImpl::Equals(const nsIStyleContext* aOther) const
{
  PRBool  result = PR_TRUE;
  const StyleContextImpl* other = (StyleContextImpl*)aOther;

  if (other != this) {
    if (mParent != other->mParent) {
      result = PR_FALSE;
    }
    else if (mDataCode != other->mDataCode) {
      result = PR_FALSE;
    }
    else if (mPseudoTag != other->mPseudoTag) {
      result = PR_FALSE;
    }
    else if (mRuleNode != other->mRuleNode) {
      result = PR_FALSE;
    }
  }
  return result;
}

//=========================================================================================================

const nsStyleStruct* StyleContextImpl::GetStyleData(nsStyleStructID aSID)
{
  nsStyleStruct*  result = nsnull;
  const nsStyleStruct* cachedData = mCachedStyleData.GetStyleData(aSID); 
      
  switch (aSID) {
    case eStyleStruct_Font:
    case eStyleStruct_Margin:
    case eStyleStruct_Border:
    case eStyleStruct_Padding:
    case eStyleStruct_Outline:
    case eStyleStruct_List:
    case eStyleStruct_Position:
    case eStyleStruct_Table:
    case eStyleStruct_TableBorder:
    case eStyleStruct_XUL: 
      if (cachedData) // First look to see if we have computed data.
        return cachedData;  // We do. Just return it.
      if (mParent && (mInheritBits & nsCachedStyleData::GetBitForSID(aSID))) // Now check inheritance
        return mParent->GetStyleData(aSID); // We inherit from our parent in the style context tree.
      else
        return mRuleNode->GetStyleData(aSID, this, this); // Our rule node will take care of it for us.
    case eStyleStruct_Color:
      result = & GETSCDATA(Color);
      break;
    case eStyleStruct_Text:
      result = & GETSCDATA(Text);
      break;
    case eStyleStruct_Display:
      result = & GETSCDATA(Display);
      break;
    case eStyleStruct_Content:
      result = & GETSCDATA(Content);
      break;
    case eStyleStruct_UserInterface:
      result = & GETSCDATA(UserInterface);
      break;
    default:
      NS_ERROR("Invalid style struct id");
      break;
  }

  return result;
}

NS_IMETHODIMP
StyleContextImpl::GetBorderPaddingFor(nsStyleBorderPadding& aBorderPadding)
{
  nsMargin border, padding;
  const nsStyleBorder* borderData = (const nsStyleBorder*)GetStyleData(eStyleStruct_Border);
  const nsStylePadding* paddingData = (const nsStylePadding*)GetStyleData(eStyleStruct_Padding);
  if (borderData->GetBorder(border)) {
	  if (paddingData->GetPadding(padding)) {
	    border += padding;
	    aBorderPadding.SetBorderPadding(border);
	  }
  }

  return NS_OK;
}


nsStyleStruct* StyleContextImpl::GetMutableStyleData(nsStyleStructID aSID)
{
  nsStyleStruct*  result = nsnull;

    switch (aSID) {
    case eStyleStruct_Font:
    case eStyleStruct_Border:
    case eStyleStruct_Margin:
    case eStyleStruct_Padding:
    case eStyleStruct_Outline:
    case eStyleStruct_List:
    case eStyleStruct_XUL:
    case eStyleStruct_Position:
    case eStyleStruct_Table:
    case eStyleStruct_TableBorder:
      NS_ERROR("YOU CANNOT CALL THIS!  IT'S GOING TO BE REMOVED!\n");
      return nsnull;
    case eStyleStruct_Color:
      result = & GETSCDATA(Color);
      break;
    case eStyleStruct_Text:
      result = & GETSCDATA(Text);
      break;
    case eStyleStruct_Display:
      result = & GETSCDATA(Display);
      break;
    case eStyleStruct_Content:
      result = & GETSCDATA(Content);
      break;
    case eStyleStruct_UserInterface:
      result = & GETSCDATA(UserInterface);
      break;
    default:
      NS_ERROR("Invalid style struct id");
      break;
  }

  if (nsnull != result) {
    if (0 == mDataCode) {
//      mDataCode = ++gLastDataCode;  // XXX temp disable, this is still used but not needed to force unique
    }
  }

  return result;
}

NS_IMETHODIMP
StyleContextImpl::GetStyle(nsStyleStructID aSID, nsStyleStruct** aStruct)
{
  *aStruct = (nsStyleStruct*)(GetStyleData(aSID));
  return NS_OK;
}

NS_IMETHODIMP
StyleContextImpl::SetStyle(nsStyleStructID aSID, const nsStyleStruct& aStruct)
{
  // This method should only be called from nsRuleNode!  It is not a public
  // method!
  nsresult result = NS_OK;
  
  PRBool isReset = mCachedStyleData.IsReset(aSID);
  if (isReset && !mCachedStyleData.mResetData) {
    nsCOMPtr<nsIPresContext> presContext;
    mRuleNode->GetPresContext(getter_AddRefs(presContext));
    mCachedStyleData.mResetData = new (presContext.get()) nsResetStyleData;
  }
  else if (!isReset && !mCachedStyleData.mInheritedData) {
    nsCOMPtr<nsIPresContext> presContext;
    mRuleNode->GetPresContext(getter_AddRefs(presContext));
    mCachedStyleData.mInheritedData = new (presContext.get()) nsInheritedStyleData;
  }

  switch (aSID) {
    case eStyleStruct_Font:
      mCachedStyleData.mInheritedData->mFontData = (nsStyleFont*)(const nsStyleFont*)(&aStruct);
      break;
    case eStyleStruct_Color:
      GETSCDATA(Color).SetFrom((const nsStyleColor&)aStruct);
      break;
    case eStyleStruct_List:
      mCachedStyleData.mInheritedData->mListData = (nsStyleList*)(const nsStyleList*)(&aStruct);
      break;
    case eStyleStruct_Position:
      mCachedStyleData.mResetData->mPositionData = (nsStylePosition*)(const nsStylePosition*)(&aStruct);
      break;
    case eStyleStruct_Text:
      GETSCDATA(Text).SetFrom((const nsStyleText&)aStruct);
      break;
    case eStyleStruct_Display:
      GETSCDATA(Display).SetFrom((const nsStyleDisplay&)aStruct);
      break;
    case eStyleStruct_Table:
      mCachedStyleData.mResetData->mTableData = (nsStyleTable*)(const nsStyleTable*)(&aStruct);
      break;
    case eStyleStruct_TableBorder:
      mCachedStyleData.mInheritedData->mTableData = (nsStyleTableBorder*)(const nsStyleTableBorder*)(&aStruct);
      break;
    case eStyleStruct_Content:
      GETSCDATA(Content).SetFrom((const nsStyleContent&)aStruct);
      break;
    case eStyleStruct_UserInterface:
      GETSCDATA(UserInterface).SetFrom((const nsStyleUserInterface&)aStruct);
      break;
    case eStyleStruct_Margin:
      mCachedStyleData.mResetData->mMarginData = (nsStyleMargin*)(const nsStyleMargin*)(&aStruct);
      break;
    case eStyleStruct_Padding:
      mCachedStyleData.mResetData->mPaddingData = (nsStylePadding*)(const nsStylePadding*)(&aStruct);
      break;
    case eStyleStruct_Border:
      mCachedStyleData.mResetData->mBorderData = (nsStyleBorder*)(const nsStyleBorder*)(&aStruct);
      break;
    case eStyleStruct_Outline:
      mCachedStyleData.mResetData->mOutlineData = (nsStyleOutline*)(const nsStyleOutline*)(&aStruct);
      break;
#ifdef INCLUDE_XUL
    case eStyleStruct_XUL:
      mCachedStyleData.mResetData->mXULData = (nsStyleXUL*)(const nsStyleXUL*)(&aStruct);
      break;
#endif
    default:
      NS_ERROR("Invalid style struct id");
      result = NS_ERROR_INVALID_ARG;
      break;
  }
  return result;
}



struct MapStyleData {
  MapStyleData(nsIMutableStyleContext* aStyleContext, nsIPresContext* aPresContext)
  {
    mStyleContext = aStyleContext;
    mPresContext = aPresContext;
  }
  nsIMutableStyleContext*  mStyleContext;
  nsIPresContext*   mPresContext;
};

static void MapStyleRule(MapStyleData* aData, nsIRuleNode* aCurrNode)
{
  // For now in order to preserve compatibility with the current style system,
  // we walk the rules from least sig. to most sig.  In reality, this is the
  // wrong direction, and we should be dynamically obtaining properties by walking
  // the rules from most sig. to least sig.  Changing this will prevent us from wasting time
  // looking at rules whose values are overridden.
  nsCOMPtr<nsIRuleNode> parent;
  aCurrNode->GetParent(getter_AddRefs(parent));
  if (parent)
    MapStyleRule(aData, parent);

  nsCOMPtr<nsIStyleRule> rule;;
  aCurrNode->GetRule(getter_AddRefs(rule));
  if (rule)
    rule->MapStyleInto(aData->mStyleContext, aData->mPresContext);
}

NS_IMETHODIMP
StyleContextImpl::RemapStyle(nsIPresContext* aPresContext, PRBool aRecurse)
{
  mDataCode = -1;

  if (nsnull != mParent) {
    GETSCDATA(Color).ResetFrom(&(mParent->GETSCDATA(Color)), aPresContext);
    GETSCDATA(Text).ResetFrom(&(mParent->GETSCDATA(Text)), aPresContext);
    GETSCDATA(Display).ResetFrom(&(mParent->GETSCDATA(Display)), aPresContext);
    GETSCDATA(Content).ResetFrom(&(mParent->GETSCDATA(Content)), aPresContext);
    GETSCDATA(UserInterface).ResetFrom(&(mParent->GETSCDATA(UserInterface)), aPresContext);
  }
  else {
    GETSCDATA(Color).ResetFrom(nsnull, aPresContext);
    GETSCDATA(Text).ResetFrom(nsnull, aPresContext);
    GETSCDATA(Display).ResetFrom(nsnull, aPresContext);
    GETSCDATA(Content).ResetFrom(nsnull, aPresContext);
    GETSCDATA(UserInterface).ResetFrom(nsnull, aPresContext);
  }

  PRBool isRoot = PR_FALSE;
  mRuleNode->IsRoot(&isRoot);
  if (!isRoot) {
    MapStyleData  data(this, aPresContext);
    //MapStyleRuleFont(&data, mRuleNode);
    //if (GETSCDATA(Font).mFlags & NS_STYLE_FONT_USE_FIXED) {
    //  GETSCDATA(Font).mFont = GETSCDATA(Font).mFixedFont;
    //}
    MapStyleRule(&data, mRuleNode);
  }

  if (-1 == mDataCode) {
    mDataCode = 0;
  }

  // CSS2 specified fixups:
  //  - these must be done after all declarations are mapped since they can cross style-structs

  // 1) if float is not none, and display is not none, then we must set display to block
  //    XXX - there are problems with following the spec here: what we will do instead of
  //          following the letter of the spec is to make sure that floated elements are
  //          some kind of block, not strictly 'block' - see EnsureBlockDisplay method
  nsStyleDisplay *disp = (nsStyleDisplay *)GetMutableStyleData(eStyleStruct_Display);
  if (disp) {
    if (disp->mDisplay != NS_STYLE_DISPLAY_NONE &&
        disp->mFloats != NS_STYLE_FLOAT_NONE ) {
      EnsureBlockDisplay(disp->mDisplay);
    }
  }
  // 2) if position is 'absolute' or 'fixed' then display must be 'block and float must be 'none'
  //    XXX - see note for fixup 1) above...
  const nsStylePosition *pos = (const nsStylePosition *)GetStyleData(eStyleStruct_Position);
  if (pos) {
    if (pos->IsAbsolutelyPositioned()) {
      if (disp) {
        if(disp->mDisplay != NS_STYLE_DISPLAY_NONE) {
          EnsureBlockDisplay(disp->mDisplay);
          disp->mFloats = NS_STYLE_FLOAT_NONE;
        }
      }
    }
  }

  nsCompatibility quirkMode = eCompatibility_Standard;
  aPresContext->GetCompatibilityMode(&quirkMode);
  if (eCompatibility_NavQuirks == quirkMode) {
    if (((GETSCDATA(Display).mDisplay == NS_STYLE_DISPLAY_TABLE) || 
         (GETSCDATA(Display).mDisplay == NS_STYLE_DISPLAY_TABLE_CAPTION)) &&
        (nsnull == mPseudoTag)) {

      StyleContextImpl* holdParent = mParent;
      mParent = nsnull; // cut off all inheritance. this really blows

      // XXX the style we do preserve is visibility, direction, language
      PRUint8 visible = GETSCDATA(Display).mVisible;
      PRUint8 direction = GETSCDATA(Display).mDirection;
      nsCOMPtr<nsILanguageAtom> language = GETSCDATA(Display).mLanguage;

      // time to emulate a sub-document
      // This is ugly, but we need to map style once to determine display type
      // then reset and map it again so that all local style is preserved
      if (GETSCDATA(Display).mDisplay != NS_STYLE_DISPLAY_TABLE) {
       // GETSCDATA(Font).ResetFrom(nsnull, aPresContext);
      }
      GETSCDATA(Color).ResetFrom(nsnull, aPresContext);
      GETSCDATA(Text).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Position).ResetFrom(nsnull, aPresContext);
      GETSCDATA(Display).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Table).ResetFrom(nsnull, aPresContext);
      GETSCDATA(Content).ResetFrom(nsnull, aPresContext);
      GETSCDATA(UserInterface).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Margin).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Padding).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Border).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(Outline).ResetFrom(nsnull, aPresContext);
      //GETSCDATA(XUL).ResetFrom(nsnull, aPresContext);

      GETSCDATA(Display).mVisible = visible;
      GETSCDATA(Display).mDirection = direction;
      GETSCDATA(Display).mLanguage = language;

      if (!isRoot) {
        MapStyleData  data(this, aPresContext);
        //MapStyleRuleFont(&data, mRuleNode);
        //if (GETSCDATA(Font).mFlags & NS_STYLE_FONT_USE_FIXED) {
        //  GETSCDATA(Font).mFont = GETSCDATA(Font).mFixedFont;
        // }
        MapStyleRule(&data, mRuleNode);
      }
    
      // reset all font data for tables again
      if (GETSCDATA(Display).mDisplay == NS_STYLE_DISPLAY_TABLE) {
        // get the font-name to reset: this property we preserve
        //nsAutoString strName(GETSCDATA(Font).mFont.name);
        //nsAutoString strMixedName(GETSCDATA(Font).mFixedFont.name);
   
        //GETSCDATA(Font).ResetFrom(nsnull, aPresContext);
   
        // now reset the font names back to original
        //GETSCDATA(Font).mFont.name = strName;
        //GETSCDATA(Font).mFixedFont.name = strMixedName;
      }
      mParent = holdParent;
    }
  } else {
    // In strict mode, we still have to support one "quirky" thing
    // for tables.  HTML's alignment attributes have always worked
    // so they don't inherit into tables, but instead align the
    // tables.  We should keep doing this, because HTML alignment
    // is just weird, and we shouldn't force it to match CSS.
    if (GETSCDATA(Display).mDisplay == NS_STYLE_DISPLAY_TABLE) {
      // -moz-center and -moz-right are used for HTML's alignment
      if ((GETSCDATA(Text).mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER) ||
          (GETSCDATA(Text).mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT))
      {
        GETSCDATA(Text).mTextAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
      }
    }
  }

  if (aRecurse) {
    if (nsnull != mChild) {
      StyleContextImpl* child = mChild;
      do {
        child->RemapStyle(aPresContext);
        child = child->mNextSibling;
      } while (mChild != child);
    }
    if (nsnull != mEmptyChild) {
      StyleContextImpl* child = mEmptyChild;
      do {
        child->RemapStyle(aPresContext);
        child = child->mNextSibling;
      } while (mEmptyChild != child);
    }
  }
  return NS_OK;
}

void StyleContextImpl::ForceUnique(void)
{
  if (mDataCode <= 0) {
    mDataCode = ++gLastDataCode;
  }
}

NS_IMETHODIMP
StyleContextImpl::CalcStyleDifference(nsIStyleContext* aOther, PRInt32& aHint,PRBool aStopAtFirstDifference /*= PR_FALSE*/)
{
  if (aOther) {
    PRInt32 hint;
    const StyleContextImpl* other = (const StyleContextImpl*)aOther;

    const nsStyleFont* font = (const nsStyleFont*)GetStyleData(eStyleStruct_Font);
    const nsStyleFont* otherFont = (const nsStyleFont*)aOther->GetStyleData(eStyleStruct_Font);
    aHint = font->CalcDifference(*otherFont);
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      hint = GETSCDATA(Color).CalcDifference(other->GETSCDATA(Color));
      if (aHint < hint) {
        aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleMargin* margin = (const nsStyleMargin*)GetStyleData(eStyleStruct_Margin);
      const nsStyleMargin* otherMargin = (const nsStyleMargin*)aOther->GetStyleData(eStyleStruct_Margin);
      hint = margin->CalcDifference(*otherMargin);
      if (aHint < hint) {
        aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStylePadding* padding = (const nsStylePadding*)GetStyleData(eStyleStruct_Padding);
      const nsStylePadding* otherPadding = (const nsStylePadding*)aOther->GetStyleData(eStyleStruct_Padding);
      hint = padding->CalcDifference(*otherPadding);
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleBorder* border = (const nsStyleBorder*)GetStyleData(eStyleStruct_Border);
      const nsStyleBorder* otherBorder = (const nsStyleBorder*)aOther->GetStyleData(eStyleStruct_Border);
      hint = border->CalcDifference(*otherBorder);
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleOutline* outline = (const nsStyleOutline*)GetStyleData(eStyleStruct_Outline);
      const nsStyleOutline* otherOutline = (const nsStyleOutline*)aOther->GetStyleData(eStyleStruct_Outline);
      hint = outline->CalcDifference(*otherOutline);
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleList* list = (const nsStyleList*)GetStyleData(eStyleStruct_List);
      const nsStyleList* otherList = (const nsStyleList*)aOther->GetStyleData(eStyleStruct_List);
      hint = list->CalcDifference(*otherList);
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStylePosition* list = (const nsStylePosition*)GetStyleData(eStyleStruct_Position);
      const nsStylePosition* otherPosition = (const nsStylePosition*)aOther->GetStyleData(eStyleStruct_Position);
      hint = list->CalcDifference(*otherPosition);
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      hint = GETSCDATA(Text).CalcDifference(other->GETSCDATA(Text));
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      hint = GETSCDATA(Display).CalcDifference(other->GETSCDATA(Display));
      if (aHint < hint) {
        aHint = hint;
      }
    }

#ifdef INCLUDE_XUL
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleXUL* xul = (const nsStyleXUL*)GetStyleData(eStyleStruct_XUL);
      const nsStyleXUL* otherXUL = (const nsStyleXUL*)aOther->GetStyleData(eStyleStruct_XUL);
      hint = xul->CalcDifference(*otherXUL);
      if (aHint < hint) {
        aHint = hint;
      }
    }
#endif

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleTable* table = (const nsStyleTable*)GetStyleData(eStyleStruct_Table);
      const nsStyleTable* otherTable = (const nsStyleTable*)aOther->GetStyleData(eStyleStruct_Table);
      hint = table->CalcDifference(*otherTable);
      if (aHint < hint) {
        aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleTableBorder* table = (const nsStyleTableBorder*)GetStyleData(eStyleStruct_TableBorder);
      const nsStyleTableBorder* otherTable = (const nsStyleTableBorder*)aOther->GetStyleData(eStyleStruct_TableBorder);
      hint = table->CalcDifference(*otherTable);
      if (aHint < hint) {
        aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      hint = GETSCDATA(Content).CalcDifference(other->GETSCDATA(Content));
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      hint = GETSCDATA(UserInterface).CalcDifference(other->GETSCDATA(UserInterface));
      if (aHint < hint) {
        aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
  }
  return NS_OK;
}

void StyleContextImpl::List(FILE* out, PRInt32 aIndent)
{
  // Indent
  PRInt32 ix;
  for (ix = aIndent; --ix >= 0; ) fputs("  ", out);
  fprintf(out, "%p(%d) ", (void*)this, mRefCnt);
  if (nsnull != mPseudoTag) {
    nsAutoString  buffer;
    mPseudoTag->ToString(buffer);
    fputs(buffer, out);
    fputs(" ", out);
  }
  
  if (nsnull != mChild) {
    StyleContextImpl* child = mChild;
    do {
      child->List(out, aIndent + 1);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (nsnull != mEmptyChild) {
    StyleContextImpl* child = mEmptyChild;
    do {
      child->List(out, aIndent + 1);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}


/******************************************************************************
* SizeOf method:
*  
*  Self (reported as StyleContextImpl's size): 
*    1) sizeof(*this) which gets all of the data members
*    2) adds in the size of the PseudoTag, if there is one
*  
*  Contained / Aggregated data (not reported as StyleContextImpl's size):
*    1) the Style Rules in mRules are not counted as part of sizeof(*this)
*       (though the size of the nsISupportsArray ptr. is) so we need to 
*       count the rules seperately. For each rule in the mRules collection
*       we call the SizeOf method and let it report itself.
*
*  Children / siblings / parents:
*    1) We recurse over the mChild and mEmptyChild instances if they exist.
*       These instances will then be accumulated seperately (not part of 
*       the containing instance's size)
*    2) We recurse over the siblings of the Child and Empty Child instances
*       and count then seperately as well.
*    3) We recurse over our direct siblings (if any).
*   
******************************************************************************/
void StyleContextImpl::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  static PRBool bDetailDumpDone = PR_FALSE;
  if (!bDetailDumpDone) {
    bDetailDumpDone = PR_TRUE;
    PRUint32 totalSize=0;

    printf( "Detailed StyleContextImpl dump: basic class sizes of members\n" );
    printf( "*************************************\n");
    printf( " - StyleColorImpl:         %ld\n", (long)sizeof(GETSCDATA(Color)) );
    totalSize += (long)sizeof(GETSCDATA(Color));
    printf( " - StyleTextImpl:          %ld\n", (long)sizeof(GETSCDATA(Text)) );
    totalSize += (long)sizeof(GETSCDATA(Text));
    printf( " - StyleDisplayImpl:       %ld\n", (long)sizeof(GETSCDATA(Display)) );
    totalSize += (long)sizeof(GETSCDATA(Display));
    printf( " - StyleContentImpl:       %ld\n", (long)sizeof(GETSCDATA(Content)) );
    totalSize += (long)sizeof(GETSCDATA(Content));
    printf( " - StyleUserInterfaceImpl: %ld\n", (long)sizeof(GETSCDATA(UserInterface)) );
    totalSize += (long)sizeof(GETSCDATA(UserInterface));
	  printf( " - Total:                  %ld\n", (long)totalSize);
    printf( "*************************************\n");
  }

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);

  if(! uniqueItems->AddItem((void*)this) ){
    // object has already been accounted for
    return;
  }

  PRUint32 localSize=0;

  // get or create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("StyleContextImpl"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  // add in the size of the member mPseudoTag
  if(mPseudoTag){
    mPseudoTag->SizeOf(aSizeOfHandler, &localSize);
    aSize += localSize;
  }
  aSizeOfHandler->AddSize(tag,aSize);

  // now follow up with the child (and empty child) recursion
  if (nsnull != mChild) {
    StyleContextImpl* child = mChild;
    do {
      child->SizeOf(aSizeOfHandler, localSize);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (nsnull != mEmptyChild) {
    StyleContextImpl* child = mEmptyChild;
    do {
      child->SizeOf(aSizeOfHandler, localSize);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
  // and finally our direct siblings (if any)
  if (nsnull != mNextSibling) {
    mNextSibling->SizeOf(aSizeOfHandler, localSize);
  }
}

#ifdef DEBUG
static void IndentBy(FILE* out, PRInt32 aIndent) {
  while (--aIndent >= 0) fputs("  ", out);
}
// virtual 
void StyleContextImpl::DumpRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  nsAutoString str;

  // FONT
  IndentBy(out,aIndent);
 // fprintf(out, "<font %s %s %d />\n", 
 //         NS_ConvertUCS2toUTF8(GETSCDATA(Font).mFont.name).get(),
  //        NS_ConvertUCS2toUTF8(GETSCDATA(Font).mFixedFont.name).get(),
  //        GETSCDATA(Font).mFlags);

  // COLOR
  IndentBy(out,aIndent);
  fprintf(out, "<color data=\"%ld %d %d %d %ld %ld %ld %s %d %s %f\"/>\n", 
    (long)GETSCDATA(Color).mColor,
    (int)GETSCDATA(Color).mBackgroundAttachment,
    (int)GETSCDATA(Color).mBackgroundFlags,
    (int)GETSCDATA(Color).mBackgroundRepeat,
    (long)GETSCDATA(Color).mBackgroundColor,
    (long)GETSCDATA(Color).mBackgroundXPosition,
    (long)GETSCDATA(Color).mBackgroundYPosition,
    NS_ConvertUCS2toUTF8(GETSCDATA(Color).mBackgroundImage).get(),
    (int)GETSCDATA(Color).mCursor,
    NS_ConvertUCS2toUTF8(GETSCDATA(Color).mCursorImage).get(),
    GETSCDATA(Color).mOpacity);

  // SPACING (ie. margin, padding, border, outline)
  IndentBy(out,aIndent);
  fprintf(out, "<spacing data=\"");

 /* GETSCDATA(Margin).mMargin.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Padding).mPadding.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Border).mBorder.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Border).mBorderRadius.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Outline).mOutlineRadius.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Outline).mOutlineWidth.ToString(str);
  fprintf(out, "%s", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "%d", (int)GETSCDATA(Border).mFloatEdge);
  fprintf(out, "\" />\n");


  // LIST
  IndentBy(out,aIndent);
  fprintf(out, "<list data=\"%d %d %s\" />\n",
    (int)GETSCDATA(List).mListStyleType,
    (int)GETSCDATA(List).mListStyleType,
    NS_ConvertUCS2toUTF8(GETSCDATA(List).mListStyleImage).get());

  // POSITION
  IndentBy(out,aIndent);
  fprintf(out, "<position data=\"%d ", (int)GETSCDATA(Position).mPosition);
  GETSCDATA(Position).mOffset.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mMinWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mMaxWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mMinHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Position).mMaxHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "%d ", (int)GETSCDATA(Position).mBoxSizing);
  GETSCDATA(Position).mZIndex.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "\" />\n");
*/

  // TEXT
  IndentBy(out,aIndent);
  fprintf(out, "<text data=\"%d %d %d %d ",
    (int)GETSCDATA(Text).mTextAlign,
    (int)GETSCDATA(Text).mTextTransform,
    (int)GETSCDATA(Text).mWhiteSpace);
  GETSCDATA(Text).mLetterSpacing.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Text).mLineHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Text).mTextIndent.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Text).mWordSpacing.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Text).mVerticalAlign.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "\" />\n");
  
  // DISPLAY
  IndentBy(out,aIndent);
  fprintf(out, "<display data=\"%d %d %d %d %d %d %d %d %d %ld %ld %ld %ld\" />\n",
    (int)GETSCDATA(Display).mDirection,
    (int)GETSCDATA(Display).mDisplay,
    (int)GETSCDATA(Display).mFloats,
    (int)GETSCDATA(Display).mBreakType,
    (int)GETSCDATA(Display).mBreakBefore,
    (int)GETSCDATA(Display).mBreakAfter,
    (int)GETSCDATA(Display).mVisible,
    (int)GETSCDATA(Display).mOverflow,
    (int)GETSCDATA(Display).mClipFlags,
    (long)GETSCDATA(Display).mClip.x,
    (long)GETSCDATA(Display).mClip.y,
    (long)GETSCDATA(Display).mClip.width,
    (long)GETSCDATA(Display).mClip.height
    );
  
  // TABLE
  /*IndentBy(out,aIndent);
  fprintf(out, "<table data=\"%d %d %d %d ",
    (int)GETSCDATA(Table).mLayoutStrategy,
    (int)GETSCDATA(Table).mFrame,
    (int)GETSCDATA(Table).mRules,
    (int)GETSCDATA(Table).mBorderCollapse);
  GETSCDATA(Table).mBorderSpacingX.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  GETSCDATA(Table).mBorderSpacingY.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "%d %d %ld %ld ",
    (int)GETSCDATA(Table).mCaptionSide,
    (int)GETSCDATA(Table).mEmptyCells,
    (long)GETSCDATA(Table).mCols,
    (long)GETSCDATA(Table).mSpan);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "\" />\n");
  */

  // CONTENT
  IndentBy(out,aIndent);
  fprintf(out, "<content data=\"%ld %ld %ld %ld ",
    (long)GETSCDATA(Content).ContentCount(),
    (long)GETSCDATA(Content).CounterIncrementCount(),
    (long)GETSCDATA(Content).CounterResetCount(),
    (long)GETSCDATA(Content).QuotesCount());
  // XXX: iterate over the content, counters and quotes...
  GETSCDATA(Content).mMarkerOffset.ToString(str);
  fprintf(out, "%s ", NS_ConvertUCS2toUTF8(str).get());
  fprintf(out, "\" />\n");

  // UI
  IndentBy(out,aIndent);
  fprintf(out, "<UI data=\"%d %d %d %d %d %d %s\" />\n",
    (int)GETSCDATA(UserInterface).mUserInput,
    (int)GETSCDATA(UserInterface).mUserModify,
    (int)GETSCDATA(UserInterface).mUserSelect,
    (int)GETSCDATA(UserInterface).mUserFocus,
    (int)GETSCDATA(UserInterface).mKeyEquivalent,
    (int)GETSCDATA(UserInterface).mResizer,
    NS_ConvertUCS2toUTF8(GETSCDATA(UserInterface).mBehavior).get());

}
#endif

// Overloaded new operator. Initializes the memory to 0 and relies on an arena
// (which comes from the presShell) to perform the allocation.
void* 
StyleContextImpl::operator new(size_t sz, nsIPresContext* aPresContext)
{
  // Check the recycle list first.
  void* result = nsnull;
  aPresContext->AllocateFromShell(sz, &result);
  return result;
}

// Overridden to prevent the global delete from being called, since the memory
// came out of an nsIArena instead of the global delete operator's heap.
void 
StyleContextImpl::Destroy()
{
  // Get the pres context from our rule node.
  nsCOMPtr<nsIPresContext> presContext;
  mRuleNode->GetPresContext(getter_AddRefs(presContext));

  // Call our destructor.
  this->~StyleContextImpl();

  // Don't let the memory be freed, since it will be recycled
  // instead. Don't call the global operator delete.
  presContext->FreeToShell(sizeof(StyleContextImpl), this);
}

NS_LAYOUT nsresult
NS_NewStyleContext(nsIStyleContext** aInstancePtrResult,
                   nsIStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsIRuleNode* aRuleNode,
                   nsIPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");
  if (nsnull == aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  StyleContextImpl* context = new (aPresContext) StyleContextImpl(aParentContext, aPseudoTag, 
                                                                  aRuleNode, aPresContext);
  if (nsnull == context) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult result = context->QueryInterface(NS_GET_IID(nsIStyleContext), (void **) aInstancePtrResult);
  context->RemapStyle(aPresContext);  // remap after initial ref-count is set

  return result;
}


//----------------------------------------------------------

void EnsureBlockDisplay(/*in out*/PRUint8 &display)
{
  // see if the display value is already a block
  switch (display) {
  case NS_STYLE_DISPLAY_NONE :
    // never change display:none *ever*
    break;

  case NS_STYLE_DISPLAY_TABLE :
  case NS_STYLE_DISPLAY_BLOCK :
    // do not muck with these at all - already blocks
    break;

  case NS_STYLE_DISPLAY_LIST_ITEM :
    // do not change list items to blocks - retain the bullet/numbering
    break;

  case NS_STYLE_DISPLAY_TABLE_ROW_GROUP :
  case NS_STYLE_DISPLAY_TABLE_COLUMN :
  case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP :
  case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP :
  case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP :
  case NS_STYLE_DISPLAY_TABLE_ROW :
  case NS_STYLE_DISPLAY_TABLE_CELL :
  case NS_STYLE_DISPLAY_TABLE_CAPTION :
    // special cases: don't do anything since these cannot really be floated anyway
    break;

  case NS_STYLE_DISPLAY_INLINE_TABLE :
    // make inline tables into tables
    display = NS_STYLE_DISPLAY_TABLE;
    break;

  default :
    // make it a block
    display = NS_STYLE_DISPLAY_BLOCK;
  }
}
