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

#ifndef __NS_SVGSTROKE_H__
#define __NS_SVGSTROKE_H__

#include "nsSVGRenderItem.h"
#include "libart-incs.h"
#include "nsStyleConsts.h"

struct nsSVGStrokeStyle 
{
  nsString dasharray; // XXX
  float dashoffset;
  PRInt8 linecap;   // see nsStyleConsts.h
  PRInt8 linejoin;  // see nsStyleConsts.h
  float miterlimit; 
  float  width;
};

class nsSVGStroke : public nsSVGRenderItem
{
public:
  void Build(ArtVpath* path, const nsSVGStrokeStyle& style);
  
protected:
  double getFlatness();  
};

#endif // __NS_SVGSTROKE_H__
