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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *          Alex Fritze <alex.fritze@crocodile-clips.com>
 *
 */

#include "nsSVGGraphicFrame.h"
#include "nsIPresContext.h"
#include "nsISVGFrame.h"
#include "nsSVGPath.h"
#include "nsSVGStroke.h"
#include "nsSVGFill.h"
#include "nsSVGRenderingContext.h"
#include "nsSVGAtoms.h"
#include "nsIDOMSVGTransformable.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGPoint.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGraphicFrame

nsSVGGraphicFrame::nsSVGGraphicFrame()
    : mPath(nsnull), mStroke(nsnull), mFill(nsnull), mDirty(PR_FALSE)
{
//  printf("nsSVGGraphicFrame CTOR\n");
}

nsSVGGraphicFrame::~nsSVGGraphicFrame()
{
  nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(mContent);
  NS_ASSERTION(transformable, "wrong content element");
  nsCOMPtr<nsIDOMSVGAnimatedTransformList> transforms;
  transformable->GetTransform(getter_AddRefs(transforms));
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(transforms);
  NS_ASSERTION(transformable, "interface not found");
  if (value)
    value->RemoveObserver(this);


  if (mPath) delete mPath;
  if (mStroke) delete mStroke;
  if (mFill) delete mFill;
  
  printf("~nsSVGGraphicFrame\n");
}



//----------------------------------------------------------------------
// nsISupports methods

NS_INTERFACE_MAP_BEGIN(nsSVGGraphicFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGraphicFrameBase)

// NS_IMETHODIMP
// nsSVGGraphicFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
// {
//   NS_PRECONDITION(0 != aInstancePtr, "null ptr");
//   if (NULL == aInstancePtr) {
//     return NS_ERROR_NULL_POINTER;
//   }

//   if (aIID.Equals(NS_GET_IID(nsISVGFrame))) {
//     *aInstancePtr = (void*) ((nsISVGFrame*) this);
//     return NS_OK;
//   }
//   return nsSVGGraphicFrameBase::QueryInterface(aIID, aInstancePtr);
// }

//----------------------------------------------------------------------
// nsIFrame methods
NS_IMETHODIMP
nsSVGGraphicFrame::Init(nsIPresContext*  aPresContext,
                     nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIStyleContext* aContext,
                     nsIFrame*        aPrevInFlow)
{
  nsresult rv;
//  rv = nsSVGGraphicFrameBase::Init(aPresContext, aContent, aParent,
//                                aContext, aPrevInFlow);

  mContent = aContent;
  NS_IF_ADDREF(mContent);
  mParent = aParent;

  Init();
  
  rv = SetStyleContext(aPresContext, aContext);
    
  return rv;
}

