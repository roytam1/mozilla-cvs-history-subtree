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
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */

#include "nsSVGGraphicFrame.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGPoint.h"
#include "nsIDOMSVGCircleElement.h"
#include "nsIDOMSVGElement.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsASVGPathBuilder.h"

class nsSVGCircleFrame : public nsSVGGraphicFrame
{
  friend nsresult
  NS_NewSVGCircleFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);

  virtual ~nsSVGCircleFrame();

  virtual nsresult Init();
  virtual void ConstructPath(nsASVGPathBuilder* pathBuilder);

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
    printf("warning: trying to construct an SVGCircleFrame for a content element that doesn't support the right interfaces\n");
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

void nsSVGCircleFrame::ConstructPath(nsASVGPathBuilder* pathBuilder)
{
  float x,y,r;

  mCx->GetValue(&x);
  mCy->GetValue(&y);
  mR->GetValue(&r);

  // we should be able to build this as a single arc with startpoints
  // and endpoints infinitesimally separated. Let's use two arcs
  // though, so that the builder doesn't get confused:
  pathBuilder->Moveto(x, y-r);
  pathBuilder->Arcto(x-r, y  , r, r, 0.0, 0, 0);
  pathBuilder->Arcto(x  , y-r, r, r, 0.0, 1, 0);
  pathBuilder->ClosePath(&x,&y);  
}
