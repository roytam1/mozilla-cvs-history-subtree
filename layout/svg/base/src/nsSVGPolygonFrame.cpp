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
#include "nsIDOMSVGAnimatedPoints.h"
#include "nsIDOMSVGPointList.h"
#include "nsIDOMSVGPoint.h"
#include "nsSVGPath.h"
#include "nsIDOMSVGMatrix.h"

class nsSVGPolygonFrame : public nsSVGGraphicFrame
{
  friend nsresult
  NS_NewSVGPolygonFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);

  ~nsSVGPolygonFrame();

  virtual nsresult Init();
  virtual void BuildPath();

  nsCOMPtr<nsIDOMSVGPointList> mPoints;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGPolygonFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  nsCOMPtr<nsIDOMSVGAnimatedPoints> anim_points = do_QueryInterface(aContent);
  if (!anim_points) {
#ifdef DEBUG
    printf("warning: trying to contruct an SVGPolygonFrame for a content element that doesn't support the right interfaces\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  nsSVGPolygonFrame* it = new (aPresShell) nsSVGPolygonFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
}

nsSVGPolygonFrame::~nsSVGPolygonFrame()
{
  nsCOMPtr<nsISVGValue> value;
  if (mPoints && (value = do_QueryInterface(mPoints)))
      value->RemoveObserver(this);
}

nsresult nsSVGPolygonFrame::Init()
{
  nsCOMPtr<nsIDOMSVGAnimatedPoints> anim_points = do_QueryInterface(mContent);
  NS_ASSERTION(anim_points,"wrong content element");
  anim_points->GetPoints(getter_AddRefs(mPoints));
  NS_ASSERTION(mPoints, "no points");
  if (!mPoints) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mPoints);
  if (value)
    value->AddObserver(this);
  return nsSVGGraphicFrame::Init();
}  

void nsSVGPolygonFrame::BuildPath()
{
  if (mPath) {
    delete mPath;
    mPath = nsnull;
  }

  if (!mPoints) return;

  nsCOMPtr<nsIDOMSVGMatrix> ctm;
  GetCTM(getter_AddRefs(ctm));
  
  mPath = new nsSVGPath();
  mPath->Build(mPoints, ctm, PR_TRUE);
}
