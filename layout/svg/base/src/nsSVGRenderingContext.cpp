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

#include "nsSVGRenderingContext.h"
#include "nsSVGRenderItem.h"
#include "nsIDeviceContext.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"

#include "libart-incs.h"

#ifndef SVG_USE_SURFACE
static NS_DEFINE_IID(kCImage, NS_IMAGE_CID);
#endif

//#define DUMP_SVG_IMAGE

#ifdef DUMP_SVG_IMAGE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////
// nsSVGRenderingContext

nsSVGRenderingContext::nsSVGRenderingContext(nsIPresContext* presContext,
                                             nsIRenderingContext *renderingContext,
                                             const nsRect& dirtyRectTwips)
  : mRenderingContext(renderingContext),
    mPresContext(presContext),
    mDirtyRect(dirtyRectTwips),
    mDirtyRectTwips(dirtyRectTwips)
#ifdef SVG_USE_SURFACE
    , mTempBuffer(nsnull)
#endif
{
//  printf("------->rect=%d %d %d %d\n",mDirtyRect.x,
//         mDirtyRect.y, mDirtyRect.width, mDirtyRect.height);
  
  float twipsPerPx;
  mPresContext->GetPixelsToTwips(&twipsPerPx);
  NS_ASSERTION(twipsPerPx!=0.0f, "invalid twips/px");
  mDirtyRect *= 1.0f/twipsPerPx;

  
  
//  printf("------->rect=%d %d %d %d\n",mDirtyRect.x,
//         mDirtyRect.y, mDirtyRect.width, mDirtyRect.height);

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
  
  printf("Rendering color R=%d, G=%d, B=%d, a=%f\n",
         NS_GET_R(rgb),
         NS_GET_G(rgb),
         NS_GET_B(rgb),
         item->GetOpacity());
  
  art_render_image_solid(render, col);
  
  InvokeRender(render);
}

void nsSVGRenderingContext::ClearBuffer(nscolor color)
{

#ifndef SVG_USE_SURFACE
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

  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));
  nsRect r(0,0,mBuffer->GetWidth(),mBuffer->GetHeight());
  mBuffer->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged,&r);
#else
  nsIRenderingContext* ctx = LockMozRenderingContext();
  ctx->PushState();
  ctx->SetColor(color);
  ctx->FillRect(mDirtyRectTwips);
//  ctx->SetColor(NS_RGB(100,100,100));
//  ctx->DrawRect(mDirtyRectTwips);
  PRBool aClipEmpty;
  ctx->PopState(aClipEmpty); 
  UnlockMozRenderingContext();
#endif
}

nsIRenderingContext* nsSVGRenderingContext::LockMozRenderingContext()
{
#ifndef SVG_USE_SURFACE
  NS_ASSERTION(0, "Can't select correct target into rendering context. \n"
               "Set 'SVG_USE_SURFACE' in layout/svg/base/src/nsSVGRenderingContext.h.");
  return mRenderingContext;
#else
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
#endif
}

void nsSVGRenderingContext::UnlockMozRenderingContext()
{
#ifndef SVG_USE_SURFACE
/* */
#else
  NS_ASSERTION(mTempBuffer, "no drawing surface to restore");
  PRBool aClipEmpty;
  mRenderingContext->PopState(aClipEmpty);
  mRenderingContext->SelectOffScreenDrawingSurface(mTempBuffer);
  mTempBuffer = 0;
#endif
}

void nsSVGRenderingContext::Render()
{
  if (!mBuffer) return;

#ifndef SVG_USE_SURFACE
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

  mRenderingContext->DrawImage(mBuffer, mDirtyRectTwips);
  
#else
  mRenderingContext->CopyOffScreenBits(mBuffer, 0, 0, mDirtyRectTwips,
                                       NS_COPYBITS_TO_BACK_BUFFER |
                                       NS_COPYBITS_XFORM_DEST_VALUES);
#endif
}

//----------------------------------------------------------------------
// implementation helpers:

void nsSVGRenderingContext::DumpImage() {
#ifdef DUMP_SVG_IMAGE
  static int numOut=0;
  char buf[30];
  sprintf(buf,"svg%d.ppm",numOut);
  ++numOut;

  printf("dumping %s\n",buf);

  FILE* f = fopen(buf, "wb");
  
  PRUint32 width = mDirtyRect.width;
  PRUint32 height = mDirtyRect.height;

  fprintf(f,"P6\n%d %d\n255\n",width,height);

  int stride = mBuffer->GetLineStride();
  PRUint8* bits = mBuffer->GetBits();
  for(int row=0; row < mDirtyRect.height;++row) {
    fwrite(bits+row*stride,3,width,f);
  }

  fclose(f);
#endif
}

