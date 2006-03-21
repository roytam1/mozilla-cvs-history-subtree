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
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Peter Amstutz <tetron@interreality.org>
 *    Stuart Parmenter <pavlov@netscape.com>
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

#include "membufRenderingContext.h"

#include "nsRect.h"
#include "nsRegion.h"
#include "nsString.h"
#include "nsTransform2D.h"
#include "nsIRegion.h"
#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsGfxCIID.h"

#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIImage.h"

#include "membufFontMetrics.h"

#define IMAGE_DOWNSAMPLE

#ifdef IMAGE_DOWNSAMPLE
#include "membufImageDownSample.h"
#endif

#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MEMBUF_NATIVE_BITMAP               7700
#define MEMBUF_NATIVE_BITMAP_PARENT        7701
#define MEMBUF_NATIVE_BITMAP_DIRTY_FLAG    7702

// #define MEMBUF_DEBUG
#undef MEMBUF_DEBUG

#define CLIPPED_OP(OP) { \
    if(!mClipRegion || mClipRegion->IsEmpty()) {         \
      set_clip_rect(mBitmap, 0, 0, mBitmap->w-1, mBitmap->h-1); \
      OP; \
   } else { \
     nsRegionRectSet* _rrs = 0; \
     mClipRegion->GetRects(&_rrs); \
     for(PRUint32 _i = 0; _i < _rrs->mNumRects; _i++) { \
         set_clip_rect(mBitmap, \
                  _rrs->mRects[_i].x, \
                  _rrs->mRects[_i].y, \
                  _rrs->mRects[_i].x + _rrs->mRects[_i].width - 1, \
                  _rrs->mRects[_i].y + _rrs->mRects[_i].height - 1); \
         if(mBitmap->clip) { \
             OP; \
         } \
     } \
     mClipRegion->FreeRects(_rrs); \
   } \
 }

// Allegro looks for this symbol but doesn't use it. HACK ALERT!!!
//extern "C" void* _mangled_main_address;
//void* _mangled_main_address;

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

//////////////////////////////////////////////////////////////////////
//// membufGraphicsState

class membufGraphicsState
{
public:
    membufGraphicsState();
    membufGraphicsState(membufGraphicsState &aState);
    ~membufGraphicsState();

    membufGraphicsState *mNext;
    nsTransform2D *mMatrix;
    nsCOMPtr<nsIRegion> mClipRegion;
    nsCOMPtr<nsIFontMetrics> mFontMetrics;
    nscolor mColor;
    nsLineStyle mLineStyle;
};

membufGraphicsState :: membufGraphicsState()
{
    mNext = nsnull;
    mMatrix = nsnull;
    mColor = NS_RGB(0, 0, 0);
    mLineStyle = nsLineStyle_kSolid;
    mFontMetrics = nsnull;
}

membufGraphicsState :: ~membufGraphicsState()
{
}

//////////////////////////////////////////////////////////////////////

NS_IMPL_ISUPPORTS1(membufRenderingContext, nsIRenderingContext)

membufRenderingContext::membufRenderingContext() :
    mLineStyle(nsLineStyle_kNone),
    mColor(0),
    mState(nsnull),
    mSurfaceSize(0, 0)
{
    printf("membufRenderingContext::membufRenderingContext()[%x]\n", this);
    mTranMatrix = new nsTransform2D();

    PushState();

    mP2T = 1.0f;
}

membufRenderingContext::~membufRenderingContext()
{
    printf("membufRenderingContext::~membufRenderingContext()[%x]\n",this);
    PRInt32 cnt = mStateCache.Count();

    while (--cnt >= 0)
    {
        PopState();
    }

    delete mTranMatrix;

#ifdef MEMBUF_DEBUG
    printf("deleted rendering context\n");
#endif
    if(mWidget) {
        bool* dirty = (bool*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_DIRTY_FLAG);
        *dirty = true;
    }

    //DownSampleCleanup();
}

//////////////////////////////////////////////////////////////////////
//// nsIRenderingContext

