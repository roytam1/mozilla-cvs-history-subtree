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

#include "nsRuleNode.h"
#include "nsIDeviceContext.h"
#include "nsILookAndFeel.h"
#include "nsIPresShell.h"
#include "nsIFontMetrics.h"
#include "nsIDocShellTreeItem.h"
#include "nsStyleUtil.h"
#include "nsCSSAtoms.h"

nscoord CalcLength(const nsCSSValue& aValue,
                   nsFont* aFont, 
                   nsIStyleContext* aStyleContext,
                   nsIPresContext* aPresContext,
                   PRBool& aInherited)
{
  NS_ASSERTION(aValue.IsLengthUnit(), "not a length unit");
  if (aValue.IsFixedLengthUnit()) {
    return aValue.GetLengthTwips();
  }
  nsCSSUnit unit = aValue.GetUnit();
  switch (unit) {
    case eCSSUnit_EM:
    case eCSSUnit_Char: {
      aInherited = PR_TRUE;
      nsFont* font = aStyleContext ? &(((nsStyleFont*)aStyleContext->GetStyleData(eStyleStruct_Font))->mFont) : aFont;
      return NSToCoordRound(aValue.GetFloatValue() * (float)font->size);
      // XXX scale against font metrics height instead?
    }
    case eCSSUnit_EN: {
      aInherited = PR_TRUE;
      nsFont* font = aStyleContext ? &(((nsStyleFont*)aStyleContext->GetStyleData(eStyleStruct_Font))->mFont) : aFont;
      return NSToCoordRound((aValue.GetFloatValue() * (float)font->size) / 2.0f);
    }
    case eCSSUnit_XHeight: {
      aInherited = PR_TRUE;
      nsFont* font = aStyleContext ? &(((nsStyleFont*)aStyleContext->GetStyleData(eStyleStruct_Font))->mFont) : aFont;
      nsIFontMetrics* fm;
      aPresContext->GetMetricsFor(*font, &fm);
      NS_ASSERTION(nsnull != fm, "can't get font metrics");
      nscoord xHeight;
      if (nsnull != fm) {
        fm->GetXHeight(xHeight);
        NS_RELEASE(fm);
      }
      else {
        xHeight = ((font->size * 2) / 3);
      }
      return NSToCoordRound(aValue.GetFloatValue() * (float)xHeight);
    }
    case eCSSUnit_CapHeight: {
      NS_NOTYETIMPLEMENTED("cap height unit");
      aInherited = PR_TRUE;
      nsFont* font = aStyleContext ? &(((nsStyleFont*)aStyleContext->GetStyleData(eStyleStruct_Font))->mFont) : aFont;
      nscoord capHeight = ((font->size / 3) * 2); // XXX HACK!
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
                       PRInt32 aMask, nsIStyleContext* aStyleContext,
                       nsIPresContext* aPresContext, PRBool& aInherited)
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
    aCoord.SetCoordValue(CalcLength(aValue, nsnull, aStyleContext, aPresContext, aInherited));
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
    aInherited = PR_TRUE;
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
                       nsIPresContext* aPresContext, nscolor& aResult, PRBool& aInherited)
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
    aInherited = PR_TRUE;
  }
  return result;
}

// nsRuleNode globals
PRUint32 nsRuleNode::gRefCnt = 0;

NS_IMPL_ADDREF(nsRuleNode)
NS_IMPL_RELEASE_WITH_DESTROY(nsRuleNode, Destroy())
NS_IMPL_QUERY_INTERFACE1(nsRuleNode, nsIRuleNode)

// Overloaded new operator. Initializes the memory to 0 and relies on an arena
// (which comes from the presShell) to perform the allocation.
void* 
nsRuleNode::operator new(size_t sz, nsIPresContext* aPresContext)
{
  // Check the recycle list first.
  void* result = nsnull;
  aPresContext->AllocateFromShell(sz, &result);
  return result;
}

// Overridden to prevent the global delete from being called, since the memory
// came out of an nsIArena instead of the global delete operator's heap.
void 
nsRuleNode::Destroy()
{
  // Destroy ourselves.
  this->~nsRuleNode();
  
  // Don't let the memory be freed, since it will be recycled
  // instead. Don't call the global operator delete.
  mPresContext->FreeToShell(sizeof(nsRuleNode), this);
}

void nsRuleNode::CreateRootNode(nsIPresContext* aPresContext, nsIRuleNode** aResult)
{
  *aResult = new (aPresContext) nsRuleNode(aPresContext);
  NS_IF_ADDREF(*aResult);
}

nsRuleNode::nsRuleNode(nsIPresContext* aContext, nsIStyleRule* aRule, nsRuleNode* aParent)
    :mPresContext(aContext), mRule(aRule), mParent(aParent), mChildren(nsnull), mInheritBits(0)
{
  NS_INIT_REFCNT();
  gRefCnt++;
}

nsRuleNode::~nsRuleNode()
{
  delete mChildren;
  gRefCnt--;
}

NS_IMETHODIMP 
nsRuleNode::Transition(nsIStyleRule* aRule, nsIRuleNode** aResult)
{
  nsCOMPtr<nsIRuleNode> next;
  nsISupportsKey key(aRule);
  if (mChildren)
    next = getter_AddRefs(NS_STATIC_CAST(nsIRuleNode*, mChildren->Get(&key)));
  
  if (!next) {
    // Create the new entry in our table.
    nsRuleNode* newNode = new (mPresContext) nsRuleNode(mPresContext, aRule, this);
    next = newNode;

    if (!mChildren)
      mChildren = new nsSupportsHashtable(4);
    mChildren->Put(&key, next);
  }

  *aResult = next;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsRuleNode::GetParent(nsIRuleNode** aResult)
{
  *aResult = mParent;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsRuleNode::IsRoot(PRBool* aResult)
{
  *aResult = (mParent == nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsRuleNode::GetRule(nsIStyleRule** aResult)
{
  *aResult = mRule;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsRuleNode::GetPresContext(nsIPresContext** aResult)
{
  *aResult = mPresContext;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

void
nsRuleNode::PropagateBit(PRUint32 aBit, nsRuleNode* aHighestNode)
{
  if (mInheritBits & aBit)
    return; // Already set.

  nsRuleNode* curr = this;
  while (curr && curr != aHighestNode) {
    curr->mInheritBits |= aBit;
    curr = curr->mParent;
  }
}

inline PRBool 
nsRuleNode::InheritsFromParentRule(const nsStyleStructID& aSID)
{
  return mInheritBits & nsCachedStyleData::GetBitForSID(aSID);
}

nsRuleNode::RuleDetail
nsRuleNode::CheckSpecifiedProperties(const nsStyleStructID& aSID, const nsCSSStruct& aCSSStruct)
{
  switch (aSID) {
  case eStyleStruct_Font:
    return CheckFontProperties((const nsCSSFont&)aCSSStruct);
  case eStyleStruct_Margin:
      return CheckMarginProperties((const nsCSSMargin&)aCSSStruct);
  case eStyleStruct_Border:
    return CheckBorderProperties((const nsCSSMargin&)aCSSStruct);
  case eStyleStruct_Padding:
    return CheckPaddingProperties((const nsCSSMargin&)aCSSStruct);
  case eStyleStruct_Outline:
    return CheckOutlineProperties((const nsCSSMargin&)aCSSStruct);
  case eStyleStruct_List:
    return CheckListProperties((const nsCSSList&)aCSSStruct);
  case eStyleStruct_Position:
    return CheckPositionProperties((const nsCSSPosition&)aCSSStruct);
  case eStyleStruct_Table:
    return CheckTableProperties((const nsCSSTable&)aCSSStruct);
  case eStyleStruct_TableBorder:
    return CheckTableBorderProperties((const nsCSSTable&)aCSSStruct);
#ifdef INCLUDE_XUL
  case eStyleStruct_XUL:
    return CheckXULProperties((const nsCSSXUL&)aCSSStruct);
#endif
  }

  return eRuleNone;
}

const nsStyleStruct*
nsRuleNode::GetFontData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSFont fontData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Font, mPresContext, aContext);
  ruleData.mFontData = &fontData;

  return WalkRuleTree(eStyleStruct_Font, aContext, aMutableContext, &ruleData, &fontData);
}

const nsStyleStruct*
nsRuleNode::GetMarginData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSMargin marginData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Margin, mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  nsCSSRect margin;
  marginData.mMargin = &margin;
  
  const nsStyleStruct* res = WalkRuleTree(eStyleStruct_Margin, aContext, aMutableContext, &ruleData, &marginData);
  
  marginData.mMargin = nsnull;
  return res;
}

const nsStyleStruct*
nsRuleNode::GetBorderData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSMargin marginData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Border, mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  nsCSSRect borderWidth;
  nsCSSRect borderColor;
  nsCSSRect borderStyle;
  nsCSSRect borderRadius;
  marginData.mBorderWidth = &borderWidth;
  marginData.mBorderColor = &borderColor;
  marginData.mBorderStyle = &borderStyle;
  marginData.mBorderRadius = &borderRadius;
  
  const nsStyleStruct* res = WalkRuleTree(eStyleStruct_Border, aContext, aMutableContext, &ruleData, &marginData);
  
  marginData.mBorderWidth = marginData.mBorderColor = marginData.mBorderStyle = marginData.mBorderRadius = nsnull;
  return res;
}

const nsStyleStruct*
nsRuleNode::GetPaddingData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSMargin marginData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Padding, mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  nsCSSRect padding;
  marginData.mPadding = &padding;
  
  const nsStyleStruct* res = WalkRuleTree(eStyleStruct_Padding, aContext, aMutableContext, &ruleData, &marginData);
  
  marginData.mPadding = nsnull;
  return res;
}

const nsStyleStruct*
nsRuleNode::GetOutlineData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSMargin marginData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Outline, mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  nsCSSRect outlineRadius;
  marginData.mOutlineRadius = &outlineRadius;
  
  const nsStyleStruct* res = WalkRuleTree(eStyleStruct_Outline, aContext, aMutableContext, &ruleData, &marginData);
  
  marginData.mOutlineRadius = nsnull;
  return res;
}

