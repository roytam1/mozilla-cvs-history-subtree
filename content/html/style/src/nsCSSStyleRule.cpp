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
 *   Daniel Glazman <glazman@netscape.com>
 */
#include "nsCOMPtr.h"
#include "nsCSSRule.h"
#include "nsICSSStyleRule.h"
#include "nsICSSDeclaration.h"
#include "nsICSSStyleSheet.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsIHTMLContentContainer.h"
#include "nsIURL.h"
#include "nsIStyleContext.h"
#include "nsIMutableStyleContext.h"
#include "nsIPresContext.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsIArena.h"
#include "nsIAtom.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsStyleConsts.h"
#include "nsHTMLAtoms.h"
#include "nsUnitConversion.h"
#include "nsStyleUtil.h"
#include "nsIFontMetrics.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsDOMCSSDeclaration.h"
#include "nsINameSpaceManager.h"
#include "nsINameSpace.h"
#include "nsILookAndFeel.h"
#include "xp_core.h"
#include "nsIRuleNode.h"

#include "nsIStyleSet.h"
#include "nsISizeOfHandler.h"

#include "nsContentUtils.h"

// MJA: bug 31816
#include "nsIPresShell.h"
#include "nsIDocShellTreeItem.h"
// - END MJA

// #define DEBUG_REFS

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

static NS_DEFINE_IID(kCSSFontSID, NS_CSS_FONT_SID);
static NS_DEFINE_IID(kCSSColorSID, NS_CSS_COLOR_SID);
static NS_DEFINE_IID(kCSSTextSID, NS_CSS_TEXT_SID);
static NS_DEFINE_IID(kCSSMarginSID, NS_CSS_MARGIN_SID);
static NS_DEFINE_IID(kCSSPositionSID, NS_CSS_POSITION_SID);
static NS_DEFINE_IID(kCSSListSID, NS_CSS_LIST_SID);
static NS_DEFINE_IID(kCSSDisplaySID, NS_CSS_DISPLAY_SID);
static NS_DEFINE_IID(kCSSTableSID, NS_CSS_TABLE_SID);
static NS_DEFINE_IID(kCSSContentSID, NS_CSS_CONTENT_SID);
static NS_DEFINE_IID(kCSSUserInterfaceSID, NS_CSS_USER_INTERFACE_SID);
static NS_DEFINE_IID(kCSSBreaksSID, NS_CSS_BREAKS_SID);
static NS_DEFINE_IID(kCSSPageSID, NS_CSS_PAGE_SID);
#ifdef INCLUDE_XUL
static NS_DEFINE_IID(kCSSXULSID, NS_CSS_XUL_SID);
#endif

// -- nsCSSSelector -------------------------------

#define NS_IF_COPY(dest,source,type)  \
  if (nsnull != source)  dest = new type(*(source))

#define NS_IF_DELETE(ptr)   \
  if (nsnull != ptr) { delete ptr; ptr = nsnull; }

#define NS_IF_NEGATED_START(bool,str)  \
  if (bool) { str.Append(NS_LITERAL_STRING(":not(")); }

#define NS_IF_NEGATED_END(bool,str)  \
  if (bool) { str.Append(PRUnichar(')')); }

MOZ_DECL_CTOR_COUNTER(nsAtomList)

nsAtomList::nsAtomList(nsIAtom* aAtom)
  : mAtom(aAtom),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
  NS_IF_ADDREF(mAtom);
}

nsAtomList::nsAtomList(const nsString& aAtomValue)
  : mAtom(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
  mAtom = NS_NewAtom(aAtomValue);
}

nsAtomList::nsAtomList(const nsAtomList& aCopy)
  : mAtom(aCopy.mAtom),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAtomList);
  NS_IF_ADDREF(mAtom);
  NS_IF_COPY(mNext, aCopy.mNext, nsAtomList);
}

nsAtomList::~nsAtomList(void)
{
  MOZ_COUNT_DTOR(nsAtomList);
  NS_IF_RELEASE(mAtom);
  NS_IF_DELETE(mNext);
}

PRBool nsAtomList::Equals(const nsAtomList* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if (mAtom == aOther->mAtom) {
      if (nsnull != mNext) {
        return mNext->Equals(aOther->mNext);
      }
      return PRBool(nsnull == aOther->mNext);
    }
  }
  return PR_FALSE;
}

MOZ_DECL_CTOR_COUNTER(nsAttrSelector)

#ifdef DEBUG_REFS
PRUint32 gAttrSelectorCount=0;
#endif

nsAttrSelector::nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr)
  : mNameSpace(aNameSpace),
    mAttr(nsnull),
    mFunction(NS_ATTR_FUNC_SET),
    mCaseSensitive(1),
    mValue(),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

#ifdef DEBUG_REFS
  gAttrSelectorCount++;
  printf( "nsAttrSelector Instances (ctor): %ld\n", (long)gAttrSelectorCount);
#endif

  mAttr = NS_NewAtom(aAttr);
}

nsAttrSelector::nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunction, 
                               const nsString& aValue, PRBool aCaseSensitive)
  : mNameSpace(aNameSpace),
    mAttr(nsnull),
    mFunction(aFunction),
    mCaseSensitive(aCaseSensitive),
    mValue(aValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

#ifdef DEBUG_REFS
  gAttrSelectorCount++;
  printf( "nsAttrSelector Instances (ctor): %ld\n", (long)gAttrSelectorCount);
#endif

  mAttr = NS_NewAtom(aAttr);
}

nsAttrSelector::nsAttrSelector(const nsAttrSelector& aCopy)
  : mNameSpace(aCopy.mNameSpace),
    mAttr(aCopy.mAttr),
    mFunction(aCopy.mFunction),
    mCaseSensitive(aCopy.mCaseSensitive),
    mValue(aCopy.mValue),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

#ifdef DEBUG_REFS
  gAttrSelectorCount++;
  printf( "nsAttrSelector Instances (cp-ctor): %ld\n", (long)gAttrSelectorCount);
#endif

  NS_IF_ADDREF(mAttr);
  NS_IF_COPY(mNext, aCopy.mNext, nsAttrSelector);
}

nsAttrSelector::~nsAttrSelector(void)
{
  MOZ_COUNT_DTOR(nsAttrSelector);

#ifdef DEBUG_REFS
  gAttrSelectorCount--;
  printf( "nsAttrSelector Instances (dtor): %ld\n", (long)gAttrSelectorCount);
#endif

  NS_IF_RELEASE(mAttr);
  NS_IF_DELETE(mNext);
}

PRBool nsAttrSelector::Equals(const nsAttrSelector* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if ((mNameSpace == aOther->mNameSpace) &&
        (mAttr == aOther->mAttr) && 
        (mFunction == aOther->mFunction) && 
        (mCaseSensitive == aOther->mCaseSensitive) &&
        mValue.Equals(aOther->mValue)) {
      if (nsnull != mNext) {
        return mNext->Equals(aOther->mNext);
      }
      return PRBool(nsnull == aOther->mNext);
    }
  }
  return PR_FALSE;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as nsAttrSelector's size): 
*    1) sizeof(*this) + the size of mAttr atom (if it exists and is unique)
*
*  Contained / Aggregated data (not reported as nsAttrSelector's size):
*    none
*
*  Children / siblings / parents:
*    1) Recurses to the mMext instance which is reported as a seperate instance
*    
******************************************************************************/
void nsAttrSelector::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("nsAttrSelector"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);

  // add in the mAttr atom
  if (mAttr && uniqueItems->AddItem(mAttr)){
    mAttr->SizeOf(aSizeOfHandler, &localSize);
    aSize += localSize;
  }
  aSizeOfHandler->AddSize(tag,aSize);

  // recurse to the next one...
  if(mNext){
    mNext->SizeOf(aSizeOfHandler, localSize);
  }
}

MOZ_DECL_CTOR_COUNTER(nsCSSSelector)

#ifdef DEBUG_REFS
PRUint32 gSelectorCount=0;
#endif

nsCSSSelector::nsCSSSelector(void)
  : mNameSpace(kNameSpaceID_Unknown), mTag(nsnull), 
    mIDList(nsnull), 
    mClassList(nsnull), 
    mPseudoClassList(nsnull),
    mAttrList(nsnull), 
    mOperator(0),
    mNegations(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSelector);

#ifdef DEBUG_REFS
  gSelectorCount++;
  printf( "nsCSSSelector Instances (ctor): %ld\n", (long)gSelectorCount);
#endif
}

nsCSSSelector::nsCSSSelector(const nsCSSSelector& aCopy) 
  : mNameSpace(aCopy.mNameSpace), mTag(aCopy.mTag), 
    mIDList(nsnull), 
    mClassList(nsnull), 
    mPseudoClassList(nsnull),
    mAttrList(nsnull), 
    mOperator(aCopy.mOperator),
    mNegations(nsnull),
    mNext(nsnull)
{
  MOZ_COUNT_CTOR(nsCSSSelector);
  NS_IF_ADDREF(mTag);
  NS_IF_COPY(mIDList, aCopy.mIDList, nsAtomList);
  NS_IF_COPY(mClassList, aCopy.mClassList, nsAtomList);
  NS_IF_COPY(mPseudoClassList, aCopy.mPseudoClassList, nsAtomList);
  NS_IF_COPY(mAttrList, aCopy.mAttrList, nsAttrSelector);
  NS_IF_COPY(mNegations, aCopy.mNegations, nsCSSSelector);
  
#ifdef DEBUG_REFS
  gSelectorCount++;
  printf( "nsCSSSelector Instances (cp-ctor): %ld\n", (long)gSelectorCount);
#endif
}

nsCSSSelector::~nsCSSSelector(void)  
{
  MOZ_COUNT_DTOR(nsCSSSelector);
  Reset();

#ifdef DEBUG_REFS
  gSelectorCount--;
  printf( "nsCSSSelector Instances (dtor): %ld\n", (long)gSelectorCount);
#endif
}

nsCSSSelector& nsCSSSelector::operator=(const nsCSSSelector& aCopy)
{
  NS_IF_RELEASE(mTag);
  NS_IF_DELETE(mIDList);
  NS_IF_DELETE(mClassList);
  NS_IF_DELETE(mPseudoClassList);
  NS_IF_DELETE(mAttrList);
  NS_IF_DELETE(mNegations);
  
  mNameSpace    = aCopy.mNameSpace;
  mTag          = aCopy.mTag;
  NS_IF_COPY(mIDList, aCopy.mIDList, nsAtomList);
  NS_IF_COPY(mClassList, aCopy.mClassList, nsAtomList);
  NS_IF_COPY(mPseudoClassList, aCopy.mPseudoClassList, nsAtomList);
  NS_IF_COPY(mAttrList, aCopy.mAttrList, nsAttrSelector);
  mOperator     = aCopy.mOperator;
  NS_IF_COPY(mNegations, aCopy.mNegations, nsCSSSelector);

  NS_IF_ADDREF(mTag);
  return *this;
}

