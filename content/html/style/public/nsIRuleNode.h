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
 * Original Author: David W. Hyatt (hyatt@netscape.com)
 *
 * Contributor(s): 
 */
#ifndef nsIRuleNode_h___
#define nsIRuleNode_h___

#include "nsCOMPtr.h"
#include "nsHashtable.h"
#include "nsIStyleRule.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsICSSDeclaration.h"

struct nsCachedStyleData
{
  nsStyleFont* mFontData;
  nsStyleMargin* mMarginData;
  nsStyleBorder* mBorderData;
  nsStylePadding* mPaddingData;
  nsStyleOutline* mOutlineData;
  nsStyleList* mListData;
  nsStylePosition* mPositionData;
#ifdef INCLUDE_XUL
  nsStyleXUL* mXULData;
#endif

  void* operator new(size_t sz, nsIPresContext* aContext) {
    void* result = nsnull;
    aContext->AllocateFromShell(sz, &result);
    if (result)
    nsCRT::zero(result, sz);
    return result;
  }
  void Destroy(nsIPresContext* aContext) {
    if (mFontData)
      mFontData->Destroy(aContext);
    if (mMarginData)
      mMarginData->Destroy(aContext);
    if (mBorderData)
      mBorderData->Destroy(aContext);
    if (mPaddingData)
      mPaddingData->Destroy(aContext);
    if (mOutlineData)
      mOutlineData->Destroy(aContext);
    if (mListData)
      mListData->Destroy(aContext);
    if (mPositionData)
      mPositionData->Destroy(aContext);
#ifdef INCLUDE_XUL
    if (mXULData)
      mXULData->Destroy(aContext);
#endif
    aContext->FreeToShell(sizeof(nsCachedStyleData), this);
  };

  static PRBool IsReset(const nsStyleStructID& aSID);
  static PRUint32 nsCachedStyleData::GetBitForSID(const nsStyleStructID& aSID)
  {
    switch (aSID) {
      case eStyleStruct_Font:
        return NS_STYLE_INHERIT_FONT;   
      case eStyleStruct_Color:
        return NS_STYLE_INHERIT_COLOR;
      case eStyleStruct_List:
        return NS_STYLE_INHERIT_LIST;
      case eStyleStruct_Position:
        return NS_STYLE_INHERIT_POSITION;
      case eStyleStruct_Text:
        return NS_STYLE_INHERIT_TEXT;
      case eStyleStruct_Display:
        return NS_STYLE_INHERIT_DISPLAY;
      case eStyleStruct_Table:
        return NS_STYLE_INHERIT_TABLE;
      case eStyleStruct_Content:
        return NS_STYLE_INHERIT_CONTENT;
      case eStyleStruct_UserInterface:
        return NS_STYLE_INHERIT_UI;
      case eStyleStruct_Print:
    	  return NS_STYLE_INHERIT_PRINT;
      case eStyleStruct_Margin:
    	  return NS_STYLE_INHERIT_MARGIN;
      case eStyleStruct_Padding:
    	  return NS_STYLE_INHERIT_PADDING;
      case eStyleStruct_Border:
    	  return NS_STYLE_INHERIT_BORDER;
      case eStyleStruct_Outline:
    	  return NS_STYLE_INHERIT_OUTLINE;
#ifdef INCLUDE_XUL
      case eStyleStruct_XUL:
    	  return NS_STYLE_INHERIT_XUL;
#endif
    }

    return 0;
  };
  nsStyleStruct* GetStyleData(const nsStyleStructID& aSID) {
    switch (aSID) {
      case eStyleStruct_Font:
        return mFontData;
      case eStyleStruct_Margin:
    	  return mMarginData;
      case eStyleStruct_Padding:
    	  return mPaddingData;
      case eStyleStruct_Border:
    	  return mBorderData;
      case eStyleStruct_Outline:
    	  return mOutlineData;
      case eStyleStruct_List:
        return mListData;
      case eStyleStruct_Position:
        return mPositionData;
#ifdef INCLUDE_XUL
      case eStyleStruct_XUL:
    	  return mXULData;
#endif
    }
    return nsnull;
  };

  nsCachedStyleData() :mFontData(nsnull), mMarginData(nsnull), mBorderData(nsnull), mPaddingData(nsnull), 
    mOutlineData(nsnull), mListData(nsnull), mPositionData(nsnull) {
#ifdef INCLUDE_XUL
    mXULData = nsnull;
#endif
  };
  ~nsCachedStyleData() {};
};

struct nsRuleData
{
  nsStyleStructID mSID;
  nsIPresContext* mPresContext;
  nsIStyleContext* mStyleContext;

  nsCSSFont* mFontData; // Should always be stack-allocated! We don't own these structures!
  nsCSSMargin* mMarginData;
  nsCSSList* mListData;
  nsCSSPosition* mPositionData;
#ifdef INCLUDE_XUL
  nsCSSXUL* mXULData;
#endif

  nsRuleData(const nsStyleStructID& aSID, nsIPresContext* aContext, nsIStyleContext* aStyleContext) 
    :mSID(aSID), mPresContext(aContext), mStyleContext(aStyleContext),
     mFontData(nsnull), mMarginData(nsnull), mListData(nsnull), mPositionData(nsnull)
  {
#ifdef INCLUDE_XUL
    mXULData = nsnull;
#endif
  };
  ~nsRuleData() {};
};

// {ED86146A-00A0-4a9d-B075-91C164D8FF20}
#define NS_IRULENODE_IID \
{ 0xed86146a, 0xa0, 0x4a9d, { 0xb0, 0x75, 0x91, 0xc1, 0x64, 0xd8, 0xff, 0x20 } }

class nsIRuleNode : public nsISupports {
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IRULENODE_IID; return iid; }

  NS_IMETHOD Transition(nsIStyleRule* aRule, nsIRuleNode** aResult)=0;
  
  NS_IMETHOD GetParent(nsIRuleNode** aResult)=0;
  NS_IMETHOD IsRoot(PRBool* aResult)=0;

  NS_IMETHOD GetRule(nsIStyleRule** aResult)=0;

  NS_IMETHOD GetPresContext(nsIPresContext** aResult)=0;

  virtual const nsStyleStruct* GetStyleData(nsStyleStructID aSID, 
                                            nsIStyleContext* aContext,
                                            nsIMutableStyleContext* aMutableContext)=0;
};

#endif
