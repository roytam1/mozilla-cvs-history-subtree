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
#include "nsIDOMSVGAnimatedPathData.h"
#include "nsIDOMSVGPathSegList.h"
#include "nsIDOMSVGPathSeg.h"
#include "nsSVGPath.h"
#include "nsIDOMSVGMatrix.h"

class nsSVGPathFrame : public nsSVGGraphicFrame
{
  friend nsresult
  NS_NewSVGPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);

  ~nsSVGPathFrame();

  virtual nsresult Init();
  virtual void BuildPath();

  nsCOMPtr<nsIDOMSVGPathSegList> mSegments;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;
  
  nsCOMPtr<nsIDOMSVGAnimatedPathData> anim_data = do_QueryInterface(aContent);
  if (!anim_data) {
#ifdef DEBUG
    printf("warning: trying to contruct an SVGPathFrame for a content element that doesn't support the right interfaces\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  nsSVGPathFrame* it = new (aPresShell) nsSVGPathFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;
  return NS_OK;
}

nsSVGPathFrame::~nsSVGPathFrame()
{
  nsCOMPtr<nsISVGValue> value;
  if (mSegments && (value = do_QueryInterface(mSegments)))
      value->RemoveObserver(this);
}

nsresult nsSVGPathFrame::Init()
{
  nsCOMPtr<nsIDOMSVGAnimatedPathData> anim_data = do_QueryInterface(mContent);
  NS_ASSERTION(anim_data,"wrong content element");
  anim_data->GetAnimatedPathSegList(getter_AddRefs(mSegments));
  NS_ASSERTION(mSegments, "no pathseglist");
  if (!mSegments) return NS_ERROR_FAILURE;
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mSegments);
  if (value)
    value->AddObserver(this);
  
  return nsSVGGraphicFrame::Init();
}  

void nsSVGPathFrame::BuildPath()
{
#ifdef DEBUG
  printf("nsSVGPathFrame::BuildPath()\n");
#endif
  if (mPath) {
    mPath->Clear();
  }

  if (!mSegments) return;

  if (!mPath)
    mPath = new nsSVGPath();
  
  PRUint32 count;
  mSegments->GetNumberOfItems(&count);
  if (count == 0) return;

  PRUint32 i;
  float cx = 0.0f;
  float cy = 0.0f;
  for (i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMSVGPathSeg> segment;
    mSegments->GetItem(i, getter_AddRefs(segment));

    PRUint16 type = nsIDOMSVGPathSeg::PATHSEG_UNKNOWN;
    segment->GetPathSegType(&type);

    PRBool absCoords = PR_TRUE;
    
    switch (type) {
      case nsIDOMSVGPathSeg::PATHSEG_CLOSEPATH:
        mPath->ClosePath();
        break;
        
      case nsIDOMSVGPathSeg::PATHSEG_MOVETO_REL:
        absCoords = PR_FALSE;
      case nsIDOMSVGPathSeg::PATHSEG_MOVETO_ABS:
        {
          nsCOMPtr<nsIDOMSVGPathSegMovetoAbs> moveseg = do_QueryInterface(segment);
          NS_ASSERTION(moveseg, "interface not implemented");
          float x, y;
          moveseg->GetX(&x);
          moveseg->GetY(&y);
          if (!absCoords) {
            x += cx;
            y += cy;
          }
          TransformPoint(x, y);
          mPath->Moveto(x,y);
          cx = x;
          cy = y;
        }
        break;
        
      case nsIDOMSVGPathSeg::PATHSEG_LINETO_REL:
        absCoords = PR_FALSE;
      case nsIDOMSVGPathSeg::PATHSEG_LINETO_ABS:
        {
          nsCOMPtr<nsIDOMSVGPathSegLinetoAbs> lineseg = do_QueryInterface(segment);
          NS_ASSERTION(lineseg, "interface not implemented");
          float x, y;
          lineseg->GetX(&x);
          lineseg->GetY(&y);
          if (!absCoords) {
            x += cx;
            y += cy;
          }
          TransformPoint(x, y);
          mPath->Lineto(x,y);
          cx = x;
          cy = y;
        }
        break;        

      case nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_REL:
        absCoords = PR_FALSE;
      case nsIDOMSVGPathSeg::PATHSEG_CURVETO_CUBIC_ABS:
        {
          nsCOMPtr<nsIDOMSVGPathSegCurvetoCubicAbs> curveseg = do_QueryInterface(segment);
          NS_ASSERTION(curveseg, "interface not implemented");
          float x, y, x1, y1, x2, y2;
          curveseg->GetX(&x);
          curveseg->GetY(&y);
          curveseg->GetX1(&x1);
          curveseg->GetY1(&y1);
          curveseg->GetX2(&x2);
          curveseg->GetY2(&y2);
          if (!absCoords) {
            x  += cx;
            y  += cy;
            x1 += cx;
            y1 += cy;
            x2 += cx;
            y2 += cy;
          }
          TransformPoint(x, y);
          TransformPoint(x1, y1);
          TransformPoint(x2, y2);
          mPath->Curveto(x, y, x1, y1, x2, y2);
          cx = x;
          cy = y;
        }
        break;        
      default:
        /* */
        break;
    }
  }
  
}
