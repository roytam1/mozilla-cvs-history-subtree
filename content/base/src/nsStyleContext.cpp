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
    mCursor = aParent->mCursor; // fix for bugzilla bug 51113
    mCursorImage = aParent->mCursorImage; // fix for bugzilla bug 51113
  }
  else {
    mUserInput = NS_STYLE_USER_INPUT_AUTO;
    mUserModify = NS_STYLE_USER_MODIFY_READ_ONLY;
    mUserFocus = NS_STYLE_USER_FOCUS_NONE;

    mCursor = NS_STYLE_CURSOR_AUTO; // fix for bugzilla bug 51113
  }

  mUserSelect = NS_STYLE_USER_SELECT_AUTO;
  mKeyEquivalent = PRUnichar(0); // XXX what type should this be?
  mResizer = NS_STYLE_RESIZER_AUTO;
}

void StyleUserInterfaceImpl::SetFrom(const nsStyleUserInterface& aSource)
{
  mUserInput = aSource.mUserInput;
  mUserModify = aSource.mUserModify;
  mUserFocus = aSource.mUserFocus;

  mUserSelect = aSource.mUserSelect;
  mKeyEquivalent = aSource.mKeyEquivalent;
  mResizer = aSource.mResizer;
  
  mCursor = aSource.mCursor;
  mCursorImage = aSource.mCursorImage;
}

void StyleUserInterfaceImpl::CopyTo(nsStyleUserInterface& aDest) const
{
  aDest.mUserInput = mUserInput;
  aDest.mUserModify = mUserModify;
  aDest.mUserFocus = mUserFocus;

  aDest.mUserSelect = mUserSelect;
  aDest.mKeyEquivalent = mKeyEquivalent;
  aDest.mResizer = mResizer;
  
  aDest.mCursor = mCursor;
  aDest.mCursorImage = mCursorImage;
}