PRBool nsCSSSelector::Equals(const nsCSSSelector* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }
  if (nsnull != aOther) {
    if ((aOther->mNameSpace == mNameSpace) && 
        (aOther->mTag == mTag) && 
        (aOther->mOperator == mOperator)) {
      if (nsnull != mIDList) {
        if (PR_FALSE == mIDList->Equals(aOther->mIDList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mIDList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mClassList) {
        if (PR_FALSE == mClassList->Equals(aOther->mClassList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mClassList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mPseudoClassList) {
        if (PR_FALSE == mPseudoClassList->Equals(aOther->mPseudoClassList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mPseudoClassList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mAttrList) {
        if (PR_FALSE == mAttrList->Equals(aOther->mAttrList)) {
          return PR_FALSE;
        }
      }
      else {
        if (nsnull != aOther->mAttrList) {
          return PR_FALSE;
        }
      }
      if (nsnull != mNegations) {
        if (PR_FALSE == mNegations->Equals(aOther->mNegations)) {
          return PR_FALSE;
        }
      }
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


void nsCSSSelector::Reset(void)
{
  mNameSpace = kNameSpaceID_Unknown;
  NS_IF_RELEASE(mTag);
  NS_IF_DELETE(mIDList);
  NS_IF_DELETE(mClassList);
  NS_IF_DELETE(mPseudoClassList);
  NS_IF_DELETE(mAttrList);
  NS_IF_DELETE(mNegations);
  mOperator = PRUnichar(0);
}

void nsCSSSelector::SetNameSpace(PRInt32 aNameSpace)
{
  mNameSpace = aNameSpace;
}

void nsCSSSelector::SetTag(const nsString& aTag)
{
  NS_IF_RELEASE(mTag);
  if (0 < aTag.Length()) {
    mTag = NS_NewAtom(aTag);
  }
}

void nsCSSSelector::AddID(const nsString& aID)
{
  if (0 < aID.Length()) {
    nsAtomList** list = &mIDList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aID);
  }
}

void nsCSSSelector::AddClass(const nsString& aClass)
{
  if (0 < aClass.Length()) {
    nsAtomList** list = &mClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aClass);
  }
}

void nsCSSSelector::AddPseudoClass(const nsString& aPseudoClass)
{
  if (0 < aPseudoClass.Length()) {
    nsAtomList** list = &mPseudoClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aPseudoClass);
  }
}

void nsCSSSelector::AddPseudoClass(nsIAtom* aPseudoClass)
{
  if (nsnull != aPseudoClass) {
    nsAtomList** list = &mPseudoClassList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aPseudoClass);
  }
}

void nsCSSSelector::AddAttribute(PRInt32 aNameSpace, const nsString& aAttr)
{
  if (0 < aAttr.Length()) {
    nsAttrSelector** list = &mAttrList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr);
  }
}

void nsCSSSelector::AddAttribute(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunc, 
                                 const nsString& aValue, PRBool aCaseSensitive)
{
  if (0 < aAttr.Length()) {
    nsAttrSelector** list = &mAttrList;
    while (nsnull != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr, aFunc, aValue, aCaseSensitive);
  }
}

void nsCSSSelector::SetOperator(PRUnichar aOperator)
{
  mOperator = aOperator;
}

PRInt32 nsCSSSelector::CalcWeight(void) const
{
  PRInt32 weight = 0;

  if (nsnull != mTag) {
    weight += 0x000001;
  }
  nsAtomList* list = mIDList;
  while (nsnull != list) {
    weight += 0x010000;
    list = list->mNext;
  }
  list = mClassList;
  while (nsnull != list) {
    weight += 0x000100;
    list = list->mNext;
  }
  list = mPseudoClassList;
  while (nsnull != list) {
    weight += 0x000100;
    list = list->mNext;
  }
  nsAttrSelector* attr = mAttrList;
  while (nsnull != attr) {
    weight += 0x000100;
    attr = attr->mNext;
  }
  if (nsnull != mNegations) {
    weight += mNegations->CalcWeight();
  }
  return weight;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as nsCSSSelector's size): 
*    1) sizeof(*this) + the size of the mTag 
*       + the size of the mIDList unique items 
*       + the size of the mClassList and mPseudoClassList unique items
*
*  Contained / Aggregated data (not reported as nsCSSSelector's size):
*    1) AttributeList is called out to seperately if it exists
*
*  Children / siblings / parents:
*    1) Recurses to mNext which is counted as it's own instance
*    
******************************************************************************/
void nsCSSSelector::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("nsCSSSelector"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  
  // now get the member-atoms and add them in
  if(mTag && uniqueItems->AddItem(mTag)){
    localSize = 0;
    mTag->SizeOf(aSizeOfHandler, &localSize);
    aSize += localSize;
  }


  // XXX ????



  // a couple of simple atom lists
  if(mIDList && uniqueItems->AddItem(mIDList)){
    aSize += sizeof(*mIDList);
    nsAtomList *pNext = nsnull;
    pNext = mIDList;    
    while(pNext){
      if(pNext->mAtom && uniqueItems->AddItem(pNext->mAtom)){
        localSize = 0;
        pNext->mAtom->SizeOf(aSizeOfHandler, &localSize);
        aSize += localSize;
      }
      pNext = pNext->mNext;
    }
  }
  if(mClassList && uniqueItems->AddItem(mClassList)){
    aSize += sizeof(*mClassList);
    nsAtomList *pNext = nsnull;
    pNext = mClassList;    
    while(pNext){
      if(pNext->mAtom && uniqueItems->AddItem(pNext->mAtom)){
        localSize = 0;
        pNext->mAtom->SizeOf(aSizeOfHandler, &localSize);
        aSize += localSize;
      }
      pNext = pNext->mNext;
    }
  }
  if(mPseudoClassList && uniqueItems->AddItem(mPseudoClassList)){
    nsAtomList *pNext = nsnull;
    pNext = mPseudoClassList;    
    while(pNext){
      if(pNext->mAtom && uniqueItems->AddItem(pNext->mAtom)){
        localSize = 0;
        pNext->mAtom->SizeOf(aSizeOfHandler, &localSize);
        aSize += localSize;
      }
      pNext = pNext->mNext;
    }
  }
  // done with undelegated sizes 
  aSizeOfHandler->AddSize(tag, aSize);

  // the AttributeList gets its own delegation-call
  if(mAttrList){
    localSize = 0;
    mAttrList->SizeOf(aSizeOfHandler, localSize);
  }

  // don't forget the negated selectors
  if(mNegations) {
    localSize = 0;
    mNegations->SizeOf(aSizeOfHandler, localSize);
  }
  
  // finally chain to the next...
  if(mNext){
    localSize = 0;
    mNext->SizeOf(aSizeOfHandler, localSize);
  }
}

// pseudo-elements are stored in the selectors' chain using fictional elements;
// these fictional elements have mTag starting with a colon
static PRBool IsPseudoElement(nsIAtom* aAtom)
{
  if (aAtom) {
    const PRUnichar *str;
    aAtom->GetUnicode(&str);
    return str && (*str == ':');
  }

  return PR_FALSE;
}

void nsCSSSelector::AppendNegationToString(nsAWritableString& aString)
{
  aString.Append(NS_LITERAL_STRING(":not("));
}

//
// Builds the textual representation of a selector. Called by DOM 2 CSS 
// StyleRule:selectorText
//
nsresult nsCSSSelector::ToString( nsAWritableString& aString, nsICSSStyleSheet* aSheet, PRBool aIsPseudoElem,
                                  PRInt8 aNegatedIndex) const
{
  const PRUnichar* temp;
  PRBool aIsNegated = PRBool(0 < aNegatedIndex);

  // selectors are linked from right-to-left, so the next selector in the linked list
  // actually precedes this one in the resulting string
  if (mNext) {
    mNext->ToString(aString, aSheet, IsPseudoElement(mTag), PR_FALSE);
    if (!aIsNegated && !IsPseudoElement(mTag)) {
      // don't add a leading whitespace if we have a pseudo-element
      // or a negated simple selector
      aString.Append(PRUnichar(' '));
    }
  }
  if (1 < aNegatedIndex) {
    // the first mNegations does not contain a negated type element selector
    // or a negated universal selector
    NS_IF_NEGATED_START(aIsNegated, aString)
  }

  // append the namespace prefix
  if (mNameSpace > 0) {
    nsCOMPtr<nsINameSpace> sheetNS;
    aSheet->GetNameSpace(*getter_AddRefs(sheetNS));
    nsCOMPtr<nsIAtom> prefixAtom;
    // will return null if namespace was the default
    sheetNS->FindNameSpacePrefix(mNameSpace, *getter_AddRefs(prefixAtom));
    if (prefixAtom) {
      const PRUnichar* prefix;
      prefixAtom->GetUnicode(&prefix);
      aString.Append(prefix);
      aString.Append(PRUnichar('|'));
    }
  }
  // smells like a universal selector
  if (!mTag && !mIDList && !mClassList) {
    if (1 != aNegatedIndex) {
      aString.Append(PRUnichar('*'));
    }
    if (1 < aNegatedIndex) {
      NS_IF_NEGATED_END(aIsNegated, aString)
    }
  } else {
    // Append the tag name, if there is one
    if (mTag) {
      mTag->GetUnicode(&temp);
      aString.Append(temp);
      NS_IF_NEGATED_END(aIsNegated, aString)
    }
    // Append the id, if there is one
    if (mIDList) {
      nsAtomList* list = mIDList;
      while (list != nsnull) {
        list->mAtom->GetUnicode(&temp);
        NS_IF_NEGATED_START(aIsNegated, aString)
        aString.Append(PRUnichar('#'));
        aString.Append(temp);
        NS_IF_NEGATED_END(aIsNegated, aString)
        list = list->mNext;
      }
    }
    // Append each class in the linked list
    if (mClassList) {
      nsAtomList* list = mClassList;
      while (list != nsnull) {
        list->mAtom->GetUnicode(&temp);
        NS_IF_NEGATED_START(aIsNegated, aString)
        aString.Append(PRUnichar('.'));
        aString.Append(temp);
        NS_IF_NEGATED_END(aIsNegated, aString)
        list = list->mNext;
      }
    }
  }

  // Append each attribute selector in the linked list
  if (mAttrList) {
    nsAttrSelector* list = mAttrList;
    while (list != nsnull) {
      NS_IF_NEGATED_START(aIsNegated, aString)
      aString.Append(PRUnichar('['));
      // Append the namespace prefix
      if (list->mNameSpace > 0) {
        nsCOMPtr<nsINameSpace> sheetNS;
        aSheet->GetNameSpace(*getter_AddRefs(sheetNS));
        nsCOMPtr<nsIAtom> prefixAtom;
        // will return null if namespace was the default
        sheetNS->FindNameSpacePrefix(list->mNameSpace, *getter_AddRefs(prefixAtom));
        if (prefixAtom) { 
          const PRUnichar* prefix;
          prefixAtom->GetUnicode(&prefix);
          aString.Append(prefix);
          aString.Append(PRUnichar('|'));
        }
      }
      // Append the attribute name
      list->mAttr->GetUnicode(&temp);
      aString.Append(temp);
      // Append the function
      if (list->mFunction == NS_ATTR_FUNC_EQUALS) {
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_INCLUDES) {
        aString.Append(PRUnichar('~'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_DASHMATCH) {
        aString.Append(PRUnichar('|'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_BEGINSMATCH) {
        aString.Append(PRUnichar('^'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_ENDSMATCH) {
        aString.Append(PRUnichar('$'));
        aString.Append(PRUnichar('='));
      } else if (list->mFunction == NS_ATTR_FUNC_CONTAINSMATCH) {
        aString.Append(PRUnichar('*'));
        aString.Append(PRUnichar('='));
      }
      // Append the value
      aString.Append(list->mValue);
      aString.Append(PRUnichar(']'));
      NS_IF_NEGATED_END(aIsNegated, aString)
      list = list->mNext;
    }
  }

  // Append each pseudo-class in the linked list
  if (mPseudoClassList) {
    nsAtomList* list = mPseudoClassList;
    while (list != nsnull) {
      list->mAtom->GetUnicode(&temp);
      NS_IF_NEGATED_START(aIsNegated, aString)
      aString.Append(temp);
      NS_IF_NEGATED_END(aIsNegated, aString)
      list = list->mNext;
    }
  }

  if (mNegations) {
    // chain all the negated selectors
    mNegations->ToString(aString, aSheet, PR_FALSE, aNegatedIndex + 1);
  }

  // Append the operator only if the selector is not negated and is not
  // a pseudo-element
  if (!aIsNegated && mOperator && !aIsPseudoElem) {
    aString.Append(PRUnichar(' '));
    aString.Append(mOperator);
  }
  return NS_OK;
}


// -- CSSImportantRule -------------------------------

static nscoord CalcLength(const nsCSSValue& aValue, const nsFont& aFont, 
                          nsIPresContext* aPresContext);
static PRBool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord, 
                       const nsStyleCoord& aParentCoord,
                       PRInt32 aMask, const nsFont& aFont, 
                       nsIPresContext* aPresContext);

static void MapDeclarationFontInto(nsICSSDeclaration* aDeclaration, 
                                   nsIMutableStyleContext* aContext, 
                                   nsIPresContext* aPresContext);
static void MapDeclarationInto(nsICSSDeclaration* aDeclaration, 
                               nsIMutableStyleContext* aContext, 
                               nsIPresContext* aPresContext);

// New map helpers shared by both important and regular rules.
static nsresult MapFontForDeclaration(nsICSSDeclaration* aDecl, nsCSSFont& aFont);
static nsresult MapMarginForDeclaration(nsICSSDeclaration* aDecl, const nsStyleStructID& aID, nsCSSMargin& aMargin); 
static nsresult MapListForDeclaration(nsICSSDeclaration* aDecl, nsCSSList& aList);
static nsresult MapPositionForDeclaration(nsICSSDeclaration* aDecl, nsCSSPosition& aPosition);
#ifdef INCLUDE_XUL
static nsresult MapXULForDeclaration(nsICSSDeclaration* aDecl, nsCSSXUL& aXUL);
#endif

class CSSStyleRuleImpl;

class CSSImportantRule : public nsIStyleRule {
public:
  CSSImportantRule(nsICSSStyleSheet* aSheet, nsICSSDeclaration* aDeclaration);

  NS_DECL_ISUPPORTS

//  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aResult) const;
//  NS_IMETHOD HashValue(PRUint32& aValue) const;

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;

  // Strength is an out-of-band weighting, useful for mapping CSS ! important
  NS_IMETHOD GetStrength(PRInt32& aStrength) const;

  NS_IMETHOD MapFontStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext);
  NS_IMETHOD MapStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext);

  // The new mapping functions.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize);

protected:
  virtual ~CSSImportantRule(void);

  nsICSSDeclaration*  mDeclaration;
  nsICSSStyleSheet*   mSheet;

friend class CSSStyleRuleImpl;
};

CSSImportantRule::CSSImportantRule(nsICSSStyleSheet* aSheet, nsICSSDeclaration* aDeclaration)
  : mDeclaration(aDeclaration),
    mSheet(aSheet)
{
  NS_INIT_REFCNT();
  NS_IF_ADDREF(mDeclaration);
}

CSSImportantRule::~CSSImportantRule(void)
{
  NS_IF_RELEASE(mDeclaration);
}

NS_IMPL_ISUPPORTS(CSSImportantRule, NS_GET_IID(nsIStyleRule));

#if 0
NS_IMETHODIMP
CSSImportantRule::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  aResult = PRBool(aRule == this);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::HashValue(PRUint32& aValue) const
{
  aValue = PRUint32(mDeclaration);
  return NS_OK;
}
#endif

NS_IMETHODIMP
CSSImportantRule::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  NS_IF_ADDREF(mSheet);
  aSheet = mSheet;
  return NS_OK;
}

// Strength is an out-of-band weighting, useful for mapping CSS ! important
NS_IMETHODIMP
CSSImportantRule::GetStrength(PRInt32& aStrength) const
{
  aStrength = 1;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::MapFontStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationFontInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::MapStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (!aRuleData)
    return NS_OK;

  if (aRuleData->mFontData)
    return MapFontForDeclaration(mDeclaration, *aRuleData->mFontData);
  else if (aRuleData->mMarginData)
    return MapMarginForDeclaration(mDeclaration, aRuleData->mSID, *aRuleData->mMarginData);
  else if (aRuleData->mListData)
    return MapListForDeclaration(mDeclaration, *aRuleData->mListData);
  else if (aRuleData->mPositionData)
    return MapPositionForDeclaration(mDeclaration, *aRuleData->mPositionData);
#ifdef INCLUDE_XUL
  else if (aRuleData->mXULData)
    return MapXULForDeclaration(mDeclaration, *aRuleData->mXULData);
#endif

  return NS_OK;
}

NS_IMETHODIMP
CSSImportantRule::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  fputs("! Important rule ", out);
  if (nsnull != mDeclaration) {
    mDeclaration->List(out);
  }
  else {
    fputs("{ null declaration }", out);
  }
  fputs("\n", out);

  return NS_OK;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as CSSImportantRule's size): 
*    1) sizeof(*this) 
*
*  Contained / Aggregated data (not reported as CSSImportantRule's size):
*    1) mDeclaration is sized seperately
*    2) mSheet is sized seperately
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void CSSImportantRule::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("CSSImportantRule"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(CSSImportantRule);
  aSizeOfHandler->AddSize(tag,aSize);

  // now dump the mDeclaration and mSheet
  if(mDeclaration){
    mDeclaration->SizeOf(aSizeOfHandler, localSize);
  }
  if(mSheet){
    mSheet->SizeOf(aSizeOfHandler, localSize);
  }
}

// -- nsDOMStyleRuleDeclaration -------------------------------

class DOMCSSDeclarationImpl : public nsDOMCSSDeclaration
{
public:
  DOMCSSDeclarationImpl(nsICSSStyleRule *aRule);
  ~DOMCSSDeclarationImpl(void);

  NS_IMETHOD RemoveProperty(const nsAReadableString& aPropertyName, 
                            nsAWritableString& aReturn);

  virtual void DropReference(void);
  virtual nsresult GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                     PRBool aAllocate);
  virtual nsresult SetCSSDeclaration(nsICSSDeclaration *aDecl);
  virtual nsresult ParseDeclaration(const nsAReadableString& aDecl,
                                    PRBool aParseOnlyOneDecl,
                                    PRBool aClearOldDecl);
  virtual nsresult GetParent(nsISupports **aParent);

protected:
  nsICSSStyleRule *mRule;
};

MOZ_DECL_CTOR_COUNTER(DOMCSSDeclarationImpl)

DOMCSSDeclarationImpl::DOMCSSDeclarationImpl(nsICSSStyleRule *aRule)
{
  MOZ_COUNT_CTOR(DOMCSSDeclarationImpl);

  // This reference is not reference-counted. The rule
  // object tells us when its about to go away.
  mRule = aRule;
}

DOMCSSDeclarationImpl::~DOMCSSDeclarationImpl(void)
{
  MOZ_COUNT_DTOR(DOMCSSDeclarationImpl);
}

NS_IMETHODIMP
DOMCSSDeclarationImpl::RemoveProperty(const nsAReadableString& aPropertyName, 
                                      nsAWritableString& aReturn)
{
  aReturn.Truncate();

  nsCOMPtr<nsICSSDeclaration> decl;
  nsresult rv = GetCSSDeclaration(getter_AddRefs(decl), PR_TRUE);

  if (NS_SUCCEEDED(rv) && decl) {
    nsCSSProperty prop = nsCSSProps::LookupProperty(aPropertyName);
    nsCSSValue val;

    rv = decl->RemoveProperty(prop, val);

    if (NS_SUCCEEDED(rv)) {
      // We pass in eCSSProperty_UNKNOWN here so that we don't get the
      // property name in the return string.
      val.ToString(aReturn, eCSSProperty_UNKNOWN);
    } else {
      // If we tried to remove an invalid property or a property that wasn't 
      //  set we simply return success and an empty string
      rv = NS_OK;
    }
  }

  return rv;
}

void 
DOMCSSDeclarationImpl::DropReference(void)
{
  mRule = nsnull;
}

nsresult
DOMCSSDeclarationImpl::GetCSSDeclaration(nsICSSDeclaration **aDecl,
                                             PRBool aAllocate)
{
  if (nsnull != mRule) {
    *aDecl = mRule->GetDeclaration();
  }
  else {
    *aDecl = nsnull;
  }

  return NS_OK;
}

nsresult
DOMCSSDeclarationImpl::SetCSSDeclaration(nsICSSDeclaration *aDecl)
{
  if (nsnull != mRule) {
    mRule->SetDeclaration(aDecl);
  }

  return NS_OK;
}

nsresult 
DOMCSSDeclarationImpl::ParseDeclaration(const nsAReadableString& aDecl,
                                        PRBool aParseOnlyOneDecl,
                                        PRBool aClearOldDecl)
{
  nsICSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);

  if (NS_SUCCEEDED(result) && (decl)) {
    nsICSSLoader* cssLoader = nsnull;
    nsICSSParser* cssParser = nsnull;
    nsIURI* baseURI = nsnull;
    nsICSSStyleSheet* cssSheet = nsnull;
    nsIDocument*  owningDoc = nsnull;

    nsIStyleSheet* sheet = nsnull;
    if (mRule) {
      mRule->GetStyleSheet(sheet);
      if (sheet) {
        sheet->GetURL(baseURI);
        sheet->GetOwningDocument(owningDoc);
        sheet->QueryInterface(NS_GET_IID(nsICSSStyleSheet), (void**)&cssSheet);
        if (owningDoc) {
          nsIHTMLContentContainer* htmlContainer;
          result = owningDoc->QueryInterface(NS_GET_IID(nsIHTMLContentContainer), (void**)&htmlContainer);
          if (NS_SUCCEEDED(result)) {
            result = htmlContainer->GetCSSLoader(cssLoader);
            NS_RELEASE(htmlContainer);
          }
        }
        NS_RELEASE(sheet);
      }
    }
    if (cssLoader) {
      result = cssLoader->GetParserFor(nsnull, &cssParser);
    }
    else {
      result = NS_NewCSSParser(&cssParser);
    }

    if (NS_SUCCEEDED(result)) {
      nsCOMPtr<nsICSSDeclaration> declClone;
      decl->Clone(*getter_AddRefs(declClone));
      NS_ENSURE_TRUE(declClone, NS_ERROR_OUT_OF_MEMORY);

      if (aClearOldDecl) {
        // This should be done with decl->Clear() once such a method exists.
        nsAutoString propName;
        PRUint32 count, i;

        decl->Count(&count);

        for (i = 0; i < count; i++) {
          decl->GetNthProperty(0, propName);

          nsCSSProperty prop = nsCSSProps::LookupProperty(propName);
          nsCSSValue val;
 
          decl->RemoveProperty(prop, val);
        }
      }

      PRInt32 hint;
      result = cssParser->ParseAndAppendDeclaration(aDecl, baseURI, decl,
                                                    aParseOnlyOneDecl, &hint);

      if (result == NS_CSS_PARSER_DROP_DECLARATION) {
        SetCSSDeclaration(declClone);
        result = NS_OK;
      } else if (NS_SUCCEEDED(result)) {
        if (cssSheet) {
          cssSheet->SetModified(PR_TRUE);
        }
        if (owningDoc) {
          owningDoc->StyleRuleChanged(cssSheet, mRule, hint);
        }
      }
      if (cssLoader) {
        cssLoader->RecycleParser(cssParser);
      }
      else {
        NS_RELEASE(cssParser);
      }
    }
    NS_IF_RELEASE(cssLoader);
    NS_IF_RELEASE(baseURI);
    NS_IF_RELEASE(cssSheet);
    NS_IF_RELEASE(owningDoc);
    NS_RELEASE(decl);
  }

  return result;
}



