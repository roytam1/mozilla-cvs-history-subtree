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

#ifndef __NS_SVGRENDERINGCONTEXT_H__
#define __NS_SVGRENDERINGCONTEXT_H__


// SVG_USE_SURFACE: this macro needs to be defined for
// <foreignObject>s to work. They need to be able to access the pixel
// buffer through device context calls, which cannot easily be
// accomplished when our pixel buffer is wrapped by an nsImage (which
// is the alternative mechanism to surfaces). The disadvantage of
// using surfaces is that there isn't (yet) a x-platform mechanism for
// getting a surface with a preset color-format. Thus - when setting
// this macro - on some platforms SVG might not render for display
// settings other than 24bit...  

// Drawing to nsIImages is an ugly hack, but at least it should work
// on every platform. In particular for XServer-based platforms
// (Linux!), the 24bit surface approach can't easily be made to work
// (at least not efficiently). We'll have to live without
// <foreignObject>s on Linux then until we think of something clever
// (XRENDER?)

#if defined(XP_PC) || defined(XP_MAC)
#define SVG_USE_SURFACE
#endif


#include "nsCOMPtr.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIPresContext.h"
#include "nsColor.h"
#include "nsRect.h"

#include "libart-incs.h"

#ifndef SVG_USE_SURFACE
#include "nsIImage.h"
#endif

class nsSVGRenderItem;


class nsSVGRenderingContext
{
public:
  nsSVGRenderingContext(nsIPresContext* presContext,
                        nsIRenderingContext *renderingContext,
                        const nsRect& dirtyRectTwips);
  ~nsSVGRenderingContext();

  void PaintSVGRenderItem(nsSVGRenderItem* item);
  void ClearBuffer(nscolor color);

  // return an nsIRenderingContext for our pixel buffer.  XXX this
  // function currently fails on platforms that don't use SVG_USE_SURFACE.
  nsIRenderingContext* LockMozRenderingContext();
  void UnlockMozRenderingContext();

  nsIPresContext* GetPresContext() { return mPresContext; }
  const nsRect& GetDirtyRectTwips() { return mDirtyRectTwips; }
  
  void Render(); // blt our pixel buffer to the rendering context
  
  void DumpImage();
  
protected:
  ArtRender* NewRender();
  void InvokeRender(ArtRender* render); 
  
  void InitializeBuffer();
  
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<nsIPresContext>      mPresContext;
  nsRect mDirtyRect;
  nsRect mDirtyRectTwips;

#ifdef SVG_USE_SURFACE
  nsCOMPtr<nsIDrawingSurface> mBuffer;  
  nsDrawingSurface mTempBuffer; // temp storage for during DC locking 
#else
  nsCOMPtr<nsIImage> mBuffer;
#endif

#ifdef SVG_LIBART_PIXEL_FORMAT
  int mArtPixelFormat;
#endif
};

#endif // __NS_SVGRENDERINGCONTEXT_H__
