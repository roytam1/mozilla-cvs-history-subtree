/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsFrame.h"
#include "nsISVGRendererGlyphGeometry.h"
#include "nsISVGRendererGlyphMetrics.h"
#include "nsISVGRenderer.h"
#include "nsISVGPosGlyphGeometrySrc.h"
#include "nsISVGGlyphFragmentLeaf.h"
#include "nsITextContent.h"
#include "nsISVGChildFrame.h"
#include "nsISVGOuterSVGFrame.h"
#include "nsISVGTextFrame.h"
#include "nsISVGRendererRegion.h"
#include "nsISVGContainerFrame.h"
#include "nsISVGTextContainerFrame.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prdtoa.h"

typedef nsFrame nsSVGGlyphFrameBase;

class nsSVGGlyphFrame : public nsSVGGlyphFrameBase,
                        public nsISVGPositionedGlyphGeometrySource, // : nsISVGGlyphGeometrySource : nsISVGGeometrySource
                        public nsISVGGlyphFragmentLeaf, // : nsISVGGlyphFragmentNode
                        public nsISVGChildFrame
{
protected:
  friend nsresult
  NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                      nsIFrame* parentFrame, nsIFrame** aNewFrame);
  nsSVGGlyphFrame();
  virtual ~nsSVGGlyphFrame();

public:
   // nsISupports interface:
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }

  // nsIFrame interface:
  NS_IMETHOD
  Init(nsIPresContext*  aPresContext,
       nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIStyleContext* aContext,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType,
                               PRInt32         aHint);

  NS_IMETHOD DidSetStyleContext(nsIPresContext* aPresContext);

  // nsISVGChildFrame interface:
  NS_IMETHOD Paint(nsISVGRendererRenderContext* renderingContext);
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit);
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyCTMChanged();
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  
  // nsISVGGeometrySource interface: 
  NS_DECL_NSISVGGEOMETRYSOURCE

  // nsISVGGlyphGeometrySource interface:
  NS_DECL_NSISVGGLYPHGEOMETRYSOURCE

  // nsISVGPositionedGlyphGeometrySource interface:
  NS_DECL_NSISVGPOSITIONEDGLYPHGEOMETRYSOURCE

  // nsISVGGlyphFragmentLeaf interface:
  NS_IMETHOD_(void) SetGlyphPosition(float x, float y);
  NS_IMETHOD GetGlyphMetrics(nsISVGRendererGlyphMetrics** metrics);
  NS_IMETHOD_(PRBool) IsStartOfChunk(); // == is new absolutely positioned chunk.
  NS_IMETHOD_(void) GetAdjustedPosition(/* inout */ float &x, /* inout */ float &y);

  // nsISVGGlyphFragmentNode interface:
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetFirstGlyphFragment();
  NS_IMETHOD_(nsISVGGlyphFragmentLeaf *) GetNextGlyphFragment();
  NS_IMETHOD_(PRUint32) BuildGlyphFragmentTree(PRUint32 charNum, PRBool lastBranch);
  NS_IMETHOD_(void) NotifyMetricsSuspended();
  NS_IMETHOD_(void) NotifyMetricsUnsuspended();
  NS_IMETHOD_(void) NotifyGlyphFragmentTreeSuspended();
  NS_IMETHOD_(void) NotifyGlyphFragmentTreeUnsuspended();
  
protected:
  void UpdateGeometry(PRUint32 flags);
  void UpdateMetrics(PRUint32 flags);
  void UpdateFragmentTree();
  nsISVGOuterSVGFrame *GetOuterSVGFrame();
  nsISVGTextFrame *GetTextFrame();
  
  nsCOMPtr<nsISVGRendererGlyphGeometry> mGeometry;
  nsCOMPtr<nsISVGRendererGlyphMetrics> mMetrics;
  float mX, mY;
  PRUint32 mCharOffset;
  PRUint32 mGeometryUpdateFlags;
  PRUint32 mMetricsUpdateFlags;
  PRBool mFragmentTreeDirty;
  nsString mCharacterData;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGGlyphFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame* parentFrame,
                    nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;

