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

#include "nsSVGRenderItem.h"

nsSVGRenderItem::nsSVGRenderItem()
    : mSvp(nsnull),
 mUta(nsnull)
{
  
}

nsSVGRenderItem::~nsSVGRenderItem()
{
  Clear();
}

void nsSVGRenderItem::Clear()
{
  if (mSvp)
    art_svp_free(mSvp);
  mSvp = nsnull;
}  

void nsSVGRenderItem::SetSVP(ArtSVP *svp)
{
 Clear();
 mSvp = svp;
 mUta = nsArtUtaRef(art_uta_from_svp(mSvp));
}

nsArtUtaRef
nsSVGRenderItem::GetUta()
{
  return mUta;
}

PRBool
nsSVGRenderItem::Contains(float x, float y)
{
  if (!mSvp) return PR_FALSE;
  return (art_svp_point_wind(GetSvp(), x, y) != 0);
}
