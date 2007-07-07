/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Tomi Leppikangas <tomi.leppikangas@oulu.fi>
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsFixedSizeAllocator.h"
#include "nsRenderingContextGTK.h"
#include "nsRegionGTK.h"
#include "nsImageGTK.h"
#include "nsGraphicsStateGTK.h"
#include "nsCompressedCharMap.h"
#include <math.h>
#include "nsGCCache.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "prmem.h"
#include "prenv.h"

#include "nsIFontMetricsGTK.h"
#include "nsDeviceContextGTK.h"
#include "nsFontMetricsUtils.h"

#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIImage.h"

#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"

#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIImage.h"

#include "nsIServiceManager.h"
#include "nsIInterfaceRequestorUtils.h"

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdkwindow.h>
#endif

NS_IMPL_ISUPPORTS1(nsRenderingContextGTK, nsIRenderingContext)

#define NSRECT_TO_GDKRECT(ns,gdk) \
  PR_BEGIN_MACRO \
  gdk.x = ns.x; \
  gdk.y = ns.y; \
  gdk.width = ns.width; \
  gdk.height = ns.height; \
  PR_END_MACRO

#define FROM_TWIPS_INT(_x)  (NSToIntRound((float)((_x)/(mContext->AppUnitsPerDevPixel()))))
#define FROM_TWIPS_INT2(_x)  (NSToIntRound((float)((_x)/(mContext->AppUnitsPerDevPixel()))+0.5))
#define FROM_TWIPS(_x)  ((float)((_x)/(mContext->AppUnitsPerDevPixel())))
#define NS_RECT_FROM_TWIPS_RECT(_r)   (nsRect(FROM_TWIPS_INT((_r).x), FROM_TWIPS_INT((_r).y), FROM_TWIPS_INT2((_r).width), FROM_TWIPS_INT2((_r).height)))

static nsGCCache *gcCache = nsnull;
static nsFixedSizeAllocator *gStatePool = nsnull;

nsRenderingContextGTK::nsRenderingContextGTK()
{
  mFontMetrics = nsnull;
  mContext = nsnull;
  mSurface = nsnull;
  mOffscreenSurface = nsnull;
  mCurrentColor = NS_RGB(255, 255, 255);  // set it to white
  mCurrentLineStyle = nsLineStyle_kSolid;
  mTranMatrix = nsnull;
  mClipRegion = nsnull;
  mDrawStringBuf = nsnull;
  mGC = nsnull;

  mFunction = GDK_COPY;

  PushState();
}

nsRenderingContextGTK::~nsRenderingContextGTK()
{
  // Destroy the State Machine
  PRInt32 cnt = mStateCache.Count();

  while (--cnt >= 0)
    PopState();

  if (mTranMatrix) {
    if (gStatePool) {
      mTranMatrix->~nsTransform2D();
      gStatePool->Free(mTranMatrix, sizeof(nsTransform2D));
    } else {
      delete mTranMatrix;
    }
  }
  NS_IF_RELEASE(mOffscreenSurface);
  NS_IF_RELEASE(mFontMetrics);
  NS_IF_RELEASE(mContext);

  if (nsnull != mDrawStringBuf) {
    delete [] mDrawStringBuf;
  }

  if (nsnull != mGC) {
    gdk_gc_unref(mGC);
  }
}

/*static*/ nsresult
nsRenderingContextGTK::Shutdown()
{
  delete gcCache;
  delete gStatePool;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::Init(nsIDeviceContext* aContext,
                                          nsIWidget *aWindow)
{
  mContext = aContext;
  NS_IF_ADDREF(mContext);

//  ::gdk_rgb_init();

  mSurface = new nsDrawingSurfaceGTK();
  mDisplay = GDK_DISPLAY();

  if (mSurface)
  {
    if (!aWindow) return NS_ERROR_NULL_POINTER;

    // we want to ref the window here so that we can unref in the drawing surface.
    // otherwise, we can not unref and that causes windows that are created in the
    // drawing surface not to be freed.
    GdkDrawable *win = (GdkDrawable*)aWindow->GetNativeData(NS_NATIVE_WINDOW);
    if (win)
      gdk_window_ref((GdkWindow*)win);
    else
    {
      GtkWidget *w = (GtkWidget *) aWindow->GetNativeData(NS_NATIVE_WIDGET);

      if (!w)
      {
          delete mSurface;
          mSurface = nsnull;
          return NS_ERROR_NULL_POINTER;
      }

      gdk_error_trap_push();
      win = gdk_pixmap_new(nsnull,
                           w->allocation.width,
                           w->allocation.height,
                           gdk_rgb_get_visual()->depth);
      gdk_flush();
      if (gdk_error_trap_pop() || !win)
      {
        delete mSurface;
        mSurface = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
      }
#ifdef MOZ_WIDGET_GTK2
      gdk_drawable_set_colormap(win, gdk_rgb_get_colormap());
#endif
    }

    GdkGC *gc = (GdkGC *)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);
    mSurface->Init(win,gc);

    mOffscreenSurface = mSurface;

    NS_ADDREF(mSurface);

    // aWindow->GetNativeData() ref'd the gc.
    // only Win32 has a FreeNativeData() method.
    // so do this manually here.
    gdk_gc_unref(gc);
  }
  return (CommonInit());
}

NS_IMETHODIMP nsRenderingContextGTK::Init(nsIDeviceContext* aContext,
                                          nsIDrawingSurface* aSurface)
{
  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mSurface = (nsDrawingSurfaceGTK *) aSurface;
  NS_ADDREF(mSurface);
  mOffscreenSurface = mSurface;

  return (CommonInit());
}