nsresult 
DOMCSSDeclarationImpl::GetParent(nsISupports **aParent)
{
  if (nsnull != mRule) {
    return mRule->QueryInterface(kISupportsIID, (void **)aParent);
  } else {
    NS_ENSURE_ARG_POINTER(aParent);
    *aParent = nsnull;
  }

  return NS_OK;
}

// -- nsCSSStyleRule -------------------------------

class CSSStyleRuleImpl : public nsCSSRule,
                         public nsICSSStyleRule, 
                         public nsIDOMCSSStyleRule
{
public:
  CSSStyleRuleImpl(const nsCSSSelector& aSelector);
  CSSStyleRuleImpl(const CSSStyleRuleImpl& aCopy); 

  NS_DECL_ISUPPORTS_INHERITED

//  NS_IMETHOD Equals(const nsIStyleRule* aRule, PRBool& aResult) const;
//  NS_IMETHOD HashValue(PRUint32& aValue) const;
  // Strength is an out-of-band weighting, useful for mapping CSS ! important
  NS_IMETHOD GetStrength(PRInt32& aStrength) const;

  virtual nsCSSSelector* FirstSelector(void);
  virtual void AddSelector(const nsCSSSelector& aSelector);
  virtual void DeleteSelector(nsCSSSelector* aSelector);
  virtual void SetSourceSelectorText(const nsString& aSelectorText);
  virtual void GetSourceSelectorText(nsString& aSelectorText) const;

  virtual PRUint32 GetLineNumber(void) const;
  virtual void SetLineNumber(PRUint32 aLineNumber);

  virtual nsICSSDeclaration* GetDeclaration(void) const;
  virtual void SetDeclaration(nsICSSDeclaration* aDeclaration);

  virtual PRInt32 GetWeight(void) const;
  virtual void SetWeight(PRInt32 aWeight);

  virtual nsIStyleRule* GetImportantRule(void);

  NS_IMETHOD GetStyleSheet(nsIStyleSheet*& aSheet) const;
  NS_IMETHOD SetStyleSheet(nsICSSStyleSheet* aSheet);

  NS_IMETHOD GetType(PRInt32& aType) const;
  NS_IMETHOD Clone(nsICSSRule*& aClone) const;

  NS_IMETHOD MapFontStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext);
  NS_IMETHOD MapStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext);

  // The new mapping functions.
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);

  NS_IMETHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;

  virtual void SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize);

  // nsIDOMCSSRule interface
  NS_DECL_NSIDOMCSSRULE

  // nsIDOMCSSStyleRule interface
  NS_DECL_NSIDOMCSSSTYLERULE

private: 
  // These are not supported and are not implemented! 
  CSSStyleRuleImpl& operator=(const CSSStyleRuleImpl& aCopy); 

protected:
  virtual ~CSSStyleRuleImpl(void);

protected:
  nsCSSSelector           mSelector;
  nsICSSDeclaration*      mDeclaration;
  PRInt32                 mWeight;
  CSSImportantRule*       mImportantRule;
  DOMCSSDeclarationImpl*  mDOMDeclaration;                          
  PRUint32                mLineNumber;
};

#ifdef DEBUG_REFS
PRUint32 gStyleRuleCount=0;
#endif

CSSStyleRuleImpl::CSSStyleRuleImpl(const nsCSSSelector& aSelector)
  : nsCSSRule(),
    mSelector(aSelector), mDeclaration(nsnull), 
    mWeight(0), mImportantRule(nsnull),
    mDOMDeclaration(nsnull)
{
#ifdef DEBUG_REFS
  gStyleRuleCount++;
  printf( "CSSStyleRuleImpl Instances (ctor): %ld\n", (long)gStyleRuleCount);
#endif
}

CSSStyleRuleImpl::CSSStyleRuleImpl(const CSSStyleRuleImpl& aCopy)
  : nsCSSRule(aCopy),
    mSelector(aCopy.mSelector),
    mDeclaration(nsnull),
    mWeight(aCopy.mWeight),
    mImportantRule(nsnull),
    mDOMDeclaration(nsnull)
{
#ifdef DEBUG_REFS
  gStyleRuleCount++;
  printf( "CSSStyleRuleImpl Instances (cp-ctor): %ld\n", (long)gStyleRuleCount);
#endif

  nsCSSSelector* copySel = aCopy.mSelector.mNext;
  nsCSSSelector* ourSel = &mSelector;

  while (copySel && ourSel) {
    ourSel->mNext = new nsCSSSelector(*copySel);
    ourSel = ourSel->mNext;
    copySel = copySel->mNext;
  }

  if (aCopy.mDeclaration) {
    aCopy.mDeclaration->Clone(mDeclaration);
  }
  // rest is constructed lazily on existing data
}


CSSStyleRuleImpl::~CSSStyleRuleImpl(void)
{
#ifdef DEBUG_REFS
  gStyleRuleCount--;
  printf( "CSSStyleRuleImpl Instances (dtor): %ld\n", (long)gStyleRuleCount);
#endif

  nsCSSSelector*  next = mSelector.mNext;

  while (nsnull != next) {
    nsCSSSelector*  selector = next;
    next = selector->mNext;
    delete selector;
  }
  NS_IF_RELEASE(mDeclaration);
  if (nsnull != mImportantRule) {
    mImportantRule->mSheet = nsnull;
    NS_RELEASE(mImportantRule);
  }
  if (nsnull != mDOMDeclaration) {
    mDOMDeclaration->DropReference();
  }
}

// XPConnect interface list for CSSStyleRuleImpl
NS_CLASSINFO_MAP_BEGIN(CSSStyleRule)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMCSSStyleRule)
NS_CLASSINFO_MAP_END


// QueryInterface implementation for CSSStyleRuleImpl
NS_INTERFACE_MAP_BEGIN(CSSStyleRuleImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSStyleRule)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSStyleRule)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF_INHERITED(CSSStyleRuleImpl, nsCSSRule);
NS_IMPL_RELEASE_INHERITED(CSSStyleRuleImpl, nsCSSRule);


