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
#include "nsISVGLibartBitmap.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIPresContext.h"
#include "nsRect.h"
#include "nsIImage.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"

static NS_DEFINE_IID(kCImage, NS_IMAGE_CID);

////////////////////////////////////////////////////////////////////////
// nsSVGLibartBitmapDefault: an implementation based on nsIImage that
// should work on all platforms but doesn't support obtaining
// RenderingContexts with Lock/UnlockRenderingContext

class nsSVGLibartBitmapDefault : public nsISVGLibartBitmap
{
public:
  nsSVGLibartBitmapDefault();
  ~nsSVGLibartBitmapDefault();
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
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<nsIImage> mBuffer;
  nsRect mRectTwips;
};

//----------------------------------------------------------------------
// implementation:

nsSVGLibartBitmapDefault::nsSVGLibartBitmapDefault()
{
  NS_INIT_ISUPPORTS();
}

nsSVGLibartBitmapDefault::~nsSVGLibartBitmapDefault()
{

}

nsresult
nsSVGLibartBitmapDefault::Init(nsIRenderingContext* ctx,
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

  mBuffer = do_CreateInstance(kCImage);
  mBuffer->Init(rect.width, rect.height, 24,
                nsMaskRequirements_kNoMask);
  
  mBuffer->LockImagePixels(PR_FALSE);
  
  return NS_OK;
}

nsresult
NS_NewSVGLibartBitmapDefault(nsISVGLibartBitmap **result,
                             nsIRenderingContext *ctx,
                             nsIPresContext* presContext,
                             const nsRect & rect)
{
  nsSVGLibartBitmapDefault* bm = new nsSVGLibartBitmapDefault();
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

NS_IMPL_ADDREF(nsSVGLibartBitmapDefault)
NS_IMPL_RELEASE(nsSVGLibartBitmapDefault)

NS_INTERFACE_MAP_BEGIN(nsSVGLibartBitmapDefault)
  NS_INTERFACE_MAP_ENTRY(nsISVGLibartBitmap)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGLibartBitmap methods:

NS_IMETHODIMP_(PRUint8 *)
nsSVGLibartBitmapDefault::GetBits()
{
  return mBuffer->GetBits();
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetIndexR()
{
  // XXX we should have a method on nsIImage to extract the color order
#ifdef XP_WIN
  return 2;
#else
  return 0;
#endif
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetIndexG()
{
  return 1;
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetIndexB()
{
#ifdef XP_WIN
  return 0;
#else
  return 2;
#endif
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetLineStride()
{
  return mBuffer->GetLineStride();
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetWidth()
{
  return mBuffer->GetWidth();
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapDefault::GetHeight()
{
  return mBuffer->GetHeight();
}

NS_IMETHODIMP_(nsIRenderingContext*)
nsSVGLibartBitmapDefault::LockRenderingContext(const nsRect& rect)
{
  // doesn't work on default bitmap!
  return nsnull;
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapDefault::UnlockRenderingContext()
{
  // doesn't work on default bitmap!
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapDefault::Flush()
{
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

  mRenderingContext->DrawImage(mBuffer, mRectTwips);

  mBuffer->LockImagePixels(PR_FALSE);
}