const nsStyleStruct*
nsRuleNode::GetListData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSList listData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_List, mPresContext, aContext);
  ruleData.mListData = &listData;

  return WalkRuleTree(eStyleStruct_List, aContext, aMutableContext, &ruleData, &listData);
}

const nsStyleStruct*
nsRuleNode::GetPositionData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSPosition posData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Position, mPresContext, aContext);
  ruleData.mPositionData = &posData;

  nsCSSRect offset;
  posData.mOffset = &offset;
  
  const nsStyleStruct* res = WalkRuleTree(eStyleStruct_Position, aContext, aMutableContext, &ruleData, &posData);
  
  posData.mOffset = nsnull;
  return res;
}

const nsStyleStruct*
nsRuleNode::GetTableData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSTable tableData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_Table, mPresContext, aContext);
  ruleData.mTableData = &tableData;

  return WalkRuleTree(eStyleStruct_Table, aContext, aMutableContext, &ruleData, &tableData);
}

const nsStyleStruct*
nsRuleNode::GetTableBorderData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSTable tableData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_TableBorder, mPresContext, aContext);
  ruleData.mTableData = &tableData;

  return WalkRuleTree(eStyleStruct_TableBorder, aContext, aMutableContext, &ruleData, &tableData);
}

#ifdef INCLUDE_XUL
const nsStyleStruct*
nsRuleNode::GetXULData(nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext)
{
  nsCSSXUL xulData; // Declare a struct with null CSS values.
  nsRuleData ruleData(eStyleStruct_XUL, mPresContext, aContext);
  ruleData.mXULData = &xulData;

  return WalkRuleTree(eStyleStruct_XUL, aContext, aMutableContext, &ruleData, &xulData);
}
#endif

const nsStyleStruct*
nsRuleNode::WalkRuleTree(const nsStyleStructID& aSID, nsIStyleContext* aContext, 
                         nsIMutableStyleContext* aMutableContext, nsRuleData* aRuleData,
                         nsCSSStruct* aSpecificData)
{
  // We start at the most specific rule in the tree.  
  nsStyleStruct* startStruct = nsnull;
  
  nsCOMPtr<nsIStyleRule> rule = mRule;
  nsRuleNode* ruleNode = this;
  nsRuleNode* highestNode = nsnull;
  nsRuleNode* rootNode = this;
  RuleDetail detail = eRuleNone;
  while (ruleNode) {
    startStruct = ruleNode->mStyleData.GetStyleData(aSID);
    if (startStruct)
      break; // We found a rule with fully specified data.  We don't need to go up
             // the tree any further, since the remainder of this branch has already
             // been computed.

    // Ask the rule to fill in the properties that it specifies.
    ruleNode->GetRule(getter_AddRefs(rule));
    if (rule)
      rule->MapRuleInfoInto(aRuleData);

    // Now we check to see how many properties have been specified by the rules
    // we've examined so far.
    RuleDetail oldDetail = detail;
    detail = CheckSpecifiedProperties(aSID, *aSpecificData);
    
    if (oldDetail == eRuleNone && detail != oldDetail)
      highestNode = ruleNode;

    if (detail == eRuleFullMixed || detail == eRuleFullInherited)
      break; // We don't need to examine any more rules.  All properties have been fully specified.
    
    rootNode = ruleNode;
    ruleNode = ruleNode->mParent; // Climb up to the next rule in the tree (a less specific rule).
  }

  PRBool isReset = nsCachedStyleData::IsReset(aSID);
  if (!highestNode)
    highestNode = rootNode;

  if (detail == eRuleNone && startStruct) {
    // We specified absolutely no rule information, but a parent rule in the tree
    // specified all the rule information.  We set a bit along the branch from our
    // node in the tree to the node that specified the data that tells nodes on that
    // branch that they never need to examine their rules for this particular struct type
    // ever again.
    PropagateBit(nsCachedStyleData::GetBitForSID(aSID), ruleNode);

    // Now we just return the parent rule node's computed data.
    return startStruct;
  }
  else if (!startStruct && ((!isReset && (detail == eRuleNone || detail == eRulePartialInherited)) 
                             || detail == eRuleFullInherited)) {
    // All information must necessarily be inherited from our parent style context.
    // In the absence of any computed data in the rule tree and with
    // no rules specified that didn't have values of 'inherit', we should check our parent.
    nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
    if (parentContext) {
      // We have a parent, and so we should just inherit from the parent.
      // Set the inherit bits on our context.  These bits tell the style context that
      // it never has to go back to the rule tree for data.  Instead the style context tree
      // should be walked to find the data.
      const nsStyleStruct* parentStruct = parentContext->GetStyleData(aSID);
      aContext->AddInheritBit(nsCachedStyleData::GetBitForSID(aSID));
      return parentStruct;
    }
    else
      // XXXdwh In quirks mode, tables must simulate a "root".  Figure out how
      // to do this!
      // We are the root.  In the case of fonts, the default values just
      // come from the pres context.
      return SetDefaultOnRoot(aSID, aMutableContext);
  }

  // We need to compute the data from the information that the rules specified.
  return ComputeStyleData(aSID, startStruct, *aSpecificData, aContext, aMutableContext, highestNode, detail);
}

const nsStyleStruct*
nsRuleNode::SetDefaultOnRoot(const nsStyleStructID& aSID, nsIMutableStyleContext* aMutableContext)
{
  switch (aSID) {
    case eStyleStruct_Font: 
    {
      const nsFont& defaultFont = mPresContext->GetDefaultFontDeprecated();
      const nsFont& defaultFixedFont = mPresContext->GetDefaultFixedFontDeprecated();
      nsStyleFont* fontData = new (mPresContext) nsStyleFont(defaultFont, defaultFixedFont);
      aMutableContext->SetStyle(eStyleStruct_Font, *fontData);
      return fontData;
    }
    case eStyleStruct_Margin:
    {
      nsStyleMargin* margin = new (mPresContext) nsStyleMargin();
      aMutableContext->SetStyle(eStyleStruct_Margin, *margin);
      return margin;
    }
    case eStyleStruct_Border:
    {
      nsStyleBorder* border = new (mPresContext) nsStyleBorder(mPresContext);
      aMutableContext->SetStyle(eStyleStruct_Border, *border);
      return border;
    }
    case eStyleStruct_Padding:
    {
      nsStylePadding* padding = new (mPresContext) nsStylePadding();
      aMutableContext->SetStyle(eStyleStruct_Padding, *padding);
      return padding;
    }
    case eStyleStruct_Outline:
    {
      nsStyleOutline* outline = new (mPresContext) nsStyleOutline(mPresContext);
      aMutableContext->SetStyle(eStyleStruct_Outline, *outline);
      return outline;
    }
    case eStyleStruct_List:
    {
      nsStyleList* list = new (mPresContext) nsStyleList();
      aMutableContext->SetStyle(eStyleStruct_List, *list);
      return list;
    }
    case eStyleStruct_Position:
    {
      nsStylePosition* pos = new (mPresContext) nsStylePosition();
      aMutableContext->SetStyle(eStyleStruct_Position, *pos);
      return pos;
    }
    case eStyleStruct_Table:
    {
      nsStyleTable* table = new (mPresContext) nsStyleTable();
      aMutableContext->SetStyle(eStyleStruct_Table, *table);
      return table;
    }
    case eStyleStruct_TableBorder:
    {
      nsStyleTableBorder* table = new (mPresContext) nsStyleTableBorder(mPresContext);
      aMutableContext->SetStyle(eStyleStruct_TableBorder, *table);
      return table;
    }
#ifdef INCLUDE_XUL
    case eStyleStruct_XUL:
    {
      nsStyleXUL* xul = new (mPresContext) nsStyleXUL();
      aMutableContext->SetStyle(eStyleStruct_XUL, *xul);
      return xul;
    }
#endif
  }
  return nsnull;
}