#if 0
NS_IMETHODIMP CSSStyleRuleImpl::Equals(const nsIStyleRule* aRule, PRBool& aResult) const
{
  nsICSSStyleRule* iCSSRule;

  if (this == aRule) {
    aResult = PR_TRUE;
  }
  else {
    aResult = PR_FALSE;
    if ((nsnull != aRule) && 
        (NS_OK == ((nsIStyleRule*)aRule)->QueryInterface(NS_GET_IID(nsICSSStyleRule), (void**) &iCSSRule))) {

      CSSStyleRuleImpl* rule = (CSSStyleRuleImpl*)iCSSRule;
      const nsCSSSelector* local = &mSelector;
      const nsCSSSelector* other = &(rule->mSelector);
      aResult = PR_TRUE;

      if ((rule->mDeclaration != mDeclaration) || 
          (rule->mWeight != mWeight)) {
        aResult = PR_FALSE;
      }
      while ((PR_TRUE == aResult) && (nsnull != local) && (nsnull != other)) {
        if (! local->Equals(other)) {
          aResult = PR_FALSE;
        }
        local = local->mNext;
        other = other->mNext;
      }
      if ((nsnull != local) || (nsnull != other)) { // more were left
        aResult = PR_FALSE;
      }
      NS_RELEASE(iCSSRule);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::HashValue(PRUint32& aValue) const
{
  aValue = (PRUint32)this;
  return NS_OK;
}
#endif

// Strength is an out-of-band weighting, useful for mapping CSS ! important
NS_IMETHODIMP
CSSStyleRuleImpl::GetStrength(PRInt32& aStrength) const
{
  aStrength = 0;
  return NS_OK;
}

nsCSSSelector* CSSStyleRuleImpl::FirstSelector(void)
{
  return &mSelector;
}

void CSSStyleRuleImpl::AddSelector(const nsCSSSelector& aSelector)
{
  nsCSSSelector*  selector = new nsCSSSelector(aSelector);
  nsCSSSelector*  last = &mSelector;

  while (nsnull != last->mNext) {
    last = last->mNext;
  }
  last->mNext = selector;
}


void CSSStyleRuleImpl::DeleteSelector(nsCSSSelector* aSelector)
{
  if (nsnull != aSelector) {
    if (&mSelector == aSelector) {  // handle first selector
      if (nsnull != mSelector.mNext) {
        nsCSSSelector* nextOne = mSelector.mNext;
        mSelector = *nextOne; // assign values
        mSelector.mNext = nextOne->mNext;
        delete nextOne;
      }
      else {
        mSelector.Reset();
      }
    }
    else {
      nsCSSSelector*  selector = &mSelector;

      while (nsnull != selector->mNext) {
        if (aSelector == selector->mNext) {
          selector->mNext = aSelector->mNext;
          delete aSelector;
          return;
        }
        selector = selector->mNext;
      }
    }
  }
}

void CSSStyleRuleImpl::SetSourceSelectorText(const nsString& aSelectorText)
{
    /* no need for set, since get recreates the string */
}

void CSSStyleRuleImpl::GetSourceSelectorText(nsString& aSelectorText) const
{
  mSelector.ToString( aSelectorText, mSheet, IsPseudoElement(mSelector.mTag),
                      0 );
}

PRUint32 CSSStyleRuleImpl::GetLineNumber(void) const
{
  return mLineNumber;
}

void CSSStyleRuleImpl::SetLineNumber(PRUint32 aLineNumber)
{
  mLineNumber = aLineNumber;
}

nsICSSDeclaration* CSSStyleRuleImpl::GetDeclaration(void) const
{
  nsICSSDeclaration* result = mDeclaration;
  NS_IF_ADDREF(result);
  return result;
}

void CSSStyleRuleImpl::SetDeclaration(nsICSSDeclaration* aDeclaration)
{
  if (mDeclaration != aDeclaration) {
    NS_IF_RELEASE(mImportantRule); 
    NS_IF_RELEASE(mDeclaration);
    mDeclaration = aDeclaration;
    NS_IF_ADDREF(mDeclaration);
  }
}

PRInt32 CSSStyleRuleImpl::GetWeight(void) const
{
  return mWeight;
}

void CSSStyleRuleImpl::SetWeight(PRInt32 aWeight)
{
  mWeight = aWeight;
}

nsIStyleRule* CSSStyleRuleImpl::GetImportantRule(void)
{
  if ((nsnull == mImportantRule) && (nsnull != mDeclaration)) {
    nsICSSDeclaration*  important;
    mDeclaration->GetImportantValues(important);
    if (nsnull != important) {
      mImportantRule = new CSSImportantRule(mSheet, important);
      NS_ADDREF(mImportantRule);
      NS_RELEASE(important);
    }
  }
  NS_IF_ADDREF(mImportantRule);
  return mImportantRule;
}

NS_IMETHODIMP
CSSStyleRuleImpl::GetStyleSheet(nsIStyleSheet*& aSheet) const
{
  return nsCSSRule::GetStyleSheet(aSheet);
}

NS_IMETHODIMP
CSSStyleRuleImpl::SetStyleSheet(nsICSSStyleSheet* aSheet)
{
  nsCSSRule::SetStyleSheet(aSheet);
  if (nsnull != mImportantRule) { // we're responsible for this guy too
    mImportantRule->mSheet = aSheet;
  }
  return NS_OK;
}

nscoord CalcLength(const nsCSSValue& aValue,
                   const nsFont& aFont, 
                   nsIPresContext* aPresContext)
{
  NS_ASSERTION(aValue.IsLengthUnit(), "not a length unit");
  if (aValue.IsFixedLengthUnit()) {
    return aValue.GetLengthTwips();
  }
  nsCSSUnit unit = aValue.GetUnit();
  switch (unit) {
    case eCSSUnit_EM:
    case eCSSUnit_Char:
      return NSToCoordRound(aValue.GetFloatValue() * (float)aFont.size);
      // XXX scale against font metrics height instead?
    case eCSSUnit_EN:
      return NSToCoordRound((aValue.GetFloatValue() * (float)aFont.size) / 2.0f);
    case eCSSUnit_XHeight: {
      nsIFontMetrics* fm;
      aPresContext->GetMetricsFor(aFont, &fm);
      NS_ASSERTION(nsnull != fm, "can't get font metrics");
      nscoord xHeight;
      if (nsnull != fm) {
        fm->GetXHeight(xHeight);
        NS_RELEASE(fm);
      }
      else {
        xHeight = ((aFont.size * 2) / 3);
      }
      return NSToCoordRound(aValue.GetFloatValue() * (float)xHeight);
    }
    case eCSSUnit_CapHeight: {
      NS_NOTYETIMPLEMENTED("cap height unit");
      nscoord capHeight = ((aFont.size / 3) * 2); // XXX HACK!
      return NSToCoordRound(aValue.GetFloatValue() * (float)capHeight);
    }
    case eCSSUnit_Pixel:
      float p2t;
      aPresContext->GetScaledPixelsToTwips(&p2t);
      return NSFloatPixelsToTwips(aValue.GetFloatValue(), p2t);

    default:
      break;
  }
  return 0;
}


#define SETCOORD_NORMAL       0x01
#define SETCOORD_AUTO         0x02
#define SETCOORD_INHERIT      0x04
#define SETCOORD_PERCENT      0x08
#define SETCOORD_FACTOR       0x10
#define SETCOORD_LENGTH       0x20
#define SETCOORD_INTEGER      0x40
#define SETCOORD_ENUMERATED   0x80

#define SETCOORD_LP     (SETCOORD_LENGTH | SETCOORD_PERCENT)
#define SETCOORD_LH     (SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_AH     (SETCOORD_AUTO | SETCOORD_INHERIT)
#define SETCOORD_LPH    (SETCOORD_LP | SETCOORD_INHERIT)
#define SETCOORD_LPFHN  (SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_NORMAL)
#define SETCOORD_LPAH   (SETCOORD_LP | SETCOORD_AH)
#define SETCOORD_LPEH   (SETCOORD_LP | SETCOORD_ENUMERATED | SETCOORD_INHERIT)
#define SETCOORD_LE     (SETCOORD_LENGTH | SETCOORD_ENUMERATED)
#define SETCOORD_LEH    (SETCOORD_LE | SETCOORD_INHERIT)
#define SETCOORD_IA     (SETCOORD_INTEGER | SETCOORD_AUTO)
#define SETCOORD_LAE		(SETCOORD_LENGTH | SETCOORD_AUTO | SETCOORD_ENUMERATED)

static PRBool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord, 
                       const nsStyleCoord& aParentCoord,
                       PRInt32 aMask, const nsFont& aFont, 
                       nsIPresContext* aPresContext)
{
  PRBool  result = PR_TRUE;
  if (aValue.GetUnit() == eCSSUnit_Null) {
    result = PR_FALSE;
  }
  else if (((aMask & SETCOORD_LENGTH) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Char)) {
    aCoord.SetIntValue(NSToIntFloor(aValue.GetFloatValue()), eStyleUnit_Chars);
  } 
  else if (((aMask & SETCOORD_LENGTH) != 0) && 
           aValue.IsLengthUnit()) {
    aCoord.SetCoordValue(CalcLength(aValue, aFont, aPresContext));
  } 
  else if (((aMask & SETCOORD_PERCENT) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Percent)) {
    aCoord.SetPercentValue(aValue.GetPercentValue());
  } 
  else if (((aMask & SETCOORD_INTEGER) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Integer)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Integer);
  } 
  else if (((aMask & SETCOORD_ENUMERATED) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Enumerated)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Enumerated);
  } 
  else if (((aMask & SETCOORD_AUTO) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Auto)) {
    aCoord.SetAutoValue();
  } 
  else if (((aMask & SETCOORD_INHERIT) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Inherit)) {
    nsStyleUnit unit = aParentCoord.GetUnit();
    if ((eStyleUnit_Null == unit) ||  // parent has explicit computed value
        (eStyleUnit_Factor == unit) ||
        (eStyleUnit_Coord == unit) ||
        (eStyleUnit_Integer == unit) ||
        (eStyleUnit_Enumerated == unit) ||
        (eStyleUnit_Normal == unit) ||
        (eStyleUnit_Chars == unit)) {
      aCoord = aParentCoord;  // just inherit value from parent
    }
    else {
      aCoord.SetInheritValue(); // needs to be computed by client
    }
  }
  else if (((aMask & SETCOORD_NORMAL) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Normal)) {
    aCoord.SetNormalValue();
  }
  else if (((aMask & SETCOORD_FACTOR) != 0) && 
           (aValue.GetUnit() == eCSSUnit_Number)) {
    aCoord.SetFactorValue(aValue.GetFloatValue());
  }
  else {
    result = PR_FALSE;  // didn't set anything
  }
  return result;
}

static PRBool SetColor(const nsCSSValue& aValue, const nscolor aParentColor, 
                       nsIPresContext* aPresContext, nscolor& aResult)
{
  PRBool  result = PR_FALSE;
  nsCSSUnit unit = aValue.GetUnit();

  if (eCSSUnit_Color == unit) {
    aResult = aValue.GetColorValue();
    result = PR_TRUE;
  }
  else if (eCSSUnit_String == unit) {
    nsAutoString  value;
    aValue.GetStringValue(value);
    nscolor rgba;
    if (NS_ColorNameToRGB(value, &rgba)) {
      aResult = rgba;
      result = PR_TRUE;
    }
  }
  else if (eCSSUnit_Integer == unit) {
    nsILookAndFeel* look = nsnull;
    if (NS_SUCCEEDED(aPresContext->GetLookAndFeel(&look)) && look) {
      nsILookAndFeel::nsColorID colorID = (nsILookAndFeel::nsColorID)aValue.GetIntValue();
      if (NS_SUCCEEDED(look->GetColor(colorID, aResult))) {
        result = PR_TRUE;
      }
      NS_RELEASE(look);
    }
  }
  else if (eCSSUnit_Inherit == unit) {
    aResult = aParentColor;
    result = PR_TRUE;
  }
  return result;
}