NS_IMETHODIMP nsRenderingContextGTK::CommonInit()
{
  float app2dev = 1.0;
  mTranMatrix->AddScale(app2dev, app2dev);

  return NS_OK;
}

void*
nsRenderingContextGTK::GetNativeGraphicData(GraphicDataType aType)
{
  if (aType == NATIVE_GDK_DRAWABLE)
    return mSurface->GetDrawable();
  return nsnull;
}

NS_IMETHODIMP nsRenderingContextGTK::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;

  // Most X servers implement 8 bit text rendering a lot faster than
  // XChar2b rendering. In addition, we can avoid the PRUnichar to
  // XChar2b conversion. So we set this bit...
  result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;

  // XXX see if we are rendering to the local display or to a remote
  // dispaly and set the NS_RENDERING_HINT_REMOTE_RENDERING accordingly

  // see if the font metrics has anything more to offer
  result |= NS_FontMetricsGetHints();

  aResult = result;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PushState();

  return mSurface->Lock(aX, aY, aWidth, aHeight,
                        aBits, aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP nsRenderingContextGTK::UnlockDrawingSurface(void)
{
  PopState();

  mSurface->Unlock();
  
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
  if (nsnull == aSurface)
    mSurface = mOffscreenSurface;
  else
    mSurface = (nsDrawingSurfaceGTK *)aSurface;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetDrawingSurface(nsIDrawingSurface* *aSurface)
{
  *aSurface = mSurface;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::Reset()
{
#ifdef DEBUG
  g_print("nsRenderingContextGTK::Reset() called\n");
#endif
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetDeviceContext(nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}
#if 0
NS_IMETHODIMP nsRenderingContextGTK::PushState(PRInt32 aFlags)
{
  //  Get a new GS
#ifdef USE_GS_POOL
  nsGraphicsState *state = nsGraphicsStatePool::GetNewGS();
#else
  nsGraphicsState *state = new nsGraphicsState;
#endif
  // Push into this state object, add to vector
  if (!state)
    return NS_ERROR_FAILURE;

  if (aFlags & NS_STATE_COLOR) {
    state->mColor = mCurrentColor;
  }

  if (aFlags & NS_STATE_TRANSFORM) {
    state->mMatrix = mTranMatrix;
    if (nsnull == mTranMatrix) {
      mTranMatrix = new nsTransform2D();
    } else {
      mTranMatrix = new nsTransform2D(mTranMatrix);
    }
  }

  if (aFlags & NS_STATE_FONT) {
    NS_IF_ADDREF(mFontMetrics);
    state->mFontMetrics = mFontMetrics;
  }

  if (aFlags & NS_STATE_CLIP) {
    state->mClipRegion = mClipRegion;
  }

  if (aFlags & NS_STATE_LINESTYLE) {
    state->mLineStyle = mCurrentLineStyle;
  }

  mStateCache.AppendElement(state);
  
  return NS_OK;
}
#endif

NS_IMETHODIMP nsRenderingContextGTK::PushState(void)
{
  //  Get a new GS
  if (!gStatePool) {
    gStatePool = new nsFixedSizeAllocator();
    size_t sizes[] = {sizeof(nsGraphicsState), sizeof(nsTransform2D)};
    if (gStatePool)
      gStatePool->Init("GTKStatePool", sizes, sizeof(sizes)/sizeof(size_t),
                       sizeof(nsGraphicsState)*64);
  }

  nsGraphicsState *state = nsnull;
  if (gStatePool) {
    void *space = gStatePool->Alloc(sizeof(nsGraphicsState));
    if (space)
      state = ::new(space) nsGraphicsState;
  } else {
    state = new nsGraphicsState;
  }

  // Push into this state object, add to vector
  if (!state)
    return NS_ERROR_FAILURE;

  state->mMatrix = mTranMatrix;

  if (gStatePool) {
    void *space = gStatePool->Alloc(sizeof(nsTransform2D));
    if (mTranMatrix)
      mTranMatrix = ::new(space) nsTransform2D(mTranMatrix);
    else
      mTranMatrix = ::new(space) nsTransform2D();
  } else {
    if (mTranMatrix)
      mTranMatrix = ::new nsTransform2D(mTranMatrix);
    else
      mTranMatrix = ::new nsTransform2D();
  }

  // set state to mClipRegion.. SetClip{Rect,Region}() will do copy-on-write stuff
  state->mClipRegion = mClipRegion;

  NS_IF_ADDREF(mFontMetrics);
  state->mFontMetrics = mFontMetrics;

  state->mColor = mCurrentColor;
  state->mLineStyle = mCurrentLineStyle;

  mStateCache.AppendElement(state);
  
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::PopState(void)
{
  PRUint32 cnt = mStateCache.Count();
  nsGraphicsState * state;

  if (cnt > 0) {
    state = (nsGraphicsState *)mStateCache.ElementAt(cnt - 1);
    mStateCache.RemoveElementAt(cnt - 1);

    // Assign all local attributes from the state object just popped
    if (state->mMatrix) {
      if (mTranMatrix) {
        if (gStatePool) {
          mTranMatrix->~nsTransform2D();
          gStatePool->Free(mTranMatrix, sizeof(nsTransform2D));
        } else {
          delete mTranMatrix;
        }
      }
      mTranMatrix = state->mMatrix;
    }

    mClipRegion.swap(state->mClipRegion);

    if (state->mFontMetrics && (mFontMetrics != state->mFontMetrics))
      SetFont(state->mFontMetrics);

    if (state->mColor != mCurrentColor)
      SetColor(state->mColor);    

    if (state->mLineStyle != mCurrentLineStyle)
      SetLineStyle(state->mLineStyle);

    // Delete this graphics state object
    if (gStatePool) {
      state->~nsGraphicsState();
      gStatePool->Free(state, sizeof(nsGraphicsState));
    } else {
      delete state;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::IsVisibleRect(const nsRect& aRect,
                                                   PRBool &aVisible)
{
  aVisible = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
  PRInt32 x, y, w, h;
  
  if (!mClipRegion)
    return NS_ERROR_FAILURE;

  if (!mClipRegion->IsEmpty()) {
    mClipRegion->GetBoundingBox(&x,&y,&w,&h);
    aRect.SetRect(x,y,w,h);
    aClipValid = PR_TRUE;
  } else {
    aRect.SetRect(0,0,0,0);
    aClipValid = PR_FALSE;
  }

  return NS_OK;
}

#ifdef DEBUG
// #define TRACE_SET_CLIP
#endif

#ifdef TRACE_SET_CLIP
static char *
nsClipCombine_to_string(nsClipCombine aCombine)
{
  switch(aCombine)
    {
      case nsClipCombine_kIntersect:
        return "nsClipCombine_kIntersect";
        break;

      case nsClipCombine_kUnion:
        return "nsClipCombine_kUnion";
        break;

      case nsClipCombine_kSubtract:
        return "nsClipCombine_kSubtract";
        break;

      case nsClipCombine_kReplace:
        return "nsClipCombine_kReplace";
        break;
    }

  return "something got screwed";
}
#endif // TRACE_SET_CLIP

void
nsRenderingContextGTK::CreateClipRegion()
{
  // We have 3 cases to deal with:
  //  1 - There is no mClipRegion -> Create one
  //  2 - There is an mClipRegion shared w/ stack -> Duplicate and unshare
  //  3 - There is an mClipRegion and its not shared -> return

  if (mClipRegion) {
    PRUint32 cnt = mStateCache.Count();

    if (cnt > 0) {
      nsGraphicsState *state;
      state = (nsGraphicsState *)mStateCache.ElementAt(cnt - 1);

      if (state->mClipRegion == mClipRegion) {
        mClipRegion = new nsRegionGTK;
        if (mClipRegion) {
          mClipRegion->SetTo(*state->mClipRegion);
        }
      }
    }
  } else {

    PRUint32 w, h;
    mSurface->GetSize(&w, &h);

    mClipRegion = new nsRegionGTK;
    if (mClipRegion) {
      mClipRegion->Init();
      mClipRegion->SetTo(0, 0, w, h);
    }
  }
}

NS_IMETHODIMP nsRenderingContextGTK::SetClipRect(const nsRect& aRect,
                                                 nsClipCombine aCombine)
{
  nsRect trect(NS_RECT_FROM_TWIPS_RECT(aRect));
  mTranMatrix->TransformCoord(&trect.x, &trect.y,
                              &trect.width, &trect.height);
  SetClipRectInPixels(trect, aCombine);
  return NS_OK;
}

void nsRenderingContextGTK::SetClipRectInPixels(const nsRect& aRect,
                                                nsClipCombine aCombine)
{
  CreateClipRegion();

#ifdef TRACE_SET_CLIP
  printf("nsRenderingContextGTK::SetClipRect(%s)\n",
         nsClipCombine_to_string(aCombine));
#endif // TRACE_SET_CLIP

  switch(aCombine)
  {
    case nsClipCombine_kIntersect:
      mClipRegion->Intersect(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kUnion:
      mClipRegion->Union(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kSubtract:
      mClipRegion->Subtract(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kReplace:
      mClipRegion->SetTo(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
  }
#if 0
  nscolor color = mCurrentColor;
  SetColor(NS_RGB(255,   0,   0));
  FillRect(aRect);
  SetColor(color);
#endif
}

void nsRenderingContextGTK::UpdateGC()
{
  GdkGCValues values;
  GdkGCValuesMask valuesMask;

  if (mGC)
    gdk_gc_unref(mGC);

  memset(&values, 0, sizeof(GdkGCValues));

  values.foreground.pixel =
    gdk_rgb_xpixel_from_rgb(NS_TO_GDK_RGB(mCurrentColor));
  values.foreground.red = (NS_GET_R(mCurrentColor) << 8) | NS_GET_R(mCurrentColor);
  values.foreground.green = (NS_GET_G(mCurrentColor) << 8) | NS_GET_G(mCurrentColor);
  values.foreground.blue = (NS_GET_B(mCurrentColor) << 8) | NS_GET_B(mCurrentColor);
  valuesMask = GDK_GC_FOREGROUND;

#ifdef MOZ_ENABLE_COREXFONTS
  if (mFontMetrics) {
    GdkFont *font = mFontMetrics->GetCurrentGDKFont();
    if (font) {
      valuesMask = GdkGCValuesMask(valuesMask | GDK_GC_FONT);
      values.font = font;
    }
  }
#endif

  valuesMask = GdkGCValuesMask(valuesMask | GDK_GC_LINE_STYLE);
  values.line_style = mLineStyle;

  valuesMask = GdkGCValuesMask(valuesMask | GDK_GC_FUNCTION);
  values.function = mFunction;

  GdkRegion *rgn = nsnull;
  if (mClipRegion) {
    mClipRegion->GetNativeRegion((void*&)rgn);
  }

  if (!gcCache) {
    gcCache = new nsGCCache();
    if (!gcCache) return;
  }

  mGC = gcCache->GetGC(mOffscreenSurface->GetDrawable(),
                       &values,
                       valuesMask,
                       rgn);

  if (mDashes)
    ::XSetDashes(mDisplay, GDK_GC_XGC(mGC),
                 0, mDashList, mDashes);
}

NS_IMETHODIMP nsRenderingContextGTK::SetClipRegion(const nsIRegion& aRegion,
                                                   nsClipCombine aCombine)
{
  CreateClipRegion();

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

/**
 * Fills in |aRegion| with a copy of the current clip region.
 */
NS_IMETHODIMP nsRenderingContextGTK::CopyClipRegion(nsIRegion &aRegion)
{
  if (!mClipRegion)
    return NS_ERROR_FAILURE;

  aRegion.SetTo(*mClipRegion);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetClipRegion(nsIRegion **aRegion)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (!aRegion || !mClipRegion)
    return NS_ERROR_NULL_POINTER;

  if (mClipRegion) {
    if (*aRegion) { // copy it, they should be using CopyClipRegion 
      (*aRegion)->SetTo(*mClipRegion);
      rv = NS_OK;
    } else {
      nsCOMPtr<nsIRegion> newRegion = new nsRegionGTK;
      if (newRegion) {
        newRegion->Init();
        newRegion->SetTo(*mClipRegion);
        NS_ADDREF(*aRegion = newRegion);
      }
    }
  } else {
#ifdef DEBUG
    printf("null clip region, can't make a valid copy\n");
#endif
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

NS_IMETHODIMP nsRenderingContextGTK::SetColor(nscolor aColor)
{
  if (nsnull == mContext)  
    return NS_ERROR_FAILURE;

  mCurrentColor = aColor;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetColor(nscolor &aColor) const
{
  aColor = mCurrentColor;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
  nsCOMPtr<nsIFontMetrics> newMetrics;
  nsresult rv = mContext->GetMetricsFor(aFont, aLangGroup, *getter_AddRefs(newMetrics));
  if (NS_SUCCEEDED(rv)) {
    rv = SetFont(newMetrics);
  }
  return rv;
}

NS_IMETHODIMP nsRenderingContextGTK::SetFont(nsIFontMetrics *aFontMetrics)
{
  NS_IF_RELEASE(mFontMetrics);
  mFontMetrics = NS_REINTERPRET_CAST(nsIFontMetricsGTK *, aFontMetrics);
  NS_IF_ADDREF(mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::SetLineStyle(nsLineStyle aLineStyle)
{
  if (aLineStyle != mCurrentLineStyle)
  {
    switch(aLineStyle)
    { 
      case nsLineStyle_kSolid:
        {
          mLineStyle = GDK_LINE_SOLID;
          mDashes = 0;
          /*          ::gdk_gc_set_line_attributes(mSurface->GetGC(),
                                       1,
                                       GDK_LINE_SOLID,
                                       (GdkCapStyle)0,
                                       (GdkJoinStyle)0);
          */
        }
        break;

      case nsLineStyle_kDashed:
        {
          mLineStyle = GDK_LINE_ON_OFF_DASH;
          mDashList[0] = mDashList[1] = 4;
          mDashes = 2;

          /*          ::gdk_gc_set_dashes(mSurface->GetGC(), 
                      0, dashed, 2);
          */
        }
        break;

      case nsLineStyle_kDotted:
        {
          mDashList[0] = mDashList[1] = 1;
          mLineStyle = GDK_LINE_ON_OFF_DASH;
          mDashes = 2;

          /*          ::gdk_gc_set_dashes(mSurface->GetGC(), 
                      0, dotted, 2);
          */
        }
        break;

    default:
        break;

    }
    
    mCurrentLineStyle = aLineStyle ;
  }

  return NS_OK;

}

NS_IMETHODIMP nsRenderingContextGTK::GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mCurrentLineStyle;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  NS_IF_ADDREF(mFontMetrics);
  aFontMetrics = mFontMetrics;
  return NS_OK;
}

// add the passed in translation to the current translation
NS_IMETHODIMP nsRenderingContextGTK::Translate(nscoord aX, nscoord aY)
{
  mTranMatrix->AddTranslation((float)FROM_TWIPS(aX),(float)FROM_TWIPS(aY));
  return NS_OK;
}

// add the passed in scale to the current scale
NS_IMETHODIMP nsRenderingContextGTK::Scale(float aSx, float aSy)
{
  mTranMatrix->AddScale(aSx, aSy);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::GetCurrentTransform(nsTransform2D *&aTransform)
{
  aTransform = mTranMatrix;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::CreateDrawingSurface(const nsRect &aBounds,
                                                          PRUint32 aSurfFlags,
                                                          nsIDrawingSurface* &aSurface)
{
  if (nsnull == mSurface) {
    aSurface = nsnull;
    return NS_ERROR_FAILURE;
  }

  g_return_val_if_fail ((aBounds.width > 0) && (aBounds.height > 0), NS_ERROR_FAILURE);
 
  nsresult rv = NS_OK;
  nsDrawingSurfaceGTK *surf = new nsDrawingSurfaceGTK();

  if (surf)
  {
    NS_ADDREF(surf);
    PushState();
    mClipRegion = nsnull;
    UpdateGC();
    rv = surf->Init(mGC, aBounds.width, aBounds.height, aSurfFlags);
    PopState();
  } else {
    rv = NS_ERROR_FAILURE;
  }

  aSurface = surf;

  return rv;
}

NS_IMETHODIMP nsRenderingContextGTK::DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
  nsDrawingSurfaceGTK *surf = (nsDrawingSurfaceGTK *) aDS;

  g_return_val_if_fail ((surf != NULL), NS_ERROR_FAILURE);

  NS_IF_RELEASE(surf);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  nscoord x,y,x1,y1;
  nscoord diffX,diffY;

  x = FROM_TWIPS_INT(aX0);
  y = FROM_TWIPS_INT(aY0);
  x1 = FROM_TWIPS_INT(aX1);
  y1 = FROM_TWIPS_INT(aY1);

  mTranMatrix->TransformCoord(&x,&y);
  mTranMatrix->TransformCoord(&x1,&y1);

  diffX = x1-x;
  diffY = y1-y;

  if (0!=diffX) {
    diffX = (diffX>0?1:-1);
  }
  if (0!=diffY) {
    diffY = (diffY>0?1:-1);
  }

  UpdateGC();

  ::XDrawLine(mDisplay, GDK_DRAWABLE_XID(mSurface->GetDrawable()),
              GDK_GC_XGC(mGC),
              x,
              y,
              x1 - diffX,
              y1 - diffY);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  PRInt32  i;
  XPoint * xpoints;
  XPoint * thispoint;

  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = FROM_TWIPS_INT(aPoints[i].x);
    thispoint->y = FROM_TWIPS_INT(aPoints[i].y);
    mTranMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }

  UpdateGC();

  ::XDrawLines(mDisplay,
               GDK_DRAWABLE_XID(mSurface->GetDrawable()),
               GDK_GC_XGC(mGC),
               xpoints, aNumPoints, CoordModeOrigin);

  free((void *)xpoints);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawRect(const nsRect& aRect)
{
  return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextGTK::DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTranMatrix || nsnull == mSurface) {
    return NS_ERROR_FAILURE;
  }

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  g_return_val_if_fail ((mSurface->GetDrawable() != NULL) ||
                        (mGC != NULL), NS_ERROR_FAILURE);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  // After the transform, if the numbers are huge, chop them, because
  // they're going to be converted from 32 bit to 16 bit.
  // It's all way off the screen anyway.
  ConditionRect(x,y,w,h);

  // Don't draw empty rectangles; also, w/h are adjusted down by one
  // so that the right number of pixels are drawn.
  if (w && h) {

    UpdateGC();

    ::XDrawRectangle(mDisplay,
                     GDK_DRAWABLE_XID(mSurface->GetDrawable()),
                     GDK_GC_XGC(mGC),
                     x,
                     y,
                     w-1,
                     h-1);

  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::FillRect(const nsRect& aRect)
{
  return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextGTK::FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTranMatrix || nsnull == mSurface) {
    return NS_ERROR_FAILURE;
  }

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  // After the transform, if the numbers are huge, chop them, because
  // they're going to be converted from 32 bit to 16 bit.
  // It's all way off the screen anyway.
  ConditionRect(x,y,w,h);

  UpdateGC();

  if (!mDisplay)
    mDisplay = GDK_DISPLAY();

  ::XFillRectangle(mDisplay,
                   GDK_DRAWABLE_XID(mSurface->GetDrawable()),
                   GDK_GC_XGC(mGC),
                   x,y,w,h);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::InvertRect(const nsRect& aRect)
{
  return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextGTK::InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsnull == mTranMatrix || nsnull == mSurface) {
    return NS_ERROR_FAILURE;
  }

  // Back up the current color, and use GXxor against white to get a
  // visible result.
  nscolor backupColor = mCurrentColor;
  mCurrentColor = NS_RGB(255, 255, 255);
  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  // After the transform, if the numbers are huge, chop them, because
  // they're going to be converted from 32 bit to 16 bit.
  // It's all way off the screen anyway.
  ConditionRect(x,y,w,h);

  mFunction = GDK_XOR;

  UpdateGC();

  // Fill the rect
  ::XFillRectangle(mDisplay,
                   GDK_DRAWABLE_XID(mSurface->GetDrawable()),
                   GDK_GC_XGC(mGC),
                   x,y,w,h);

  // Back to normal copy drawing mode
  mFunction = GDK_COPY;

  // Restore current color
  mCurrentColor = backupColor;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  PRInt32 i ;
  XPoint * xpoints;
  XPoint * thispoint;

  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = FROM_TWIPS_INT(aPoints[i].x);
    thispoint->y = FROM_TWIPS_INT(aPoints[i].y);
    mTranMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }

  UpdateGC();

  ::XDrawLines(mDisplay,
               GDK_DRAWABLE_XID(mSurface->GetDrawable()),
               GDK_GC_XGC(mGC),
               xpoints, aNumPoints, CoordModeOrigin);

  free((void *)xpoints);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  PRInt32 i ;
  XPoint * xpoints;

  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);

  for (i = 0; i < aNumPoints; ++i) {
    nsPoint p(FROM_TWIPS_INT(aPoints[i].x), FROM_TWIPS_INT(aPoints[i].y));
    mTranMatrix->TransformCoord(&p.x, &p.y);
    xpoints[i].x = p.x;
    xpoints[i].y = p.y;
  }

  UpdateGC();

  ::XFillPolygon(mDisplay,
                 GDK_DRAWABLE_XID(mSurface->GetDrawable()),
                 GDK_GC_XGC(mGC),
                 xpoints, aNumPoints, Complex, CoordModeOrigin);

  free((void *)xpoints);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawEllipse(const nsRect& aRect)
{
  return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextGTK::DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  UpdateGC();

  ::XDrawArc(mDisplay,
             GDK_DRAWABLE_XID(mSurface->GetDrawable()),
             GDK_GC_XGC(mGC),
             x, y, w, h, 0, 360 * 64);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::FillEllipse(const nsRect& aRect)
{
  return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextGTK::FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  UpdateGC();

  if (w < 16 || h < 16) {
    /* Fix for bug 91816 ("bullets are not displayed correctly on certain text zooms")
     * De-uglify bullets on some X servers:
     * 1st: Draw... */
     ::XDrawArc(mDisplay,
               GDK_DRAWABLE_XID(mSurface->GetDrawable()),
               GDK_GC_XGC(mGC),
               x, y, w, h, 0, 360*64);
    /*  ...then fill. */
  }
  ::XFillArc(mDisplay,
             GDK_DRAWABLE_XID(mSurface->GetDrawable()),
             GDK_GC_XGC(mGC),
             x, y, w, h, 0, 360*64);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::DrawArc(const nsRect& aRect,
                                             float aStartAngle, float aEndAngle)
{
  return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

NS_IMETHODIMP nsRenderingContextGTK::DrawArc(nscoord aX, nscoord aY,
                                             nscoord aWidth, nscoord aHeight,
                                             float aStartAngle, float aEndAngle)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  UpdateGC();

  ::XDrawArc(mDisplay,
             GDK_DRAWABLE_XID(mSurface->GetDrawable()),
             GDK_GC_XGC(mGC),
             x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextGTK::FillArc(const nsRect& aRect,
                                             float aStartAngle, float aEndAngle)
{
  return FillArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}


NS_IMETHODIMP nsRenderingContextGTK::FillArc(nscoord aX, nscoord aY,
                                             nscoord aWidth, nscoord aHeight,
                                             float aStartAngle, float aEndAngle)
{
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

  nscoord x,y,w,h;

  x = FROM_TWIPS_INT(aX);
  y = FROM_TWIPS_INT(aY);
  w = FROM_TWIPS_INT(aWidth);
  h = FROM_TWIPS_INT(aHeight);

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  UpdateGC();

  ::XFillArc(mDisplay,
             GDK_DRAWABLE_XID(mSurface->GetDrawable()),
             GDK_GC_XGC(mGC),
             x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextGTK::GetWidth(char aC, nscoord &aWidth)
{
    // Check for the very common case of trying to get the width of a single
    // space.
  if ((aC == ' ') && (nsnull != mFontMetrics)) {
    return mFontMetrics->GetSpaceWidth(aWidth);
  }
  return GetWidth(&aC, 1, aWidth);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetWidth(PRUnichar aC, nscoord& aWidth,
                                PRInt32* aFontID)
{
  return GetWidth(&aC, 1, aWidth, aFontID);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetWidthInternal(const char* aString, PRUint32 aLength,
                                        nscoord& aWidth)
{
  if (0 == aLength) {
    aWidth = 0;
    return NS_OK;
  }

  g_return_val_if_fail(aString != NULL, NS_ERROR_FAILURE);

  return mFontMetrics->GetWidth(aString, aLength, aWidth, this);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetWidthInternal(const PRUnichar* aString, PRUint32 aLength,
                                        nscoord& aWidth, PRInt32* aFontID)
{
  if (0 == aLength) {
    aWidth = 0;
    return NS_OK;
  }

  g_return_val_if_fail(aString != NULL, NS_ERROR_FAILURE);

  return mFontMetrics->GetWidth(aString, aLength, aWidth, aFontID, this);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetTextDimensionsInternal(const char* aString, PRUint32 aLength,
                                                 nsTextDimensions& aDimensions)
{
  mFontMetrics->GetMaxAscent(aDimensions.ascent);
  mFontMetrics->GetMaxDescent(aDimensions.descent);
  return GetWidth(aString, aLength, aDimensions.width);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetTextDimensionsInternal(const PRUnichar* aString,
                                                 PRUint32 aLength,
                                                 nsTextDimensions& aDimensions, 
                                                 PRInt32* aFontID)
{
  return mFontMetrics->GetTextDimensions(aString, aLength, aDimensions,
                                         aFontID, this);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetTextDimensionsInternal(const char*       aString,
                                                 PRInt32           aLength,
                                                 PRInt32           aAvailWidth,
                                                 PRInt32*          aBreaks,
                                                 PRInt32           aNumBreaks,
                                                 nsTextDimensions& aDimensions,
                                                 PRInt32&          aNumCharsFit,
                                                 nsTextDimensions& aLastWordDimensions,
                                                 PRInt32*          aFontID)
{
  return mFontMetrics->GetTextDimensions(aString, aLength, aAvailWidth,
                                         aBreaks, aNumBreaks, aDimensions,
                                         aNumCharsFit,
                                         aLastWordDimensions, aFontID,
                                         this);
}
NS_IMETHODIMP
nsRenderingContextGTK::GetTextDimensionsInternal(const PRUnichar*  aString,
                                                 PRInt32           aLength,
                                                 PRInt32           aAvailWidth,
                                                 PRInt32*          aBreaks,
                                                 PRInt32           aNumBreaks,
                                                 nsTextDimensions& aDimensions,
                                                 PRInt32&          aNumCharsFit,
                                                 nsTextDimensions& aLastWordDimensions,
                                                 PRInt32*          aFontID)
{
  return mFontMetrics->GetTextDimensions(aString, aLength, aAvailWidth,
                                         aBreaks, aNumBreaks, aDimensions,
                                         aNumCharsFit,
                                         aLastWordDimensions, aFontID,
                                         this);
}

NS_IMETHODIMP
nsRenderingContextGTK::DrawStringInternal(const char *aString, PRUint32 aLength,
                                          nscoord aX, nscoord aY,
                                          const nscoord* aSpacing)
{
  return mFontMetrics->DrawString(aString, aLength, aX, aY, aSpacing,
                                  this, mSurface);
}

NS_IMETHODIMP
nsRenderingContextGTK::DrawStringInternal(const PRUnichar* aString, PRUint32 aLength,
                                          nscoord aX, nscoord aY,
                                          PRInt32 aFontID,
                                          const nscoord* aSpacing)
{
  return mFontMetrics->DrawString(aString, aLength, aX, aY, aFontID,
                                  aSpacing, this, mSurface);
}

NS_IMETHODIMP
nsRenderingContextGTK::CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                         const nsRect &aDestBounds,
                                         PRUint32 aCopyFlags)
{
  PRInt32               srcX = aSrcX;
  PRInt32               srcY = aSrcY;
  nsRect                drect = aDestBounds;
  nsDrawingSurfaceGTK  *destsurf;

  g_return_val_if_fail(aSrcSurf != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mTranMatrix != NULL, NS_ERROR_FAILURE);
  g_return_val_if_fail(mSurface != NULL, NS_ERROR_FAILURE);

#if 0
  printf("nsRenderingContextGTK::CopyOffScreenBits()\nflags=\n");

  if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
    printf("NS_COPYBITS_USE_SOURCE_CLIP_REGION\n");

  if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
    printf("NS_COPYBITS_XFORM_SOURCE_VALUES\n");

  if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
    printf("NS_COPYBITS_XFORM_DEST_VALUES\n");

  if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
    printf("NS_COPYBITS_TO_BACK_BUFFER\n");

  printf("\n");
#endif

  if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
  {
    NS_ASSERTION(!(nsnull == mSurface), "no back buffer");
    destsurf = mSurface;
  }
  else
  {
    NS_ENSURE_TRUE(mOffscreenSurface != nsnull, NS_ERROR_FAILURE);
    destsurf = mOffscreenSurface;
  }

  if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
    mTranMatrix->TransformCoord(&srcX, &srcY);

  if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
    mTranMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

#if 0
  // XXX implement me
  if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
  {
    // we should use the source clip region if this flag is used...
    nsIRegion *region;
    CopyClipRegion();
  }
#endif

  //XXX flags are unused. that would seem to mean that there is
  //inefficiency somewhere... MMP

  // gdk_draw_pixmap and copy_area do the same thing internally.
  // copy_area sounds better

  UpdateGC();

  ::gdk_window_copy_area(destsurf->GetDrawable(),
                         mGC,
                         drect.x, drect.y,
                         ((nsDrawingSurfaceGTK *)aSrcSurf)->GetDrawable(),
                         srcX, srcY,
                         drect.width, drect.height);
                     

  return NS_OK;
}

#ifdef MOZ_MATHML

NS_IMETHODIMP
nsRenderingContextGTK::GetBoundingMetricsInternal(const char*        aString, 
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics)
{
  return mFontMetrics->GetBoundingMetrics(aString, aLength, aBoundingMetrics,
                                          this);
}

NS_IMETHODIMP
nsRenderingContextGTK::GetBoundingMetricsInternal(const PRUnichar*   aString, 
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics,
                                                  PRInt32*           aFontID)
{
  return mFontMetrics->GetBoundingMetrics(aString, aLength, aBoundingMetrics,
                                          aFontID, this);
}

#endif /* MOZ_MATHML */

NS_IMETHODIMP nsRenderingContextGTK::SetRightToLeftText(PRBool aIsRTL)
{
  return mFontMetrics->SetRightToLeftText(aIsRTL);
}

NS_IMETHODIMP nsRenderingContextGTK::GetRightToLeftText(PRBool* aIsRTL)
{
  *aIsRTL = mFontMetrics->GetRightToLeftText();
  return NS_OK;
}

PRInt32 nsRenderingContextGTK::GetMaxStringLength()
{
  if (!mFontMetrics)
    return 1;
  return mFontMetrics->GetMaxStringLength();
}

NS_IMETHODIMP nsRenderingContextGTK::GetClusterInfo(const PRUnichar *aText,
                                                    PRUint32 aLength,
                                                    PRUint8 *aClusterStarts)
{
  return mFontMetrics->GetClusterInfo(aText, aLength, aClusterStarts);
}

PRInt32 nsRenderingContextGTK::GetPosition(const PRUnichar *aText, PRUint32 aLength,
                                           nsPoint aPt)
{
  return mFontMetrics->GetPosition(aText, aLength, aPt);
}

NS_IMETHODIMP nsRenderingContextGTK::GetRangeWidth(const PRUnichar *aText, PRUint32 aLength,
                                                   PRUint32 aStart, PRUint32 aEnd,
                                                   PRUint32 &aWidth)
{
  return mFontMetrics->GetRangeWidth(aText, aLength, aStart, aEnd, aWidth);
}

NS_IMETHODIMP nsRenderingContextGTK::GetRangeWidth(const char *aText, PRUint32 aLength,
                                                   PRUint32 aStart, PRUint32 aEnd,
                                                   PRUint32 &aWidth)
{
  return mFontMetrics->GetRangeWidth(aText, aLength, aStart, aEnd, aWidth);
}

NS_IMETHODIMP nsRenderingContextGTK::DrawImage(imgIContainer *aImage, const nsRect & twSrcRect, const nsRect & twDestRect)
{
  UpdateGC();
  nsRect aDestRect = NS_RECT_FROM_TWIPS_RECT(twDestRect);
  #define NS_RECT_FROM_TWIPS_RECT2(_r)   (nsRect(FROM_TWIPS_INT2((_r).x), FROM_TWIPS_INT2((_r).y), FROM_TWIPS_INT2((_r).width), FROM_TWIPS_INT2((_r).height)))
  nsRect aSrcRect = NS_RECT_FROM_TWIPS_RECT2(twSrcRect);
  //1,2,3... Some problems with images... stipes....;
  nsRect dr = aDestRect;
  mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);

  // We should NOT be transforming the source rect (which is based on the image
  // origin) using the rendering context's translation!
  // However, given that we are, remember that the transformation of a
  // height depends on the position, since what we are really doing is
  // transforming the edges.  So transform *with* a translation, based
  // on the origin of the *destination* rect, and then fix up the
  // origin.
  nsRect sr(aDestRect.TopLeft(), aSrcRect.Size());
  mTranMatrix->TransformCoord(&sr.x, &sr.y, &sr.width, &sr.height);

  if (sr.IsEmpty() || dr.IsEmpty())
    return NS_OK;

  sr.MoveTo(aSrcRect.TopLeft());
  mTranMatrix->TransformNoXLateCoord(&sr.x, &sr.y);

  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;

  nsIDrawingSurface *surface = nsnull;
  GetDrawingSurface(&surface);
  if (!surface) return NS_ERROR_FAILURE;

  // For Bug 87819
  // iframe may want image to start at different position, so adjust
  nsRect iframeRect;
  iframe->GetRect(iframeRect);

  if (iframeRect.x > 0) {
    // Adjust for the iframe offset before we do scaling.
    sr.x -= iframeRect.x;

    nscoord scaled_x = sr.x;
    if (dr.width != sr.width) {
      PRFloat64 scale_ratio = PRFloat64(dr.width) / PRFloat64(sr.width);
      scaled_x = NSToCoordRound(scaled_x * scale_ratio);
    }
    if (sr.x < 0) {
      dr.x -= scaled_x;
      sr.width += sr.x;
      dr.width += scaled_x;
      if (sr.width <= 0 || dr.width <= 0)
        return NS_OK;
      sr.x = 0;
    } else if (sr.x > iframeRect.width) {
      return NS_OK;
    }
  }

  if (iframeRect.y > 0) {
    // Adjust for the iframe offset before we do scaling.
    sr.y -= iframeRect.y;

    nscoord scaled_y = sr.y;
    if (dr.height != sr.height) {
      PRFloat64 scale_ratio = PRFloat64(dr.height) / PRFloat64(sr.height);
      scaled_y = NSToCoordRound(scaled_y * scale_ratio);
    }
    if (sr.y < 0) {
      dr.y -= scaled_y;
      sr.height += sr.y;
      dr.height += scaled_y;
      if (sr.height <= 0 || dr.height <= 0)
        return NS_OK;
      sr.y = 0;
    } else if (sr.y > iframeRect.height) {
      return NS_OK;
    }
  }

  // Multiple paint rects may have been coalesced into a bounding box, so
  // ensure that this rect is actually within the clip region before we draw.
  nsCOMPtr<nsIRegion> clipRegion;
  GetClipRegion(getter_AddRefs(clipRegion));
  if (clipRegion && !clipRegion->ContainsRect(dr.x, dr.y, dr.width, dr.height))
    return NS_OK;

  return img->Draw(*this, surface, sr.x, sr.y, sr.width, sr.height,
                   dr.x, dr.y, dr.width, dr.height);

}

NS_IMETHODIMP nsRenderingContextGTK::GetBackbuffer(const nsRect &aRequestedSize,
                                                   const nsRect &aMaxSize,
                                                   PRBool aForBlending,
                                                   nsIDrawingSurface* &aBackbuffer)
{
  // Do not cache the backbuffer. On GTK it is more efficient to allocate
  // the backbuffer as needed and it doesn't cause a performance hit. @see bug 95952
  return AllocateBackbuffer(aRequestedSize, aMaxSize, aBackbuffer, PR_FALSE, 0);
}
 
NS_IMETHODIMP nsRenderingContextGTK::ReleaseBackbuffer(void) {
  // Do not cache the backbuffer. On GTK it is more efficient to allocate
  // the backbuffer as needed and it doesn't cause a performance hit. @see bug 95952
  return DestroyCachedBackbuffer();
}

NS_IMETHODIMP
nsRenderingContextGTK::DrawTile(imgIContainer *aImage,
                                nscoord aXImageStart, nscoord aYImageStart,
                                const nsRect *aTargetRect)
{
  nsRect dr(NS_RECT_FROM_TWIPS_RECT(*aTargetRect));
  aXImageStart = FROM_TWIPS_INT(aXImageStart);
  aYImageStart = FROM_TWIPS_INT(aYImageStart);
  mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);
  mTranMatrix->TransformCoord(&aXImageStart, &aYImageStart);

  // may have become empty due to transform shinking small number to 0
  if (dr.IsEmpty())
    return NS_OK;

  nscoord width, height;
  aImage->GetWidth(&width);
  aImage->GetHeight(&height);

  if (width == 0 || height == 0)
    return NS_OK;

  nscoord xOffset = (dr.x - aXImageStart) % width;
  nscoord yOffset = (dr.y - aYImageStart) % height;

  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;

  nsIDrawingSurface *surface = nsnull;
  GetDrawingSurface(&surface);
  if (!surface) return NS_ERROR_FAILURE;

  /* bug 113561 - frame can be smaller than container */
  nsRect iframeRect;
  iframe->GetRect(iframeRect);
  PRInt32 padx = width - iframeRect.width;
  PRInt32 pady = height - iframeRect.height;

  return img->DrawTile(*this, surface,
                       xOffset - iframeRect.x, yOffset - iframeRect.y,
                       padx, pady,
                       dr);

}