const nsStyleStruct* 
nsRuleNode::ComputeStyleData(const nsStyleStructID& aSID, nsStyleStruct* aStartStruct, const nsCSSStruct& aStartData, 
                             nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail& aRuleDetail)
{

  switch (aSID) {
  case eStyleStruct_Font:
    return ComputeFontData((nsStyleFont*)aStartStruct, (const nsCSSFont&)aStartData,
                           aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Margin:
    return ComputeMarginData((nsStyleMargin*)aStartStruct, (const nsCSSMargin&)aStartData, 
                             aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Border:
    return ComputeBorderData((nsStyleBorder*)aStartStruct, (const nsCSSMargin&)aStartData, 
                             aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Padding:
    return ComputePaddingData((nsStylePadding*)aStartStruct, (const nsCSSMargin&)aStartData, 
                              aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Outline:
    return ComputeOutlineData((nsStyleOutline*)aStartStruct, (const nsCSSMargin&)aStartData, 
                              aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_List:
    return ComputeListData((nsStyleList*)aStartStruct, (const nsCSSList&)aStartData, 
                           aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Position:
    return ComputePositionData((nsStylePosition*)aStartStruct, (const nsCSSPosition&)aStartData, 
                               aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_Table:
    return ComputeTableData((nsStyleTable*)aStartStruct, (const nsCSSTable&)aStartData, 
                            aContext, aMutableContext, aHighestNode, aRuleDetail);
  case eStyleStruct_TableBorder:
    return ComputeTableBorderData((nsStyleTableBorder*)aStartStruct, (const nsCSSTable&)aStartData, 
                                  aContext, aMutableContext, aHighestNode, aRuleDetail);
#ifdef INCLUDE_XUL
  case eStyleStruct_XUL:
    return ComputeXULData((nsStyleXUL*)aStartStruct, (const nsCSSXUL&)aStartData, 
                           aContext, aMutableContext, aHighestNode, aRuleDetail);
#endif
  }

  return nsnull;
}

const nsStyleStruct* 
nsRuleNode::ComputeFontData(nsStyleFont* aStartFont, const nsCSSFont& aFontData, 
                            nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW FONT CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  const nsFont& defaultFont = mPresContext->GetDefaultFontDeprecated();
  const nsFont& defaultFixedFont = mPresContext->GetDefaultFixedFontDeprecated();

  nsStyleFont* font = nsnull;
  nsStyleFont* parentFont = font;
  PRBool inherited = PR_FALSE;

  if (aStartFont)
    // We only need to compute the delta between this computed data and our
    // computed data.
    font = new (mPresContext) nsStyleFont(*aStartFont);
  else {
    if (aRuleDetail != eRuleFullMixed) {
      // No question. We will have to inherit. Go ahead and init
      // with inherited vals from parent.
      inherited = PR_TRUE;
      if (parentContext)
        parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
      if (parentFont)
        font = new (mPresContext) nsStyleFont(*parentFont);
    }
  }

  if (!font)
    font = parentFont = new (mPresContext) nsStyleFont(defaultFont, defaultFixedFont);

  // font-family: string list, enum, inherit
  if (eCSSUnit_String == aFontData.mFamily.GetUnit()) {
    nsCOMPtr<nsIDeviceContext> dc;
    mPresContext->GetDeviceContext(getter_AddRefs(dc));
    if (dc) {
      nsAutoString  familyList;
      aFontData.mFamily.GetStringValue(familyList);
      font->mFont.name = familyList;
      nsAutoString  face;

      // MJA: bug 31816
      // if we are not using document fonts, but this is a xul document,
      // then we set the chromeOverride bit so we use the document fonts anyway
      PRBool chromeOverride = PR_FALSE;
      PRBool useDocumentFonts = PR_TRUE;
      mPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts, useDocumentFonts);
      if (!useDocumentFonts) {
        // check if the prefs have been disabled for this shell
        // - if prefs are disabled then we use the document fonts anyway (yet another override)
        PRBool prefsEnabled = PR_TRUE;
        nsCOMPtr<nsIPresShell> shell;
        mPresContext->GetShell(getter_AddRefs(shell));
        if (shell)
          shell->ArePrefStyleRulesEnabled(prefsEnabled);
        if (!prefsEnabled)
          useDocumentFonts = PR_TRUE;
        else {
          // see if we are in the chrome, if so, use the document fonts (override the useDocFonts setting)
          nsresult result = NS_OK;
          nsCOMPtr<nsISupports> container;
          result = mPresContext->GetContainer(getter_AddRefs(container));
          if (NS_SUCCEEDED(result) && container) {
            nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(container, &result));
            if (NS_SUCCEEDED(result) && docShell){
              PRInt32 docShellType;
              result = docShell->GetItemType(&docShellType);
              if (NS_SUCCEEDED(result)){
                if (nsIDocShellTreeItem::typeChrome == docShellType){
                  chromeOverride = PR_TRUE;
                }
              }      
            }
          }
        }
      }

      // find the correct font if we are usingDocumentFonts OR we are overriding for XUL
      // MJA: bug 31816
      PRBool fontFaceOK = PR_TRUE;
      PRBool isMozFixed = font->mFont.name.EqualsIgnoreCase("-moz-fixed");
      if (chromeOverride || useDocumentFonts) {
        font->mFont.name += nsAutoString(NS_LITERAL_STRING(",")) + defaultFont.name;
        font->mFixedFont.name += nsAutoString(NS_LITERAL_STRING(",")) + defaultFixedFont.name;
      }
      else {
        // now set to defaults
        font->mFont.name = defaultFont.name;
        font->mFixedFont.name = defaultFixedFont.name;
      }

      // set to monospace if using moz-fixed
      if (isMozFixed)
        font->mFlags |= NS_STYLE_FONT_USE_FIXED;
      else
        font->mFlags &= ~NS_STYLE_FONT_USE_FIXED;
      font->mFlags |= NS_STYLE_FONT_FACE_EXPLICIT;
    }
  }
  else if (eCSSUnit_Enumerated == aFontData.mFamily.GetUnit()) {
    nsSystemAttrID sysID;
    switch (aFontData.mFamily.GetIntValue()) {
      // If you add fonts to this list, you need to also patch the list
      // in CheckFontProperties (also in this file above).
      case NS_STYLE_FONT_CAPTION:       sysID = eSystemAttr_Font_Caption;       break;    // css2
      case NS_STYLE_FONT_ICON:          sysID = eSystemAttr_Font_Icon;          break;
      case NS_STYLE_FONT_MENU:          sysID = eSystemAttr_Font_Menu;          break;
      case NS_STYLE_FONT_MESSAGE_BOX:   sysID = eSystemAttr_Font_MessageBox;    break;
      case NS_STYLE_FONT_SMALL_CAPTION: sysID = eSystemAttr_Font_SmallCaption;  break;
      case NS_STYLE_FONT_STATUS_BAR:    sysID = eSystemAttr_Font_StatusBar;     break;
      case NS_STYLE_FONT_WINDOW:        sysID = eSystemAttr_Font_Window;        break;    // css3
      case NS_STYLE_FONT_DOCUMENT:      sysID = eSystemAttr_Font_Document;      break;
      case NS_STYLE_FONT_WORKSPACE:     sysID = eSystemAttr_Font_Workspace;     break;
      case NS_STYLE_FONT_DESKTOP:       sysID = eSystemAttr_Font_Desktop;       break;
      case NS_STYLE_FONT_INFO:          sysID = eSystemAttr_Font_Info;          break;
      case NS_STYLE_FONT_DIALOG:        sysID = eSystemAttr_Font_Dialog;        break;
      case NS_STYLE_FONT_BUTTON:        sysID = eSystemAttr_Font_Button;        break;
      case NS_STYLE_FONT_PULL_DOWN_MENU:sysID = eSystemAttr_Font_PullDownMenu;  break;
      case NS_STYLE_FONT_LIST:          sysID = eSystemAttr_Font_List;          break;
      case NS_STYLE_FONT_FIELD:         sysID = eSystemAttr_Font_Field;         break;
    }

    nsCompatibility mode;
    mPresContext->GetCompatibilityMode(&mode);
		nsCOMPtr<nsIDeviceContext> dc;
    mPresContext->GetDeviceContext(getter_AddRefs(dc));
    if (dc) {
      SystemAttrStruct sysInfo;
      sysInfo.mFont = &font->mFont;
      font->mFont.size = defaultFont.size; // GetSystemAttribute sets the font face but not necessarily the size
      if (NS_FAILED(dc->GetSystemAttribute(sysID, &sysInfo))) {
        font->mFont.name = defaultFont.name;
        font->mFixedFont.name = defaultFixedFont.name;
      }
      font->mFlags |= NS_STYLE_FONT_FACE_EXPLICIT;
    }

    // NavQuirks uses sans-serif instead of whatever the native font is
    if (eCompatibility_NavQuirks == mode) {
#ifdef XP_MAC
      switch (sysID) {
        case eSystemAttr_Font_Field:
        case eSystemAttr_Font_List:
          font->mFont.name.AssignWithConversion("monospace");
          font->mFont.size = defaultFixedFont.size;
          break;
        case eSystemAttr_Font_Button:
          font->mFont.name.AssignWithConversion("serif");
          font->mFont.size = defaultFont.size;
          break;
      }
#endif

#ifdef XP_PC
      switch (sysID) {
        case eSystemAttr_Font_Field:
          font->mFont.name.AssignWithConversion("monospace");
          font->mFont.size = defaultFixedFont.size;
          break;
        case eSystemAttr_Font_Button:
        case eSystemAttr_Font_List:
          font->mFont.name.AssignWithConversion("sans-serif");
          font->mFont.size = PR_MAX(defaultFont.size - NSIntPointsToTwips(2), 0);
          break;
      }
#endif

#ifdef XP_UNIX
      switch (sysID) {
        case eSystemAttr_Font_Field:
          font->mFont.name.AssignWithConversion("monospace");
          font->mFont.size = defaultFixedFont.size;
          break;
        case eSystemAttr_Font_Button:
        case eSystemAttr_Font_List:
          font->mFont.name.AssignWithConversion("serif");
          font->mFont.size = defaultFont.size;
          break;
      }
#endif
    }
  }
  else if (eCSSUnit_Inherit == aFontData.mFamily.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.name = parentFont->mFont.name;
    font->mFixedFont.name = parentFont->mFixedFont.name;
    font->mFlags &= ~(NS_STYLE_FONT_FACE_EXPLICIT | NS_STYLE_FONT_USE_FIXED);
    font->mFlags |= (parentFont->mFlags & (NS_STYLE_FONT_FACE_EXPLICIT | NS_STYLE_FONT_USE_FIXED));
  }

  // font-style: enum, normal, inherit
  if (eCSSUnit_Enumerated == aFontData.mStyle.GetUnit()) {
    font->mFont.style = aFontData.mStyle.GetIntValue();
    font->mFixedFont.style = aFontData.mStyle.GetIntValue();
  }
  else if (eCSSUnit_Normal == aFontData.mStyle.GetUnit()) {
    font->mFont.style = NS_STYLE_FONT_STYLE_NORMAL;
    font->mFixedFont.style = NS_STYLE_FONT_STYLE_NORMAL;
  }
  else if (eCSSUnit_Inherit == aFontData.mStyle.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.style = parentFont->mFont.style;
    font->mFixedFont.style = parentFont->mFixedFont.style;
  }

  // font-variant: enum, normal, inherit
  if (eCSSUnit_Enumerated == aFontData.mVariant.GetUnit()) {
    font->mFont.variant = aFontData.mVariant.GetIntValue();
    font->mFixedFont.variant = aFontData.mVariant.GetIntValue();
  }
  else if (eCSSUnit_Normal == aFontData.mVariant.GetUnit()) {
    font->mFont.variant = NS_STYLE_FONT_VARIANT_NORMAL;
    font->mFixedFont.variant = NS_STYLE_FONT_VARIANT_NORMAL;
  }
  else if (eCSSUnit_Inherit == aFontData.mVariant.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.variant = parentFont->mFont.variant;
    font->mFixedFont.variant = parentFont->mFixedFont.variant;
  }

  // font-weight: int, enum, normal, inherit
  if (eCSSUnit_Integer == aFontData.mWeight.GetUnit()) {
    font->mFont.weight = aFontData.mWeight.GetIntValue();
    font->mFixedFont.weight = aFontData.mWeight.GetIntValue();
  }
  else if (eCSSUnit_Enumerated == aFontData.mWeight.GetUnit()) {
    PRInt32 value = aFontData.mWeight.GetIntValue();
    switch (value) {
      case NS_STYLE_FONT_WEIGHT_NORMAL:
      case NS_STYLE_FONT_WEIGHT_BOLD:
        font->mFont.weight = value;
        font->mFixedFont.weight = value;
        break;
      case NS_STYLE_FONT_WEIGHT_BOLDER:
      case NS_STYLE_FONT_WEIGHT_LIGHTER:
        if (parentContext && !inherited) {
          inherited = PR_TRUE;
          parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
        }
        font->mFont.weight = nsStyleUtil::ConstrainFontWeight(parentFont->mFont.weight + value);
        font->mFixedFont.weight = nsStyleUtil::ConstrainFontWeight(parentFont->mFixedFont.weight + value);
        break;
    }
  }
  else if (eCSSUnit_Normal == aFontData.mWeight.GetUnit()) {
    font->mFont.weight = NS_STYLE_FONT_WEIGHT_NORMAL;
    font->mFixedFont.weight = NS_STYLE_FONT_WEIGHT_NORMAL;
  }
  else if (eCSSUnit_Inherit == aFontData.mWeight.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.weight = parentFont->mFont.weight;
    font->mFixedFont.weight = parentFont->mFixedFont.weight;
  }

  // font-size: enum, length, percent, inherit
  if (eCSSUnit_Enumerated == aFontData.mSize.GetUnit()) {
    PRInt32 value = aFontData.mSize.GetIntValue();
    PRInt32 scaler;
    mPresContext->GetFontScaler(&scaler);
    float scaleFactor = nsStyleUtil::GetScalingFactor(scaler);

    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    if ((NS_STYLE_FONT_SIZE_XXSMALL <= value) && 
        (value <= NS_STYLE_FONT_SIZE_XXLARGE)) {
      font->mFont.size = nsStyleUtil::CalcFontPointSize(value, (PRInt32)defaultFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      font->mFixedFont.size = nsStyleUtil::CalcFontPointSize(value, (PRInt32)defaultFixedFont.size, scaleFactor, mPresContext, eFontSize_CSS);
    }
    else if (NS_STYLE_FONT_SIZE_LARGER == value) {
      PRInt32 index = nsStyleUtil::FindNextLargerFontSize(parentFont->mFont.size, (PRInt32)defaultFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      nscoord largerSize = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      nscoord largerFixedSize = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFixedFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      font->mFont.size = PR_MAX(largerSize, parentFont->mFont.size);
      font->mFixedFont.size = PR_MAX(largerFixedSize, parentFont->mFixedFont.size);
    }
    else if (NS_STYLE_FONT_SIZE_SMALLER == value) {
      PRInt32 index = nsStyleUtil::FindNextSmallerFontSize(parentFont->mFont.size, (PRInt32)defaultFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      nscoord smallerSize = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      nscoord smallerFixedSize = nsStyleUtil::CalcFontPointSize(index, (PRInt32)defaultFixedFont.size, scaleFactor, mPresContext, eFontSize_CSS);
      font->mFont.size = PR_MIN(smallerSize, parentFont->mFont.size);
      font->mFixedFont.size = PR_MIN(smallerFixedSize, parentFont->mFixedFont.size);
    }
    // this does NOT explicitly set font size
    font->mFlags &= ~NS_STYLE_FONT_SIZE_EXPLICIT;
  }
  else if (aFontData.mSize.IsLengthUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.size = CalcLength(aFontData.mSize, &parentFont->mFont, nsnull, mPresContext, inherited);
    font->mFixedFont.size = CalcLength(aFontData.mSize, &parentFont->mFixedFont, nsnull, mPresContext, inherited);
    font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
  }
  else if (eCSSUnit_Percent == aFontData.mSize.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.size = (nscoord)((float)(parentFont->mFont.size) * aFontData.mSize.GetPercentValue());
    font->mFixedFont.size = (nscoord)((float)(parentFont->mFixedFont.size) * aFontData.mSize.GetPercentValue());
    font->mFlags |= NS_STYLE_FONT_SIZE_EXPLICIT;
  }
  else if (eCSSUnit_Inherit == aFontData.mSize.GetUnit()) {
    if (parentContext && !inherited) {
      inherited = PR_TRUE;
      parentFont = (nsStyleFont*)parentContext->GetStyleData(eStyleStruct_Font);
    }

    font->mFont.size = parentFont->mFont.size;
    font->mFixedFont.size = parentFont->mFixedFont.size;
    font->mFlags &= ~NS_STYLE_FONT_SIZE_EXPLICIT;
    font->mFlags |= (parentFont->mFlags & NS_STYLE_FONT_SIZE_EXPLICIT);
  }

  if (font->mFlags & NS_STYLE_FONT_USE_FIXED)
    font->mFont = font->mFixedFont;

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Font, *font);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mInheritedData)
      aHighestNode->mStyleData.mInheritedData = new (mPresContext) nsInheritedStyleData;
    aHighestNode->mStyleData.mInheritedData->mFontData = font;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_FONT, aHighestNode);
  }

  return font;
}

const nsStyleStruct*
nsRuleNode::ComputeMarginData(nsStyleMargin* aStartMargin, const nsCSSMargin& aMarginData, 
                              nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW MARGIN CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleMargin* margin;
  if (aStartMargin)
    // We only need to compute the delta between this computed data and our
    // computed data.
    margin = new (mPresContext) nsStyleMargin(*aStartMargin);
  else
    margin = new (mPresContext) nsStyleMargin();
  nsStyleMargin* parentMargin = margin;

  if (parentContext)
    parentMargin = (nsStyleMargin*)parentContext->GetStyleData(eStyleStruct_Margin);
  PRBool inherited = PR_FALSE;

  // margin: length, percent, auto, inherit
  if (aMarginData.mMargin) {
    nsStyleCoord  coord;
    nsStyleCoord  parentCoord;
    parentMargin->mMargin.GetLeft(parentCoord);
    if (SetCoord(aMarginData.mMargin->mLeft, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      margin->mMargin.SetLeft(coord);
    }
    parentMargin->mMargin.GetTop(parentCoord);
    if (SetCoord(aMarginData.mMargin->mTop, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      margin->mMargin.SetTop(coord);
    }
    parentMargin->mMargin.GetRight(parentCoord);
    if (SetCoord(aMarginData.mMargin->mRight, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      margin->mMargin.SetRight(coord);
    }
    parentMargin->mMargin.GetBottom(parentCoord);
    if (SetCoord(aMarginData.mMargin->mBottom, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      margin->mMargin.SetBottom(coord);
    }
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Margin, *margin);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mMarginData = margin;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_MARGIN, aHighestNode);
  }

  margin->RecalcData();
  return margin;
}

const nsStyleStruct* 
nsRuleNode::ComputeBorderData(nsStyleBorder* aStartBorder, const nsCSSMargin& aMarginData, 
                              nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW BORDER CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleBorder* border;
  if (aStartBorder)
    // We only need to compute the delta between this computed data and our
    // computed data.
    border = new (mPresContext) nsStyleBorder(*aStartBorder);
  else
    border = new (mPresContext) nsStyleBorder(mPresContext);
  
  nsStyleBorder* parentBorder = border;
  if (parentContext)
    parentBorder = (nsStyleBorder*)parentContext->GetStyleData(eStyleStruct_Border);
  PRBool inherited = PR_FALSE;

  // border-size: length, enum, inherit
  if (aMarginData.mBorderWidth) {
    nsStyleCoord  coord;
    nsStyleCoord  parentCoord;
    if (SetCoord(aMarginData.mBorderWidth->mLeft, coord, parentCoord, SETCOORD_LE, aContext, mPresContext, inherited))
      border->mBorder.SetLeft(coord);
    else if (eCSSUnit_Inherit == aMarginData.mBorderWidth->mLeft.GetUnit())
      border->mBorder.SetLeft(parentBorder->mBorder.GetLeft(coord));

    if (SetCoord(aMarginData.mBorderWidth->mTop, coord, parentCoord, SETCOORD_LE, aContext, mPresContext, inherited))
      border->mBorder.SetTop(coord);
    else if (eCSSUnit_Inherit == aMarginData.mBorderWidth->mTop.GetUnit())
      border->mBorder.SetTop(parentBorder->mBorder.GetTop(coord));

    if (SetCoord(aMarginData.mBorderWidth->mRight, coord, parentCoord, SETCOORD_LE, aContext, mPresContext, inherited))
      border->mBorder.SetRight(coord);
    else if (eCSSUnit_Inherit == aMarginData.mBorderWidth->mRight.GetUnit())
      border->mBorder.SetRight(parentBorder->mBorder.GetRight(coord));

    if (SetCoord(aMarginData.mBorderWidth->mBottom, coord, parentCoord, SETCOORD_LE, aContext, mPresContext, inherited))
      border->mBorder.SetBottom(coord);
    else if (eCSSUnit_Inherit == aMarginData.mBorderWidth->mBottom.GetUnit())
      border->mBorder.SetBottom(parentBorder->mBorder.GetBottom(coord));
  }

  // border-style: enum, none, inhert
  if (nsnull != aMarginData.mBorderStyle) {
    nsCSSRect* ourStyle = aMarginData.mBorderStyle;
    if (eCSSUnit_Enumerated == ourStyle->mTop.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_TOP, ourStyle->mTop.GetIntValue());
    }
    else if (eCSSUnit_None == ourStyle->mTop.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_TOP, NS_STYLE_BORDER_STYLE_NONE);
    }
    else if (eCSSUnit_Inherit == ourStyle->mTop.GetUnit()) {
      inherited = PR_TRUE;
      border->SetBorderStyle(NS_SIDE_TOP, parentBorder->GetBorderStyle(NS_SIDE_TOP));
    }

    if (eCSSUnit_Enumerated == ourStyle->mRight.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_RIGHT, ourStyle->mRight.GetIntValue());
    }
    else if (eCSSUnit_None == ourStyle->mRight.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_RIGHT, NS_STYLE_BORDER_STYLE_NONE);
    }
    else if (eCSSUnit_Inherit == ourStyle->mRight.GetUnit()) {
      inherited = PR_TRUE;
      border->SetBorderStyle(NS_SIDE_RIGHT, parentBorder->GetBorderStyle(NS_SIDE_RIGHT));
    }

    if (eCSSUnit_Enumerated == ourStyle->mBottom.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_BOTTOM, ourStyle->mBottom.GetIntValue());
    }
    else if (eCSSUnit_None == ourStyle->mBottom.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_BOTTOM, NS_STYLE_BORDER_STYLE_NONE);
    }
    else if (eCSSUnit_Inherit == ourStyle->mBottom.GetUnit()) { 
      inherited = PR_TRUE;
      border->SetBorderStyle(NS_SIDE_BOTTOM, parentBorder->GetBorderStyle(NS_SIDE_BOTTOM));
    }

    if (eCSSUnit_Enumerated == ourStyle->mLeft.GetUnit()) {
      border->SetBorderStyle(NS_SIDE_LEFT, ourStyle->mLeft.GetIntValue());
    }
    else if (eCSSUnit_None == ourStyle->mLeft.GetUnit()) {
     border->SetBorderStyle(NS_SIDE_LEFT, NS_STYLE_BORDER_STYLE_NONE);
    }
    else if (eCSSUnit_Inherit == ourStyle->mLeft.GetUnit()) {
      inherited = PR_TRUE;
      border->SetBorderStyle(NS_SIDE_LEFT, parentBorder->GetBorderStyle(NS_SIDE_LEFT));
    }
  }

  // border-color: color, string, enum, inherit
  if (nsnull != aMarginData.mBorderColor) {
    nsCSSRect* ourBorderColor = aMarginData.mBorderColor;
    nscolor borderColor;
    nscolor unused = NS_RGB(0,0,0);
    PRBool transparent;
    PRBool foreground;

    // top
    if (eCSSUnit_Inherit == ourBorderColor->mTop.GetUnit()) {
      inherited = PR_TRUE;
      parentBorder->GetBorderColor(NS_SIDE_TOP, borderColor, transparent, foreground);      
      if (transparent)
        border->SetBorderTransparent(NS_SIDE_TOP);
      else if (foreground)
        border->SetBorderToForeground(NS_SIDE_TOP);
      else
        border->SetBorderColor(NS_SIDE_TOP, borderColor);
    }
    else if (SetColor(ourBorderColor->mTop, unused, mPresContext, borderColor, inherited)) {
      border->SetBorderColor(NS_SIDE_TOP, borderColor);
    }
    else if (eCSSUnit_Enumerated == ourBorderColor->mTop.GetUnit()) {
      switch (ourBorderColor->mTop.GetIntValue()) {
        case NS_STYLE_COLOR_TRANSPARENT:
          border->SetBorderTransparent(NS_SIDE_TOP);
          break;
        case NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR:
          border->SetBorderToForeground(NS_SIDE_TOP);
          break;
      }
    }
    // right
    if (eCSSUnit_Inherit == ourBorderColor->mRight.GetUnit()) {
      inherited = PR_TRUE;
      parentBorder->GetBorderColor(NS_SIDE_RIGHT, borderColor, transparent, foreground);      
      if (transparent)
        border->SetBorderTransparent(NS_SIDE_RIGHT);
      else if (foreground)
        border->SetBorderToForeground(NS_SIDE_RIGHT);
      else
        border->SetBorderColor(NS_SIDE_RIGHT, borderColor);
    }
    else if (SetColor(ourBorderColor->mRight, unused, mPresContext, borderColor, inherited)) {
      border->SetBorderColor(NS_SIDE_RIGHT, borderColor);
    }
    else if (eCSSUnit_Enumerated == ourBorderColor->mRight.GetUnit()) {
      switch (ourBorderColor->mRight.GetIntValue()) {
        case NS_STYLE_COLOR_TRANSPARENT:
          border->SetBorderTransparent(NS_SIDE_RIGHT);
          break;
        case NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR:
          border->SetBorderToForeground(NS_SIDE_RIGHT);
          break;
      }
    }
    // bottom
    if (eCSSUnit_Inherit == ourBorderColor->mBottom.GetUnit()) {
      inherited = PR_TRUE;
      parentBorder->GetBorderColor(NS_SIDE_BOTTOM, borderColor, transparent, foreground);      
      if (transparent)
        border->SetBorderTransparent(NS_SIDE_BOTTOM);
      else if (foreground)
        border->SetBorderToForeground(NS_SIDE_BOTTOM);
      else
        border->SetBorderColor(NS_SIDE_BOTTOM, borderColor);
    }
    else if (SetColor(ourBorderColor->mBottom, unused, mPresContext, borderColor, inherited)) {
      border->SetBorderColor(NS_SIDE_BOTTOM, borderColor);
    }
    else if (eCSSUnit_Enumerated == ourBorderColor->mBottom.GetUnit()) {
      switch (ourBorderColor->mBottom.GetIntValue()) {
        case NS_STYLE_COLOR_TRANSPARENT:
          border->SetBorderTransparent(NS_SIDE_BOTTOM);
          break;
        case NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR:
          border->SetBorderToForeground(NS_SIDE_BOTTOM);
          break;
      }
    }
    // left
    if (eCSSUnit_Inherit == ourBorderColor->mLeft.GetUnit()) {
      inherited = PR_TRUE;
      parentBorder->GetBorderColor(NS_SIDE_LEFT, borderColor, transparent, foreground);      
      if (transparent)
        border->SetBorderTransparent(NS_SIDE_LEFT);
      else if (foreground)
        border->SetBorderToForeground(NS_SIDE_LEFT);
      else
        border->SetBorderColor(NS_SIDE_LEFT, borderColor);
    }
    else if (SetColor(ourBorderColor->mLeft, unused, mPresContext, borderColor, inherited)) {
      border->SetBorderColor(NS_SIDE_LEFT, borderColor);
    }
    else if (eCSSUnit_Enumerated == ourBorderColor->mLeft.GetUnit()) {
      switch (ourBorderColor->mLeft.GetIntValue()) {
        case NS_STYLE_COLOR_TRANSPARENT:
          border->SetBorderTransparent(NS_SIDE_LEFT);
          break;
        case NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR:
          border->SetBorderToForeground(NS_SIDE_LEFT);
          break;
      }
    }
  }

  // -moz-border-radius: length, percent, inherit
  if (aMarginData.mBorderRadius) {
    nsStyleCoord  coord;
    nsStyleCoord  parentCoord;
    parentBorder->mBorderRadius.GetLeft(parentCoord);
    if (SetCoord(aMarginData.mBorderRadius->mLeft, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited))
      border->mBorderRadius.SetLeft(coord);
    parentBorder->mBorderRadius.GetTop(parentCoord);
    if (SetCoord(aMarginData.mBorderRadius->mTop, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited))
      border->mBorderRadius.SetTop(coord);
    parentBorder->mBorderRadius.GetRight(parentCoord);
    if (SetCoord(aMarginData.mBorderRadius->mRight, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited))
      border->mBorderRadius.SetRight(coord);
    parentBorder->mBorderRadius.GetBottom(parentCoord);
    if (SetCoord(aMarginData.mBorderRadius->mBottom, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited))
      border->mBorderRadius.SetBottom(coord);
  }

  // float-edge: enum, inherit
  if (eCSSUnit_Enumerated == aMarginData.mFloatEdge.GetUnit())
    border->mFloatEdge = aMarginData.mFloatEdge.GetIntValue();
  else if (eCSSUnit_Inherit == aMarginData.mFloatEdge.GetUnit()) {
    inherited = PR_TRUE;
    border->mFloatEdge = parentBorder->mFloatEdge;
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Border, *border);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mBorderData = border;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_BORDER, aHighestNode);
  }

  border->RecalcData();
  return border;
}
  