NS_IMETHODIMP
CSSStyleRuleImpl::GetType(PRInt32& aType) const
{
  aType = nsICSSRule::STYLE_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::Clone(nsICSSRule*& aClone) const
{
  CSSStyleRuleImpl* clone = new CSSStyleRuleImpl(*this);
  if (clone) {
    return clone->QueryInterface(NS_GET_IID(nsICSSRule), (void **)&aClone);
  }
  aClone = nsnull;
  return NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
CSSStyleRuleImpl::MapFontStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationFontInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::MapStyleInto(nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
  MapDeclarationInto(mDeclaration, aContext, aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
CSSStyleRuleImpl::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (!aRuleData)
    return NS_OK;

  if (aRuleData->mFontData)
    return MapFontForDeclaration(mDeclaration, *aRuleData->mFontData);
  else if (aRuleData->mMarginData)
    return MapMarginForDeclaration(mDeclaration, aRuleData->mSID, *aRuleData->mMarginData);
  else if (aRuleData->mListData)
    return MapListForDeclaration(mDeclaration, *aRuleData->mListData);
  else if (aRuleData->mPositionData)
    return MapPositionForDeclaration(mDeclaration, *aRuleData->mPositionData);
#ifdef INCLUDE_XUL
  else if (aRuleData->mXULData)
    return MapXULForDeclaration(mDeclaration, *aRuleData->mXULData);
#endif

  return NS_OK;
}


nsString& Unquote(nsString& aString)
{
  PRUnichar start = aString.First();
  PRUnichar end = aString.Last();

  if ((start == end) && 
      ((start == PRUnichar('\"')) || 
       (start == PRUnichar('\'')))) {
    PRInt32 length = aString.Length();
    aString.Truncate(length - 1);
    aString.Cut(0, 1);
  }
  return aString;
}

static nsresult 
MapFontForDeclaration(nsICSSDeclaration* aDecl, nsCSSFont& aFont)
{
  if (!aDecl)
    return NS_OK; // The rule must have a declaration.

  nsCSSFont* ourFont;
  aDecl->GetData(kCSSFontSID, (nsCSSStruct**)&ourFont);
  if (!ourFont)
    return NS_OK; // We don't have any rules for fonts.

  if (eCSSUnit_Null == aFont.mFamily.GetUnit() && eCSSUnit_Null != ourFont->mFamily.GetUnit())
    aFont.mFamily = ourFont->mFamily;

  if (eCSSUnit_Null == aFont.mStyle.GetUnit() && eCSSUnit_Null != ourFont->mStyle.GetUnit())
    aFont.mStyle = ourFont->mStyle;

  if (eCSSUnit_Null == aFont.mVariant.GetUnit() && eCSSUnit_Null != ourFont->mVariant.GetUnit())
    aFont.mVariant = ourFont->mVariant;

  if (eCSSUnit_Null == aFont.mWeight.GetUnit() && eCSSUnit_Null != ourFont->mWeight.GetUnit())
    aFont.mWeight = ourFont->mWeight;

  if (eCSSUnit_Null == aFont.mSize.GetUnit() && eCSSUnit_Null != ourFont->mSize.GetUnit())
    aFont.mSize = ourFont->mSize;

  return NS_OK;
}

#ifdef INCLUDE_XUL
static nsresult 
MapXULForDeclaration(nsICSSDeclaration* aDecl, nsCSSXUL& aXUL)
{
  if (!aDecl)
    return NS_OK; // The rule must have a declaration.

  nsCSSXUL* ourXUL;
  aDecl->GetData(kCSSXULSID, (nsCSSStruct**)&ourXUL);
  if (!ourXUL)
    return NS_OK; // We don't have any rules for XUL.

  // box-orient: enum, inherit
  if (aXUL.mBoxOrient.GetUnit() == eCSSUnit_Null && ourXUL->mBoxOrient.GetUnit() != eCSSUnit_Null)
    aXUL.mBoxOrient = ourXUL->mBoxOrient;

  return NS_OK;
}
#endif

static nsresult 
MapPositionForDeclaration(nsICSSDeclaration* aDecl, nsCSSPosition& aPosition)
{
  if (!aDecl)
    return NS_OK; // The rule must have a declaration.

  nsCSSPosition* ourPosition;
  aDecl->GetData(kCSSPositionSID, (nsCSSStruct**)&ourPosition);
  if (!ourPosition)
    return NS_OK; // We don't have any rules for position.

  // position: enum, inherit
  if (aPosition.mPosition.GetUnit() == eCSSUnit_Null && ourPosition->mPosition.GetUnit() != eCSSUnit_Null)
    aPosition.mPosition = ourPosition->mPosition;

  // box offsets: length, percent, auto, inherit
  if (ourPosition->mOffset) {
    if (aPosition.mOffset->mLeft.GetUnit() == eCSSUnit_Null && ourPosition->mOffset->mLeft.GetUnit() != eCSSUnit_Null)
      aPosition.mOffset->mLeft = ourPosition->mOffset->mLeft;

    if (aPosition.mOffset->mRight.GetUnit() == eCSSUnit_Null && ourPosition->mOffset->mRight.GetUnit() != eCSSUnit_Null)
      aPosition.mOffset->mRight = ourPosition->mOffset->mRight;

    if (aPosition.mOffset->mTop.GetUnit() == eCSSUnit_Null && ourPosition->mOffset->mTop.GetUnit() != eCSSUnit_Null)
      aPosition.mOffset->mTop = ourPosition->mOffset->mTop;

    if (aPosition.mOffset->mBottom.GetUnit() == eCSSUnit_Null && ourPosition->mOffset->mBottom.GetUnit() != eCSSUnit_Null)
      aPosition.mOffset->mBottom = ourPosition->mOffset->mBottom;
  }

  // width/min-width/max-width
  if (aPosition.mWidth.GetUnit() == eCSSUnit_Null && ourPosition->mWidth.GetUnit() != eCSSUnit_Null)
    aPosition.mWidth = ourPosition->mWidth;
  if (aPosition.mMinWidth.GetUnit() == eCSSUnit_Null && ourPosition->mMinWidth.GetUnit() != eCSSUnit_Null)
    aPosition.mMinWidth = ourPosition->mMinWidth;
  if (aPosition.mMaxWidth.GetUnit() == eCSSUnit_Null && ourPosition->mMaxWidth.GetUnit() != eCSSUnit_Null)
    aPosition.mMaxWidth = ourPosition->mMaxWidth;

  // height/min-height/max-height
  if (aPosition.mHeight.GetUnit() == eCSSUnit_Null && ourPosition->mHeight.GetUnit() != eCSSUnit_Null)
    aPosition.mHeight = ourPosition->mHeight;
  if (aPosition.mMinHeight.GetUnit() == eCSSUnit_Null && ourPosition->mMinHeight.GetUnit() != eCSSUnit_Null)
    aPosition.mMinHeight = ourPosition->mMinHeight;
  if (aPosition.mMaxHeight.GetUnit() == eCSSUnit_Null && ourPosition->mMaxHeight.GetUnit() != eCSSUnit_Null)
    aPosition.mMaxHeight = ourPosition->mMaxHeight;

  // box-sizing: enum, inherit
  if (aPosition.mBoxSizing.GetUnit() == eCSSUnit_Null && ourPosition->mBoxSizing.GetUnit() != eCSSUnit_Null)
    aPosition.mBoxSizing = ourPosition->mBoxSizing;

  // z-index
  if (aPosition.mZIndex.GetUnit() == eCSSUnit_Null && ourPosition->mZIndex.GetUnit() != eCSSUnit_Null)
    aPosition.mZIndex = ourPosition->mZIndex;

  return NS_OK;
}

static nsresult 
MapListForDeclaration(nsICSSDeclaration* aDecl, nsCSSList& aList)
{
  if (!aDecl)
    return NS_OK; // The rule must have a declaration.

  nsCSSList* ourList;
  aDecl->GetData(kCSSListSID, (nsCSSStruct**)&ourList);
  if (!ourList)
    return NS_OK; // We don't have any rules for lists.

  // list-style-type: enum, none, inherit
  if (aList.mType.GetUnit() == eCSSUnit_Null && ourList->mType.GetUnit() != eCSSUnit_Null)
    aList.mType = ourList->mType;

  // list-style-image: url, none, inherit
  if (aList.mImage.GetUnit() == eCSSUnit_Null && ourList->mImage.GetUnit() != eCSSUnit_Null)
    aList.mImage = ourList->mImage;
      
  // list-style-position: enum, inherit
  if (aList.mPosition.GetUnit() == eCSSUnit_Null && ourList->mPosition.GetUnit() != eCSSUnit_Null)
    aList.mPosition = ourList->mPosition;

  return NS_OK;
}
    
static nsresult
MapMarginForDeclaration(nsICSSDeclaration* aDeclaration, const nsStyleStructID& aSID, nsCSSMargin& aMargin)
{
  nsCSSMargin*  ourMargin;
  aDeclaration->GetData(kCSSMarginSID, (nsCSSStruct**)&ourMargin);
  if (!ourMargin)
    return NS_OK;

  // Margins
  if (aSID == eStyleStruct_Margin && ourMargin->mMargin) {
    if (eCSSUnit_Null == aMargin.mMargin->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mMargin->mLeft.GetUnit())
      aMargin.mMargin->mLeft = ourMargin->mMargin->mLeft;

    if (eCSSUnit_Null == aMargin.mMargin->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mMargin->mTop.GetUnit())
      aMargin.mMargin->mTop = ourMargin->mMargin->mTop;

    if (eCSSUnit_Null == aMargin.mMargin->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mMargin->mRight.GetUnit())
      aMargin.mMargin->mRight = ourMargin->mMargin->mRight;

    if (eCSSUnit_Null == aMargin.mMargin->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mMargin->mBottom.GetUnit())
      aMargin.mMargin->mBottom = ourMargin->mMargin->mBottom;
  }

  // Padding
  if (aSID == eStyleStruct_Padding && ourMargin->mPadding) {
    if (eCSSUnit_Null == aMargin.mPadding->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mPadding->mLeft.GetUnit())
      aMargin.mPadding->mLeft = ourMargin->mPadding->mLeft;

    if (eCSSUnit_Null == aMargin.mPadding->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mPadding->mTop.GetUnit())
      aMargin.mPadding->mTop = ourMargin->mPadding->mTop;

    if (eCSSUnit_Null == aMargin.mPadding->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mPadding->mRight.GetUnit())
      aMargin.mPadding->mRight = ourMargin->mPadding->mRight;

    if (eCSSUnit_Null == aMargin.mPadding->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mPadding->mBottom.GetUnit())
      aMargin.mPadding->mBottom = ourMargin->mPadding->mBottom;
  }

  // Borders
  if (aSID == eStyleStruct_Border) {
    // border-size
    if (ourMargin->mBorderWidth) {
      if (eCSSUnit_Null == aMargin.mBorderWidth->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mBorderWidth->mLeft.GetUnit())
        aMargin.mBorderWidth->mLeft = ourMargin->mBorderWidth->mLeft;

      if (eCSSUnit_Null == aMargin.mBorderWidth->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mBorderWidth->mTop.GetUnit())
        aMargin.mBorderWidth->mTop = ourMargin->mBorderWidth->mTop;

      if (eCSSUnit_Null == aMargin.mBorderWidth->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mBorderWidth->mRight.GetUnit())
        aMargin.mBorderWidth->mRight = ourMargin->mBorderWidth->mRight;

      if (eCSSUnit_Null == aMargin.mBorderWidth->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mBorderWidth->mBottom.GetUnit())
        aMargin.mBorderWidth->mBottom = ourMargin->mBorderWidth->mBottom;
    }

    // border-style
    if (ourMargin->mBorderStyle) {
      if (eCSSUnit_Null == aMargin.mBorderStyle->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mBorderStyle->mLeft.GetUnit())
        aMargin.mBorderStyle->mLeft = ourMargin->mBorderStyle->mLeft;

      if (eCSSUnit_Null == aMargin.mBorderStyle->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mBorderStyle->mTop.GetUnit())
        aMargin.mBorderStyle->mTop = ourMargin->mBorderStyle->mTop;

      if (eCSSUnit_Null == aMargin.mBorderStyle->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mBorderStyle->mRight.GetUnit())
        aMargin.mBorderStyle->mRight = ourMargin->mBorderStyle->mRight;

      if (eCSSUnit_Null == aMargin.mBorderStyle->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mBorderStyle->mBottom.GetUnit())
        aMargin.mBorderStyle->mBottom = ourMargin->mBorderStyle->mBottom;
    }

    // border-color
    if (ourMargin->mBorderColor) {
      if (eCSSUnit_Null == aMargin.mBorderColor->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mBorderColor->mLeft.GetUnit())
        aMargin.mBorderColor->mLeft = ourMargin->mBorderColor->mLeft;

      if (eCSSUnit_Null == aMargin.mBorderColor->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mBorderColor->mTop.GetUnit())
        aMargin.mBorderColor->mTop = ourMargin->mBorderColor->mTop;

      if (eCSSUnit_Null == aMargin.mBorderColor->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mBorderColor->mRight.GetUnit())
        aMargin.mBorderColor->mRight = ourMargin->mBorderColor->mRight;

      if (eCSSUnit_Null == aMargin.mBorderColor->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mBorderColor->mBottom.GetUnit())
        aMargin.mBorderColor->mBottom = ourMargin->mBorderColor->mBottom;
    }

    // -moz-border-radius
    if (ourMargin->mBorderRadius) {
      if (eCSSUnit_Null == aMargin.mBorderRadius->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mBorderRadius->mLeft.GetUnit())
        aMargin.mBorderRadius->mLeft = ourMargin->mBorderRadius->mLeft;

      if (eCSSUnit_Null == aMargin.mBorderRadius->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mBorderRadius->mTop.GetUnit())
        aMargin.mBorderRadius->mTop = ourMargin->mBorderRadius->mTop;

      if (eCSSUnit_Null == aMargin.mBorderRadius->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mBorderRadius->mRight.GetUnit())
        aMargin.mBorderRadius->mRight = ourMargin->mBorderRadius->mRight;

      if (eCSSUnit_Null == aMargin.mBorderRadius->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mBorderRadius->mBottom.GetUnit())
        aMargin.mBorderRadius->mBottom = ourMargin->mBorderRadius->mBottom;
    }

    // float-edge
    if (eCSSUnit_Null == aMargin.mFloatEdge.GetUnit() && eCSSUnit_Null != ourMargin->mFloatEdge.GetUnit())
      aMargin.mFloatEdge = ourMargin->mFloatEdge;
  }

  // Outline
  if (aSID == eStyleStruct_Outline) {
    // -moz-outline-radius
    if (ourMargin->mOutlineRadius) {
      if (eCSSUnit_Null == aMargin.mOutlineRadius->mLeft.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineRadius->mLeft.GetUnit())
        aMargin.mOutlineRadius->mLeft = ourMargin->mOutlineRadius->mLeft;

      if (eCSSUnit_Null == aMargin.mOutlineRadius->mTop.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineRadius->mTop.GetUnit())
        aMargin.mOutlineRadius->mTop = ourMargin->mOutlineRadius->mTop;

      if (eCSSUnit_Null == aMargin.mOutlineRadius->mRight.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineRadius->mRight.GetUnit())
        aMargin.mOutlineRadius->mRight = ourMargin->mOutlineRadius->mRight;

      if (eCSSUnit_Null == aMargin.mOutlineRadius->mBottom.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineRadius->mBottom.GetUnit())
        aMargin.mOutlineRadius->mBottom = ourMargin->mOutlineRadius->mBottom;
    }

    // outline-width
    if (eCSSUnit_Null == aMargin.mOutlineWidth.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineWidth.GetUnit())
      aMargin.mOutlineWidth = ourMargin->mOutlineWidth;

    // outline-color
    if (eCSSUnit_Null == aMargin.mOutlineColor.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineColor.GetUnit())
      aMargin.mOutlineColor = ourMargin->mOutlineColor;

    // outline-style
    if (eCSSUnit_Null == aMargin.mOutlineStyle.GetUnit() && eCSSUnit_Null != ourMargin->mOutlineStyle.GetUnit())
      aMargin.mOutlineStyle = ourMargin->mOutlineStyle;
  }

  return NS_OK;
}

static void 
MapDeclarationFontInto(nsICSSDeclaration* aDeclaration, 
                       nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
}

static void 
MapDeclarationTextInto(nsICSSDeclaration* aDeclaration, 
                       nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                       nsStyleFont* aFont, nsIPresContext* aPresContext)
{
  const nsStyleFont* parentFont = aFont;
  if (nsnull != aParentContext) {
    parentFont = (const nsStyleFont*)aParentContext->GetStyleData(eStyleStruct_Font);
  }

  nsCSSText*  ourText;
  if (NS_OK == aDeclaration->GetData(kCSSTextSID, (nsCSSStruct**)&ourText)) {
    if (nsnull != ourText) {
      // Get our text style and our parent's text style
      nsStyleText* text = (nsStyleText*) aContext->GetMutableStyleData(eStyleStruct_Text);
      const nsStyleText* parentText = text;
      if (nsnull != aParentContext) {
        parentText = (const nsStyleText*)aParentContext->GetStyleData(eStyleStruct_Text);
      }

      // letter-spacing: normal, length, inherit
      SetCoord(ourText->mLetterSpacing, text->mLetterSpacing, parentText->mLetterSpacing,
               SETCOORD_LH | SETCOORD_NORMAL, aFont->mFont, aPresContext);

      // line-height: normal, number, length, percent, inherit
      SetCoord(ourText->mLineHeight, text->mLineHeight, parentText->mLineHeight,
               SETCOORD_LPFHN, aFont->mFont, aPresContext);

      // text-align: enum, string, inherit
      if (eCSSUnit_Enumerated == ourText->mTextAlign.GetUnit()) {
        text->mTextAlign = ourText->mTextAlign.GetIntValue();
      }
      else if (eCSSUnit_String == ourText->mTextAlign.GetUnit()) {
        NS_NOTYETIMPLEMENTED("align string");
      }
      else if (eCSSUnit_Inherit == ourText->mTextAlign.GetUnit()) {
        text->mTextAlign = parentText->mTextAlign;
      }

      // text-indent: length, percent, inherit
      SetCoord(ourText->mTextIndent, text->mTextIndent, parentText->mTextIndent,
               SETCOORD_LPH, aFont->mFont, aPresContext);

      // text-decoration: none, enum (bit field), inherit
      if (eCSSUnit_Enumerated == ourText->mDecoration.GetUnit()) {
        PRInt32 td = ourText->mDecoration.GetIntValue();
        text->mTextDecorations = parentText->mTextDecorations | td;
        text->mTextDecoration = td;
      }
      else if (eCSSUnit_None == ourText->mDecoration.GetUnit()) {
        text->mTextDecorations = parentText->mTextDecorations;
        text->mTextDecoration = NS_STYLE_TEXT_DECORATION_NONE;
      }
      else if (eCSSUnit_Inherit == ourText->mDecoration.GetUnit()) {
        text->mTextDecorations = parentText->mTextDecorations;
        text->mTextDecoration = parentText->mTextDecoration;
      }

      // text-transform: enum, none, inherit
      if (eCSSUnit_Enumerated == ourText->mTextTransform.GetUnit()) {
        text->mTextTransform = ourText->mTextTransform.GetIntValue();
      }
      else if (eCSSUnit_None == ourText->mTextTransform.GetUnit()) {
        text->mTextTransform = NS_STYLE_TEXT_TRANSFORM_NONE;
      }
      else if (eCSSUnit_Inherit == ourText->mTextTransform.GetUnit()) {
        text->mTextTransform = parentText->mTextTransform;
      }

      // vertical-align: enum, length, percent, inherit
      if (! SetCoord(ourText->mVerticalAlign, text->mVerticalAlign, parentText->mVerticalAlign,
                     SETCOORD_LPH | SETCOORD_ENUMERATED, aFont->mFont, aPresContext)) {
      }

      // white-space: enum, normal, inherit
      if (eCSSUnit_Enumerated == ourText->mWhiteSpace.GetUnit()) {
        text->mWhiteSpace = ourText->mWhiteSpace.GetIntValue();
      }
      else if (eCSSUnit_Normal == ourText->mWhiteSpace.GetUnit()) {
        text->mWhiteSpace = NS_STYLE_WHITESPACE_NORMAL;
      }
      else if (eCSSUnit_Inherit == ourText->mWhiteSpace.GetUnit()) {
        text->mWhiteSpace = parentText->mWhiteSpace;
      }

      // word-spacing: normal, length, inherit
      SetCoord(ourText->mWordSpacing, text->mWordSpacing, parentText->mWordSpacing,
               SETCOORD_LH | SETCOORD_NORMAL, aFont->mFont, aPresContext);
#ifdef IBMBIDI
      // unicode-bidi: enum, normal, inherit
      // normal means that override prohibited
      if (eCSSUnit_Normal == ourText->mUnicodeBidi.GetUnit() ) {
        text->mUnicodeBidi = NS_STYLE_UNICODE_BIDI_NORMAL;
      }
      else {
        if (eCSSUnit_Enumerated == ourText->mUnicodeBidi.GetUnit() ) {
          text->mUnicodeBidi = ourText->mUnicodeBidi.GetIntValue();
        }
        if (NS_STYLE_UNICODE_BIDI_INHERIT == text->mUnicodeBidi) {
          text->mUnicodeBidi = parentText->mUnicodeBidi;
        }
      }
#endif // IBMBIDI
    }
  }
}

static void 
MapDeclarationDisplayInto(nsICSSDeclaration* aDeclaration, 
                          nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                          nsStyleFont* aFont, nsIPresContext* aPresContext)
{
  nsCSSDisplay*  ourDisplay;
  if (NS_OK == aDeclaration->GetData(kCSSDisplaySID, (nsCSSStruct**)&ourDisplay)) {
    if (nsnull != ourDisplay) {
      // Get our style and our parent's style
      nsStyleDisplay* display = (nsStyleDisplay*)
        aContext->GetMutableStyleData(eStyleStruct_Display);

      const nsStyleDisplay* parentDisplay = display;
      if (nsnull != aParentContext) {
        parentDisplay = (const nsStyleDisplay*)aParentContext->GetStyleData(eStyleStruct_Display);
      }

      // display: enum, none, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mDisplay.GetUnit()) {
        display->mDisplay = ourDisplay->mDisplay.GetIntValue();
      }
      else if (eCSSUnit_None == ourDisplay->mDisplay.GetUnit()) {
        display->mDisplay = NS_STYLE_DISPLAY_NONE;
      }
      else if (eCSSUnit_Inherit == ourDisplay->mDisplay.GetUnit()) {
        display->mDisplay = parentDisplay->mDisplay;
      }

      // direction: enum, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mDirection.GetUnit()) {
        display->mDirection = ourDisplay->mDirection.GetIntValue();
#ifdef IBMBIDI
        display->mExplicitDirection = display->mDirection;

        if (NS_STYLE_DIRECTION_RTL == display->mDirection) {
          aPresContext->EnableBidi();
        }
#endif // IBMBIDI
      }
      else if (eCSSUnit_Inherit == ourDisplay->mDirection.GetUnit()) {
        display->mDirection = parentDisplay->mDirection;
      }

      // clear: enum, none, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mClear.GetUnit()) {
        display->mBreakType = ourDisplay->mClear.GetIntValue();
      }
      else if (eCSSUnit_None == ourDisplay->mClear.GetUnit()) {
        display->mBreakType = NS_STYLE_CLEAR_NONE;
      }
      else if (eCSSUnit_Inherit == ourDisplay->mClear.GetUnit()) {
        display->mBreakType = parentDisplay->mBreakType;
      }

      // float: enum, none, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mFloat.GetUnit()) {
        display->mFloats = ourDisplay->mFloat.GetIntValue();
      }
      else if (eCSSUnit_None == ourDisplay->mFloat.GetUnit()) {
        display->mFloats = NS_STYLE_FLOAT_NONE;
      }
      else if (eCSSUnit_Inherit == ourDisplay->mFloat.GetUnit()) {
        display->mFloats = parentDisplay->mFloats;
      }

      // visibility: enum, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mVisibility.GetUnit()) {
        display->mVisible = ourDisplay->mVisibility.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourDisplay->mVisibility.GetUnit()) {
        display->mVisible = parentDisplay->mVisible;
      }

      // overflow: enum, auto, inherit
      if (eCSSUnit_Enumerated == ourDisplay->mOverflow.GetUnit()) {
        display->mOverflow = ourDisplay->mOverflow.GetIntValue();
      }
      else if (eCSSUnit_Auto == ourDisplay->mOverflow.GetUnit()) {
        display->mOverflow = NS_STYLE_OVERFLOW_AUTO;
      }
      else if (eCSSUnit_Inherit == ourDisplay->mOverflow.GetUnit()) {
        display->mOverflow = parentDisplay->mOverflow;
      }

      // clip property: length, auto, inherit
      if (nsnull != ourDisplay->mClip) {
        if (eCSSUnit_Inherit == ourDisplay->mClip->mTop.GetUnit()) { // if one is inherit, they all are
          display->mClipFlags = NS_STYLE_CLIP_INHERIT;
        }
        else {
          PRBool  fullAuto = PR_TRUE;

          display->mClipFlags = 0; // clear it

          if (eCSSUnit_Auto == ourDisplay->mClip->mTop.GetUnit()) {
            display->mClip.y = 0;
            display->mClipFlags |= NS_STYLE_CLIP_TOP_AUTO;
          } 
          else if (ourDisplay->mClip->mTop.IsLengthUnit()) {
            display->mClip.y = CalcLength(ourDisplay->mClip->mTop, aFont->mFont, aPresContext);
            fullAuto = PR_FALSE;
          }
          if (eCSSUnit_Auto == ourDisplay->mClip->mBottom.GetUnit()) {
            display->mClip.height = 0;
            display->mClipFlags |= NS_STYLE_CLIP_BOTTOM_AUTO;
          } 
          else if (ourDisplay->mClip->mBottom.IsLengthUnit()) {
            display->mClip.height = CalcLength(ourDisplay->mClip->mBottom, aFont->mFont, aPresContext) -
                                    display->mClip.y;
            fullAuto = PR_FALSE;
          }
          if (eCSSUnit_Auto == ourDisplay->mClip->mLeft.GetUnit()) {
            display->mClip.x = 0;
            display->mClipFlags |= NS_STYLE_CLIP_LEFT_AUTO;
          } 
          else if (ourDisplay->mClip->mLeft.IsLengthUnit()) {
            display->mClip.x = CalcLength(ourDisplay->mClip->mLeft, aFont->mFont, aPresContext);
            fullAuto = PR_FALSE;
          }
          if (eCSSUnit_Auto == ourDisplay->mClip->mRight.GetUnit()) {
            display->mClip.width = 0;
            display->mClipFlags |= NS_STYLE_CLIP_RIGHT_AUTO;
          } 
          else if (ourDisplay->mClip->mRight.IsLengthUnit()) {
            display->mClip.width = CalcLength(ourDisplay->mClip->mRight, aFont->mFont, aPresContext) -
                                   display->mClip.x;
            fullAuto = PR_FALSE;
          }
          display->mClipFlags &= ~NS_STYLE_CLIP_TYPE_MASK;
          if (fullAuto) {
            display->mClipFlags |= NS_STYLE_CLIP_AUTO;
          }
          else {
            display->mClipFlags |= NS_STYLE_CLIP_RECT;
          }
        }
      }
    }
  }
}

