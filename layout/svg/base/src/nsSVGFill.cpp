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

#include "nsSVGFill.h"
#include "nsSVGPath.h"


void
nsSVGFill::Build(nsSVGPath* path, const nsSVGFillStyle& style)
{
  if (mSvp) {
    art_free(mSvp);
    mSvp = nsnull;
  }
  
  mStyle = style;


  if (!path->IsClosed()) {
    NS_ASSERTION(1==0, "filling open paths not implemented yet!");
    // XXX create a closed path
    return;
  }

  ArtVpath* perturbedVP = art_vpath_perturb(path->GetVPath());

  ArtSVP* svp = art_svp_from_vpath(perturbedVP);
  art_free(perturbedVP);
  
  ArtSVP* uncrossedSVP = art_svp_uncross(svp);
  art_svp_free(svp);
  
  ArtWindRule wind = ART_WIND_RULE_NONZERO;
  mSvp = art_svp_rewind_uncrossed (uncrossedSVP, wind);
  art_svp_free (uncrossedSVP);
}
