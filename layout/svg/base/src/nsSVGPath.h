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

#ifndef __NS_SVGPATH_H__
#define __NS_SVGPATH_H__

#include "nscore.h"
#include "libart-incs.h"

class nsIDOMSVGPointList;
class nsIDOMSVGMatrix;

class nsSVGPath
{
public:
  nsSVGPath();
  ~nsSVGPath();

  // XXX this needs an api change
  void Build(nsIDOMSVGPointList* points,
             nsIDOMSVGMatrix* ctm,
             PRBool bClose=PR_FALSE);

  void SetCircle(float cx, float cy, float r);

  void Clear();
  void Moveto(float x, float y);
  void Lineto(float x, float y);
  void Curveto(float x, float y, float x1, float y1, float x2, float y2);
  void ClosePath();
  
  
  PRBool IsClosed() { return PR_TRUE; }
  PRBool IsEmpty();
  ArtVpath* GetVPath();
  
protected:
  // helpers
  void EnsureBPathSpace(PRUint32 space=1);
  void EnsureBPathTerminated();
  PRInt32 GetLastOpenBPath();
  void RebuildVPathFromBPath();
  double getBezierFlatness();
  
  ArtVpath* mVPath;
  
  // XXX there is no reason for caching the ArtBpath. It's just done
  // for convenience, so that we can use art_bez_path_to_vec(). We
  // should really build everything directly to a vpath.
  ArtBpath* mBPath;
  PRUint32  mBPathSize;
  PRUint32  mBPathEnd; // one-past-the-end
  
  PRBool   mIsClosed;
};
#endif // __NS_SVGPATH_H__
