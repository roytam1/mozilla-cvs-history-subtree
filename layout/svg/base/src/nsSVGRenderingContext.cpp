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

#include "nsSVGRenderingContext.h"
#include "nsSVGRenderItem.h"
#include "nsIDeviceContext.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"

#include "libart-incs.h"

#ifndef SVG_USE_SURFACE
static NS_DEFINE_IID(kCImage, NS_IMAGE_CID);
#endif


////////////////////////////////////////////////////////////////////////
// nsSVGRenderingContext

nsSVGRenderingContext::nsSVGRenderingContext(nsIPresContext* presContext,
                                             nsIRenderingContext *renderingContext,
                                             const nsRect& dirtyRectTwips)
    : mRenderingContext(renderingContext),
      mPresContext(presContext),
#ifdef SVG_USE_SURFACE
      mTempBuffer(nsnull),
#endif
      mDirtyRect(dirtyRectTwips),
      mDirtyRectTwips(dirtyRectTwips)
{
  float twipsPerPx;
  mPresContext->GetPixelsToTwips(&twipsPerPx);
  NS_ASSERTION(twipsPerPx!=0.0f, "invalid twips/px");
  mDirtyRect *= 1.0f/twipsPerPx;
}

nsSVGRenderingContext::~nsSVGRenderingContext()
{

}

//----------------------------------------------------------------------

void
nsSVGRenderingContext::PaintSVGRenderItem(nsSVGRenderItem* item)
{
  if (!item->GetSvp()) return;
  
  ArtRender* render = NewRender();
  if (!render) return;
  
  art_render_mask_solid(render, (int)(0x10000 * item->GetOpacity()));

  art_render_svp(render, item->GetSvp());

  nscolor rgb = item->GetColor();
  ArtPixMaxDepth col[3];
#ifdef XP_PC
  col[0] = ART_PIX_MAX_FROM_8(NS_GET_B(rgb));
  col[1] = ART_PIX_MAX_FROM_8(NS_GET_G(rgb));
  col[2] = ART_PIX_MAX_FROM_8(NS_GET_R(rgb));
#else
  col[0] = ART_PIX_MAX_FROM_8(NS_GET_R(rgb));
  col[1] = ART_PIX_MAX_FROM_8(NS_GET_G(rgb));
  col[2] = ART_PIX_MAX_FROM_8(NS_GET_B(rgb));
#endif
  
  art_render_image_solid(render, col);
  
  InvokeRender(render);
}

void nsSVGRenderingContext::ClearBuffer(nscolor color)
{

#ifdef SVG_USE_SURFACE
  nsIRenderingContext* ctx = LockMozRenderingContext();
  ctx->PushState();
  ctx->SetColor(color);
  ctx->FillRect(mDirtyRectTwips);
  PRBool aClipEmpty;
  ctx->PopState(aClipEmpty); 
  UnlockMozRenderingContext();
#else
  InitializeBuffer();

  PRUint8 red   = NS_GET_R(color);
  PRUint8 green = NS_GET_G(color);
  PRUint8 blue  = NS_GET_B(color);  

  mBuffer->LockImagePixels(PR_FALSE);
  
  PRInt32 stride = mBuffer->GetLineStride();
  PRUint8* buf = mBuffer->GetBits();
  PRUint8* end = buf + stride*mDirtyRect.height;
  for ( ; buf<end ; buf += stride) {
    art_rgb_fill_run(buf, red, green, blue, mDirtyRect.width);
  }

  mBuffer->UnlockImagePixels(PR_FALSE);  

#endif
}

nsIRenderingContext* nsSVGRenderingContext::LockMozRenderingContext()
{
#ifdef SVG_USE_SURFACE
  InitializeBuffer();
  NS_ASSERTION(mTempBuffer==0, "nested LockMozRenderingContext() calls?");
  mRenderingContext->GetDrawingSurface(&mTempBuffer);
  mRenderingContext->SelectOffScreenDrawingSurface(mBuffer);

  // prepare the context for the new surface:
  mRenderingContext->PushState();
  nsTransform2D* xform;
  mRenderingContext->GetCurrentTransform(xform);

  xform->SetTranslation((float)-mDirtyRect.x, (float)-mDirtyRect.y);

  // setting the clip rectangle is essential here. If we don't, and
  // none of the saved parent states has a valid clip rect, then bad
  // things can happen if our caller (usually a <foreignObject>) sets
  // up a clip rect. In that case, the rendering context (at least on
  // Windows) doesn't know which clip area to restore when the state
  // is popped and we're stuck with the caller's clip rect for all
  // eternity. The result is erratic redraw-behaviour...
  PRBool aClipEmpty;
  mRenderingContext->SetClipRect(mDirtyRectTwips, nsClipCombine_kReplace, aClipEmpty);
  
  return mRenderingContext;
#else
  NS_ERROR("Can't select correct target into rendering context. \n"
           "Set 'SVG_USE_SURFACE' in layout/svg/base/src/nsSVGRenderingContext.h.");
  return mRenderingContext;
#endif
}