#ifdef DEBUG
  NS_ASSERTION(parentFrame, "null parent");
  nsISVGTextContainerFrame *text_container;
  parentFrame->QueryInterface(NS_GET_IID(nsISVGTextContainerFrame), (void**)&text_container);
  NS_ASSERTION(text_container, "trying to construct an SVGGlyphFrame for an invalid container");
  
  nsCOMPtr<nsITextContent> tc = do_QueryInterface(aContent);
  NS_ASSERTION(tc, "trying to construct an SVGGlyphFrame for wrong content element");
#endif
  
  nsSVGGlyphFrame* it = new (aPresShell) nsSVGGlyphFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;

  return NS_OK;
}

nsSVGGlyphFrame::nsSVGGlyphFrame()
    : mGeometryUpdateFlags(0), mMetricsUpdateFlags(0),
      mCharOffset(0), mFragmentTreeDirty(PR_FALSE)
{
}

nsSVGGlyphFrame::~nsSVGGlyphFrame()
{

}


//----------------------------------------------------------------------
// nsISupports methods

NS_INTERFACE_MAP_BEGIN(nsSVGGlyphFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGGeometrySource)
  NS_INTERFACE_MAP_ENTRY(nsISVGGlyphGeometrySource)
  NS_INTERFACE_MAP_ENTRY(nsISVGPositionedGlyphGeometrySource)
  NS_INTERFACE_MAP_ENTRY(nsISVGGlyphFragmentLeaf)
  NS_INTERFACE_MAP_ENTRY(nsISVGGlyphFragmentNode)
  NS_INTERFACE_MAP_ENTRY(nsISVGChildFrame)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGlyphFrameBase)

//----------------------------------------------------------------------
// nsIFrame methods