static void 
MapDeclarationColorInto(nsICSSDeclaration* aDeclaration, 
                        nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                        nsStyleFont* aFont, nsIPresContext* aPresContext)
{
  nsCSSColor*  ourColor;
  if (NS_OK == aDeclaration->GetData(kCSSColorSID, (nsCSSStruct**)&ourColor)) {
    if (nsnull != ourColor) {
      nsStyleColor* color = (nsStyleColor*)aContext->GetMutableStyleData(eStyleStruct_Color);

      const nsStyleColor* parentColor = color;
      if (nsnull != aParentContext) {
        parentColor = (const nsStyleColor*)aParentContext->GetStyleData(eStyleStruct_Color);
      }

      // color: color, string, inherit
      if (! SetColor(ourColor->mColor, parentColor->mColor, aPresContext, color->mColor)) {
      }

      // cursor: enum, auto, url, inherit
      nsCSSValueList*  list = ourColor->mCursor;
      if (nsnull != list) {
        // XXX need to deal with multiple URL values
        if (eCSSUnit_Enumerated == list->mValue.GetUnit()) {
          color->mCursor = list->mValue.GetIntValue();
        }
        else if (eCSSUnit_Auto == list->mValue.GetUnit()) {
          color->mCursor = NS_STYLE_CURSOR_AUTO;
        }
        else if (eCSSUnit_URL == list->mValue.GetUnit()) {
          list->mValue.GetStringValue(color->mCursorImage);
        }
        else if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
          color->mCursor = parentColor->mCursor;
        }
      }

      // background-color: color, string, enum (flags), inherit
      if (eCSSUnit_Inherit == ourColor->mBackColor.GetUnit()) { // do inherit first, so SetColor doesn't do it
        const nsStyleColor* inheritColor = parentColor;
        if (inheritColor->mBackgroundFlags & NS_STYLE_BG_PROPAGATED_TO_PARENT) {
          // walk up the contexts until we get to a context that does not have its
          // background propagated to its parent (or a context that has had its background
          // propagated from its child)
          if (nsnull != aParentContext) {
            nsCOMPtr<nsIStyleContext> higherContext = getter_AddRefs(aParentContext->GetParent());
            do {
              if (higherContext) {
                inheritColor = (const nsStyleColor*)higherContext->GetStyleData(eStyleStruct_Color);
                if (inheritColor && 
                    (!(inheritColor->mBackgroundFlags & NS_STYLE_BG_PROPAGATED_TO_PARENT)) ||
                    (inheritColor->mBackgroundFlags & NS_STYLE_BG_PROPAGATED_FROM_CHILD)) {
                  // done walking up the higher contexts
                  break;
                }
                higherContext = getter_AddRefs(higherContext->GetParent());
              }
            } while (higherContext);
          }
        }
        color->mBackgroundColor = inheritColor->mBackgroundColor;
        color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
        color->mBackgroundFlags |= (inheritColor->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT);
      }
      else if (SetColor(ourColor->mBackColor, parentColor->mBackgroundColor, 
                        aPresContext, color->mBackgroundColor)) {
        color->mBackgroundFlags &= ~NS_STYLE_BG_COLOR_TRANSPARENT;
      }
      else if (eCSSUnit_Enumerated == ourColor->mBackColor.GetUnit()) {
        color->mBackgroundColor = parentColor->mBackgroundColor;
        color->mBackgroundFlags |= NS_STYLE_BG_COLOR_TRANSPARENT;
      }

      // background-image: url, none, inherit
      if (eCSSUnit_URL == ourColor->mBackImage.GetUnit()) {
        ourColor->mBackImage.GetStringValue(color->mBackgroundImage);
        color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
      }
      else if (eCSSUnit_None == ourColor->mBackImage.GetUnit()) {
        color->mBackgroundImage.Truncate();
        color->mBackgroundFlags |= NS_STYLE_BG_IMAGE_NONE;
      }
      else if (eCSSUnit_Inherit == ourColor->mBackImage.GetUnit()) {
        color->mBackgroundImage = parentColor->mBackgroundImage;
        color->mBackgroundFlags &= ~NS_STYLE_BG_IMAGE_NONE;
        color->mBackgroundFlags |= (parentColor->mBackgroundFlags & NS_STYLE_BG_IMAGE_NONE);
      }

      // background-repeat: enum, inherit
      if (eCSSUnit_Enumerated == ourColor->mBackRepeat.GetUnit()) {
        color->mBackgroundRepeat = ourColor->mBackRepeat.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourColor->mBackRepeat.GetUnit()) {
        color->mBackgroundRepeat = parentColor->mBackgroundRepeat;
      }

      // background-attachment: enum, inherit
      if (eCSSUnit_Enumerated == ourColor->mBackAttachment.GetUnit()) {
        color->mBackgroundAttachment = ourColor->mBackAttachment.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourColor->mBackAttachment.GetUnit()) {
        color->mBackgroundAttachment = parentColor->mBackgroundAttachment;
      }

      // background-position: enum, length, percent (flags), inherit
      if (eCSSUnit_Percent == ourColor->mBackPositionX.GetUnit()) {
        color->mBackgroundXPosition = (nscoord)(100.0f * ourColor->mBackPositionX.GetPercentValue());
        color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_PERCENT;
        color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_LENGTH;
      }
      else if (ourColor->mBackPositionX.IsLengthUnit()) {
        color->mBackgroundXPosition = CalcLength(ourColor->mBackPositionX,
                                                 aFont->mFont, aPresContext);
        color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_LENGTH;
        color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_PERCENT;
      }
      else if (eCSSUnit_Enumerated == ourColor->mBackPositionX.GetUnit()) {
        color->mBackgroundXPosition = (nscoord)ourColor->mBackPositionX.GetIntValue();
        color->mBackgroundFlags |= NS_STYLE_BG_X_POSITION_PERCENT;
        color->mBackgroundFlags &= ~NS_STYLE_BG_X_POSITION_LENGTH;
      }
      else if (eCSSUnit_Inherit == ourColor->mBackPositionX.GetUnit()) {
        color->mBackgroundXPosition = parentColor->mBackgroundXPosition;
        color->mBackgroundFlags &= ~(NS_STYLE_BG_X_POSITION_LENGTH | NS_STYLE_BG_X_POSITION_PERCENT);
        color->mBackgroundFlags |= (parentColor->mBackgroundFlags & (NS_STYLE_BG_X_POSITION_LENGTH | NS_STYLE_BG_X_POSITION_PERCENT));
      }

      if (eCSSUnit_Percent == ourColor->mBackPositionY.GetUnit()) {
        color->mBackgroundYPosition = (nscoord)(100.0f * ourColor->mBackPositionY.GetPercentValue());
        color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_PERCENT;
        color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_LENGTH;
      }
      else if (ourColor->mBackPositionY.IsLengthUnit()) {
        color->mBackgroundYPosition = CalcLength(ourColor->mBackPositionY,
                                                 aFont->mFont, aPresContext);
        color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_LENGTH;
        color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_PERCENT;
      }
      else if (eCSSUnit_Enumerated == ourColor->mBackPositionY.GetUnit()) {
        color->mBackgroundYPosition = (nscoord)ourColor->mBackPositionY.GetIntValue();
        color->mBackgroundFlags |= NS_STYLE_BG_Y_POSITION_PERCENT;
        color->mBackgroundFlags &= ~NS_STYLE_BG_Y_POSITION_LENGTH;
      }
      else if (eCSSUnit_Inherit == ourColor->mBackPositionY.GetUnit()) {
        color->mBackgroundYPosition = parentColor->mBackgroundYPosition;
        color->mBackgroundFlags &= ~(NS_STYLE_BG_Y_POSITION_LENGTH | NS_STYLE_BG_Y_POSITION_PERCENT);
        color->mBackgroundFlags |= (parentColor->mBackgroundFlags & (NS_STYLE_BG_Y_POSITION_LENGTH | NS_STYLE_BG_Y_POSITION_PERCENT));
      }

      // opacity: factor, percent, inherit
      if (eCSSUnit_Percent == ourColor->mOpacity.GetUnit()) {
        float opacity = parentColor->mOpacity * ourColor->mOpacity.GetPercentValue();
        if (opacity < 0.0f) {
          color->mOpacity = 0.0f;
        } else if (1.0 < opacity) {
          color->mOpacity = 1.0f;
        }
        else {
          color->mOpacity = opacity;
        }
      }
      else if (eCSSUnit_Number == ourColor->mOpacity.GetUnit()) {
        color->mOpacity = ourColor->mOpacity.GetFloatValue();
      }
      else if (eCSSUnit_Inherit == ourColor->mOpacity.GetUnit()) {
        color->mOpacity = parentColor->mOpacity;
      }
    }
  }
}