void nsSVGRenderingContext::UnlockMozRenderingContext()
{
#ifdef SVG_USE_SURFACE
  NS_ASSERTION(mTempBuffer, "no drawing surface to restore");
  PRBool aClipEmpty;
  mRenderingContext->PopState(aClipEmpty);
  mRenderingContext->SelectOffScreenDrawingSurface(mTempBuffer);
  mTempBuffer = nsnull;
#endif
}

void nsSVGRenderingContext::Render()
{
  if (!mBuffer) return;

#ifdef SVG_USE_SURFACE
  mRenderingContext->CopyOffScreenBits(mBuffer, 0, 0, mDirtyRectTwips,
                                       NS_COPYBITS_TO_BACK_BUFFER |
                                       NS_COPYBITS_XFORM_DEST_VALUES);
#else
  if (!mBuffer->GetIsRowOrderTopToBottom()) { // need to flip image
    // XXX I know this is silly. Blt should take care of it.
    int stride = mBuffer->GetLineStride();
    PRUint8* bits   = mBuffer->GetBits();
    PRUint8* rowbuf = new PRUint8[stride];
    for (int row=0; row < mDirtyRect.height/2; ++row) {
      memcpy(rowbuf, bits+row*stride, stride);
      memcpy(bits+row*stride, bits+(mDirtyRect.height-1-row)*stride,
             stride);
      memcpy(bits+(mDirtyRect.height-1-row)*stride, rowbuf, stride);
    }
    delete[] rowbuf;
  }

  mBuffer->SetDecodedRect(0,0,mDirtyRect.width, mDirtyRect.height);
  mBuffer->SetNaturalWidth(mDirtyRect.width);
  mBuffer->SetNaturalHeight(mDirtyRect.height);

  // and tell the image that we've changed it...
  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));
  nsRect r(0,0,mBuffer->GetWidth(),mBuffer->GetHeight());
  mBuffer->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged,&r);

  mRenderingContext->DrawImage(mBuffer, mDirtyRectTwips);
#endif
}

//----------------------------------------------------------------------
// implementation helpers:


ArtRender* nsSVGRenderingContext::NewRender()
{
  InitializeBuffer();

  ArtRender* render=nsnull;
  
#ifdef SVG_USE_SURFACE
  PRInt32 stride;
  PRUint8* buf;  
  PRInt32 bytesPerWidth;

  mBuffer->Lock(0, 0, mDirtyRect.width, mDirtyRect.height,(void **)&buf,
                &stride, &bytesPerWidth,0);

  nsPixelFormat format;
  mBuffer->GetPixelFormat(&format);
  PRInt32 bbp = format.mRedCount + format.mGreenCount + format.mBlueCount + format.mAlphaCount;

#ifndef XP_MAC	// Mac doesn't implement this yet, <sigh>
  NS_ASSERTION(bbp == 24,
               "The SVG rendering backend currenly only supports 24bit color depths...\n"
               "Try compiling nsSVGRenderingContext.cpp with SVG_USE_SURFACE unset!");

  if (bbp != 24) return nsnull;
#endif
    
  render = art_render_new(mDirtyRect.x, mDirtyRect.y,
                          mDirtyRect.x+ mDirtyRect.width,
                          mDirtyRect.y+ mDirtyRect.height,
                          buf,
                          stride,
                    #ifdef XP_MAC
                    	1,		// pixel padding parameter
                    #endif	    
                          3, // channels 
                          8, // bits per channel
                          ART_ALPHA_NONE, // alpha
                          NULL);
#else
  mBuffer->LockImagePixels(PR_FALSE);

  render = art_render_new(mDirtyRect.x, mDirtyRect.y,
                          mDirtyRect.x+ mDirtyRect.width,
                          mDirtyRect.y+ mDirtyRect.height,
                          mBuffer->GetBits(), mBuffer->GetLineStride(),
                          3, 8, ART_ALPHA_NONE, NULL);
#endif

  return render;
}

void nsSVGRenderingContext::InvokeRender(ArtRender* render)
{
  art_render_invoke(render); // also frees the render

#ifdef SVG_USE_SURFACE
  mBuffer->Unlock();
#else
  mBuffer->UnlockImagePixels(PR_FALSE);
#endif
}


void nsSVGRenderingContext::InitializeBuffer()
{
  if (mBuffer) return;

#ifdef SVG_USE_SURFACE
   mRenderingContext->CreateDrawingSurface(&mDirtyRect,
                                           NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS |
                                           NS_CREATEDRAWINGSURFACE_24BIT,
                                           (void*&)*getter_AddRefs(mBuffer));
#else
  mBuffer = do_CreateInstance(kCImage);
  mBuffer->Init(mDirtyRect.width, mDirtyRect.height, 24,
                nsMaskRequirements_kNoMask);
#endif
}