ArtRender* nsSVGRenderingContext::NewRender()
{
  InitializeBuffer();

  ArtRender* render=0;
  
#ifndef SVG_USE_SURFACE
  mBuffer->LockImagePixels(PR_FALSE);

  render = art_render_new(mDirtyRect.x, mDirtyRect.y,
                          mDirtyRect.x+ mDirtyRect.width,
                          mDirtyRect.y+ mDirtyRect.height,
                          mBuffer->GetBits(), mBuffer->GetLineStride(),
                          3, 8, ART_ALPHA_NONE, NULL);
#else
  PRInt32 stride;
  PRUint8* buf;  
  PRInt32 bytesPerWidth;

  mBuffer->Lock(0, 0, mDirtyRect.width, mDirtyRect.height,(void **)&buf,
                &stride, &bytesPerWidth,0);

  nsPixelFormat format;
  mBuffer->GetPixelFormat(&format);
  PRInt32 bbp = format.mRedCount + format.mGreenCount + format.mBlueCount + format.mAlphaCount;

  NS_ASSERTION(bbp == 24,
               "The SVG rendering backend currenly only supports 24bit color depths...");
  if (bbp != 24) return 0;
    
  render = art_render_new(mDirtyRect.x, mDirtyRect.y,
                          mDirtyRect.x+ mDirtyRect.width,
                          mDirtyRect.y+ mDirtyRect.height,
                          buf,
                          stride, 
                          3, // channels 
                          8, // bits per channel
                          ART_ALPHA_NONE, // alpha
                          NULL);
#endif

  return render;
}

void nsSVGRenderingContext::InvokeRender(ArtRender* render)
{
  art_render_invoke(render); // also frees the render

#ifndef SVG_USE_SURFACE
  mBuffer->UnlockImagePixels(PR_FALSE);

  // and tell the image that we've changed it...
  // XXXXX - we could do better than this, and use UTAs
  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));
  nsRect r(0,0,mBuffer->GetWidth(),mBuffer->GetHeight());
  mBuffer->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged,&r);
  #ifdef DUMP_SVG_IMAGE
    DumpImage();
  #endif
#else
  mBuffer->Unlock();
#endif
}


void nsSVGRenderingContext::InitializeBuffer()
{
  if (mBuffer) return;

#ifndef SVG_USE_SURFACE
  mBuffer = do_CreateInstance(kCImage);
  mBuffer->Init(mDirtyRect.width, mDirtyRect.height, 24,
                nsMaskRequirements_kNoMask);
#else
   mRenderingContext->CreateDrawingSurface(&mDirtyRect,
                                           NS_CREATEDRAWINGSURFACE_FOR_PIXEL_ACCESS |
                                           NS_CREATEDRAWINGSURFACE_24BIT,
                                           (void*&)*getter_AddRefs(mBuffer));
#endif
}




//----------------------------------------------------------------------
// Test stuff

ArtVpath * testpath(int n)
{
  ArtVpath *vec;
  int i;
  double r, th;

  vec = art_new (ArtVpath, n + 2);
  for (i = 0; i < n; i++)
    {
      vec[i].code = i ? ART_LINETO : ART_MOVETO;
      r = rand () * (250.0 / RAND_MAX);
#if 0
      r = r + 0.9 * (250 - r);
#endif
      th = i * 2 * M_PI / n;
      vec[i].x = 250 + r * cos (th);
      vec[i].y = 250 - r * sin (th);
    }
  vec[i].code = ART_LINETO;
  vec[i].x = vec[0].x;
  vec[i].y = vec[0].y;
  i++;
  vec[i].code = ART_END;
  vec[i].x = 0;
  vec[i].y = 0;
  return vec;
}


// XXX
ArtSVP* svp=0;  
ArtSVP* svp2=0;



void nsSVGRenderingContext::Test()
{
  if (!svp) {
    ArtVpath* vpath = testpath(30);
    svp = art_svp_vpath_stroke (vpath,
                                ART_PATH_STROKE_JOIN_ROUND,
                                ART_PATH_STROKE_CAP_BUTT,
                                15,
                                4,
                                0.5);
    
    art_free(vpath);

    vpath = testpath(30);
    svp2 = art_svp_vpath_stroke (vpath,
                                ART_PATH_STROKE_JOIN_MITER,
                                ART_PATH_STROKE_CAP_BUTT,
                                20,
                                4,
                                0.5);
    
    art_free(vpath);

  }

  InitializeBuffer();
  
  
  
//  mContext->GetDrawingSurface((void**)&surface);

//  PRInt32 stride;
//  PRInt32 bytesPerWidth;
//  surface->Lock(0,0,bounds.width,bounds.height,(void **)&buf,
//                &stride, &bytesPerWidth,0);

//  nsPixelFormat format;
//  surface->GetPixelFormat(&format);
//  printf("r:%d g:%d b:%d a:%d\n", (int)format.mRedCount,
//         (int)format.mGreenCount, (int)format.mBlueCount, (int)format.mAlphaCount);

  ClearBuffer(NS_RGB(0,255,0));
  
  ArtRender* render;
  render = NewRender();
    
//  art_render_clear_rgb(render, 0xFFFFFF);

  art_render_svp(render,svp);

  art_render_mask_solid(render, 0x10000);
  
  ArtPixMaxDepth red[3] = {0xFFFF, 0x0000, 0x0000 };
  art_render_image_solid(render,red);  
  InvokeRender(render);
  
  render = NewRender();
    
  art_render_mask_solid(render, 0x08000);

  art_render_svp(render,svp2);
  
  ArtPixMaxDepth blue[3] = {0x0000, 0x0000, 0xFFFF };
  art_render_image_solid(render,blue);  
  InvokeRender(render);
}