const nsStyleStruct*
nsRuleNode::ComputePaddingData(nsStylePadding* aStartPadding, const nsCSSMargin& aMarginData, 
                               nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW PADDING CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStylePadding* padding;
  if (aStartPadding)
    // We only need to compute the delta between this computed data and our
    // computed data.
    padding = new (mPresContext) nsStylePadding(*aStartPadding);
  else
    padding = new (mPresContext) nsStylePadding();
  
  nsStylePadding* parentPadding = padding;
  if (parentContext)
    parentPadding = (nsStylePadding*)parentContext->GetStyleData(eStyleStruct_Padding);
  PRBool inherited = PR_FALSE;

  // padding: length, percent, inherit
  if (aMarginData.mPadding) {
    nsStyleCoord  coord;
    nsStyleCoord  parentCoord;
    parentPadding->mPadding.GetLeft(parentCoord);
    if (SetCoord(aMarginData.mPadding->mLeft, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited)) {
      padding->mPadding.SetLeft(coord);
    }
    parentPadding->mPadding.GetTop(parentCoord);
    if (SetCoord(aMarginData.mPadding->mTop, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited)) {
      padding->mPadding.SetTop(coord);
    }
    parentPadding->mPadding.GetRight(parentCoord);
    if (SetCoord(aMarginData.mPadding->mRight, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited)) {
      padding->mPadding.SetRight(coord);
    }
    parentPadding->mPadding.GetBottom(parentCoord);
    if (SetCoord(aMarginData.mPadding->mBottom, coord, parentCoord, SETCOORD_LPH, aContext, mPresContext, inherited)) {
      padding->mPadding.SetBottom(coord);
    }
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Padding, *padding);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mPaddingData = padding;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_PADDING, aHighestNode);
  }

  padding->RecalcData();
  return padding;
}