NS_IMETHODIMP
nsSVGGraphicFrame::AttributeChanged(nsIPresContext* aPresContext,
                                    nsIContent*     aChild,
                                    PRInt32         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    PRInt32         aHint)
{
#ifdef DEBUG
  printf("** nsSVGGraphicFrame::AttributeChanged(");
  nsAutoString str;
  aAttribute->ToString(str);
  nsCAutoString cstr;
  cstr.AssignWithConversion(str);
  printf(cstr.get());
  printf(")\n");
#endif
  
  // XXX is this still needed? does it even work?
  if (aAttribute == nsSVGAtoms::style) {
    InvalidateRegion(GetUta(), PR_FALSE);  
    BuildRenderItems();
    InvalidateRegion(GetUta(), PR_TRUE);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::DidSetStyleContext(nsIPresContext* aPresContext)
{
  // XXX: we'd like to use the style_hint mechanism and the
  // ContentStateChanged/AttributeChanged functions for style changes
  // to get slightly finer granularity, but unfortunately the
  // style_hints don't map very well onto svg. Here seems to be the
  // best place to deal with style changes:
  InvalidateRegion(GetUta(), PR_FALSE);
  BuildRenderItems();
  InvalidateRegion(GetUta(), PR_TRUE);
  return NS_OK;
}


//----------------------------------------------------------------------
// nsISVGValueObserver methods:

NS_IMETHODIMP
nsSVGGraphicFrame::WillModifySVGObservable(nsISVGValue* observable)
{
  InvalidateRegion(GetUta(), PR_FALSE);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGGraphicFrame::DidModifySVGObservable (nsISVGValue* observable)
{
  PRBool suspended;
  IsRedrawSuspended(&suspended);
  if (!suspended) {
    Build();
    InvalidateRegion(GetUta(), PR_TRUE);
  }
  else
    mDirty = PR_TRUE;
  return NS_OK;
}


//----------------------------------------------------------------------
// nsISVGFrame methods

NS_IMETHODIMP
nsSVGGraphicFrame::Paint(nsSVGRenderingContext* renderingContext)
{
  if (mFill && !mFill->IsEmpty()) {
    renderingContext->PaintSVGRenderItem(mFill);
  }
  
  if (mStroke && !mStroke->IsEmpty())
    renderingContext->PaintSVGRenderItem(mStroke);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::InvalidateRegion(ArtUta* uta, PRBool bRedraw)
{
  if (!uta && !bRedraw) return NS_OK;

  NS_ASSERTION(mParent, "need parent to invalidate!");
  if (!mParent) {
    if (uta)
      art_free(uta);
    return NS_OK;
  }

  nsCOMPtr<nsISVGFrame> SVGFrame = do_QueryInterface(mParent);
  NS_ASSERTION(SVGFrame, "wrong frame type!");
  if (!SVGFrame) {
    if (uta)
      art_free(uta);
    return NS_OK;
  }

  return SVGFrame->InvalidateRegion(uta, bRedraw);
}

NS_IMETHODIMP
nsSVGGraphicFrame::GetFrameForPoint(float x, float y, nsIFrame** hit)
{
  *hit = nsnull;

  if (mStroke && !mStroke->IsEmpty()) {
    int wind = art_svp_point_wind(mStroke->GetSvp(), x, y);
    if (wind) {
      *hit = this;
      return NS_OK;
    }
  }

  if (mFill && !mFill->IsEmpty()) {
    int wind = art_svp_point_wind(mFill->GetSvp(), x, y);
    if (wind) {
      *hit = this;
      return NS_OK;
    }
  }

  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::NotifyCTMChanged()
{
  // XXX complete rebuild is wasteful. transform our path & visuals
  // instead

  InvalidateRegion(GetUta(), PR_FALSE);
  PRBool suspended;
  IsRedrawSuspended(&suspended);
  if (!suspended) {
    Build();
    InvalidateRegion(GetUta(), PR_TRUE);
  }
  else
    mDirty = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::NotifyRedrawSuspended()
{
#ifdef DEBUG
  printf("nsSVGGraphicFrame %p::NotifyRedrawSuspended()\n",this);
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::NotifyRedrawUnsuspended()
{
#ifdef DEBUG
  printf("nsSVGGraphicFrame %p::NotifyRedrawUnsuspended()\n",this);
#endif
  if (mDirty) {
    mDirty = PR_FALSE;
    Build();
    InvalidateRegion(GetUta(), PR_TRUE);
   }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGraphicFrame::IsRedrawSuspended(PRBool* isSuspended)
{
#ifdef DEBUG
  printf("nsSVGGraphicFrame %p::IsRedrawSuspended()\n",this);
#endif
  nsCOMPtr<nsISVGFrame> SVGFrame = do_QueryInterface(mParent);
  if (!SVGFrame) {
    NS_ASSERTION(SVGFrame, "no parent frame");
    *isSuspended = PR_FALSE;
    return NS_OK;
  }

  return SVGFrame->IsRedrawSuspended(isSuspended);  
}

//----------------------------------------------------------------------
//

nsresult nsSVGGraphicFrame::Init()
{
  nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(mContent);
  NS_ASSERTION(transformable, "wrong content element");
  nsCOMPtr<nsIDOMSVGAnimatedTransformList> transforms;
  transformable->GetTransform(getter_AddRefs(transforms));
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(transforms);
  NS_ASSERTION(transformable, "interface not found");
  if (value)
    value->AddObserver(this);
    
  return NS_OK;
}

void nsSVGGraphicFrame::Build()
{
#ifdef DEBUG
  printf("nsSVGGraphicFrame(%p)::Build()\n",this);
#endif
  BuildPath();
  BuildRenderItems();
}

void nsSVGGraphicFrame::BuildRenderItems()
{
  if (!mPath) {
    PRBool suspended;
    IsRedrawSuspended(&suspended);
    if (suspended) {
      mDirty = PR_TRUE;
      return;
    }
    BuildPath();
  }
  
  if (mFill) {
    delete mFill;
    mFill = nsnull;
  }
  if (mStroke) {
    delete mStroke;
    mStroke = nsnull;
  }

  if (!mPath || mPath->IsEmpty()) return;

  const nsStyleSVG* svgStyle = (const nsStyleSVG*)
    mStyleContext->GetStyleData(eStyleStruct_SVG);

  if (svgStyle->mStroke.mType == eStyleSVGPaintType_Color) {
    nsSVGStrokeStyle strokeStyle;
    strokeStyle.color   = svgStyle->mStroke.mColor;
    strokeStyle.opacity = svgStyle->mStrokeOpacity;
    strokeStyle.width   = svgStyle->mStrokeWidth;
    mStroke = new nsSVGStroke();
    mStroke->Build(mPath, strokeStyle);
    if (mStroke->IsEmpty()) {
      delete mStroke;
      mStroke = nsnull;
    }
  }

  if (svgStyle->mFill.mType == eStyleSVGPaintType_Color) {
    nsSVGFillStyle fillStyle;
    fillStyle.color   = svgStyle->mFill.mColor;
    fillStyle.opacity = svgStyle->mFillOpacity;
    mFill = new nsSVGFill();
    mFill->Build(mPath, fillStyle);
    if (mFill->IsEmpty()) {
      delete mFill;
      mFill = nsnull;
    }
  }
  
}

ArtUta*
nsSVGGraphicFrame::GetUta()
{
  if (!mFill && !mStroke) {
    return nsnull;
  }
  
  ArtUta* f = mFill ? mFill->GetUta() : nsnull;
  ArtUta* s = mStroke ? mStroke->GetUta() : nsnull;

  if (f == nsnull)
    return s;
  if (s == nsnull)
    return f;

  ArtUta* u = art_uta_union(f, s);
  art_free(f);
  art_free(s);
  
  return u;
}

void nsSVGGraphicFrame::GetCTM(nsIDOMSVGMatrix** ctm)
{
  *ctm = nsnull;
  
  nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(mContent);
  NS_ASSERTION(transformable, "wrong content type");
  
  transformable->GetScreenCTM(ctm);  
}

void nsSVGGraphicFrame::TransformPoint(float& x, float& y)
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm;
  GetCTM(getter_AddRefs(ctm));
  if (!ctm) return;

  // XXX this is absurd! we need to add another method (interface
  // even?) to nsIDOMSVGMatrix to make this easier. (something like
  // nsIDOMSVGMatrix::Transform(float*x,float*y))
  
  nsCOMPtr<nsIDOMSVGElement> el = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMSVGSVGElement> svg_el;
  el->GetOwnerSVGElement(getter_AddRefs(svg_el));
  if (!svg_el) return;
  nsCOMPtr<nsIDOMSVGPoint> point;
  svg_el->CreateSVGPoint(getter_AddRefs(point));
  NS_ASSERTION(point, "couldn't create point!");
  if (!point) return;
  
  point->SetX(x);
  point->SetY(y);
  nsCOMPtr<nsIDOMSVGPoint> xfpoint;
  point->MatrixTransform(ctm, getter_AddRefs(xfpoint));
  xfpoint->GetX(&x);
  xfpoint->GetY(&y);
}
