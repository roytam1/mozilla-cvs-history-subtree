/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Alex Fritze.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */


#include "nsCOMPtr.h"
#include "nsSVGLibartCanvas.h"
#include "nsISVGLibartCanvas.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsTransform2D.h"
#include "nsIPresContext.h"
#include "nsRect.h"
#include "libart-incs.h"
#include "nsIImage.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"

static NS_DEFINE_IID(kCImage, NS_IMAGE_CID);

////////////////////////////////////////////////////////////////////////
// nsSVGLibartCanvas class

class nsSVGLibartCanvas : public nsISVGLibartCanvas
{
public:
  nsSVGLibartCanvas();
  ~nsSVGLibartCanvas();
  nsresult Init(nsIRenderingContext* ctx, nsIPresContext* presContext,
                const nsRect & dirtyRect);

  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRendererCanvas interface:
  NS_DECL_NSISVGRENDERERCANVAS

  // nsISVGLibartCanvas interface:
  NS_IMETHOD_(ArtRender*) NewRender();
  NS_IMETHOD_(void) InvokeRender(ArtRender* render);  
  
private:
  //helpers:
  void InitBuffer();
  
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<nsIPresContext> mPresContext;
  nsCOMPtr<nsIImage> mBuffer;
  nsRect mDirtyRect;
  nsRect mDirtyRectTwips;

};

//----------------------------------------------------------------------
// implementation:

nsSVGLibartCanvas::nsSVGLibartCanvas()
{
  NS_INIT_ISUPPORTS();
}

nsSVGLibartCanvas::~nsSVGLibartCanvas()
{
  Flush();
}

nsresult
nsSVGLibartCanvas::Init(nsIRenderingContext* ctx,
                        nsIPresContext* presContext,
                        const nsRect & dirtyRect)
{
  mPresContext = presContext;
  NS_ASSERTION(mPresContext, "empty prescontext");
  mRenderingContext = ctx;
  NS_ASSERTION(mRenderingContext, "empty rendering context");

  mDirtyRect = dirtyRect;

  float twipsPerPx;
  mPresContext->GetPixelsToTwips(&twipsPerPx);
  mDirtyRectTwips.x = (nscoord)(dirtyRect.x*twipsPerPx);
  mDirtyRectTwips.y = (nscoord)(dirtyRect.y*twipsPerPx);
  mDirtyRectTwips.width = (nscoord)(dirtyRect.width*twipsPerPx);
  mDirtyRectTwips.height = (nscoord)(dirtyRect.height*twipsPerPx);
  
  return NS_OK;
}

nsresult
NS_NewSVGLibartCanvas(nsISVGRendererCanvas **result,
                      nsIRenderingContext *ctx,
                      nsIPresContext *presContext,
                      const nsRect & dirtyRect)
{
  nsSVGLibartCanvas* pg = new nsSVGLibartCanvas();
  if (!pg) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(pg);

  nsresult rv = pg->Init(ctx, presContext, dirtyRect);

  if (NS_FAILED(rv)) {
    NS_RELEASE(pg);
    return rv;
  }
  
  *result = pg;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGLibartCanvas)
NS_IMPL_RELEASE(nsSVGLibartCanvas)

NS_INTERFACE_MAP_BEGIN(nsSVGLibartCanvas)
  NS_INTERFACE_MAP_ENTRY(nsISVGRendererCanvas)
  NS_INTERFACE_MAP_ENTRY(nsISVGLibartCanvas)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
//  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGRendererCanvas)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGRendererCanvas methods:

/* nsIRenderingContext lockRenderingContext (); */
NS_IMETHODIMP
nsSVGLibartCanvas::LockRenderingContext(nsIRenderingContext **_retval)
{
  //XXX 
  *_retval = mRenderingContext;
  NS_ADDREF(*_retval);
  return NS_OK;
}

/* void unlockRenderingContext (); */
NS_IMETHODIMP 
nsSVGLibartCanvas::UnlockRenderingContext()
{
  return NS_OK;
}

/* nsIPresContext getPresContext (); */
NS_IMETHODIMP
nsSVGLibartCanvas::GetPresContext(nsIPresContext **_retval)
{
  *_retval = mPresContext;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

/* void clear (in nscolor color); */
NS_IMETHODIMP
nsSVGLibartCanvas::Clear(nscolor color)
{
  InitBuffer();
  PRUint8 red   = NS_GET_R(color);
  PRUint8 green = NS_GET_G(color);
  PRUint8 blue  = NS_GET_B(color);

  PRInt32 stride = mBuffer->GetLineStride();
  PRInt32 width  = mBuffer->GetWidth();
  PRUint8* buf = mBuffer->GetBits();
  PRUint8* end = buf + stride*mBuffer->GetHeight();
  for ( ; buf<end; buf += stride) {
    art_rgb_fill_run(buf, red, green, blue, width);
  }
  
  return NS_OK;
}

/* void flush (); */
NS_IMETHODIMP
nsSVGLibartCanvas::Flush()
{
  if (!mBuffer) return NS_OK;

  if (!mBuffer->GetIsRowOrderTopToBottom()) {
    // XXX we need to flip the image. This is silly. Blt should take
    // care of it
    int stride = mBuffer->GetLineStride();
    int height = mBuffer->GetHeight();
    PRUint8* bits = mBuffer->GetBits();
    PRUint8* rowbuf = new PRUint8[stride];
    for (int row=0; row<height/2; ++row) {
      memcpy(rowbuf, bits+row*stride, stride);
      memcpy(bits+row*stride, bits+(height-1-row)*stride, stride);
      memcpy(bits+(height-1-row)*stride, rowbuf, stride);
    }
    delete[] rowbuf;
  }

  mBuffer->UnlockImagePixels(PR_FALSE);

  mBuffer->SetDecodedRect(0, 0, mBuffer->GetWidth(), mBuffer->GetHeight());
  mBuffer->SetNaturalWidth(mBuffer->GetWidth());
  mBuffer->SetNaturalHeight(mBuffer->GetHeight());
  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));
  nsRect r(0, 0, mBuffer->GetWidth(), mBuffer->GetHeight());
  mBuffer->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged, &r);

  mRenderingContext->DrawImage(mBuffer, mDirtyRectTwips);
  
  mBuffer = nsnull;
  
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGLibartCanvas methods:
NS_IMETHODIMP_(ArtRender*)
nsSVGLibartCanvas::NewRender()
{
  InitBuffer();
  return art_render_new(mDirtyRect.x, mDirtyRect.y, // x0,y0
                        mDirtyRect.x+mDirtyRect.width, // x1
                        mDirtyRect.y+mDirtyRect.height, // y1
                        mBuffer->GetBits(), // pixels
                        mBuffer->GetLineStride(), // rowstride
                        3, // n_chan
                        8, // depth
                        ART_ALPHA_NONE, // alpha_type
                        NULL //alphagamma
                        );
}

NS_IMETHODIMP_(void)
nsSVGLibartCanvas::InvokeRender(ArtRender* render)
{
  art_render_invoke(render);
}

//----------------------------------------------------------------------
// helpers:

void
nsSVGLibartCanvas::InitBuffer()
{
  if (mBuffer) return; // already initialized

  mBuffer = do_CreateInstance(kCImage);
  mBuffer->Init(mDirtyRect.width, mDirtyRect.height, 24,
                nsMaskRequirements_kNoMask);
  
  mBuffer->LockImagePixels(PR_FALSE);
}