const nsStyleStruct*
nsRuleNode::ComputeOutlineData(nsStyleOutline* aStartOutline, const nsCSSMargin& aMarginData, 
                               nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW OUTLINE CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleOutline* outline;
  if (aStartOutline)
    // We only need to compute the delta between this computed data and our
    // computed data.
    outline = new (mPresContext) nsStyleOutline(*aStartOutline);
  else
    outline = new (mPresContext) nsStyleOutline(mPresContext);
  
  nsStyleOutline* parentOutline = outline;
  if (parentContext)
    parentOutline = (nsStyleOutline*)parentContext->GetStyleData(eStyleStruct_Outline);
  PRBool inherited = PR_FALSE;

  // outline-width: length, enum, inherit
  SetCoord(aMarginData.mOutlineWidth, outline->mOutlineWidth, parentOutline->mOutlineWidth,
           SETCOORD_LEH, aContext, mPresContext, inherited);
  
  // outline-color: color, string, enum, inherit
  nscolor outlineColor;
  nscolor unused = NS_RGB(0,0,0);
  if (eCSSUnit_Inherit == aMarginData.mOutlineColor.GetUnit()) {
    inherited = PR_TRUE;
    if (parentOutline->GetOutlineColor(outlineColor))
      outline->SetOutlineColor(outlineColor);
    else
      outline->SetOutlineInvert();
  }
  else if (SetColor(aMarginData.mOutlineColor, unused, mPresContext, outlineColor, inherited))
    outline->SetOutlineColor(outlineColor);
  else if (eCSSUnit_Enumerated == aMarginData.mOutlineColor.GetUnit())
    outline->SetOutlineInvert();

  // outline-style: enum, none, inherit
  if (eCSSUnit_Enumerated == aMarginData.mOutlineStyle.GetUnit())
    outline->SetOutlineStyle(aMarginData.mOutlineStyle.GetIntValue());
  else if (eCSSUnit_None == aMarginData.mOutlineStyle.GetUnit())
    outline->SetOutlineStyle(NS_STYLE_BORDER_STYLE_NONE);
  else if (eCSSUnit_Inherit == aMarginData.mOutlineStyle.GetUnit()) {
    inherited = PR_TRUE;
    outline->SetOutlineStyle(parentOutline->GetOutlineStyle());
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Outline, *outline);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mOutlineData = outline;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_OUTLINE, aHighestNode);
  }

  outline->RecalcData();
  return outline;
}

