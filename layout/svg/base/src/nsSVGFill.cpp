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
 *    Bradley Baetz <bbaetz@cs.mcgill.ca>
 *
 */

#include <math.h>

#include "nsSVGFill.h"

// static helper functions 

#define EPSILON 1e-6

static PRBool ContainsOpenPath(ArtVpath* src)
{
  int pos;
  for(pos=0;src[pos].code != ART_END;++pos) {
    if (src[pos].code==ART_MOVETO_OPEN)
      return PR_TRUE;
  }
  return PR_FALSE;
}

// Closes an open path, returning a new one
static ArtVpath* CloseOpenPath(ArtVpath* src)
{
  // We need to insert something extra for each open subpath
  // So we realloc stuff as needed
  int currSize = 0;
  int maxSize = 4;
  ArtVpath *ret = art_new(ArtVpath, maxSize);

  PRBool isOpenSubpath = PR_FALSE;
  double startX=0;
  double startY=0;

  int srcPos;
  for (srcPos=0;src[srcPos].code != ART_END;++srcPos) {
    if (currSize==maxSize)
      art_expand(ret, ArtVpath, maxSize);
    if (src[srcPos].code == ART_MOVETO_OPEN)
      ret[currSize].code = ART_MOVETO; // close it
    else
      ret[currSize].code = src[srcPos].code;
    ret[currSize].x = src[srcPos].x;
    ret[currSize].y = src[srcPos].y;
    ++currSize;

    // OK, it was open
    if (src[srcPos].code == ART_MOVETO_OPEN) {
      startX = src[srcPos].x;
      startY = src[srcPos].y;
      isOpenSubpath = PR_TRUE;
    } else if (src[srcPos+1].code != ART_LINETO) {
      if (isOpenSubpath &&
          ((fabs(startX - src[srcPos].x) > EPSILON) ||
           (fabs(startY - src[srcPos].y) > EPSILON))) {
        // The next one isn't a line, so lets close this
        if (currSize == maxSize)
          art_expand(ret, ArtVpath, maxSize);
        ret[currSize].code = ART_LINETO;
        ret[currSize].x = startX;
        ret[currSize].y = startY;
        ++currSize;
      }
      isOpenSubpath = PR_FALSE;
    }
  }
  if (currSize == maxSize)
    art_expand(ret, ArtVpath, maxSize);
  ret[currSize].code = ART_END;
  ret[currSize].x = 0;
  ret[currSize].y = 0;
  return ret;
}

// nsSVGFill members
  
void
nsSVGFill::Build(ArtVpath* path, const nsSVGFillStyle& style)
{
  if (mSvp) {
    art_svp_free(mSvp);
    mSvp = nsnull;
  }
  
  PRBool hasOpen = ContainsOpenPath(path);
  if (hasOpen)
    path = CloseOpenPath(path);
  
  ArtVpath* perturbedVP = art_vpath_perturb(path);

  if (hasOpen)
    art_free(path);
  
  ArtSVP* svp = art_svp_from_vpath(perturbedVP);
  art_free(perturbedVP);
  
  ArtSVP* uncrossedSVP = art_svp_uncross(svp);
  art_svp_free(svp);
  
  ArtWindRule wind;
  switch (style.fillrule) {
    case NS_STYLE_FILL_RULE_NONZERO:
      wind = ART_WIND_RULE_NONZERO;
      break;
    case NS_STYLE_FILL_RULE_EVENODD:
      wind = ART_WIND_RULE_ODDEVEN;
      break;
    default:
      NS_ERROR("not reached");
  }
  
  mSvp = art_svp_rewind_uncrossed (uncrossedSVP, wind);
  art_svp_free (uncrossedSVP);
}