static void 
MapDeclarationMarginInto(nsICSSDeclaration* aDeclaration, 
                         nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                         nsStyleFont* aFont, nsIPresContext* aPresContext)
{
}

static void 
MapDeclarationPositionInto(nsICSSDeclaration* aDeclaration, 
                           nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                           nsStyleFont* aFont, nsIPresContext* aPresContext)
{
}

static void 
MapDeclarationListInto(nsICSSDeclaration* aDeclaration, 
                       nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                       nsStyleFont* /*aFont*/, nsIPresContext* aPresContext)
{
}

static void 
MapDeclarationTableInto(nsICSSDeclaration* aDeclaration, 
                        nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                        nsStyleFont* aFont, nsIPresContext* aPresContext)
{
  nsCSSTable* ourTable;
  if (NS_OK == aDeclaration->GetData(kCSSTableSID, (nsCSSStruct**)&ourTable)) {
    if (nsnull != ourTable) {
      nsStyleTable* table = (nsStyleTable*)aContext->GetMutableStyleData(eStyleStruct_Table);

      const nsStyleTable* parentTable = table;
      if (nsnull != aParentContext) {
        parentTable = (const nsStyleTable*)aParentContext->GetStyleData(eStyleStruct_Table);
      }
      nsStyleCoord  coord;

      // border-collapse: enum, inherit
      if (eCSSUnit_Enumerated == ourTable->mBorderCollapse.GetUnit()) {
        table->mBorderCollapse = ourTable->mBorderCollapse.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourTable->mBorderCollapse.GetUnit()) {
        table->mBorderCollapse = parentTable->mBorderCollapse;
      }

      // border-spacing-x: length, inherit
      if (SetCoord(ourTable->mBorderSpacingX, coord, coord, SETCOORD_LENGTH, aFont->mFont, aPresContext)) {
        table->mBorderSpacingX = coord.GetCoordValue();
      }
      else if (eCSSUnit_Inherit == ourTable->mBorderSpacingX.GetUnit()) {
        table->mBorderSpacingX = parentTable->mBorderSpacingX;
      }
      // border-spacing-y: length, inherit
      if (SetCoord(ourTable->mBorderSpacingY, coord, coord, SETCOORD_LENGTH, aFont->mFont, aPresContext)) {
        table->mBorderSpacingY = coord.GetCoordValue();
      }
      else if (eCSSUnit_Inherit == ourTable->mBorderSpacingY.GetUnit()) {
        table->mBorderSpacingY = parentTable->mBorderSpacingY;
      }

      // caption-side: enum, inherit
      if (eCSSUnit_Enumerated == ourTable->mCaptionSide.GetUnit()) {
        table->mCaptionSide = ourTable->mCaptionSide.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourTable->mCaptionSide.GetUnit()) {
        table->mCaptionSide = parentTable->mCaptionSide;
      }

      // empty-cells: enum, inherit
      if (eCSSUnit_Enumerated == ourTable->mEmptyCells.GetUnit()) {
        table->mEmptyCells = ourTable->mEmptyCells.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourTable->mEmptyCells.GetUnit()) {
        table->mEmptyCells = parentTable->mEmptyCells;
      }

      // table-layout: auto, enum, inherit
      if (eCSSUnit_Enumerated == ourTable->mLayout.GetUnit()) {
        table->mLayoutStrategy = ourTable->mLayout.GetIntValue();
      }
      else if (eCSSUnit_Auto == ourTable->mLayout.GetUnit()) {
        table->mLayoutStrategy = NS_STYLE_TABLE_LAYOUT_AUTO;
      }
      else if (eCSSUnit_Inherit == ourTable->mLayout.GetUnit()) {
        table->mLayoutStrategy = parentTable->mLayoutStrategy;
      }
    }
  }
}

static void 
MapDeclarationContentInto(nsICSSDeclaration* aDeclaration, 
                          nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                          nsStyleFont* aFont, nsIPresContext* aPresContext)
{
  nsCSSContent* ourContent;
  if (NS_OK == aDeclaration->GetData(kCSSContentSID, (nsCSSStruct**)&ourContent)) {
    if (ourContent) {
      nsStyleContent* content = (nsStyleContent*)aContext->GetMutableStyleData(eStyleStruct_Content);

      const nsStyleContent* parentContent = content;
      if (nsnull != aParentContext) {
        parentContent = (const nsStyleContent*)aParentContext->GetStyleData(eStyleStruct_Content);
      }

      PRUint32 count;
      nsAutoString  buffer;

      // content: [string, url, counter, attr, enum]+, inherit
      nsCSSValueList* contentValue = ourContent->mContent;
      if (contentValue) {
        if (eCSSUnit_Inherit == contentValue->mValue.GetUnit()) {
          count = parentContent->ContentCount();
          if (NS_SUCCEEDED(content->AllocateContents(count))) {
            nsStyleContentType type;
            while (0 < count--) {
              parentContent->GetContentAt(count, type, buffer);
              content->SetContentAt(count, type, buffer);
            }
          }
        }
        else {
          count = 0;
          while (contentValue) {
            count++;
            contentValue = contentValue->mNext;
          }
          if (NS_SUCCEEDED(content->AllocateContents(count))) {
            const nsAutoString  nullStr;
            count = 0;
            contentValue = ourContent->mContent;
            while (contentValue) {
              const nsCSSValue& value = contentValue->mValue;
              nsCSSUnit unit = value.GetUnit();
              nsStyleContentType type;
              switch (unit) {
                case eCSSUnit_String:   type = eStyleContentType_String;    break;
                case eCSSUnit_URL:      type = eStyleContentType_URL;       break;
                case eCSSUnit_Attr:     type = eStyleContentType_Attr;      break;
                case eCSSUnit_Counter:  type = eStyleContentType_Counter;   break;
                case eCSSUnit_Counters: type = eStyleContentType_Counters;  break;
                case eCSSUnit_Enumerated:
                  switch (value.GetIntValue()) {
                    case NS_STYLE_CONTENT_OPEN_QUOTE:     
                      type = eStyleContentType_OpenQuote;     break;
                    case NS_STYLE_CONTENT_CLOSE_QUOTE:
                      type = eStyleContentType_CloseQuote;    break;
                    case NS_STYLE_CONTENT_NO_OPEN_QUOTE:
                      type = eStyleContentType_NoOpenQuote;   break;
                    case NS_STYLE_CONTENT_NO_CLOSE_QUOTE:
                      type = eStyleContentType_NoCloseQuote;  break;
                    default:
                      NS_ERROR("bad content value");
                  }
                  break;
                default:
                  NS_ERROR("bad content type");
              }
              if (type < eStyleContentType_OpenQuote) {
                value.GetStringValue(buffer);
                Unquote(buffer);
                content->SetContentAt(count++, type, buffer);
              }
              else {
                content->SetContentAt(count++, type, nullStr);
              }
              contentValue = contentValue->mNext;
            }
          } 
        }
      }

      // counter-increment: [string [int]]+, none, inherit
      nsCSSCounterData* ourIncrement = ourContent->mCounterIncrement;
      if (ourIncrement) {
        PRInt32 increment;
        if (eCSSUnit_Inherit == ourIncrement->mCounter.GetUnit()) {
          count = parentContent->CounterIncrementCount();
          if (NS_SUCCEEDED(content->AllocateCounterIncrements(count))) {
            while (0 < count--) {
              parentContent->GetCounterIncrementAt(count, buffer, increment);
              content->SetCounterIncrementAt(count, buffer, increment);
            }
          }
        }
        else if (eCSSUnit_None == ourIncrement->mCounter.GetUnit()) {
          content->AllocateCounterIncrements(0);
        }
        else if (eCSSUnit_String == ourIncrement->mCounter.GetUnit()) {
          count = 0;
          while (ourIncrement) {
            count++;
            ourIncrement = ourIncrement->mNext;
          }
          if (NS_SUCCEEDED(content->AllocateCounterIncrements(count))) {
            count = 0;
            ourIncrement = ourContent->mCounterIncrement;
            while (ourIncrement) {
              if (eCSSUnit_Integer == ourIncrement->mValue.GetUnit()) {
                increment = ourIncrement->mValue.GetIntValue();
              }
              else {
                increment = 1;
              }
              ourIncrement->mCounter.GetStringValue(buffer);
              content->SetCounterIncrementAt(count++, buffer, increment);
              ourIncrement = ourIncrement->mNext;
            }
          }
        }
      }

      // counter-reset: [string [int]]+, none, inherit
      nsCSSCounterData* ourReset = ourContent->mCounterReset;
      if (ourReset) {
        PRInt32 reset;
        if (eCSSUnit_Inherit == ourReset->mCounter.GetUnit()) {
          count = parentContent->CounterResetCount();
          if (NS_SUCCEEDED(content->AllocateCounterResets(count))) {
            while (0 < count--) {
              parentContent->GetCounterResetAt(count, buffer, reset);
              content->SetCounterResetAt(count, buffer, reset);
            }
          }
        }
        else if (eCSSUnit_None == ourReset->mCounter.GetUnit()) {
          content->AllocateCounterResets(0);
        }
        else if (eCSSUnit_String == ourReset->mCounter.GetUnit()) {
          count = 0;
          while (ourReset) {
            count++;
            ourReset = ourReset->mNext;
          }
          if (NS_SUCCEEDED(content->AllocateCounterResets(count))) {
            count = 0;
            ourReset = ourContent->mCounterReset;
            while (ourReset) {
              if (eCSSUnit_Integer == ourReset->mValue.GetUnit()) {
                reset = ourReset->mValue.GetIntValue();
              }
              else {
                reset = 0;
              }
              ourReset->mCounter.GetStringValue(buffer);
              content->SetCounterResetAt(count++, buffer, reset);
              ourReset = ourReset->mNext;
            }
          }
        }
      }

      // marker-offset: length, auto, inherit
      if (! SetCoord(ourContent->mMarkerOffset, content->mMarkerOffset, parentContent->mMarkerOffset,
                     SETCOORD_LH | SETCOORD_AUTO, aFont->mFont, aPresContext)) {
      }

      // quotes: [string string]+, none, inherit
      nsCSSQuotes* ourQuotes = ourContent->mQuotes;
      if (ourQuotes) {
        nsAutoString  closeBuffer;
        if (eCSSUnit_Inherit == ourQuotes->mOpen.GetUnit()) {
          count = parentContent->QuotesCount();
          if (NS_SUCCEEDED(content->AllocateQuotes(count))) {
            while (0 < count--) {
              parentContent->GetQuotesAt(count, buffer, closeBuffer);
              content->SetQuotesAt(count, buffer, closeBuffer);
            }
          }
        }
        else if (eCSSUnit_None == ourQuotes->mOpen.GetUnit()) {
          content->AllocateQuotes(0);
        }
        else if (eCSSUnit_String == ourQuotes->mOpen.GetUnit()) {
          count = 0;
          while (ourQuotes) {
            count++;
            ourQuotes = ourQuotes->mNext;
          }
          if (NS_SUCCEEDED(content->AllocateQuotes(count))) {
            count = 0;
            ourQuotes = ourContent->mQuotes;
            while (ourQuotes) {
              ourQuotes->mOpen.GetStringValue(buffer);
              ourQuotes->mClose.GetStringValue(closeBuffer);
              Unquote(buffer);
              Unquote(closeBuffer);
              content->SetQuotesAt(count++, buffer, closeBuffer);
              ourQuotes = ourQuotes->mNext;
            }
          }
        }
      }
    }
  }
}