const nsStyleStruct* 
nsRuleNode::ComputeListData(nsStyleList* aStartList, const nsCSSList& aListData, 
                            nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW LIST CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleList* list = nsnull;
  nsStyleList* parentList = list;
  PRBool inherited = PR_FALSE;

  if (aStartList)
    // We only need to compute the delta between this computed data and our
    // computed data.
    list = new (mPresContext) nsStyleList(*aStartList);
  else {
    if (aRuleDetail != eRuleFullMixed) {
      // No question. We will have to inherit. Go ahead and init
      // with inherited vals from parent.
      inherited = PR_TRUE;
      if (parentContext)
        parentList = (nsStyleList*)parentContext->GetStyleData(eStyleStruct_List);
      if (parentList)
        list = new (mPresContext) nsStyleList(*parentList);
    }
  }

  if (!list)
    list = parentList = new (mPresContext) nsStyleList();

  // list-style-type: enum, none, inherit
  if (eCSSUnit_Enumerated == aListData.mType.GetUnit()) {
    list->mListStyleType = aListData.mType.GetIntValue();
  }
  else if (eCSSUnit_None == aListData.mType.GetUnit()) {
    list->mListStyleType = NS_STYLE_LIST_STYLE_NONE;
  }
  else if (eCSSUnit_Inherit == aListData.mType.GetUnit()) {
    inherited = PR_TRUE;
    list->mListStyleType = parentList->mListStyleType;
  }

  // list-style-image: url, none, inherit
  if (eCSSUnit_URL == aListData.mImage.GetUnit()) {
    aListData.mImage.GetStringValue(list->mListStyleImage);
  }
  else if (eCSSUnit_None == aListData.mImage.GetUnit()) {
    list->mListStyleImage.Truncate();
  }
  else if (eCSSUnit_Inherit == aListData.mImage.GetUnit()) {
    inherited = PR_TRUE;
    list->mListStyleImage = parentList->mListStyleImage;
  }

  // list-style-position: enum, inherit
  if (eCSSUnit_Enumerated == aListData.mPosition.GetUnit()) {
    list->mListStylePosition = aListData.mPosition.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aListData.mPosition.GetUnit()) {
    inherited = PR_TRUE;
    list->mListStylePosition = parentList->mListStylePosition;
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_List, *list);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mInheritedData)
      aHighestNode->mStyleData.mInheritedData = new (mPresContext) nsInheritedStyleData;
    aHighestNode->mStyleData.mInheritedData->mListData = list;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_LIST, aHighestNode);
  }

  return list;
}

const nsStyleStruct* 
nsRuleNode::ComputePositionData(nsStylePosition* aStartPos, const nsCSSPosition& aPosData, 
                                nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                                nsRuleNode* aHighestNode,
                                const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW POSITION CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStylePosition* pos;
  if (aStartPos)
    // We only need to compute the delta between this computed data and our
    // computed data.
    pos = new (mPresContext) nsStylePosition(*aStartPos);
  else
    pos = new (mPresContext) nsStylePosition();
  
  nsStylePosition* parentPos = pos;
  if (parentContext)
    parentPos = (nsStylePosition*)parentContext->GetStyleData(eStyleStruct_Position);
  PRBool inherited = PR_FALSE;

  // position: enum, inherit
  if (eCSSUnit_Enumerated == aPosData.mPosition.GetUnit()) {
    pos->mPosition = aPosData.mPosition.GetIntValue();
    if (pos->mPosition != NS_STYLE_POSITION_NORMAL) {
      // :before and :after elts cannot be positioned.  We need to check for this
      // case.
      nsCOMPtr<nsIAtom> tag;
      aContext->GetPseudoType(*getter_AddRefs(tag));
      if (tag && tag.get() == nsCSSAtoms::beforePseudo || tag.get() == nsCSSAtoms::afterPseudo)
        pos->mPosition = NS_STYLE_POSITION_NORMAL;
    }
  }
  else if (eCSSUnit_Inherit == aPosData.mPosition.GetUnit()) {
    pos->mPosition = parentPos->mPosition;
  }

  // box offsets: length, percent, auto, inherit
  if (aPosData.mOffset) {
    nsStyleCoord  coord;
    nsStyleCoord  parentCoord;
    parentPos->mOffset.GetTop(parentCoord);
    if (SetCoord(aPosData.mOffset->mTop, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      pos->mOffset.SetTop(coord);            
    }
    parentPos->mOffset.GetRight(parentCoord);
    if (SetCoord(aPosData.mOffset->mRight, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      pos->mOffset.SetRight(coord);            
    }
    parentPos->mOffset.GetBottom(parentCoord);
    if (SetCoord(aPosData.mOffset->mBottom, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      pos->mOffset.SetBottom(coord);
    }
    parentPos->mOffset.GetLeft(parentCoord);
    if (SetCoord(aPosData.mOffset->mLeft, coord, parentCoord, SETCOORD_LPAH, aContext, mPresContext, inherited)) {
      pos->mOffset.SetLeft(coord);
    }
  }

  if (aPosData.mWidth.GetUnit() == eCSSUnit_Proportional)
    pos->mWidth.SetIntValue(aPosData.mWidth.GetIntValue(), eStyleUnit_Proportional);
  else 
    SetCoord(aPosData.mWidth, pos->mWidth, parentPos->mWidth,
             SETCOORD_LPAH, aContext, mPresContext, inherited);
  SetCoord(aPosData.mMinWidth, pos->mMinWidth, parentPos->mMinWidth,
           SETCOORD_LPH, aContext, mPresContext, inherited);
  if (! SetCoord(aPosData.mMaxWidth, pos->mMaxWidth, parentPos->mMaxWidth,
                 SETCOORD_LPH, aContext, mPresContext, inherited)) {
    if (eCSSUnit_None == aPosData.mMaxWidth.GetUnit()) {
      pos->mMaxWidth.Reset();
    }
  }

  SetCoord(aPosData.mHeight, pos->mHeight, parentPos->mHeight,
           SETCOORD_LPAH, aContext, mPresContext, inherited);
  SetCoord(aPosData.mMinHeight, pos->mMinHeight, parentPos->mMinHeight,
           SETCOORD_LPH, aContext, mPresContext, inherited);
  if (! SetCoord(aPosData.mMaxHeight, pos->mMaxHeight, parentPos->mMaxHeight,
                 SETCOORD_LPH, aContext, mPresContext, inherited)) {
    if (eCSSUnit_None == aPosData.mMaxHeight.GetUnit()) {
      pos->mMaxHeight.Reset();
    }
  }

  // box-sizing: enum, inherit
  if (eCSSUnit_Enumerated == aPosData.mBoxSizing.GetUnit()) {
    pos->mBoxSizing = aPosData.mBoxSizing.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aPosData.mBoxSizing.GetUnit()) {
    inherited = PR_TRUE;
    pos->mBoxSizing = parentPos->mBoxSizing;
  }

  // z-index
  if (! SetCoord(aPosData.mZIndex, pos->mZIndex, parentPos->mZIndex,
                 SETCOORD_IA, aContext, nsnull, inherited)) {
    if (eCSSUnit_Inherit == aPosData.mZIndex.GetUnit()) {
      // handle inherit, because it's ok to inherit 'auto' here
      inherited = PR_TRUE;
      pos->mZIndex = parentPos->mZIndex;
    }
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Position, *pos);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mPositionData = pos;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_POSITION, aHighestNode);
  }

  return pos;
}

const nsStyleStruct* 
nsRuleNode::ComputeTableData(nsStyleTable* aStartTable, const nsCSSTable& aTableData, 
                             nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW TABLE CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleTable* table;
  if (aStartTable)
    // We only need to compute the delta between this computed data and our
    // computed data.
    table = new (mPresContext) nsStyleTable(*aStartTable);
  else
    table = new (mPresContext) nsStyleTable();
  
  nsStyleTable* parentTable = table;
  if (parentContext)
    parentTable = (nsStyleTable*)parentContext->GetStyleData(eStyleStruct_Table);
  PRBool inherited = PR_FALSE;

  // table-layout: auto, enum, inherit
  if (eCSSUnit_Enumerated == aTableData.mLayout.GetUnit())
    table->mLayoutStrategy = aTableData.mLayout.GetIntValue();
  else if (eCSSUnit_Auto == aTableData.mLayout.GetUnit())
    table->mLayoutStrategy = NS_STYLE_TABLE_LAYOUT_AUTO;
  else if (eCSSUnit_Inherit == aTableData.mLayout.GetUnit()) {
    inherited = PR_TRUE;
    table->mLayoutStrategy = parentTable->mLayoutStrategy;
  }

  // rules: enum (not a real CSS prop)
  if (eCSSUnit_Enumerated == aTableData.mRules.GetUnit())
    table->mRules = aTableData.mRules.GetIntValue();

  // frame: enum (not a real CSS prop)
  if (eCSSUnit_Enumerated == aTableData.mFrame.GetUnit())
    table->mFrame = aTableData.mFrame.GetIntValue();

  // cols: enum, int (not a real CSS prop)
  if (eCSSUnit_Enumerated == aTableData.mCols.GetUnit() ||
      eCSSUnit_Integer == aTableData.mCols.GetUnit())
    table->mCols = aTableData.mCols.GetIntValue();

  // span: pixels (not a real CSS prop)
  if (eCSSUnit_Enumerated == aTableData.mSpan.GetUnit() ||
      eCSSUnit_Integer == aTableData.mSpan.GetUnit())
    table->mSpan = aTableData.mSpan.GetIntValue();
    
  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_Table, *table);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mTableData = table;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_TABLE, aHighestNode);
  }

  return table;
}

