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
 * Portions created by the Initial Developer are Copyright (C) 2003
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

#include"gdk-pixbuf/gdk-pixbuf.h"
#include "nsCOMPtr.h"
#include "nsISVGLibartBitmap.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIPresContext.h"
#include "nsRect.h"
#include "nsIImage.h"
#include "nsIComponentManager.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

////////////////////////////////////////////////////////////////////////
// nsSVGLibartBitmapGdk

class nsSVGLibartBitmapGdk : public nsISVGLibartBitmap
{
public:
  nsSVGLibartBitmapGdk();
  ~nsSVGLibartBitmapGdk();
  nsresult Init(nsIRenderingContext *ctx,
                nsIPresContext* presContext,
                const nsRect & rect);

  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGLibartBitmap interface:
  NS_IMETHOD_(PRUint8 *) GetBits();
  NS_IMETHOD_(int) GetIndexR();
  NS_IMETHOD_(int) GetIndexG();
  NS_IMETHOD_(int) GetIndexB();
  NS_IMETHOD_(int) GetLineStride();
  NS_IMETHOD_(int) GetWidth();
  NS_IMETHOD_(int) GetHeight();
  NS_IMETHOD_(nsIRenderingContext*) LockRenderingContext(const nsRect& rect);
  NS_IMETHOD_(void) UnlockRenderingContext();
  NS_IMETHOD_(void) Flush();
  
private:
  void LockBuffer();
  void UnlockBuffer();

  PRBool mLocked;
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<imgIContainer> mContainer;
  nsCOMPtr<gfxIImageFrame> mBuffer;
  nsRect mRectTwips;
  nsRect mRect;
};

//----------------------------------------------------------------------
// implementation:

nsSVGLibartBitmapGdk::nsSVGLibartBitmapGdk()
    : mLocked(PR_FALSE)
{
}

nsSVGLibartBitmapGdk::~nsSVGLibartBitmapGdk()
{

}

nsresult
nsSVGLibartBitmapGdk::Init(nsIRenderingContext* ctx,
                               nsIPresContext* presContext,
                               const nsRect & rect)
{
  mRenderingContext = ctx;

  float twipsPerPx;
  presContext->GetPixelsToTwips(&twipsPerPx);
  mRectTwips.x = (nscoord)(rect.x*twipsPerPx);
  mRectTwips.y = (nscoord)(rect.y*twipsPerPx);
  mRectTwips.width = (nscoord)(rect.width*twipsPerPx);
  mRectTwips.height = (nscoord)(rect.height*twipsPerPx);
  mRect = rect;
  
  mContainer = do_CreateInstance("@mozilla.org/image/container;1");
  mContainer->Init(rect.width, rect.height, nsnull);
    
  mBuffer = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  mBuffer->Init(0, 0, rect.width, rect.height, gfxIFormats::RGB, 24);
  mContainer->AppendFrame(mBuffer);
  
  return NS_OK;
}

nsresult
NS_NewSVGLibartBitmapDefault(nsISVGLibartBitmap **result,
                             nsIRenderingContext *ctx,
                             nsIPresContext* presContext,
                             const nsRect & rect)
{
  nsSVGLibartBitmapGdk* bm = new nsSVGLibartBitmapGdk();
  if (!bm) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(bm);

  nsresult rv = bm->Init(ctx, presContext, rect);

  if (NS_FAILED(rv)) {
    NS_RELEASE(bm);
    return rv;
  }
  
  *result = bm;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGLibartBitmapGdk)
NS_IMPL_RELEASE(nsSVGLibartBitmapGdk)

NS_INTERFACE_MAP_BEGIN(nsSVGLibartBitmapGdk)
  NS_INTERFACE_MAP_ENTRY(nsISVGLibartBitmap)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// Implementation helpers:
void
nsSVGLibartBitmapGdk::LockBuffer()
{
  if (mLocked) return;

  mBuffer->LockImageData();    
  mLocked = PR_TRUE;
}

void
nsSVGLibartBitmapGdk::UnlockBuffer()
{
  if (!mLocked) return;

  mBuffer->UnlockImageData();
  mLocked = PR_FALSE;
}


//----------------------------------------------------------------------
// nsISVGLibartBitmap methods:

NS_IMETHODIMP_(PRUint8 *)
nsSVGLibartBitmapGdk::GetBits()
{
  LockBuffer();
  PRUint8* bits=nsnull;
  PRUint32 length;
  mBuffer->GetImageData(&bits, &length);
  return bits;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetIndexR()
{
  return 0;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetIndexG()
{
  return 1;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetIndexB()
{
  return 2;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetLineStride()
{
  PRUint32 bytesPerRow=0;
  mBuffer->GetImageBytesPerRow(&bytesPerRow);
  return (int) bytesPerRow;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetWidth()
{
  return mRect.width; 
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapGdk::GetHeight()
{
  return mRect.height;
}

NS_IMETHODIMP_(nsIRenderingContext*)
nsSVGLibartBitmapGdk::LockRenderingContext(const nsRect& rect)
{
  // doesn't work on default bitmap!
  return nsnull;
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapGdk::UnlockRenderingContext()
{
  // doesn't work on default bitmap!
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapGdk::Flush()
{
  UnlockBuffer();

  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));

  nsCOMPtr<nsIInterfaceRequestor> ireq(do_QueryInterface(mBuffer));
  if (ireq) {
    nsCOMPtr<nsIImage> img(do_GetInterface(ireq));

    if (!img->GetIsRowOrderTopToBottom()) {
      // XXX we need to flip the image. This is silly. Blt should take
      // care of it
      int stride = img->GetLineStride();
      int height = GetHeight();
      PRUint8* bits = img->GetBits();
      PRUint8* rowbuf = new PRUint8[stride];
      for (int row=0; row<height/2; ++row) {
        memcpy(rowbuf, bits+row*stride, stride);
        memcpy(bits+row*stride, bits+(height-1-row)*stride, stride);
        memcpy(bits+(height-1-row)*stride, rowbuf, stride);
      }
      delete[] rowbuf;
    }
    
    nsRect r(0, 0, GetWidth(), GetHeight());
    img->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged, &r);
  }
  
  mContainer->DecodingComplete();
  mRenderingContext->DrawTile(mContainer, mRectTwips.x, mRectTwips.y, &mRectTwips);
}
