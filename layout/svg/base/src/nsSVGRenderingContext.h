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

#include "nsCOMPtr.h"
#include "nsIRenderingContext.h"
#include "nsTransform2D.h"
#include "nsIPresContext.h"
#include "nsColor.h"
#include "nsRect.h"

#include "libart-incs.h"

class nsSVGRenderItem;

/* This is a rendering context wrapper. Grabbing one of these LOCKS the context
 * To draw to it directly (or to the underlying buffer), you need
 * to call LockMozRenderingContext, and then call UnlockMozRenderingContext
 * before drawing to it again.
 */
class nsSVGRenderingContext
{
public:
  nsSVGRenderingContext(nsIPresContext* presContext,
                        nsIRenderingContext *renderingContext,
                        const nsRect& dirtyRectTwips);
  ~nsSVGRenderingContext();

  void PaintSVGRenderItem(nsSVGRenderItem* item);
  void ClearBuffer(nscolor color);

  // return an nsIRenderingContext for our pixel buffer.
  // Calling these can be expensive.
  nsIRenderingContext* LockMozRenderingContext();
  void UnlockMozRenderingContext();

  nsIPresContext* GetPresContext() { return mPresContext; }
  const nsRect& GetDirtyRectTwips() { return mDirtyRectTwips; }
  
  void Render(); // blt our pixel buffer to the rendering context
  
protected:
  ArtRender* NewRender();
  void InvokeRender(ArtRender* render); 
  
  void InitializeBuffer();

  void DumpImage();
  
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<nsIPresContext>      mPresContext;
  nsRect mDirtyRect;
  nsRect mDirtyRectTwips;

  nsCOMPtr<nsIDrawingSurface> mBuffer;  
  nsDrawingSurface mTempBuffer; // temp storage for during DC locking 
  int mArtPixelFormat;
  PRInt32 mStride;
  PRUint8* mBitBuf;
};

#endif // __NS_SVGRENDERINGCONTEXT_H__