const nsStyleStruct* 
nsRuleNode::ComputeTableBorderData(nsStyleTableBorder* aStartTable, const nsCSSTable& aTableData, 
                                   nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                                   nsRuleNode* aHighestNode,
                                   const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW TABLE BORDER CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleTableBorder* table = nsnull;
  nsStyleTableBorder* parentTable = table;
  PRBool inherited = PR_FALSE;

  if (aStartTable)
    // We only need to compute the delta between this computed data and our
    // computed data.
    table = new (mPresContext) nsStyleTableBorder(*aStartTable);
  else {
    if (aRuleDetail != eRuleFullMixed) {
      // No question. We will have to inherit. Go ahead and init
      // with inherited vals from parent.
      inherited = PR_TRUE;
      if (parentContext)
        parentTable = (nsStyleTableBorder*)parentContext->GetStyleData(eStyleStruct_TableBorder);
      if (parentTable)
        table = new (mPresContext) nsStyleTableBorder(*parentTable);
    }
  }

  if (!table)
    table = parentTable = new (mPresContext) nsStyleTableBorder(mPresContext);

  // border-collapse: enum, inherit
  if (eCSSUnit_Enumerated == aTableData.mBorderCollapse.GetUnit()) {
    table->mBorderCollapse = aTableData.mBorderCollapse.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aTableData.mBorderCollapse.GetUnit()) {
    inherited = PR_TRUE;
    table->mBorderCollapse = parentTable->mBorderCollapse;
  }

  nsStyleCoord coord;

  // border-spacing-x: length, inherit
  if (SetCoord(aTableData.mBorderSpacingX, coord, coord, SETCOORD_LENGTH, aContext, mPresContext, inherited)) {
    table->mBorderSpacingX = coord.GetCoordValue();
  }
  else if (eCSSUnit_Inherit == aTableData.mBorderSpacingX.GetUnit()) {
    inherited = PR_TRUE;
    table->mBorderSpacingX = parentTable->mBorderSpacingX;
  }
  // border-spacing-y: length, inherit
  if (SetCoord(aTableData.mBorderSpacingY, coord, coord, SETCOORD_LENGTH, aContext, mPresContext, inherited)) {
    table->mBorderSpacingY = coord.GetCoordValue();
  }
  else if (eCSSUnit_Inherit == aTableData.mBorderSpacingY.GetUnit()) {
    inherited = PR_TRUE;
    table->mBorderSpacingY = parentTable->mBorderSpacingY;
  }

  // caption-side: enum, inherit
  if (eCSSUnit_Enumerated == aTableData.mCaptionSide.GetUnit()) {
    table->mCaptionSide = aTableData.mCaptionSide.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aTableData.mCaptionSide.GetUnit()) {
    inherited = PR_TRUE;
    table->mCaptionSide = parentTable->mCaptionSide;
  }

  // empty-cells: enum, inherit
  if (eCSSUnit_Enumerated == aTableData.mEmptyCells.GetUnit()) {
    table->mEmptyCells = aTableData.mEmptyCells.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aTableData.mEmptyCells.GetUnit()) {
    inherited = PR_TRUE;
    table->mEmptyCells = parentTable->mEmptyCells;
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_TableBorder, *table);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mInheritedData)
      aHighestNode->mStyleData.mInheritedData = new (mPresContext) nsInheritedStyleData;
    aHighestNode->mStyleData.mInheritedData->mTableData = table;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_TABLE_BORDER, aHighestNode);
  }

  return table;
}

#ifdef INCLUDE_XUL
const nsStyleStruct* 
nsRuleNode::ComputeXULData(nsStyleXUL* aStartXUL, const nsCSSXUL& aXULData, 
                           nsIStyleContext* aContext, nsIMutableStyleContext* aMutableContext,
                           nsRuleNode* aHighestNode,
                           const RuleDetail& aRuleDetail)
{
#ifdef DEBUG_hyatt
  printf("NEW XUL CREATED!\n");
#endif
  nsCOMPtr<nsIStyleContext> parentContext = getter_AddRefs(aContext->GetParent());
  
  nsStyleXUL* xul = nsnull;
  
  if (aStartXUL)
    // We only need to compute the delta between this computed data and our
    // computed data.
    xul = new (mPresContext) nsStyleXUL(*aStartXUL);
  else
    xul = new (mPresContext) nsStyleXUL();

  nsStyleXUL* parentXUL = xul;

  if (parentContext)
    parentXUL = (nsStyleXUL*)parentContext->GetStyleData(eStyleStruct_XUL);

  PRBool inherited = PR_FALSE;

  // box-orient: enum, inherit
  if (eCSSUnit_Enumerated == aXULData.mBoxOrient.GetUnit()) {
    xul->mBoxOrient = aXULData.mBoxOrient.GetIntValue();
  }
  else if (eCSSUnit_Inherit == aXULData.mBoxOrient.GetUnit()) {
    inherited = PR_TRUE;
    xul->mBoxOrient = parentXUL->mBoxOrient;
  }

  if (inherited)
    // We inherited, and therefore can't be cached in the rule node.  We have to be put right on the
    // style context.
    aMutableContext->SetStyle(eStyleStruct_XUL, *xul);
  else {
    // We were fully specified and can therefore be cached right on the rule node.
    if (!aHighestNode->mStyleData.mResetData)
      aHighestNode->mStyleData.mResetData = new (mPresContext) nsResetStyleData;
    aHighestNode->mStyleData.mResetData->mXULData = xul;
    // Propagate the bit down.
    PropagateBit(NS_STYLE_INHERIT_XUL, aHighestNode);
  }

  return xul;
}
#endif

