/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Smith <david@igelaus.com.au>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *
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
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "membufDrawingSurface.h"
#include <stdio.h>

NS_IMPL_ISUPPORTS1(membufDrawingSurface, nsIDrawingSurface)

membufDrawingSurface::membufDrawingSurface() : mBitmap(0)
{
  printf("membufDrawingSurface[%x]::membufDrawingSurface()\n", this);
}

membufDrawingSurface::~membufDrawingSurface()
{ 
  printf("membufDrawingSurface[%x]::~membufDrawingSurface()\n", this);
}

NS_IMETHODIMP
membufDrawingSurface::Init(BITMAP *aBitmap, int xo, int yo)
{
  printf("membufDrawingSurface[%x]::Init(%x,%d,%d)\n",this, aBitmap,xo,yo);
  mBitmap = aBitmap;
  x_ofs = xo;
  y_ofs = yo;

  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::Lock(PRInt32 aX, PRInt32 aY,
                           PRUint32 aWidth, PRUint32 aHeight,
                           void **aBits, PRInt32 *aStride,
                           PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  if(mBitmap) {
    aX += x_ofs;
    aY += y_ofs;
    if(aX > mBitmap->w || aY > mBitmap->w) {
      *aBits = 0;
    } else {
      *aBits = (char*)mBitmap->dat + (aY * mBitmap->w * 4) + (aX * 4);
      *aStride = mBitmap->w * 4;
      *aWidthBytes = aWidth * 4;
    }
  } else {
    *aBits = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::Unlock(void)
{
  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
{
  *aWidth = mBitmap->w;
  *aHeight = mBitmap->h;
  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::IsOffscreen(PRBool *aOffScreen)
{
  printf("membufDrawingSurface[%x]::IsOffscreen()\n",this);
  *aOffScreen = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::IsPixelAddressable(PRBool *aAddressable)
{
  *aAddressable = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
membufDrawingSurface::GetPixelFormat(nsPixelFormat *aFormat)
{
  aFormat->mRedMask = (0xFF << _rgb_r_shift_32);
  aFormat->mGreenMask = (0xFF << _rgb_g_shift_32);
  aFormat->mBlueMask = (0xFF << _rgb_b_shift_32);
  aFormat->mAlphaMask = (0xFF << _rgb_a_shift_32);

  aFormat->mRedCount = 8;
  aFormat->mGreenCount = 8;
  aFormat->mBlueCount = 8;
  aFormat->mAlphaCount = 8;

  aFormat->mRedShift = _rgb_r_shift_32;
  aFormat->mGreenShift = _rgb_g_shift_32;
  aFormat->mBlueShift = _rgb_b_shift_32;
  aFormat->mAlphaShift = _rgb_a_shift_32;
  return NS_OK;
}

