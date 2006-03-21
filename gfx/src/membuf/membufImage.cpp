/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Membuf server code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 */

#include "membufImage.h"

#include "nsIServiceManager.h"
#include "nsRect.h"
#include "membufDrawingSurface.h"

membufImage::membufImage()
 : mWidth(0),
   mHeight(0)
{
}

membufImage::~membufImage()
{
}

NS_IMPL_ISUPPORTS1(membufImage, nsIImage);

//////////////////////////////////////////////
//// nsIImage

nsresult
membufImage::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth, nsMaskRequirements aMaskRequirements)
{
    mWidth = aWidth;
    mHeight = aHeight;

    mImage.depth = aDepth;
    mImage.bpr = (aWidth * aDepth) >> 5;
    if (((PRUint32)aWidth * aDepth) & 0x1F)
        mImage.bpr++;
    mImage.bpr <<= 2;
    mImage.size = mImage.bpr * aHeight;
    mImage.bits = (PRUint8*)new PRUint8[mImage.size];

    switch(aMaskRequirements)
    {
    case nsMaskRequirements_kNeeds1Bit:
        mAlphaImage.depth = 1;
        mAlphaImage.bpr = (aWidth + 7) / 8;
        break;
    case nsMaskRequirements_kNeeds8Bit:
        mAlphaImage.depth = 8;
        mAlphaImage.bpr = aWidth;
        break;
    default:
        break;
    }

    if (aMaskRequirements != nsMaskRequirements_kNoMask) {
        // 32-bit align each row
        mAlphaImage.bpr = (mAlphaImage.bpr + 3) & ~0x3;
        mAlphaImage.bits = (PRUint8*)new PRUint8[mAlphaImage.bpr * aHeight];
    }

    return NS_OK;
}

PRInt32
membufImage::GetBytesPix()
{
    return mImage.depth / 8;
}

PRBool
membufImage::GetIsRowOrderTopToBottom()
{
    return PR_FALSE;
}

PRInt32
membufImage::GetWidth()
{
    return mWidth;
}

PRInt32
membufImage::GetHeight()
{
    return mHeight;
}

PRUint8 *
membufImage::GetBits()
{
    return mImage.bits;
}

PRInt32
membufImage::GetLineStride()
{
    return mImage.bpr;
}

PRBool
membufImage::GetHasAlphaMask()
{
    return (mAlphaImage.depth != 0);
}

PRUint8 *
membufImage::GetAlphaBits()
{
    return mAlphaImage.bits;
}

PRInt32
membufImage::GetAlphaLineStride()
{
    return mAlphaImage.bpr;
}

void
membufImage::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
{
}

PRBool
membufImage::GetIsImageComplete()
{
  return PR_TRUE;
}

nsresult
membufImage::Optimize(nsIDeviceContext* aContext)
{
    return NS_OK;
}

nsColorMap *
membufImage::GetColorMap()
{
    return NULL;
}

NS_IMETHODIMP
membufImage::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface, PRInt32 aX,
                  PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
    Draw(aContext, aSurface, 0, 0, mWidth, mHeight, aX, aY, aWidth, aHeight);
    return NS_OK;
}