static void
ExamineRectProperties(const nsCSSRect* aRect, PRUint32& aTotalCount, PRUint32& aInheritedCount)
{
  if (!aRect)
    return;

  if (eCSSUnit_Null != aRect->mLeft.GetUnit()) {
    aTotalCount++;
    if (eCSSUnit_Inherit == aRect->mLeft.GetUnit())
      aInheritedCount++;
  }

  if (eCSSUnit_Null != aRect->mTop.GetUnit()) {
    aTotalCount++;
    if (eCSSUnit_Inherit == aRect->mTop.GetUnit())
      aInheritedCount++;
  }

  if (eCSSUnit_Null != aRect->mRight.GetUnit()) {
    aTotalCount++;
    if (eCSSUnit_Inherit == aRect->mRight.GetUnit())
      aInheritedCount++;
  }

  if (eCSSUnit_Null != aRect->mBottom.GetUnit()) {
    aTotalCount++;
    if (eCSSUnit_Inherit == aRect->mBottom.GetUnit())
      aInheritedCount++;
  }
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckFontProperties(const nsCSSFont& aFontData)
{
  const PRUint32 numFontProps = 5;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  if (eCSSUnit_Null != aFontData.mFamily.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aFontData.mFamily.GetUnit())
      inheritCount++;
    if (eCSSUnit_Enumerated == aFontData.mFamily.GetUnit()) {
      // A special case. We treat this as a fully specified font,
      // since no other font props are legal with a system font.
      switch (aFontData.mFamily.GetIntValue()) {
        case NS_STYLE_FONT_CAPTION:
        case NS_STYLE_FONT_ICON:
        case NS_STYLE_FONT_MENU:
        case NS_STYLE_FONT_MESSAGE_BOX:
        case NS_STYLE_FONT_SMALL_CAPTION:
        case NS_STYLE_FONT_STATUS_BAR:
        case NS_STYLE_FONT_WINDOW:
        case NS_STYLE_FONT_DOCUMENT:
        case NS_STYLE_FONT_WORKSPACE:
        case NS_STYLE_FONT_DESKTOP:
        case NS_STYLE_FONT_INFO:
        case NS_STYLE_FONT_DIALOG:
        case NS_STYLE_FONT_BUTTON:
        case NS_STYLE_FONT_PULL_DOWN_MENU:
        case NS_STYLE_FONT_LIST:
        case NS_STYLE_FONT_FIELD:
          return eRuleFullMixed;
      }
    }
  }

  if (eCSSUnit_Null != aFontData.mStyle.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aFontData.mStyle.GetUnit())
      inheritCount++;
  }
  if (eCSSUnit_Null != aFontData.mVariant.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aFontData.mVariant.GetUnit())
      inheritCount++;
  }
  if (eCSSUnit_Null != aFontData.mWeight.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aFontData.mWeight.GetUnit())
      inheritCount++;
  }
  if (eCSSUnit_Null != aFontData.mSize.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aFontData.mSize.GetUnit())
      inheritCount++;
  }
  
  if (inheritCount == numFontProps)
    return eRuleFullInherited;
  else if (totalCount == numFontProps) 
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckMarginProperties(const nsCSSMargin& aMarginData)
{
  // Left/Top/Right/Bottom are the four props we have to check.
  const PRUint32 numMarginProps = 4;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  ExamineRectProperties(aMarginData.mMargin, totalCount, inheritCount);

  if (inheritCount == numMarginProps)
    return eRuleFullInherited;
  else if (totalCount == numMarginProps) 
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckBorderProperties(const nsCSSMargin& aBorderData)
{
  // Left/Top/Right/Bottom are the four props we have to check.
  const PRUint32 numBorderProps = 17;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  ExamineRectProperties(aBorderData.mBorderWidth, totalCount, inheritCount);
  ExamineRectProperties(aBorderData.mBorderStyle, totalCount, inheritCount);
  ExamineRectProperties(aBorderData.mBorderColor, totalCount, inheritCount);
  ExamineRectProperties(aBorderData.mBorderRadius, totalCount, inheritCount);
  if (eCSSUnit_Null != aBorderData.mFloatEdge.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aBorderData.mFloatEdge.GetUnit())
      inheritCount++;
  }

  if (inheritCount == numBorderProps)
    return eRuleFullInherited;
  else if (totalCount == numBorderProps)
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}
  
nsRuleNode::RuleDetail 
nsRuleNode::CheckPaddingProperties(const nsCSSMargin& aPaddingData)
{
  // Left/Top/Right/Bottom are the four props we have to check.
  const PRUint32 numPaddingProps = 4;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  ExamineRectProperties(aPaddingData.mPadding, totalCount, inheritCount);

  if (inheritCount == numPaddingProps)
    return eRuleFullInherited;
  else if (totalCount == numPaddingProps) 
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}
  
nsRuleNode::RuleDetail 
nsRuleNode::CheckOutlineProperties(const nsCSSMargin& aMargin)
{
  // XXXdwh write me!
  return eRuleNone;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckListProperties(const nsCSSList& aListData)
{
  const PRUint32 numListProps = 3;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;
  if (eCSSUnit_Null != aListData.mType.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aListData.mType.GetUnit())
      inheritCount++;
  }
  if (eCSSUnit_Null != aListData.mImage.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aListData.mImage.GetUnit())
      inheritCount++;
  }
  if (eCSSUnit_Null != aListData.mPosition.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aListData.mPosition.GetUnit())
      inheritCount++;
  }
  
  if (inheritCount == numListProps)
    return eRuleFullInherited;
  else if (totalCount == numListProps)
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckPositionProperties(const nsCSSPosition& aPosData)
{
  const PRUint32 numPosProps = 13;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  ExamineRectProperties(aPosData.mOffset, totalCount, inheritCount);
  
  if (eCSSUnit_Null != aPosData.mPosition.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mPosition.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mWidth.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mWidth.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mMinWidth.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mMinWidth.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mMaxWidth.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mMaxWidth.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mHeight.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mHeight.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mMinHeight.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mMinHeight.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mMaxHeight.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mMaxHeight.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mBoxSizing.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mBoxSizing.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aPosData.mZIndex.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aPosData.mZIndex.GetUnit())
      inheritCount++;
  }

  if (inheritCount == numPosProps)
    return eRuleFullInherited;
  else if (totalCount == numPosProps)
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckTableProperties(const nsCSSTable& aTableData)
{
  const PRUint32 numTableProps = 5;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  if (eCSSUnit_Null != aTableData.mLayout.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mLayout.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mFrame.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mFrame.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mRules.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mRules.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mCols.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mCols.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mSpan.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mSpan.GetUnit())
      inheritCount++;
  }

  if (inheritCount == numTableProps)
    return eRuleFullInherited;
  else if (totalCount == numTableProps)
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

nsRuleNode::RuleDetail 
nsRuleNode::CheckTableBorderProperties(const nsCSSTable& aTableData)
{
  const PRUint32 numTableProps = 5;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  if (eCSSUnit_Null != aTableData.mBorderCollapse.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mBorderCollapse.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mBorderSpacingX.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mBorderSpacingX.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mBorderSpacingY.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mBorderSpacingY.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mCaptionSide.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mCaptionSide.GetUnit())
      inheritCount++;
  }

  if (eCSSUnit_Null != aTableData.mEmptyCells.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aTableData.mEmptyCells.GetUnit())
      inheritCount++;
  }

  if (inheritCount == numTableProps)
    return eRuleFullInherited;
  else if (totalCount == numTableProps)
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}

#ifdef INCLUDE_XUL
nsRuleNode::RuleDetail 
nsRuleNode::CheckXULProperties(const nsCSSXUL& aXULData)
{
  const PRUint32 numXULProps = 1;
  PRUint32 totalCount=0;
  PRUint32 inheritCount=0;

  if (eCSSUnit_Null != aXULData.mBoxOrient.GetUnit()) {
    totalCount++;
    if (eCSSUnit_Inherit == aXULData.mBoxOrient.GetUnit())
      inheritCount++;
  }

  if (inheritCount == numXULProps)
    return eRuleFullInherited;
  else if (totalCount == numXULProps) 
    return eRuleFullMixed;

  if (totalCount == 0)
    return eRuleNone;
  else if (totalCount == inheritCount)
    return eRulePartialInherited;

  return eRulePartialMixed;
}
#endif

inline const nsStyleStruct* 
nsRuleNode::GetParentData(const nsStyleStructID& aSID)
{
  nsRuleNode* ruleNode = mParent;
  nsStyleStruct* currStruct = nsnull;
  while (ruleNode) {
    currStruct = ruleNode->mStyleData.GetStyleData(aSID);
    if (currStruct)
      break; // We found a rule with fully specified data.  We don't need to go up
             // the tree any further, since the remainder of this branch has already
             // been computed.
    ruleNode = ruleNode->mParent; // Climb up to the next rule in the tree (a less specific rule).
  }  

  return currStruct; // Just return whatever we found.
}

inline const nsStyleStruct* 
nsRuleNode::GetStyleData(nsStyleStructID aSID, 
                         nsIStyleContext* aContext,
                         nsIMutableStyleContext* aMutableContext)
{
  const nsStyleStruct* cachedData = mStyleData.GetStyleData(aSID);
  if (cachedData)
    return cachedData; // We have a fully specified struct. Just return it.

  if (InheritsFromParentRule(aSID))
    return GetParentData(aSID); // We inherit. Just go up the rule tree and return the first
                                // cached struct we find.

  switch (aSID) {
    // Nothing is cached.  We'll have to delve further and examine our rules.
    case eStyleStruct_Font:
      return GetFontData(aContext, aMutableContext);
    case eStyleStruct_Margin:
      return GetMarginData(aContext, aMutableContext);
    case eStyleStruct_Border:
      return GetBorderData(aContext, aMutableContext);
    case eStyleStruct_Padding:
      return GetPaddingData(aContext, aMutableContext);
    case eStyleStruct_Outline:
      return GetOutlineData(aContext, aMutableContext);
    case eStyleStruct_List:
      return GetListData(aContext, aMutableContext);
    case eStyleStruct_Position:
      return GetPositionData(aContext, aMutableContext);
    case eStyleStruct_Table:
      return GetTableData(aContext, aMutableContext);
    case eStyleStruct_TableBorder:
      return GetTableBorderData(aContext, aMutableContext);
#ifdef INCLUDE_XUL
    case eStyleStruct_XUL:
      return GetXULData(aContext, aMutableContext);
#endif
  }

  return nsnull;
}
