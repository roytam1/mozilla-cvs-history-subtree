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
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGPath.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGCircleElement.h"
#include "nsIDOMSVGElement.h"
#include "nsIDOMSVGSVGElement.h"

class nsSVGCircleFrame : public nsSVGGraphicFrame
{
  friend nsresult
  NS_NewSVGCircleFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);

  virtual ~nsSVGCircleFrame();

  virtual nsresult Init();
  virtual void BuildPath();

  nsCOMPtr<nsIDOMSVGLength> mCx;
  nsCOMPtr<nsIDOMSVGLength> mCy;
  nsCOMPtr<nsIDOMSVGLength> mR;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGCircleFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;
  
  nsCOMPtr<nsIDOMSVGCircleElement> circle = do_QueryInterface(aContent);
  if (!circle) {
#ifdef DEBUG
    printf("warning: trying to contruct an SVGCircleFrame for a content element that doesn't support the right interfaces\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  nsSVGCircleFrame* it = new (aPresShell) nsSVGCircleFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
}

nsSVGCircleFrame::~nsSVGCircleFrame()
{
  nsCOMPtr<nsISVGValue> value;
  if (mCx && (value = do_QueryInterface(mCx)))
      value->RemoveObserver(this);
  if (mCy && (value = do_QueryInterface(mCy)))
      value->RemoveObserver(this);
  if (mR && (value = do_QueryInterface(mR)))
      value->RemoveObserver(this);
}

nsresult nsSVGCircleFrame::Init()
{
  nsCOMPtr<nsIDOMSVGCircleElement> circle = do_QueryInterface(mContent);
  NS_ASSERTION(circle,"wrong content element");

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    circle->GetCx(getter_AddRefs(length));
    length->GetBaseVal(getter_AddRefs(mCx));
    NS_ASSERTION(mCx, "no cx");
    if (!mCx) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mCx);
    if (value)
      value->AddObserver(this);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    circle->GetCy(getter_AddRefs(length));
    length->GetBaseVal(getter_AddRefs(mCy));
    NS_ASSERTION(mCx, "no cy");
    if (!mCx) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mCy);
    if (value)
      value->AddObserver(this);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    circle->GetR(getter_AddRefs(length));
    length->GetBaseVal(getter_AddRefs(mR));
    NS_ASSERTION(mCx, "no r");
    if (!mCx) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mR);
    if (value)
      value->AddObserver(this);
  }

  
    return nsSVGGraphicFrame::Init();
}  

void nsSVGCircleFrame::BuildPath()
{
  if (mPath) {
    delete mPath;
    mPath = nsnull;
  }

  nsCOMPtr<nsIDOMSVGMatrix> ctm;
  GetCTM(getter_AddRefs(ctm));

  float x,y,r;

  // XXX this would be nice: 
  // mCx->GetTransformedValue(ctm, &x);
  // mCy->GetTransformedValue(ctm, &y);
  // mR->GetTransformedValue(ctm, &r);

  // XXX but instead we do this. how verbose can it get? - sigh -
  
  mCx->GetValue(&x);
  mCy->GetValue(&y);
  mR->GetValue(&r);

  nsCOMPtr<nsIDOMSVGElement> el = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMSVGSVGElement> svg_el;
  el->GetOwnerSVGElement(getter_AddRefs(svg_el));
  if (!svg_el) return;
  nsCOMPtr<nsIDOMSVGPoint> point;
  svg_el->CreateSVGPoint(getter_AddRefs(point));
  NS_ASSERTION(point, "couldn't create point!");
  if (!point) return;
  
  {  // coordinates transform like points (XXX)
    point->SetX(x);
    point->SetY(y);
    nsCOMPtr<nsIDOMSVGPoint> xfpoint;
    point->MatrixTransform(ctm, getter_AddRefs(xfpoint));
    xfpoint->GetX(&x);
    xfpoint->GetY(&y);
  }

  
  {  // lengths transform like vectors (XXX)
    const float inv_root_2 = 0.707106781188f;
    point->SetX(inv_root_2*r);
    point->SetY(inv_root_2*r);
    nsCOMPtr<nsIDOMSVGPoint> xfpoint;
    point->MatrixTransform(ctm, getter_AddRefs(xfpoint));
    float r1,r2;
    xfpoint->GetX(&r1);
    xfpoint->GetY(&r2);

    point->SetX(0.0f);
    point->SetY(0.0f);
    point->MatrixTransform(ctm, getter_AddRefs(xfpoint));
    float o1,o2;
    xfpoint->GetX(&o1);
    xfpoint->GetY(&o2);
    
    float dr1, dr2;
    dr1 = r1 - o1;
    dr2 = r2 - o2;
    
    r = (float) sqrt( dr1*dr1 + dr2*dr2 );
  }
  
  nsCOMPtr<nsIDOMSVGPoint> radius;
  
  nsCOMPtr<nsIDOMSVGPoint> xFormedPoint;
  
    
  mPath = new nsSVGPath();

  
  mPath->SetCircle(x,y,r);
}
