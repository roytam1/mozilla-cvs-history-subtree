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

#include "nsSVGStroke.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIPref.h"
#include "prdtoa.h"
#include "nsMemory.h"

void
nsSVGStroke::Build(ArtVpath* path, const nsSVGStrokeStyle& style)
{
  if (mSvp)
    art_free(mSvp);

  ArtPathStrokeCapType captype;
  switch(style.linecap) {
    case NS_STYLE_STROKE_LINECAP_BUTT:
      captype = ART_PATH_STROKE_CAP_BUTT;
      break;
    case NS_STYLE_STROKE_LINECAP_ROUND:
      captype = ART_PATH_STROKE_CAP_ROUND;
      break;
    case NS_STYLE_STROKE_LINECAP_SQUARE:
      captype = ART_PATH_STROKE_CAP_SQUARE;
      break;
    default:
      NS_ERROR("not reached");
  }
  
  mSvp = art_svp_vpath_stroke (path,
                               ART_PATH_STROKE_JOIN_MITER,
                               captype,
                               style.width,
                               4, // miter limit
                               getFlatness());
}

double nsSVGStroke::getFlatness()
{
  double flatness = 0.5;
  
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
	if (!prefs) return flatness;

  // XXX: wouldn't it be great if nsIPref had a 'GetFloatPref()'-function?
  char	*valuestr = nsnull;
  if (NS_SUCCEEDED(prefs->CopyCharPref("svg.stroke_flatness",&valuestr)) && (valuestr)) {
    flatness = PR_strtod(valuestr, nsnull);
    nsMemory::Free(valuestr);
  }
  return flatness;
}