PRInt32 StyleUserInterfaceImpl::CalcDifference(const StyleUserInterfaceImpl& aOther) const
{
  if ((mCursor != aOther.mCursor) ||
      (mCursorImage != aOther.mCursorImage))
    return NS_STYLE_HINT_VISUAL;

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
  NS_IMETHOD GetFirstChild(nsIStyleContext** aContext);

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
  virtual nsStyleStruct* GetUniqueStyleData(nsIPresContext* aPresContext, const nsStyleStructID& aSID);

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

  // the style data...
  StyleTextImpl           mText;
  StyleUserInterfaceImpl  mUserInterface;
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
    mText(),
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
  if (mCachedStyleData.mResetData || mCachedStyleData.mInheritedData) {
    nsCOMPtr<nsIPresContext> presContext;
    mRuleNode->GetPresContext(getter_AddRefs(presContext));
    mCachedStyleData.Destroy(mInheritBits, presContext);
  }
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

NS_IMETHODIMP
StyleContextImpl::GetFirstChild(nsIStyleContext** aContext)
{
  *aContext = mChild;
  NS_IF_ADDREF(*aContext);
  return NS_OK;
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
          if ((!(child->mInheritBits & NS_STYLE_UNIQUE_CONTEXT)) &&  // only look at children with un-twiddled data
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
        if ((!(child->mInheritBits & NS_STYLE_UNIQUE_CONTEXT)) &&  // only look at children with un-twiddled data
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
    else if (mInheritBits != other->mInheritBits) {
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
      
  switch (aSID) {
    case eStyleStruct_Display:
    case eStyleStruct_Visibility:
    case eStyleStruct_Font:
    case eStyleStruct_Color:
    case eStyleStruct_Background:
    case eStyleStruct_Margin:
    case eStyleStruct_Border:
    case eStyleStruct_Padding:
    case eStyleStruct_Outline:
    case eStyleStruct_List:
    case eStyleStruct_Content:
    case eStyleStruct_Quotes:
    case eStyleStruct_Position:
    case eStyleStruct_Table:
    case eStyleStruct_TableBorder:
    case eStyleStruct_XUL: {
      const nsStyleStruct* cachedData = mCachedStyleData.GetStyleData(aSID); 
      if (cachedData)
        return cachedData; // We have computed data stored on this node in the context tree.
      return mRuleNode->GetStyleData(aSID, this, this); // Our rule node will take care of it for us.
    }
    case eStyleStruct_Text:
      result = & GETSCDATA(Text);
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

// This is an evil evil function, since it forces you to alloc your own separate copy of
// style data!  Do not use this function unless you absolutely have to!  You should avoid
// this at all costs! -dwh
nsStyleStruct* 
StyleContextImpl::GetUniqueStyleData(nsIPresContext* aPresContext, const nsStyleStructID& aSID)
{
  nsStyleStruct* result = nsnull;
  switch (aSID) {
  case eStyleStruct_Background: {
    if (mCachedStyleData.mResetData && mCachedStyleData.mResetData->mBackgroundData)
      result = mCachedStyleData.mResetData->mBackgroundData;
    else {
      const nsStyleBackground* bg = (const nsStyleBackground*)GetStyleData(aSID);
      nsStyleBackground* newBG = new (aPresContext) nsStyleBackground(*bg);
      SetStyle(aSID, *newBG);
      result = newBG;
    }
    break;
  }
  default:
    NS_ERROR("Struct type not supported.  Please find another way to do this if you can!\n");
  }

  return result;
}

nsStyleStruct* StyleContextImpl::GetMutableStyleData(nsStyleStructID aSID)
{
  // XXXdwh ELIMINATE ME!!!
  nsStyleStruct*  result = nsnull;

    switch (aSID) {
    case eStyleStruct_Font:
    case eStyleStruct_Color:
    case eStyleStruct_Background:
    case eStyleStruct_Border:
    case eStyleStruct_Margin:
    case eStyleStruct_Padding:
    case eStyleStruct_Outline:
    case eStyleStruct_List:
    case eStyleStruct_Content:
    case eStyleStruct_Quotes:
    case eStyleStruct_XUL:
    case eStyleStruct_Position:
    case eStyleStruct_Table:
    case eStyleStruct_TableBorder:
    case eStyleStruct_Display:
    case eStyleStruct_Visibility:
      NS_ERROR("YOU CANNOT CALL THIS!  IT'S GOING TO BE REMOVED!\n");
      return nsnull;
    case eStyleStruct_Text:
      result = & GETSCDATA(Text);
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
      mCachedStyleData.mInheritedData->mColorData = (nsStyleColor*)(const nsStyleColor*)(&aStruct);
      break;
    case eStyleStruct_Background:
      mCachedStyleData.mResetData->mBackgroundData = (nsStyleBackground*)(const nsStyleBackground*)(&aStruct);
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
      mCachedStyleData.mResetData->mDisplayData = (nsStyleDisplay*)(const nsStyleDisplay*)(&aStruct);
      break;
    case eStyleStruct_Visibility:
      mCachedStyleData.mInheritedData->mVisibilityData = (nsStyleVisibility*)(const nsStyleVisibility*)(&aStruct);
      break;
    case eStyleStruct_Table:
      mCachedStyleData.mResetData->mTableData = (nsStyleTable*)(const nsStyleTable*)(&aStruct);
      break;
    case eStyleStruct_TableBorder:
      mCachedStyleData.mInheritedData->mTableData = (nsStyleTableBorder*)(const nsStyleTableBorder*)(&aStruct);
      break;
    case eStyleStruct_Content:
      mCachedStyleData.mResetData->mContentData = (nsStyleContent*)(const nsStyleContent*)(&aStruct);
      break;
    case eStyleStruct_Quotes:
      mCachedStyleData.mInheritedData->mQuotesData = (nsStyleQuotes*)(const nsStyleQuotes*)(&aStruct);
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
  if (nsnull != mParent) {
    GETSCDATA(Text).ResetFrom(&(mParent->GETSCDATA(Text)), aPresContext);
    GETSCDATA(UserInterface).ResetFrom(&(mParent->GETSCDATA(UserInterface)), aPresContext);
  }
  else {
    GETSCDATA(Text).ResetFrom(nsnull, aPresContext);
    GETSCDATA(UserInterface).ResetFrom(nsnull, aPresContext);
  }

  PRBool isRoot = PR_FALSE;
  mRuleNode->IsRoot(&isRoot);
  if (!isRoot) {
    MapStyleData  data(this, aPresContext);
    MapStyleRule(&data, mRuleNode);
  }


  // Even in strict mode, we still have to support one "quirky" thing
  // for tables.  HTML's alignment attributes have always worked
  // so they don't inherit into tables, but instead align the
  // tables.  We should keep doing this, because HTML alignment
  // is just weird, and we shouldn't force it to match CSS.
  nsStyleDisplay* disp = (nsStyleDisplay*)GetStyleData(eStyleStruct_Display);
  if (disp->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    // -moz-center and -moz-right are used for HTML's alignment
    if ((GETSCDATA(Text).mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER) ||
        (GETSCDATA(Text).mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT))
    {
      GETSCDATA(Text).mTextAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
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
  mInheritBits |= NS_STYLE_UNIQUE_CONTEXT;
}

NS_IMETHODIMP
StyleContextImpl::CalcStyleDifference(nsIStyleContext* aOther, PRInt32& aHint,PRBool aStopAtFirstDifference /*= PR_FALSE*/)
{
  if (aOther) {
    PRInt32 hint;
    const StyleContextImpl* other = (const StyleContextImpl*)aOther;

    const nsStyleFont* font = (const nsStyleFont*)GetStyleData(eStyleStruct_Font);
    const nsStyleFont* otherFont = (const nsStyleFont*)aOther->GetStyleData(eStyleStruct_Font);
    if (font != otherFont)
      aHint = font->CalcDifference(*otherFont);
    
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleColor* color = (const nsStyleColor*)GetStyleData(eStyleStruct_Color);
      const nsStyleColor* otherColor = (const nsStyleColor*)aOther->GetStyleData(eStyleStruct_Color);
      if (color != otherColor) {
        hint = color->CalcDifference(*otherColor);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleBackground* background = (const nsStyleBackground*)GetStyleData(eStyleStruct_Background);
      const nsStyleBackground* otherBackground = (const nsStyleBackground*)aOther->GetStyleData(eStyleStruct_Background);
      if (background != otherBackground) {
        hint = background->CalcDifference(*otherBackground);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleMargin* margin = (const nsStyleMargin*)GetStyleData(eStyleStruct_Margin);
      const nsStyleMargin* otherMargin = (const nsStyleMargin*)aOther->GetStyleData(eStyleStruct_Margin);
      if (margin != otherMargin) {
        hint = margin->CalcDifference(*otherMargin);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStylePadding* padding = (const nsStylePadding*)GetStyleData(eStyleStruct_Padding);
      const nsStylePadding* otherPadding = (const nsStylePadding*)aOther->GetStyleData(eStyleStruct_Padding);
      if (padding != otherPadding) {
        hint = padding->CalcDifference(*otherPadding);
        if (aHint < hint)
          aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleBorder* border = (const nsStyleBorder*)GetStyleData(eStyleStruct_Border);
      const nsStyleBorder* otherBorder = (const nsStyleBorder*)aOther->GetStyleData(eStyleStruct_Border);
      if (border != otherBorder) {
        hint = border->CalcDifference(*otherBorder);
        if (aHint < hint)
          aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleOutline* outline = (const nsStyleOutline*)GetStyleData(eStyleStruct_Outline);
      const nsStyleOutline* otherOutline = (const nsStyleOutline*)aOther->GetStyleData(eStyleStruct_Outline);
      if (outline != otherOutline) {
        hint = outline->CalcDifference(*otherOutline);
        if (aHint < hint)
          aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleList* list = (const nsStyleList*)GetStyleData(eStyleStruct_List);
      const nsStyleList* otherList = (const nsStyleList*)aOther->GetStyleData(eStyleStruct_List);
      if (list != otherList) {
        hint = list->CalcDifference(*otherList);
        if (aHint < hint)
          aHint = hint;
      }
    }
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStylePosition* pos = (const nsStylePosition*)GetStyleData(eStyleStruct_Position);
      const nsStylePosition* otherPosition = (const nsStylePosition*)aOther->GetStyleData(eStyleStruct_Position);
      if (pos != otherPosition) {
        hint = pos->CalcDifference(*otherPosition);
        if (aHint < hint)
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
      const nsStyleVisibility* vis = (const nsStyleVisibility*)GetStyleData(eStyleStruct_Visibility);
      const nsStyleVisibility* otherVis = (const nsStyleVisibility*)aOther->GetStyleData(eStyleStruct_Visibility);
      if (vis != otherVis) {
        hint = vis->CalcDifference(*otherVis);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleDisplay* display = (const nsStyleDisplay*)GetStyleData(eStyleStruct_Display);
      const nsStyleDisplay* otherDisplay = (const nsStyleDisplay*)aOther->GetStyleData(eStyleStruct_Display);
      if (display != otherDisplay) {
        hint = display->CalcDifference(*otherDisplay);
        if (aHint < hint)
          aHint = hint;
      }
    }

#ifdef INCLUDE_XUL
    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleXUL* xul = (const nsStyleXUL*)GetStyleData(eStyleStruct_XUL);
      const nsStyleXUL* otherXUL = (const nsStyleXUL*)aOther->GetStyleData(eStyleStruct_XUL);
      if (xul != otherXUL) {
        hint = xul->CalcDifference(*otherXUL);
        if (aHint < hint)
          aHint = hint;
      }
    }
#endif

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleTable* table = (const nsStyleTable*)GetStyleData(eStyleStruct_Table);
      const nsStyleTable* otherTable = (const nsStyleTable*)aOther->GetStyleData(eStyleStruct_Table);
      if (table != otherTable) {
        hint = table->CalcDifference(*otherTable);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleTableBorder* table = (const nsStyleTableBorder*)GetStyleData(eStyleStruct_TableBorder);
      const nsStyleTableBorder* otherTable = (const nsStyleTableBorder*)aOther->GetStyleData(eStyleStruct_TableBorder);
      if (table != otherTable) {
        hint = table->CalcDifference(*otherTable);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleContent* content = (const nsStyleContent*)GetStyleData(eStyleStruct_Content);
      const nsStyleContent* otherContent = (const nsStyleContent*)aOther->GetStyleData(eStyleStruct_Content);
      if (content != otherContent) {
        hint = content->CalcDifference(*otherContent);
        if (aHint < hint)
          aHint = hint;
      }
    }

    if (aStopAtFirstDifference && aHint > NS_STYLE_HINT_NONE) return NS_OK;
    if (aHint < NS_STYLE_HINT_MAX) {
      const nsStyleQuotes* content = (const nsStyleQuotes*)GetStyleData(eStyleStruct_Quotes);
      const nsStyleQuotes* otherContent = (const nsStyleQuotes*)aOther->GetStyleData(eStyleStruct_Quotes);
      if (content != otherContent) {
        hint = content->CalcDifference(*otherContent);
        if (aHint < hint)
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
    printf( " - StyleTextImpl:          %ld\n", (long)sizeof(GETSCDATA(Text)) );
    totalSize += (long)sizeof(GETSCDATA(Text));
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