NS_IMETHODIMP
membufRenderingContext::Init(nsIDeviceContext* aContext, nsIWidget *aWidget)
{
    printf("membufRenderingContext::Init(1)\n");
    printf("   nsIDeviceContext: [%x]\n", aContext);
    printf("   nsIWidget: [%x]\n", aWidget);
    mDeviceContext = aContext;
    mWidget = aWidget;

    float app2dev = mDeviceContext->AppUnitsToDevUnits();
    mTranMatrix->AddScale(app2dev, app2dev);
    mP2T = mDeviceContext->DevUnitsToAppUnits();

    if (aWidget) {
        nsRect bounds;
        aWidget->GetBounds(bounds);
        mSurfaceSize.width = bounds.width;
        mSurfaceSize.height = bounds.height;
    }

#ifdef MEMBUF_DEBUG
    printf("created rendering context\n");
#endif

    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if (mBitmap)
      printf("   mBitmap set to: %x dat:%x\n", mBitmap, mBitmap->dat);

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface)
{
    printf("membufRenderingContext::Init(2) - danger!!!\n");
    mDeviceContext = aContext;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::Reset(void)
{
    printf("membufRenderingContext::Reset()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetDeviceContext(nsIDeviceContext *& aDeviceContext)
{
    aDeviceContext = mDeviceContext;
    NS_IF_ADDREF(aDeviceContext);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                         PRUint32 aWidth, PRUint32 aHeight,
                                         void **aBits, PRInt32 *aStride,
                                         PRInt32 *aWidthBytes,
                                         PRUint32 aFlags)
{
    printf("membufRenderingContext::LockDrawingSurface()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::UnlockDrawingSurface(void)
{
    printf("membufRenderingContext::UnlockDrawingSurface()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
    printf("membufRenderingContext::SelectOffScreenDrawingSurface()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetDrawingSurface(nsIDrawingSurface **aSurface)
{
    printf("membufRenderingContext::GetDrawingSurface()\n");
    BITMAP* bm =(BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    BITMAP* bmp =(BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT);
    mDrawingSurface.Init(bmp, bm->x_ofs, bm->y_ofs);
    *aSurface = &mDrawingSurface;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetHints(PRUint32& aResult)
{
    aResult = 0;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::PushState(void)
{
    membufGraphicsState *state = new membufGraphicsState;

    if (!state)
        return NS_ERROR_FAILURE;

    state->mMatrix = mTranMatrix;

    if (nsnull == mTranMatrix)
        mTranMatrix = new nsTransform2D();
    else
        mTranMatrix = new nsTransform2D(mTranMatrix);

    state->mClipRegion = mClipRegion;

    state->mFontMetrics = mFontMetrics;

    state->mColor = mColor;
    state->mLineStyle = mLineStyle;

    mStateCache.AppendElement(state);

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::PopState()
{
    PRUint32 cnt = mStateCache.Count();
    membufGraphicsState *state;

    if (cnt > 0) {
        state = (membufGraphicsState *)mStateCache.ElementAt(cnt - 1);
        mStateCache.RemoveElementAt(cnt - 1);

        if (state->mMatrix) {
            if (mTranMatrix)
                delete mTranMatrix;
            mTranMatrix = state->mMatrix;
        }

        mClipRegion = state->mClipRegion;

        if (state->mFontMetrics && (mFontMetrics != state->mFontMetrics))
            SetFont(state->mFontMetrics);

        if (state->mColor != mColor)
            SetColor(state->mColor);

        if (state->mLineStyle != mLineStyle)
            SetLineStyle(state->mLineStyle);

        delete state;
    }

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::IsVisibleRect(const nsRect& aRect, PRBool &aIsVisible)
{
    if(!mClipRegion || mClipRegion->IsEmpty()) {
        aIsVisible = PR_TRUE;
    } else if(mClipRegion->ContainsRect(aRect.x, aRect.y, aRect.width, aRect.height)) {
        aIsVisible = PR_TRUE;
    } else aIsVisible = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
    nsCOMPtr<nsIRegion> region(do_CreateInstance(kRegionCID));

    nsRect r = aRect;
    mTranMatrix->TransformCoord(&r.x, &r.y, &r.width, &r.height);

    region->SetTo(r.x, r.y, r.width, r.height);
    return SetClipRegion(*(region.get()), aCombine);
}

NS_IMETHODIMP
membufRenderingContext::GetClipRect(nsRect &aRect, PRBool &aHasLocalClip)
{
    PRInt32 x, y, w, h;

    if (!mClipRegion)
        return NS_ERROR_FAILURE;

    if (!mClipRegion->IsEmpty()) {
        mClipRegion->GetBoundingBox(&x,&y,&w,&h);
        aRect.SetRect(x,y,w,h);
        aHasLocalClip = PR_TRUE;
    } else {
        aRect.SetRect(0,0,0,0);
        aHasLocalClip = PR_FALSE;
    }

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::SetLineStyle(nsLineStyle aLineStyle)
{
    mLineStyle = aLineStyle;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetLineStyle(nsLineStyle &aLineStyle)
{
    aLineStyle = mLineStyle;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetPenMode(nsPenMode &aPenMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufRenderingContext::SetPenMode(nsPenMode aPenMode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufRenderingContext::SetClipRegion(const nsIRegion& aRegion,
                                    nsClipCombine aCombine)
{
    PRUint32 cnt = mStateCache.Count();
    membufGraphicsState *state = nsnull;

    if (cnt > 0) {
        state = (membufGraphicsState *)mStateCache.ElementAt(cnt - 1);
    }

    if (state && state->mClipRegion) {
        if (state->mClipRegion == mClipRegion) {
            nsCOMPtr<nsIRegion> tmpRgn;
            GetClipRegion(getter_AddRefs(tmpRgn));
            mClipRegion = tmpRgn;
        }
    }

    if (!mClipRegion) {
        mClipRegion = do_CreateInstance(kRegionCID);
        if (mClipRegion) {
            mClipRegion->Init();
            mClipRegion->SetTo(0,0,mSurfaceSize.width,mSurfaceSize.width);
        }
    }

    switch(aCombine)
    {
    case nsClipCombine_kIntersect:
        mClipRegion->Intersect(aRegion);
        break;
    case nsClipCombine_kUnion:
        mClipRegion->Union(aRegion);
        break;
    case nsClipCombine_kSubtract:
        mClipRegion->Subtract(aRegion);
        break;
    case nsClipCombine_kReplace:
        mClipRegion->SetTo(aRegion);
        break;
    }

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::CopyClipRegion(nsIRegion &aRegion)
{
    if (!mClipRegion)
        return NS_ERROR_FAILURE;

    aRegion.SetTo(*mClipRegion);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetClipRegion(nsIRegion **aRegion)
{
    nsresult rv = NS_ERROR_FAILURE;

    if (!aRegion || !mClipRegion)
        return NS_ERROR_NULL_POINTER;

    if (mClipRegion) {
        if (*aRegion) { // copy it, they should be using CopyClipRegion
            (*aRegion)->SetTo(*mClipRegion);
            rv = NS_OK;
        } else {
            nsCOMPtr<nsIRegion> newRegion = do_CreateInstance(kRegionCID, &rv);
            if (NS_SUCCEEDED(rv)) {
                newRegion->Init();
                newRegion->SetTo(*mClipRegion);
                NS_ADDREF(*aRegion = newRegion);
            }
        }
    } else {
        rv = NS_ERROR_FAILURE;
    }

    return rv;
}

NS_IMETHODIMP
membufRenderingContext::SetColor(nscolor aColor)
{
    if (aColor == mColor)
        return NS_OK;

#ifdef MEMBUF_DEBUG
    //printf("SET COLOR %d, %d, %d\n", NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor));
#endif

    mColor = aColor;
    mAllegro_color = makeacol32(NS_GET_R(aColor),
                                NS_GET_G(aColor),
                                NS_GET_B(aColor),
                                NS_GET_A(aColor));

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetColor(nscolor &aColor) const
{
    aColor = mColor;

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
    mDeviceContext->GetMetricsFor(aFont, aLangGroup, *getter_AddRefs(mFontMetrics));
    nsFontHandle fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    mCurrentFont = (FT_Face)fontHandle;

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::SetFont(nsIFontMetrics *aFontMetrics)
{
    mFontMetrics = aFontMetrics;
    nsFontHandle fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    mCurrentFont = (FT_Face)fontHandle;

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
    aFontMetrics = mFontMetrics;
    NS_IF_ADDREF(aFontMetrics);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::Translate(nscoord aX, nscoord aY)
{
    mTranMatrix->AddTranslation((float)aX,(float)aY);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::Scale(float aSx, float aSy)
{
    mTranMatrix->AddScale(aSx, aSy);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetCurrentTransform(nsTransform2D *&aTransform)
{
    aTransform = mTranMatrix;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::CreateDrawingSurface(const nsRect &aBounds,
                                             PRUint32 aSurfFlags,
                                             nsIDrawingSurface* &aSurface)
{
    printf("membufRenderingContext::CreateDrawingSurface()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
    printf("membufRenderingContext::DestroyDrawingSurface()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    mTranMatrix->TransformCoord(&aX0, &aY0);
    mTranMatrix->TransformCoord(&aX1, &aY1);

#ifdef MEMBUF_DEBUG
    //printf("DRAW LINE %d, %d, %d, %d\n", aX0, aY0, aX1, aY1);
#endif

    CLIPPED_OP(
        line(mBitmap, aX0, aY0, aX1, aY1, mAllegro_color);
    )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawStdLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    CLIPPED_OP(
        line(mBitmap, aX0, aY0, aX1, aY1, mAllegro_color);
    )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawPolyline(const nsPoint aPoints[],
                                   PRInt32 aNumPoints)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    for (PRInt32 i = 1; i < aNumPoints; ++i) {
        int x1 = aPoints[i-1].x;
        int y1 = aPoints[i-1].y;
        int x2 = aPoints[i].x;
        int y2 = aPoints[i].y;
        mTranMatrix->TransformCoord(&x1, &y1);
        mTranMatrix->TransformCoord(&x2, &y2);
        CLIPPED_OP(
            line(mBitmap, x1, y1, x2, y2, mAllegro_color);
            )
    }

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawRect(const nsRect& aRect)
{
    return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;
    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

    CLIPPED_OP(
        rect(mBitmap, aX, aY, aX+aWidth, aY+aHeight, mAllegro_color);
        )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::FillRect(const nsRect& aRect)
{
    return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;
    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

#if 0
    printf("FILL RECT %i %i %i %i (%i %i %i)\n", aX, aY, aX+aWidth, aY+aHeight,
           getr32(mAllegro_color), getg32(mAllegro_color), getb32(mAllegro_color));

    if(getb32(mAllegro_color) == 229) {
        nsRegionRectSet* _rrs = 0;
        mClipRegion->GetRects(&_rrs);
        for(PRUint32 _i = 0; _i < _rrs->mNumRects; _i++) {
            printf("clip sub rect %i %i %i %i\n",
                   _rrs->mRects[_i].x - mBitmap->x_ofs,
                   _rrs->mRects[_i].y - mBitmap->y_ofs,
                   (_rrs->mRects[_i].x - mBitmap->x_ofs) + _rrs->mRects[_i].width - 1,
                   (_rrs->mRects[_i].y - mBitmap->y_ofs) + _rrs->mRects[_i].height - 1);
        }
        mClipRegion->FreeRects(_rrs);
    }
#endif

    CLIPPED_OP(
        rectfill(mBitmap, aX, aY, aX+aWidth, aY+aHeight, mAllegro_color);
    )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::InvertRect(const nsRect& aRect)
{
    return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

    for(int y = aY; y < (aY + aHeight); y++) {
        for(int x = aX; x < (aX + aWidth); x++) {
            int backcolor = getpixel(mBitmap, x, y);
            int back_r = 255 - getr32(backcolor);
            int back_g = 255 - getg32(backcolor);
            int back_b = 255 - getb32(backcolor);
            putpixel(mBitmap, x, y, makecol32(back_r, back_g, back_b));
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::FlushRect(const nsRect& aRect)
{
    return FlushRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::FlushRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    //printf("!!! FLUSH RECT\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    for (PRInt32 i = 1; i < aNumPoints; ++i) {
        int x1 = aPoints[i-1].x;
        int y1 = aPoints[i-1].y;
        int x2 = aPoints[i].x;
        int y2 = aPoints[i].y;
        mTranMatrix->TransformCoord(&x1, &y1);
        mTranMatrix->TransformCoord(&x2, &y2);
        CLIPPED_OP(
            line(mBitmap, x1, y1, x2, y2, mAllegro_color);
            )
    }

    int x1 = aPoints[0].x;
    int y1 = aPoints[0].y;
    int x2 = aPoints[aNumPoints-1].x;
    int y2 = aPoints[aNumPoints-1].y;
    mTranMatrix->TransformCoord(&x1, &y1);
    mTranMatrix->TransformCoord(&x2, &y2);
    CLIPPED_OP(
        line(mBitmap, x1, y1, x2, y2, mAllegro_color);
        )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;
    int* points = new int[aNumPoints*2];
    for(int i = 0; i < aNumPoints; i++) {
        points[i*2+0] = aPoints[i].x;
        points[i*2+1] = aPoints[i].y;
        mTranMatrix->TransformCoord(&points[i*2+0], &points[i*2+1]);
    }

    CLIPPED_OP(
        polygon(mBitmap, aNumPoints, points, mAllegro_color);
        )
    delete[] points;

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawEllipse(const nsRect& aRect)
{
    return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

    CLIPPED_OP(
        ellipse(mBitmap, aX, aY, aWidth/2, aHeight/2, mAllegro_color);
        )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::FillEllipse(const nsRect& aRect)
{
    return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
membufRenderingContext::FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

    CLIPPED_OP(
        ellipsefill(mBitmap, aX, aY, aWidth/2, aHeight/2, mAllegro_color);
        )

    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawArc(const nsRect& aRect,
                                            float aStartAngle, float aEndAngle)
{
    return DrawArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP
membufRenderingContext::DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                              float aStartAngle, float aEndAngle)
{
#ifdef MEMBUF_DEBUG
    printf("!!! DRAW ARC \n");
#endif
    /*
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return;

    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

    CLIPPED_OP(
        arc(mBitmap, aX, aY, aWidth/2, aHeight/2, mAllegro_color);
        )
    */
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::FillArc(const nsRect& aRect,
                              float aStartAngle, float aEndAngle)
{
    return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP
membufRenderingContext::FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                              float aStartAngle, float aEndAngle)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);
#ifdef MEMBUF_DEBUG
    printf("!!! FILL ARC \n");
#endif
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(char aC, nscoord &aWidth)
{
    if (aC == ' ' && mFontMetrics)
        return mFontMetrics->GetSpaceWidth(aWidth);

    return GetWidth(&aC, 1, aWidth);
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(PRUnichar aC, nscoord &aWidth,
                               PRInt32 *aFontID)
{
    return GetWidth(&aC, 1, aWidth, aFontID);
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(const nsString& aString, nscoord &aWidth,
                               PRInt32 *aFontID)
{
    return GetWidth(aString.get(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(const char* aString, nscoord& aWidth)
{
    return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(const char* aString, PRUint32 aLength,
                               nscoord& aWidth)
{
    if (aLength == 0) {
        aWidth = 0;
        return NS_OK;
    }

    aWidth = NS_REINTERPRET_CAST(membufFontMetrics*, mFontMetrics.get())->MeasureString(aString, aLength);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetWidth(const PRUnichar *aString, PRUint32 aLength,
                               nscoord &aWidth, PRInt32 *aFontID)
{
    if (aLength == 0) {
        aWidth = 0;
        return NS_OK;
    }

    aWidth = NS_REINTERPRET_CAST(membufFontMetrics*, mFontMetrics.get())->MeasureString(aString, aLength);
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetTextDimensions(const char* aString, PRUint32 aLength,
                                        nsTextDimensions& aDimensions)
{
  mFontMetrics->GetMaxAscent(aDimensions.ascent);
  mFontMetrics->GetMaxDescent(aDimensions.descent);
  return GetWidth(aString, aLength, aDimensions.width);
}

NS_IMETHODIMP
membufRenderingContext::GetTextDimensions(const PRUnichar* aString,
                                        PRUint32 aLength,
                                        nsTextDimensions& aDimensions,
                                        PRInt32* aFontID)
{
  mFontMetrics->GetMaxAscent(aDimensions.ascent);
  mFontMetrics->GetMaxDescent(aDimensions.descent);
  return GetWidth(aString, aLength, aDimensions.width, aFontID);
}

NS_IMETHODIMP
membufRenderingContext::DrawString(const char *aString, PRUint32 aLength,
                                 nscoord aX, nscoord aY,
                                 const nscoord* aSpacing)
{
    NS_ConvertASCIItoUCS2 s(aString, aLength);
    return DrawString(s.get(), s.Length(), aX, aY, 0, aSpacing);
}

NS_IMETHODIMP
membufRenderingContext::DrawString(const PRUnichar *aString, PRUint32 aLength,
                                 nscoord aX, nscoord aY,
                                 PRInt32 aFontID,
                                 const nscoord* aSpacing)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    mTranMatrix->TransformCoord(&aX, &aY);

#ifdef MEMBUF_DEBUG
    //printf("DRAW STRING %d,%d\n", aX, aY);
#endif
    FT_GlyphSlot slot = mCurrentFont->glyph;

    CLIPPED_OP(
        {
            int pen_x = aX;
            int pen_y = aY;
            for ( int n = 0; n < aLength; n++ ) {
                /* load glyph image into the slot (erase previous one) */
                int error = FT_Load_Char( mCurrentFont, aString[n], FT_LOAD_RENDER );
                if ( error ) continue; /* ignore errors */

                FT_Bitmap* bitmap = &slot->bitmap;
                unsigned char* buffer = bitmap->buffer;
                /* now, draw to our target surface */
                switch(bitmap->pixel_mode) {
                case FT_PIXEL_MODE_MONO:
                {
                    for(unsigned int yi = 0; yi < bitmap->rows; yi++) {
                        for(unsigned int xi = 0; xi < bitmap->width; xi++) {
                            bool val = !(((*(buffer + (xi >> 3))) & (1 << (xi % 8))) == 0);
                            if(val) {
                                putpixel(mBitmap, pen_x + slot->bitmap_left + xi, pen_y - slot->bitmap_top + yi, mAllegro_color);
                            }
                        }
                        buffer += bitmap->pitch;
                    }
                }
                break;
                case FT_PIXEL_MODE_GRAY:
                {
                    for(unsigned int yi = 0; yi < bitmap->rows; yi++) {
                        for(unsigned int xi = 0; xi < bitmap->width; xi++) {
                            unsigned char val = *(buffer + xi);
                            if(val > 0) {
                                int backcolor = getpixel(mBitmap, pen_x + slot->bitmap_left + xi, pen_y - slot->bitmap_top + yi);
                                int back_r = getr32(backcolor);
                                int back_g = getg32(backcolor);
                                int back_b = getb32(backcolor);

                                int text_r = ((getr32(mAllegro_color) * val) / 255) + ((back_r * (255 - val)) / 255);
                                int text_g = ((getg32(mAllegro_color) * val) / 255) + ((back_g * (255 - val)) / 255);
                                int text_b = ((getb32(mAllegro_color) * val) / 255) + ((back_b * (255 - val)) / 255);

                                int target_color = makecol32(text_r, text_g, text_b);

                                putpixel(mBitmap, pen_x + slot->bitmap_left + xi, pen_y - slot->bitmap_top + yi, target_color);
                            }
                        }
                        buffer += bitmap->pitch;
                    }
                }
                break;
                }
                //my_draw_bitmap( &slot->bitmap, pen_x + slot->bitmap_left, pen_y - slot->bitmap_top );

                /* increment pen position */
                pen_x += (slot->advance.x >> 6);
            }
        }
        )


    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DrawString(const nsString& aString,
                                 nscoord aX, nscoord aY,
                                 PRInt32 aFontID,
                                 const nscoord* aSpacing)
{
    return DrawString(aString.get(), aString.Length(), aX, aY, aFontID, aSpacing);
}

NS_IMETHODIMP
membufRenderingContext::CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                        PRInt32 aSrcX, PRInt32 aSrcY,
                                        const nsRect &aDestBounds,
                                        PRUint32 aCopyFlags)
{
    printf("membufRenderingContext::CopyOffScreenBits()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::RetrieveCurrentNativeGraphicData(PRUint32 * ngd)
{
    printf("membufRenderingContext::RetrieveCurrentNativeGraphicData()\n");
    return NS_OK;
}

// Don't want double buffering
NS_IMETHODIMP
membufRenderingContext::UseBackbuffer(PRBool* aUseBackbuffer)
{
    printf("membufRenderingContext::UseBackbuffer()\n");
    *aUseBackbuffer = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetBackbuffer(const nsRect &aRequestedSize,
                                      const nsRect &aMaxSize,
                                      PRBool aForBlending,
                                      nsIDrawingSurface* &aBackbuffer)
{
    printf("membufRenderingContext::GetBackBuffer()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::ReleaseBackbuffer(void)
{
    printf("membufRenderingContext::ReleaseBackbuffer()\n");
    return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::DestroyCachedBackbuffer(void)
{
    printf("membufRenderingContext::DestroyCachedBackbuffer()\n");
    return NS_OK;
}

#ifdef MOZ_MATHML
NS_IMETHODIMP
membufRenderingContext::GetBoundingMetrics(const char*        aString,
                                         PRUint32           aLength,
                                         nsBoundingMetrics& aBoundingMetrics)
{
  return NS_OK;
}

NS_IMETHODIMP
membufRenderingContext::GetBoundingMetrics(const PRUnichar*   aString,
                                         PRUint32           aLength,
                                         nsBoundingMetrics& aBoundingMetrics,
                                         PRInt32*           aFontID)
{
    return NS_OK;
}
#endif // MOZ_MATHML

NS_IMETHODIMP
membufRenderingContext::DrawImage(imgIContainer * aImage, const nsRect & aSrcRect,
                                  const nsRect & aDestRect)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    set_clip_rect(mBitmap, 0, 0, mBitmap->w-1, mBitmap->h-1);

    return nsRenderingContextImpl::DrawImage(aImage, aSrcRect, aDestRect);
}

NS_IMETHODIMP
membufRenderingContext::DrawTile(imgIContainer *aImage,
                               nscoord aXOffset, nscoord aYOffset,
                               const nsRect * aTargetRect)
{
    mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return NS_OK;

    set_clip_rect(mBitmap, 0, 0, mBitmap->w-1, mBitmap->h-1);

    return nsRenderingContextImpl::DrawTile(aImage, aXOffset, aYOffset, aTargetRect);
}

NS_IMETHODIMP
membufRenderingContext::DrawScaledTile(imgIContainer *aImage,
                                     nscoord aXOffset, nscoord aYOffset,
                                     nscoord aTileWidth, nscoord aTileHeight,
                                     const nsRect * aTargetRect)
{
#ifdef MEMBUF_DEBUG
    printf("!!! DRAW SCALED TILE\n");
#endif

    return NS_OK;

    /*mBitmap = (BITMAP*)mWidget->GetNativeData(MEMBUF_NATIVE_BITMAP);
    if(! mBitmap) return;

    set_clip_rect(mBitmap, 0, 0, mBitmap->w-1, mBitmap->h-1);

    return nsRenderingContextImpl::DrawScaledTile(aImage, aXOffset, aYOffset, aTileWidth, aTileHeight, aTargetRect);*/
}

NS_IMETHODIMP
membufRenderingContext::RenderPostScriptDataFragment(const unsigned char*,
                                        long unsigned int)
{
    return NS_OK;
}