static void 
MapDeclarationUIInto(nsICSSDeclaration* aDeclaration, 
                     nsIMutableStyleContext* aContext, nsIStyleContext* aParentContext,
                     nsStyleFont* /*aFont*/, nsIPresContext* aPresContext)
{
  nsCSSUserInterface*  ourUI;
  if (NS_OK == aDeclaration->GetData(kCSSUserInterfaceSID, (nsCSSStruct**)&ourUI)) {
    if (nsnull != ourUI) {
      // Get our user interface style and our parent's user interface style
      nsStyleUserInterface* ui = (nsStyleUserInterface*) aContext->GetMutableStyleData(eStyleStruct_UserInterface);
      const nsStyleUserInterface* parentUI = ui;
      if (nsnull != aParentContext) {
        parentUI = (const nsStyleUserInterface*)aParentContext->GetStyleData(eStyleStruct_UserInterface);
      }

      // user-input: auto, none, enum, inherit
      if (eCSSUnit_Enumerated == ourUI->mUserInput.GetUnit()) {
        ui->mUserInput = ourUI->mUserInput.GetIntValue();
      }
      else if (eCSSUnit_Auto == ourUI->mUserInput.GetUnit()) {
        ui->mUserInput = NS_STYLE_USER_INPUT_AUTO;
      }
      else if (eCSSUnit_None == ourUI->mUserInput.GetUnit()) {
        ui->mUserInput = NS_STYLE_USER_INPUT_NONE;
      }
      else if (eCSSUnit_Inherit == ourUI->mUserInput.GetUnit()) {
        ui->mUserInput = parentUI->mUserInput;
      }

      // user-modify: enum, inherit
      if (eCSSUnit_Enumerated == ourUI->mUserModify.GetUnit()) {
        ui->mUserModify = ourUI->mUserModify.GetIntValue();
      }
      else if (eCSSUnit_Inherit == ourUI->mUserModify.GetUnit()) {
        ui->mUserModify = parentUI->mUserModify;
      }

      // user-select: none, enum, inherit
      if (eCSSUnit_Enumerated == ourUI->mUserSelect.GetUnit()) {
        ui->mUserSelect = ourUI->mUserSelect.GetIntValue();
      }
      else if (eCSSUnit_None == ourUI->mUserSelect.GetUnit()) {
        ui->mUserSelect = NS_STYLE_USER_SELECT_NONE;
      }
      else if (eCSSUnit_Inherit == ourUI->mUserSelect.GetUnit()) {
        ui->mUserSelect = parentUI->mUserSelect;
      }

      // behavior: url, none
      if (eCSSUnit_URL == ourUI->mBehavior.GetUnit()) {
        ourUI->mBehavior.GetStringValue(ui->mBehavior);
      }
      else if (eCSSUnit_None == ourUI->mBehavior.GetUnit()) {
        ui->mBehavior.Truncate();
      }
      else if (eCSSUnit_Inherit == ourUI->mBehavior.GetUnit()) {
        ui->mBehavior = parentUI->mBehavior;
      }

      // key-equivalent: none, enum XXX, inherit
      nsCSSValueList*  keyEquiv = ourUI->mKeyEquivalent;
      if (keyEquiv) {
        // XXX need to deal with multiple values
        if (eCSSUnit_Enumerated == keyEquiv->mValue.GetUnit()) {
          ui->mKeyEquivalent = PRUnichar(0);  // XXX To be implemented
        }
        else if (eCSSUnit_None == keyEquiv->mValue.GetUnit()) {
          ui->mKeyEquivalent = PRUnichar(0);
        }
        else if (eCSSUnit_Inherit == keyEquiv->mValue.GetUnit()) {
          ui->mKeyEquivalent = parentUI->mKeyEquivalent;
        }
      }

      // user-focus: none, normal, enum, inherit
      if (eCSSUnit_Enumerated == ourUI->mUserFocus.GetUnit()) {
        ui->mUserFocus = ourUI->mUserFocus.GetIntValue();
      }
      else if (eCSSUnit_None == ourUI->mUserFocus.GetUnit()) {
        ui->mUserFocus = NS_STYLE_USER_FOCUS_NONE;
      }
      else if (eCSSUnit_Normal == ourUI->mUserFocus.GetUnit()) {
        ui->mUserFocus = NS_STYLE_USER_FOCUS_NORMAL;
      }
      else if (eCSSUnit_Inherit == ourUI->mUserFocus.GetUnit()) {
        ui->mUserFocus = parentUI->mUserFocus;
      }

      // resizer: auto, none, enum, inherit
      if (eCSSUnit_Enumerated == ourUI->mResizer.GetUnit()) {
        ui->mResizer = ourUI->mResizer.GetIntValue();
      }
      else if (eCSSUnit_Auto == ourUI->mResizer.GetUnit()) {
        ui->mResizer = NS_STYLE_RESIZER_AUTO;
      }
      else if (eCSSUnit_None == ourUI->mResizer.GetUnit()) {
        ui->mResizer = NS_STYLE_RESIZER_NONE;
      }
      else if (eCSSUnit_Inherit == ourUI->mResizer.GetUnit()) {
        ui->mResizer = parentUI->mResizer;
      }

    }
  }
}

void MapDeclarationInto(nsICSSDeclaration* aDeclaration, 
                        nsIMutableStyleContext* aContext, nsIPresContext* aPresContext)
{
  if (nsnull != aDeclaration) {
    nsIStyleContext* parentContext = aContext->GetParent();
    nsStyleFont* font = (nsStyleFont*)aContext->GetStyleData(eStyleStruct_Font);

    MapDeclarationTextInto(aDeclaration, aContext, parentContext, font, aPresContext);
    MapDeclarationDisplayInto(aDeclaration, aContext, parentContext, font, aPresContext);
    MapDeclarationColorInto(aDeclaration, aContext, parentContext, font, aPresContext);
    MapDeclarationTableInto(aDeclaration, aContext, parentContext, font, aPresContext);
    MapDeclarationContentInto(aDeclaration, aContext, parentContext, font, aPresContext);
    MapDeclarationUIInto(aDeclaration, aContext, parentContext, font, aPresContext);
    
    NS_IF_RELEASE(parentContext);
  }
}

NS_IMETHODIMP
CSSStyleRuleImpl::List(FILE* out, PRInt32 aIndent) const
{
  // Indent
  for (PRInt32 index = aIndent; --index >= 0; ) fputs("  ", out);

  nsAutoString buffer;
  mSelector.ToString(buffer, mSheet, PR_FALSE, 0);

  buffer.AppendWithConversion(" weight: ");
  buffer.AppendInt(mWeight, 10);
  buffer.AppendWithConversion(" ");
  fputs(buffer, out);
  if (nsnull != mDeclaration) {
    mDeclaration->List(out);
  }
  else {
    fputs("{ null declaration }", out);
  }
  fputs("\n", out);

  return NS_OK;
}

/******************************************************************************
* SizeOf method:
*
*  Self (reported as CSSStyleRuleImpl's size): 
*    1) sizeof(*this) 
*       + sizeof the DOMDeclaration if it exists and is unique
*
*  Contained / Aggregated data (not reported as CSSStyleRuleImpl's size):
*    1) mDeclaration if it exists
*    2) mImportantRule if it exists
*
*  Children / siblings / parents:
*    none
*    
******************************************************************************/
void CSSStyleRuleImpl::SizeOf(nsISizeOfHandler *aSizeOfHandler, PRUint32 &aSize)
{
  NS_ASSERTION(aSizeOfHandler != nsnull, "SizeOf handler cannot be null");

  // first get the unique items collection
  UNIQUE_STYLE_ITEMS(uniqueItems);
  if(! uniqueItems->AddItem((void*)this)){
    return;
  }

  PRUint32 localSize=0;

  // create a tag for this instance
  nsCOMPtr<nsIAtom> tag;
  tag = getter_AddRefs(NS_NewAtom("CSSStyleRuleImpl"));
  // get the size of an empty instance and add to the sizeof handler
  aSize = sizeof(*this);
  // remove the sizeof the mSelector's class since we count it seperately below
  aSize -= sizeof(mSelector);

  // and add the size of the DOMDeclaration
  // XXX - investigate the size and quantity of these
  if(mDOMDeclaration && uniqueItems->AddItem(mDOMDeclaration)){
    aSize += sizeof(DOMCSSDeclarationImpl);
  }
  aSizeOfHandler->AddSize(tag,aSize);
  
  // now delegate to the Selector, Declaration, and ImportantRule
  mSelector.SizeOf(aSizeOfHandler, localSize);

  if(mDeclaration){
    mDeclaration->SizeOf(aSizeOfHandler, localSize);
  }
  if(mImportantRule){
    mImportantRule->SizeOf(aSizeOfHandler, localSize);
  }
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetType(PRUint16* aType)
{
  *aType = nsIDOMCSSRule::STYLE_RULE;
  
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetCssText(nsAWritableString& aCssText)
{
  mSelector.ToString( aCssText, mSheet, IsPseudoElement(mSelector.mTag),
                      0 );
  aCssText.Append(PRUnichar(' '));
  aCssText.Append(PRUnichar('{'));
  aCssText.Append(PRUnichar(' '));
  if (mDeclaration)
  {
    nsAutoString   tempString;
    mDeclaration->ToString( tempString );
    aCssText.Append( tempString );
  }
  aCssText.Append(PRUnichar(' '));
  aCssText.Append(PRUnichar('}'));
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::SetCssText(const nsAReadableString& aCssText)
{
  // XXX TBI - need to re-parse rule & declaration
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  if (nsnull != mSheet) {
    return mSheet->QueryInterface(NS_GET_IID(nsIDOMCSSStyleSheet), (void**)aSheet);
  }
  *aSheet = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetSelectorText(nsAWritableString& aSelectorText)
{
  mSelector.ToString( aSelectorText, mSheet, IsPseudoElement(mSelector.mTag), 0 );
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::SetSelectorText(const nsAReadableString& aSelectorText)
{
  // XXX TBI - get a parser and re-parse the selectors, 
  // XXX then need to re-compute the cascade
  // XXX and dirty sheet
  return NS_OK;
}

NS_IMETHODIMP    
CSSStyleRuleImpl::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  if (nsnull == mDOMDeclaration) {
    mDOMDeclaration = new DOMCSSDeclarationImpl(this);
    if (nsnull == mDOMDeclaration) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mDOMDeclaration);
  }
  
  *aStyle = mDOMDeclaration;
  NS_ADDREF(mDOMDeclaration);
  
  return NS_OK;
}

NS_HTML nsresult
  NS_NewCSSStyleRule(nsICSSStyleRule** aInstancePtrResult, const nsCSSSelector& aSelector)
{
  if (aInstancePtrResult == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  CSSStyleRuleImpl  *it = new CSSStyleRuleImpl(aSelector);

  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsICSSStyleRule), (void **) aInstancePtrResult);
}