NS_IMETHODIMP
membufImage::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                  PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                  PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
    //Draw(aContext, aSurface, aDX, aDY, aDWidth, aDHeight);

    membufDrawingSurface* surf = (membufDrawingSurface*)aSurface;
    void* db = 0;
    PRInt32 stride;
    PRInt32 widthbytes;

    surf->Lock(aDX, aDY, aDWidth, aDHeight, &db, &stride, &widthbytes, 0);

    if(!db) return NS_OK;

    PRInt32* destbits = (PRInt32*)(db);
    PRUint8* srcbits = mImage.bits;
    PRUint8* alphabits = mAlphaImage.bits;
    stride = stride/4;

    float src_x = aSX, src_y = aSY;
    float x_step = (float)aSWidth / (float)aDWidth;
    float y_step = (float)aSHeight / (float)aDHeight;

    for(int dest_y = 0; dest_y < aDHeight; dest_y++) {
        src_x = aSX;
        for(int dest_x = 0;  dest_x < aDWidth; dest_x++) {
            int img_x = (int)round(src_x);
            int img_y = (int)round(src_y);

            nscolor c = *(PRInt32*)(srcbits + (img_y * mImage.bpr) + img_x*3);

            int backcolor = *(destbits + (dest_y*stride) + dest_x);
            unsigned char val;
            switch(mAlphaImage.depth) {
            case 0:
                val = 255;
                break;
            case 1:
                if(*(alphabits + (img_y * mAlphaImage.bpr) + (img_x >> 3)) & (1 << (7 - (img_x % 8)))) {
                    val = 255;
                }
                else {
                    val = 0;
                }
                break;
            case 8:
                val = *(alphabits + (img_y * mAlphaImage.bpr) + img_x);
                break;
            default:
                printf("Only support alpha depth 1 and 8\n");
                break;
            }

            int back_r = getr32(backcolor);
            int back_g = getg32(backcolor);
            int back_b = getb32(backcolor);

            int img_r = ((NS_GET_R(c) * val) / 255) + ((back_r * (255 - val)) / 255);
            int img_g = ((NS_GET_G(c) * val) / 255) + ((back_g * (255 - val)) / 255);
            int img_b = ((NS_GET_B(c) * val) / 255) + ((back_b * (255 - val)) / 255);

            int target_color = makeacol32(img_r, img_g, img_b, 255);

            *(destbits + (dest_y*stride) + dest_x) = target_color;

            src_x += x_step;
        }
        src_y += y_step;
    }

    surf->Unlock();


    return NS_OK;
}

NS_IMETHODIMP
membufImage::DrawTile(nsIRenderingContext &aContext,
                      nsIDrawingSurface* aSurface,
                      PRInt32 aSXOffset, PRInt32 aSYOffset,
                      PRInt32 aPadX, PRInt32 aPadY,
                      const nsRect &aTileRect)
{
    //printf("!!! DRAW TILE\n");

    //membufDrawingSurface* surf = (membufDrawingSurface*)aSurface;

    PRInt32 aY0 = aTileRect.y,
            aX0 = aTileRect.x,
            aY1 = aTileRect.y + aTileRect.height,
            aX1 = aTileRect.x + aTileRect.width;

    PRInt32 y = aY0;

    if(aSYOffset > 0) {
        PRInt32 x = aX0;

        int h = (mHeight - aSYOffset > aTileRect.height) ? aTileRect.height : mHeight - aSYOffset;
        int w = (mWidth - aSXOffset > aTileRect.width) ? aTileRect.width : mWidth - aSXOffset;

        if(aSXOffset > 0) {
            Draw(aContext, aSurface,
                 aSXOffset, aSYOffset, w, h,
                 0,         0,         w, h);
            x += mWidth - aSXOffset;
        }

        for (; x < aX1; x += mWidth) {
            w = (aX1 - x) > mWidth ? mWidth : (aX1 - x);

            Draw(aContext, aSurface,
                 0, aSYOffset, w, h,
                 x, 0,         w, h);
        }
        y += mHeight - aSYOffset;
    }

    for (; y < aY1; y += mHeight) {
        PRInt32 x = aX0;

        int h = (aY1 - y) > mHeight ? mHeight : (aY1 - y);

        if(aSXOffset > 0) {
            int w = (mWidth - aSXOffset > aTileRect.width) ? aTileRect.width : mWidth - aSXOffset;

            Draw(aContext, aSurface,
                 aSXOffset, 0, w, h,
                 x, y, w, h);
            x += mWidth - aSXOffset;
        }

        for (; x < aX1; x += mWidth) {
            int w = (aX1 - x) > mWidth ? mWidth : (aX1 - x);

            Draw(aContext, aSurface, x, y, w, h);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
membufImage::DrawToImage(nsIImage* aDstImage, PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
    //printf("In mbI::DrawToImage\n");

    return NS_OK;
}

PRInt8
membufImage::GetAlphaDepth()
{
    return mAlphaImage.depth;
}

void *
membufImage::GetBitInfo()
{
    return NULL;
}

NS_IMETHODIMP
membufImage::LockImagePixels(PRBool aMaskPixels)
{
    return NS_OK;
}

NS_IMETHODIMP
membufImage::UnlockImagePixels(PRBool aMaskPixels)
{
    return NS_OK;
}
