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

#include "nsSVGPath.h"
#include "nsIDOMSVGPointList.h"
#include "nsIDOMSVGPoint.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "prdtoa.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"

nsSVGPath::nsSVGPath()
    : mVPath(nsnull),
      mBPath(nsnull),
      mBPathSize(0),
      mBPathEnd(0)
{
  
}

nsSVGPath::~nsSVGPath()
{
  Clear();
}

PRBool nsSVGPath::IsEmpty()
{
  return (!mVPath && !mBPathEnd);
}

ArtVpath* nsSVGPath::GetVPath()
{
  if (!mVPath) {
    RebuildVPathFromBPath();
  }
  return mVPath;
}

//----------------------------------------------------------------------
// path construction methods

void nsSVGPath::Build(nsIDOMSVGPointList* points,
                      nsIDOMSVGMatrix* ctm,
                      PRBool bClose)
{ 
  Clear();

  PRUint32 count;
  points->GetNumberOfItems(&count);
  if (count == 0) return;
  
  mVPath = art_new(ArtVpath, count + (bClose ? 2 : 1));
  PRUint32 i;
  for (i = 0; i < count; ++i) {
    
    nsCOMPtr<nsIDOMSVGPoint> point;
    points->GetItem(i, getter_AddRefs(point));

    if (ctm) {
      nsCOMPtr<nsIDOMSVGPoint> tempPoint;
      point->MatrixTransform(ctm, getter_AddRefs(tempPoint));
      point = tempPoint;
    }
      
    mVPath[i].code = i ? ART_LINETO : ART_MOVETO;

    float val;
    point->GetX(&val);
    mVPath[i].x = val;
    point->GetY(&val);
    mVPath[i].y = val;
  }
  
  if (bClose) {
    mVPath[i].code = ART_LINETO;
    mVPath[i].x = mVPath[0].x;
    mVPath[i].y = mVPath[0].y;
    ++i;
  }
  mIsClosed = bClose;
  
  mVPath[i].code = ART_END;
  mVPath[i].x = 0.0;
  mVPath[i].y = 0.0;
}

void nsSVGPath::SetCircle(float x, float y, float r)
{
  Clear();
  mVPath = art_vpath_new_circle(x, y , r);
}

void nsSVGPath::Clear()
{
  if (mVPath) {
    art_free(mVPath);
    mVPath = nsnull;
  }  
  if (mBPath) {
    art_free(mBPath);
    mBPath = nsnull;
    mBPathSize = 0;
    mBPathEnd = 0;
  }  
}

void nsSVGPath::Moveto(float x, float y)
{
  EnsureBPathSpace();

  mBPath[mBPathEnd].code = ART_MOVETO_OPEN;
  mBPath[mBPathEnd].x3 = x;
  mBPath[mBPathEnd].y3 = y;

  ++mBPathEnd;
}

void nsSVGPath::Lineto(float x, float y)
{
  EnsureBPathSpace();

  mBPath[mBPathEnd].code = ART_LINETO;
  mBPath[mBPathEnd].x3 = x;
  mBPath[mBPathEnd].y3 = y;

  ++mBPathEnd;
}

void nsSVGPath::Curveto(float x, float y, float x1, float y1, float x2, float y2)
{
  EnsureBPathSpace();

  mBPath[mBPathEnd].code = ART_CURVETO;
  mBPath[mBPathEnd].x1 = x1;
  mBPath[mBPathEnd].y1 = y1;
  mBPath[mBPathEnd].x2 = x2;
  mBPath[mBPathEnd].y2 = y2;
  mBPath[mBPathEnd].x3 = x;
  mBPath[mBPathEnd].y3 = y;

  ++mBPathEnd;
}

void nsSVGPath::ClosePath()
{
  PRInt32 subpath = GetLastOpenBPath();
  NS_ASSERTION(subpath>=0, "no open subpath");
  if (subpath<0) return;

  // insert closing line if needed:
  if (mBPath[subpath].x3 != mBPath[mBPathEnd-1].x3 ||
      mBPath[subpath].y3 != mBPath[mBPathEnd-1].y3) {
    Lineto((float)mBPath[subpath].x3, (float)mBPath[subpath].y3);
  }

  mBPath[subpath].code = ART_MOVETO;
}

//----------------------------------------------------------------------
// helpers

void nsSVGPath::EnsureBPathSpace(PRUint32 space)
{
  const PRInt32 minGrowSize = 10;

  if (mBPathSize - mBPathEnd >= space)
    return;

  if (space < minGrowSize)
    space = minGrowSize;
  
  mBPathSize += space;
  
  if (!mBPath) {
    mBPath = art_new(ArtBpath, mBPathSize);
  }
  else {
    mBPath = art_renew(mBPath, ArtBpath, mBPathSize);
  }
}

void nsSVGPath::EnsureBPathTerminated()
{
  NS_ASSERTION (mBPathEnd>0, "trying to terminate empty bpath");
  if (mBPath[mBPathEnd-1].code == ART_END) return;

  EnsureBPathSpace(1);
  mBPath[mBPathEnd++].code = ART_END;
}

PRInt32 nsSVGPath::GetLastOpenBPath()
{
  PRInt32 i = mBPathEnd;
  while (--i >= 0) {
    if (mBPath[i].code == ART_MOVETO_OPEN)
      return i;
  }
  return -1;
}

double nsSVGPath::getBezierFlatness()
{
// comment from art_vpath_path.c: The Adobe PostScript reference
// manual defines flatness as the maximum deviation between the any
// point on the vpath approximation and the corresponding point on the
// "true" curve, and we follow this definition here. A value of 0.25
// should ensure high quality for aa rendering.

  double flatness = 0.5;
  
  nsresult rv;
  NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
	if (NS_SUCCEEDED(rv) && prefs)
	{
    // XXX: wouldn't it be great if nsIPref had a 'GetFloatPref()'-function?
		char	*valuestr = nsnull;
		if (NS_SUCCEEDED(prefs->CopyCharPref("svg.bezier_flatness",&valuestr)) && (valuestr))
		{
      flatness = PR_strtod(valuestr, nsnull);
      nsMemory::Free(valuestr);
		}
  }
  return flatness;
}

void nsSVGPath::RebuildVPathFromBPath()
{
  if (mVPath) {
    art_free(mVPath);
    mVPath = nsnull;
  }
  if (mBPathEnd>0) {
    EnsureBPathTerminated();
    mVPath = art_bez_path_to_vec(mBPath, getBezierFlatness());
  }
}

