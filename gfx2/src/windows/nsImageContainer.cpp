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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Stuart Parmenter <pavlov@netscape.com>
 */

#include "nsImageContainer.h"
#include "nsIImageFrame.h"

#include "nsCOMPtr.h"

NS_IMPL_ISUPPORTS2(nsImageContainer, nsIImageContainer, nsPIImageContainerWin)

nsImageContainer::nsImageContainer()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
  mCurrentFrame = 0;
}

nsImageContainer::~nsImageContainer()
{
  /* destructor code */
  mFrames.Clear();
}




/* void init (in nscoord aWidth, in nscoord aHeight); */
NS_IMETHODIMP nsImageContainer::Init(nscoord aWidth, nscoord aHeight)
{
  if (aWidth <= 0 || aHeight <= 0) {
    printf("error - negative image size\n");
    return NS_ERROR_FAILURE;
  }

  mSize.SizeTo(aWidth, aHeight);

  return NS_OK;
}

/* readonly attribute nscoord width; */
NS_IMETHODIMP nsImageContainer::GetWidth(nscoord *aWidth)
{
  *aWidth = mSize.width;
  return NS_OK;
}

/* readonly attribute nscoord height; */
NS_IMETHODIMP nsImageContainer::GetHeight(nscoord *aHeight)
{
  *aHeight = mSize.height;
  return NS_OK;
}


/* readonly attribute nsIImageFrame currentFrame; */
NS_IMETHODIMP nsImageContainer::GetCurrentFrame(nsIImageFrame * *aCurrentFrame)
{
  return this->GetFrameAt(mCurrentFrame, aCurrentFrame);
}

/* readonly attribute unsigned long numFrames; */
NS_IMETHODIMP nsImageContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  return mFrames.Count(aNumFrames);
}

/* nsIImageFrame getFrameAt (in unsigned long index); */
NS_IMETHODIMP nsImageContainer::GetFrameAt(PRUint32 index, nsIImageFrame **_retval)
{
  nsISupports *sup = mFrames.ElementAt(index);
  if (!sup)
    return NS_ERROR_FAILURE;

  *_retval = NS_REINTERPRET_CAST(nsIImageFrame *, sup);
  return NS_OK;
}

/* void appendFrame (in nsIImageFrame item); */
NS_IMETHODIMP nsImageContainer::AppendFrame(nsIImageFrame *item)
{
    return mFrames.AppendElement(NS_REINTERPRET_CAST(nsISupports*, item));
}

/* void removeFrame (in nsIImageFrame item); */
NS_IMETHODIMP nsImageContainer::RemoveFrame(nsIImageFrame *item)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIEnumerator enumerate (); */
NS_IMETHODIMP nsImageContainer::Enumerate(nsIEnumerator **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void clear (); */
NS_IMETHODIMP nsImageContainer::Clear()
{
  return mFrames.Clear();
}

/* attribute long loopCount; */
NS_IMETHODIMP nsImageContainer::GetLoopCount(PRInt32 *aLoopCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsImageContainer::SetLoopCount(PRInt32 aLoopCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}



/** nsPIImageContainerWin methods **/

struct MONOBITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;
  RGBQUAD           bmiColors[2];

  MONOBITMAPINFO(LONG aWidth, LONG aHeight)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 1;

    // Note that the palette is being set up so the DIB and the DDB have white and
    // black reversed. This is because we need the mask to have 0 for the opaque
    // pixels of the image, and 1 for the transparent pixels. This way the SRCAND
    // operation sets the opaque pixels to 0, and leaves the transparent pixels
    // undisturbed
    bmiColors[0].rgbBlue = 255;
    bmiColors[0].rgbGreen = 255;
    bmiColors[0].rgbRed = 255;
    bmiColors[0].rgbReserved = 0;
    bmiColors[1].rgbBlue = 0;
    bmiColors[1].rgbGreen = 0;
    bmiColors[1].rgbRed = 0;
    bmiColors[1].rgbReserved = 0;
  }
};



NS_IMETHODIMP nsImageContainer::DrawImage(HDC aDestDC, const nsRect * aSrcRect, const nsPoint * aDestPoint)
{
  nsresult rv;

  nsCOMPtr<nsIImageFrame> img;
  rv = this->GetCurrentFrame(getter_AddRefs(img));

  if (NS_FAILED(rv))
    return rv;

  PRUint32 len;
  PRUint8 *bits;
  rv = img->GetImageData(&bits, &len);
  if (NS_FAILED(rv))
    return rv;

  gfx_format format;
  img->GetFormat(&format);


  PRUint32 alen;
  PRUint8 *abits = nsnull;

  if (format == nsIGFXFormat::RGB_A1 || nsIGFXFormat::BGR_A1) {
    rv = img->GetAlphaData(&abits, &alen);
  }


  PRUint32 bpr;
  img->GetImageBytesPerRow(&bpr);

  PRUint32 abpr;
  img->GetAlphaBytesPerRow(&abpr);

  LPBITMAPINFOHEADER mBHead = (LPBITMAPINFOHEADER)new char[sizeof(BITMAPINFO)];

  nscoord frHeight;
  img->GetHeight(&frHeight);

  PRInt32 height = aSrcRect->height;

  nscoord frWidth;
  img->GetWidth(&frWidth);
  PRInt32 width = frWidth;

  mBHead->biSize = sizeof(BITMAPINFOHEADER);
	mBHead->biWidth = width;
	mBHead->biHeight = -height;
	mBHead->biPlanes = 1;
	mBHead->biBitCount = 24; // XXX
	mBHead->biCompression = BI_RGB;
	mBHead->biSizeImage = len;            // not compressed, so we dont need this to be set
	mBHead->biXPelsPerMeter = 0;
	mBHead->biYPelsPerMeter = 0;
	mBHead->biClrUsed = 0;
	mBHead->biClrImportant = 0;

#if 0
  HBITMAP memBM = ::CreateDIBitmap(aDestDC,mBHead,CBM_INIT, bits + PRInt32(aSrcRect->y * bpr), (LPBITMAPINFO)mBHead,
				                           DIB_RGB_COLORS);
#endif
//  void* oldThing = ::SelectObject(aDestDC, memBM);

//	mBHead->biHeight = -mBHead->biHeight;
  int rop = SRCCOPY;
  if (abits) {
    MONOBITMAPINFO bmi(width, -height);
    bmi.bmiHeader.biSizeImage = alen;
    ::StretchDIBits(aDestDC, (aDestPoint->x + aSrcRect->x), (aDestPoint->y + aSrcRect->y), width, height,
                    aSrcRect->x, 0, width, height,
                    abits + (aSrcRect->y * abpr), 
                    (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, SRCAND);
    rop = SRCPAINT;
  }

  ::StretchDIBits(aDestDC, (aDestPoint->x + aSrcRect->x), (aDestPoint->y + aSrcRect->y), width, height,
                  aSrcRect->x, 0, width, height,
                  bits + (aSrcRect->y * bpr), 
                  (LPBITMAPINFO)mBHead, DIB_RGB_COLORS, rop);


//  ::SelectObject(mDC, oldThing);
#if 0
  DeleteObject(memBM);
#endif

  delete[] mBHead;

  return NS_OK;

}