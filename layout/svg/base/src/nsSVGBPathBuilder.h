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

#ifndef __NS_SVGBPATHBUILDER_H__
#define __NS_SVGBPATHBUILDER_H__

#include "nscore.h"
#include "libart-incs.h"
#include "nsASVGPathBuilder.h"

class nsSVGBPathBuilder : public nsASVGPathBuilder
{
public:
  nsSVGBPathBuilder();

  PRBool IsEmpty();
  ArtBpath* GetBPath();
  
  // nsASVGPathBuilder methods:
  virtual void Moveto(float x, float y);
  virtual void Lineto(float x, float y);
  virtual void Curveto(float x, float y, float x1, float y1, float x2, float y2);
  virtual void Arcto(float x, float y, float r1, float r2, float angle,
                     PRBool largeArcFlag, PRBool sweepFlag);
  virtual void ClosePath(float *newX, float *newY);
    
protected:

  // helpers
  void EnsureBPathSpace(PRUint32 space=1);
  void EnsureBPathTerminated();
  PRInt32 GetLastOpenBPath();
  double CalcVectorAngle(double ux, double uy, double vx, double vy);
  
  ArtBpath* mBPath;
  PRUint32  mBPathSize;
  PRUint32  mBPathEnd; // one-past-the-end
  
};
#endif // __NS_SVGBPATHBUILDER_H__