NS_IMETHODIMP
nsSVGGlyphFrame::Init(nsIPresContext*  aPresContext,
                      nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIStyleContext* aContext,
                      nsIFrame*        aPrevInFlow)
{
  nsresult rv;
//  rv = nsSVGGlyphFrameBase::Init(aPresContext, aContent, aParent,
//                                 aContext, aPrevInFlow);

  mContent = aContent;
  NS_IF_ADDREF(mContent);
  mParent = aParent;

  // construct our glyphmetrics & glyphgeometry objects:
  nsISVGOuterSVGFrame* outerSVGFrame = GetOuterSVGFrame();
  if (!outerSVGFrame) {
    NS_ERROR("No outerSVGFrame");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsISVGRenderer> renderer;
  outerSVGFrame->GetRenderer(getter_AddRefs(renderer));

  renderer->CreateGlyphMetrics(this, getter_AddRefs(mMetrics));
  if (!mMetrics) return NS_ERROR_FAILURE;
  
  renderer->CreateGlyphGeometry(this, getter_AddRefs(mGeometry));
  if (!mGeometry) return NS_ERROR_FAILURE;
  
  
  rv = SetStyleContext(aPresContext, aContext);
    
  return rv;
}

NS_IMETHODIMP
nsSVGGlyphFrame::AttributeChanged(nsIPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRInt32         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  PRInt32         aModType,
                                  PRInt32         aHint)
{
  // we don't use this notification mechanism
  
#ifdef DEBUG
//  printf("** nsSVGGlyphFrame::AttributeChanged(");
//  nsAutoString str;
//  aAttribute->ToString(str);
//  nsCAutoString cstr;
//  cstr.AssignWithConversion(str);
//  printf(cstr.get());
//  printf(")\n");
#endif
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGlyphFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
  // XXX: we'd like to use the style_hint mechanism and the
  // ContentStateChanged/AttributeChanged functions for style changes
  // to get slightly finer granularity, but unfortunately the
  // style_hints don't map very well onto svg. Here seems to be the
  // best place to deal with style changes:
  
  UpdateGeometry(nsISVGRendererGlyphGeometry::UPDATEMASK_ALL);

  return NS_OK;
}
  

//----------------------------------------------------------------------
// nsISVGChildFrame methods

NS_IMETHODIMP
nsSVGGlyphFrame::Paint(nsISVGRendererRenderContext* renderingContext)
{
#ifdef DEBUG
  //printf("nsSVGGlyphFrame(%p)::Paint\n", this);
#endif
  mGeometry->Render(renderingContext);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGlyphFrame::GetFrameForPoint(float x, float y, nsIFrame** hit)
{
#ifdef DEBUG
  //printf("nsSVGGlyphFrame(%p)::GetFrameForPoint\n", this);
#endif
  // test for hit:
  *hit = nsnull;
  PRBool isHit;
  mGeometry->ContainsPoint(x, y, &isHit);
  if (isHit) 
    *hit = this;
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGlyphFrame::InitialUpdate()
{
  UpdateFragmentTree();
  UpdateMetrics(nsISVGRendererGlyphMetrics::UPDATEMASK_ALL);
  UpdateGeometry(nsISVGRendererGlyphGeometry::UPDATEMASK_ALL);
  return NS_OK;
}  

NS_IMETHODIMP
nsSVGGlyphFrame::NotifyCTMChanged()
{
  UpdateGeometry(nsISVGRendererGlyphGeometry::UPDATEMASK_CTM);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGlyphFrame::NotifyRedrawSuspended()
{
  // XXX should we cache the fact that redraw is suspended?
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGlyphFrame::NotifyRedrawUnsuspended()
{
  NS_ASSERTION(!mMetricsUpdateFlags, "dirty metrics in nsSVGGlyphFrame::NotifyRedrawUnsuspended");
  NS_ASSERTION(!mFragmentTreeDirty, "dirty fragmenttree in nsSVGGlyphFrame::NotifyRedrawUnsuspended");
    
  if (mGeometryUpdateFlags != 0) {
    nsCOMPtr<nsISVGRendererRegion> dirty_region;
    mGeometry->Update(mGeometryUpdateFlags, getter_AddRefs(dirty_region));
    if (dirty_region) {
      nsISVGOuterSVGFrame* outerSVGFrame = GetOuterSVGFrame();
      if (outerSVGFrame)
        outerSVGFrame->InvalidateRegion(dirty_region, PR_TRUE);
    }
    mGeometryUpdateFlags = 0;
  }
  return NS_OK;
}


//----------------------------------------------------------------------
// nsISVGGeometrySource methods:

/* [noscript] readonly attribute nsIPresContext presContext; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetPresContext(nsIPresContext * *aPresContext)
{
  nsISVGOuterSVGFrame *outerSVGFrame = GetOuterSVGFrame();
  if (!outerSVGFrame) {
    NS_ERROR("null outerSVGFrame");
    return NS_ERROR_FAILURE;
  }
  
  return outerSVGFrame->GetPresContext(aPresContext);
}

/* readonly attribute nsIDOMSVGMatrix CTM; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetCTM(nsIDOMSVGMatrix * *aCTM)
{
  *aCTM = nsnull;
  
  nsISVGTextFrame * textframe = GetTextFrame();
  NS_ASSERTION(textframe, "null textframe");
  
  return textframe->GetCTM(aCTM);  
}

/* readonly attribute float strokeOpacity; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeOpacity(float *aStrokeOpacity)
{
  *aStrokeOpacity = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeOpacity;
  return NS_OK;
}

/* readonly attribute float strokeWidth; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeWidth(float *aStrokeWidth)
{
  *aStrokeWidth = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeWidth;
  return NS_OK;
}

/* void getStrokeDashArray ([array, size_is (count)] out float arr, out unsigned long count); */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeDashArray(float **arr, PRUint32 *count)
{
  *arr = nsnull;
  *count = 0;
  
  const nsString &dasharrayString = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeDasharray;
  if (dasharrayString.Length == 0) return NS_OK;

  // XXX parsing of the dasharray string should be done elsewhere

  char *str = ToNewCString(dasharrayString);

  // array elements are separated by commas. count them to get our max
  // no of elems.

  int i=0;
  char* cp = str;
  while (*cp) {
    if (*cp == ',')
      ++i;
    ++cp;
  }
  ++i;

  // now get the elements
  
  *arr = (float*) nsMemory::Alloc(i * sizeof(float));

  cp = str;
  char *elem;
  while ((elem = nsCRT::strtok(cp, "',", &cp))) {
    char *end;
    (*arr)[(*count)++] = (float) PR_strtod(elem, &end);
#ifdef DEBUG
    //printf("[%f]",(*arr)[(*count)-1]);
#endif
  }
  
  nsMemory::Free(str);

  return NS_OK;
}

/* readonly attribute float strokeDashoffset; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeDashoffset(float *aStrokeDashoffset)
{
  *aStrokeDashoffset = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeDashoffset;
  return NS_OK;
}

/* readonly attribute unsigned short strokeLinecap; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeLinecap(PRUint16 *aStrokeLinecap)
{
  *aStrokeLinecap = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeLinecap;
  return NS_OK;
}

/* readonly attribute unsigned short strokeLinejoin; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeLinejoin(PRUint16 *aStrokeLinejoin)
{
  *aStrokeLinejoin = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeLinejoin;
  return NS_OK;
}

/* readonly attribute float strokeMiterlimit; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokeMiterlimit(float *aStrokeMiterlimit)
{
  *aStrokeMiterlimit = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStrokeMiterlimit; 
  return NS_OK;
}

/* readonly attribute float fillOpacity; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetFillOpacity(float *aFillOpacity)
{
  *aFillOpacity = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mFillOpacity;
  return NS_OK;
}

/* readonly attribute unsigned short fillRule; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetFillRule(PRUint16 *aFillRule)
{
  *aFillRule = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mFillRule;
  return NS_OK;
}

/* readonly attribute unsigned short strokePaintType; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokePaintType(PRUint16 *aStrokePaintType)
{
  *aStrokePaintType = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStroke.mType;
  return NS_OK;
}

/* [noscript] readonly attribute nscolor strokePaint; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetStrokePaint(nscolor *aStrokePaint)
{
  *aStrokePaint = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mStroke.mColor;
  return NS_OK;
}

/* readonly attribute unsigned short fillPaintType; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetFillPaintType(PRUint16 *aFillPaintType)
{
  *aFillPaintType = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mFill.mType;
  return NS_OK;
}

/* [noscript] readonly attribute nscolor fillPaint; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetFillPaint(nscolor *aFillPaint)
{
  *aFillPaint = ((const nsStyleSVG*) mStyleContext->GetStyleData(eStyleStruct_SVG))->mFill.mColor;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGGlyphGeometrySource methods:

/* [noscript] readonly attribute nsFont font; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetFont(nsFont *aFont)
{
  *aFont = ((const nsStyleFont*)mStyleContext->GetStyleData(eStyleStruct_Font))->mFont;

  // XXX eventually we will have to treat decorations separately from
  // fonts, because they can have a different color than the current
  // glyph.
  
  nsCOMPtr<nsIStyleContext> parentContext;
  NS_ASSERTION(mParent, "no parent");
  mParent->GetStyleContext(getter_AddRefs(parentContext));
  NS_ASSERTION(parentContext, "no style context on parent");
  
  PRUint8 styleDecorations =
    ((const nsStyleTextReset*)parentContext->GetStyleData(eStyleStruct_TextReset))->mTextDecoration;
  if (styleDecorations & NS_STYLE_TEXT_DECORATION_UNDERLINE)
    aFont->decorations |= NS_FONT_DECORATION_UNDERLINE;
  if (styleDecorations & NS_STYLE_TEXT_DECORATION_OVERLINE)
    aFont->decorations |= NS_FONT_DECORATION_OVERLINE;
  if (styleDecorations & NS_STYLE_TEXT_DECORATION_LINE_THROUGH)
    aFont->decorations |= NS_FONT_DECORATION_LINE_THROUGH;    
  
  return NS_OK;
}

/* readonly attribute DOMString characterData; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetCharacterData(nsAString & aCharacterData)
{
  aCharacterData = mCharacterData;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGPositionedGlyphGeometrySource methods:

/* readonly attribute nsISVGRendererGlyphMetrics metrics; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetMetrics(nsISVGRendererGlyphMetrics * *aMetrics)
{
  *aMetrics = mMetrics;
  NS_ADDREF(*aMetrics);
  return NS_OK;
}

/* readonly attribute float x; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}

/* readonly attribute float y; */
NS_IMETHODIMP
nsSVGGlyphFrame::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGGlyphFragmentLeaf interface:

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::SetGlyphPosition(float x, float y)
{
  mX = x;
  mY = y;
  UpdateGeometry(nsISVGRendererGlyphGeometry::UPDATEMASK_POSITIONING);
}

NS_IMETHODIMP
nsSVGGlyphFrame::GetGlyphMetrics(nsISVGRendererGlyphMetrics** metrics)
{
  *metrics = mMetrics;
  NS_ADDREF(*metrics);
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsSVGGlyphFrame::IsStartOfChunk()
{
  // this fragment is a chunk if it has a corresponding absolute
  // position adjustment in an ancestors' x or y array. (At the moment
  // we don't map the full arrays, but only the first elements.)

  return PR_FALSE;
}

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::GetAdjustedPosition(/* inout */ float &x, /* inout */ float &y)
{
}


//----------------------------------------------------------------------
// nsISVGGlyphFragmentNode interface:

NS_IMETHODIMP_(nsISVGGlyphFragmentLeaf *)
nsSVGGlyphFrame::GetFirstGlyphFragment()
{
  return this;
}

NS_IMETHODIMP_(nsISVGGlyphFragmentLeaf *)
nsSVGGlyphFrame::GetNextGlyphFragment()
{
  nsIFrame* sibling = mNextSibling;
  while (sibling) {
    nsISVGGlyphFragmentNode *node = nsnull;
    sibling->QueryInterface(NS_GET_IID(nsISVGGlyphFragmentNode), (void**)&node);
    if (node)
      return node->GetFirstGlyphFragment();
    sibling->GetNextSibling(&sibling);
  }

  // no more siblings. go back up the tree.
  
  NS_ASSERTION(mParent, "null parent");
  nsISVGGlyphFragmentNode *node = nsnull;
  mParent->QueryInterface(NS_GET_IID(nsISVGGlyphFragmentNode), (void**)&node);
  return node ? node->GetNextGlyphFragment() : nsnull;
}

NS_IMETHODIMP_(PRUint32)
nsSVGGlyphFrame::BuildGlyphFragmentTree(PRUint32 charNum, PRBool lastBranch)
{
  // XXX actually we should be building a new fragment for each chunk here...


  mCharOffset = charNum;
  nsCOMPtr<nsITextContent> tc = do_QueryInterface(mContent);
  tc->CopyText(mCharacterData);
  mCharacterData.CompressWhitespace(charNum==0, lastBranch);
  
  return charNum+mCharacterData.Length();
}

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::NotifyMetricsSuspended()
{
  // do nothing
}

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::NotifyMetricsUnsuspended()
{
  NS_ASSERTION(!mFragmentTreeDirty, "dirty fragmenttree in nsSVGGlyphFrame::NotifyMetricsUnsuspended");

  if (mMetricsUpdateFlags != 0) {
    PRBool metricsDirty;
    mMetrics->Update(mMetricsUpdateFlags, &metricsDirty);
    if (metricsDirty) {
      mGeometryUpdateFlags |= nsISVGRendererGlyphGeometry::UPDATEMASK_METRICS;
      nsISVGTextFrame* text_frame = GetTextFrame();
      NS_ASSERTION(text_frame, "null text frame");
      if (text_frame)
        text_frame->NotifyGlyphMetricsChange(this);
    }
    mMetricsUpdateFlags = 0;
  }   
}

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::NotifyGlyphFragmentTreeSuspended()
{
  // do nothing
}

NS_IMETHODIMP_(void)
nsSVGGlyphFrame::NotifyGlyphFragmentTreeUnsuspended()
{
  if (mFragmentTreeDirty) {
    nsISVGTextFrame* text_frame = GetTextFrame();
    NS_ASSERTION(text_frame, "null text frame");
    if (text_frame)
      text_frame->NotifyGlyphFragmentTreeChange(this);
    mFragmentTreeDirty = PR_FALSE;
  }
}



//----------------------------------------------------------------------
//

void nsSVGGlyphFrame::UpdateGeometry(PRUint32 flags)
{
  mGeometryUpdateFlags |= flags;
  
  nsISVGOuterSVGFrame *outerSVGFrame = GetOuterSVGFrame();
  if (!outerSVGFrame) {
    NS_ERROR("null outerSVGFrame");
    return;
  }
  
  PRBool suspended;
  outerSVGFrame->IsRedrawSuspended(&suspended);
  if (!suspended) {
    NS_ASSERTION(!mMetricsUpdateFlags, "dirty metrics in nsSVGGlyphFrame::UpdateGeometry");
    NS_ASSERTION(!mFragmentTreeDirty, "dirty fragmenttree in nsSVGGlyphFrame::UpdateGeometry");
    nsCOMPtr<nsISVGRendererRegion> dirty_region;
    mGeometry->Update(mGeometryUpdateFlags, getter_AddRefs(dirty_region));
    if (dirty_region)
      outerSVGFrame->InvalidateRegion(dirty_region, PR_TRUE);
    mGeometryUpdateFlags = 0;
  }  
}

void nsSVGGlyphFrame::UpdateMetrics(PRUint32 flags)
{
  mMetricsUpdateFlags |= flags;

  nsISVGTextFrame* text_frame = GetTextFrame();
  if (!text_frame) {
    NS_ERROR("null text_frame");
    return;
  }
  
  PRBool suspended = text_frame->IsMetricsSuspended();
  if (!suspended) {
    NS_ASSERTION(!mFragmentTreeDirty, "dirty fragmenttree in nsSVGGlyphFrame::UpdateMetrics");
    PRBool metricsDirty;
    mMetrics->Update(mMetricsUpdateFlags, &metricsDirty);
    if (metricsDirty) {
      mGeometryUpdateFlags |= nsISVGRendererGlyphGeometry::UPDATEMASK_METRICS;
      text_frame->NotifyGlyphMetricsChange(this);
    }
    mMetricsUpdateFlags = 0;
  }
}

void nsSVGGlyphFrame::UpdateFragmentTree()
{
  mFragmentTreeDirty = PR_TRUE;
    
  nsISVGTextFrame* text_frame = GetTextFrame();
  if (!text_frame) {
    NS_ERROR("null text_frame");
    return;
  }
  
  PRBool suspended = text_frame->IsGlyphFragmentTreeSuspended();
  if (!suspended) {
    text_frame->NotifyGlyphFragmentTreeChange(this);
    mFragmentTreeDirty = PR_FALSE;
  }
}

nsISVGOuterSVGFrame *
nsSVGGlyphFrame::GetOuterSVGFrame()
{
  NS_ASSERTION(mParent, "null parent");
  
  nsISVGContainerFrame *containerFrame;
  mParent->QueryInterface(NS_GET_IID(nsISVGContainerFrame), (void**)&containerFrame);
  if (!containerFrame) {
    NS_ERROR("invalid container");
    return nsnull;
  }

  return containerFrame->GetOuterSVGFrame();  
}

nsISVGTextFrame *
nsSVGGlyphFrame::GetTextFrame()
{
  NS_ASSERTION(mParent, "null parent");

  nsISVGTextContainerFrame *containerFrame;
  mParent->QueryInterface(NS_GET_IID(nsISVGTextContainerFrame), (void**)&containerFrame);
  if (!containerFrame) {
    NS_ERROR("invalid container");
    return nsnull;
  }

  return containerFrame->GetTextFrame();
}
