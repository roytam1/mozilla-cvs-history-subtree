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

#include "nsSVGGraphic.h"
#include "nsSVGRenderingContext.h"
#include "nsSVGBPathBuilder.h"
#include "nsIDOMSVGMatrix.h"
#include "nsCOMPtr.h"
#include "nsASVGGraphicSource.h"
#include "nsStyleStruct.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "prdtoa.h"

nsSVGGraphic::nsSVGGraphic()
    : mVPath(nsnull), mExpansion(1.0)
{
  
}

nsSVGGraphic::~nsSVGGraphic()
{
  if (mVPath != nsnull)
    art_free(mVPath);
}

void
nsSVGGraphic::Paint(nsSVGRenderingContext* ctx)
{
  if (!mFill.IsEmpty()) {
    ctx->PaintSVGRenderItem(&mFill);
  }
  
  if (!mStroke.IsEmpty())
    ctx->PaintSVGRenderItem(&mStroke);
}

PRBool
nsSVGGraphic::IsMouseHit(float x, float y)
{
  return (mFill.Contains(x,y) || mStroke.Contains(x,y));
}

nsArtUtaRef
nsSVGGraphic::Update(nsSVGGraphicUpdateFlags flags, nsASVGGraphicSource* source)
{
  nsArtUtaRef dirtyRegion(NULL);

  if ((flags & NS_SVGGRAPHIC_UPDATE_FLAGS_PATHCHANGE) != 0 ||
      (flags & NS_SVGGRAPHIC_UPDATE_FLAGS_CTMCHANGE ) != 0 ||
      !mVPath ) {
    // 1. get a Bezier path from the graphic source:
    ArtBpath* bpath;
    {
      nsSVGBPathBuilder BPathBuilder;
      source->ConstructPath(&BPathBuilder);
      bpath = BPathBuilder.GetBPath();
    }
    
    // 2. transform the bpath into global coords:
    {
      nsCOMPtr<nsIDOMSVGMatrix> ctm;
      source->GetCTM(getter_AddRefs(ctm));
      NS_ASSERTION(ctm, "graphic source didn't have a ctm");

      double matrix[6];
      float val;
      ctm->GetA(&val);
      matrix[0] = val;

      ctm->GetB(&val);
      matrix[1] = val;
 
      ctm->GetC(&val);  
      matrix[2] = val;  
      
      ctm->GetD(&val);  
      matrix[3] = val;  
        
      ctm->GetE(&val);
      matrix[4] = val;

      ctm->GetF(&val);
      matrix[5] = val;
      
      // cache the expansion factor. we use it to scale the stroke
      // width:
      mExpansion = sqrt((float)fabs(matrix[0]*matrix[3]-matrix[2]*matrix[1]));
      
      if ( bpath &&
           ( matrix[0] != 1.0 || matrix[2] != 0.0 || matrix[4] != 0.0 ||
             matrix[1] != 0.0 || matrix[3] != 1.0 || matrix[5] != 0.0 ))
      {
        ArtBpath* temp = bpath;
        bpath = art_bpath_affine_transform(bpath, matrix);
        art_free(temp);
      }
    }
    
    // 3. convert the bpath into a vpath and cache it:
    if (mVPath != nsnull) {
      art_free(mVPath);
      mVPath = nsnull;
    }
    if (bpath != nsnull)
      mVPath = art_bez_path_to_vec(bpath, GetBezierFlatness());
    
    art_free(bpath);
  }

  // update stroke and fill:

  if (!mVPath) {
    dirtyRegion = Combine(dirtyRegion, mStroke.GetUta());
    dirtyRegion = Combine(dirtyRegion, mFill.GetUta());
    mStroke.Clear();
    mFill.Clear();
    return dirtyRegion;
  }
  
  const nsStyleSVG* svgStyle = source->GetStyle();
    
  if (!mStroke.IsEmpty()) {
    dirtyRegion = Combine(dirtyRegion, mStroke.GetUta());
  }

  if (svgStyle->mStroke.mType == eStyleSVGPaintType_Color &&
      svgStyle->mStrokeWidth > 0 &&
      mExpansion > 0) {
    nsSVGStrokeStyle strokeStyle;
    strokeStyle.dasharray = svgStyle->mStrokeDasharray; 
    strokeStyle.dashoffset = svgStyle->mStrokeDashoffset;
    strokeStyle.linecap = svgStyle->mStrokeLinecap;
    strokeStyle.linejoin = svgStyle->mStrokeLinejoin;
    strokeStyle.miterlimit = svgStyle->mStrokeMiterlimit;
    strokeStyle.width = svgStyle->mStrokeWidth * mExpansion;
    
    mStroke.SetColor(svgStyle->mStroke.mColor);
    mStroke.SetOpacity(svgStyle->mStrokeOpacity);
    
    mStroke.Build(mVPath, strokeStyle);
    dirtyRegion = Combine(dirtyRegion, mStroke.GetUta());
  }
  else {
    mStroke.Clear();
  }

  if (!mFill.IsEmpty()) {
    dirtyRegion = Combine(dirtyRegion, mFill.GetUta());
  }

  if (svgStyle->mFill.mType == eStyleSVGPaintType_Color) {
    nsSVGFillStyle fillStyle;
    fillStyle.fillrule = svgStyle->mFillRule;
    
    mFill.SetColor(svgStyle->mFill.mColor);
    mFill.SetOpacity(svgStyle->mFillOpacity);
    
    mFill.Build(mVPath, fillStyle);
    dirtyRegion = Combine(dirtyRegion, mFill.GetUta());
  }
  else {
    mFill.Clear();
  }

  return dirtyRegion;
}

nsArtUtaRef
nsSVGGraphic::GetUta()
{
 return Combine(mFill.GetUta(), mStroke.GetUta());
}



// helpers

nsArtUtaRef nsSVGGraphic::Combine(nsArtUtaRef a, nsArtUtaRef b)
{
  if (!b) return a;
  if (!a) return b;

  // they're both valid
 return nsArtUtaRef(art_uta_union(a.get(), b.get()));
}

double nsSVGGraphic::GetBezierFlatness()
{
// comment from art_vpath_path.c: The Adobe PostScript reference
// manual defines flatness as the maximum deviation between the any
// point on the vpath approximation and the corresponding point on the
// "true" curve, and we follow this definition here. A value of 0.25
// should ensure high quality for aa rendering.

  double flatness = 0.5;
  
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
	if (!prefs) return flatness;

  // XXX: wouldn't it be great if nsIPref had a 'GetFloatPref()'-function?
  char	*valuestr = nsnull;
  if (NS_SUCCEEDED(prefs->CopyCharPref("svg.bezier_flatness",&valuestr)) && (valuestr)) {
    flatness = PR_strtod(valuestr, nsnull);
    nsMemory::Free(valuestr);
  }

  return flatness;
}
 
 
