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

#include "membufWindow.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"
#include "nsIRegion.h"
#include "plevent.h"
#include "prlog.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"

#ifdef WIN32
#include <windows.h>
#endif

#define MEMBUF_DEBUG 1
//#undef MEMBUF_DEBUG

#define MEMBUF_NATIVE_BITMAP               7700
#define MEMBUF_NATIVE_BITMAP_PARENT        7701
#define MEMBUF_NATIVE_BITMAP_DIRTY_FLAG    7702
#define MEMBUF_NATIVE_BITMAP_REFCOUNT      7703

// Allegro looks for this as an external symbol, even though we don't
// use it.
extern "C" void* _mangled_main_address;
void* _mangled_main_address;

bool membufWindow::sNeed_install_allegro = true;

//static NS_DEFINE_IID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

NS_IMPL_ISUPPORTS_INHERITED0(membufWindow, nsBaseWidget)

membufWindow::membufWindow() : mParentBitmap(0),
                               mBitmap(0),
                               bitmap_refcount(0),
                               mEnabled(PR_FALSE),
                               mIsVisible(PR_TRUE),
                               mUpdateQueued(PR_FALSE)
{
    printf("membufWindow[%x]::mebufWindow()\n", this);
    hasFocus = false;
    mBounds = nsRect(0, 0, 0, 0);

    mUpdateRegion = do_CreateInstance(kRegionCID);

    // First time we need to call Allegro's initialization function
    if(sNeed_install_allegro) {
#ifdef MEMBUF_DEBUG
        printf("setting up allegro\n");
#endif
        install_allegro(SYSTEM_NONE, &errno, atexit);
        sNeed_install_allegro = false;
    }
}

membufWindow::~membufWindow()
{
    printf("membufWindow[%x]::~mebufWindow()\n", this);
}

//////////////////////////////////////////////
//// nsIWidget

/** So the basic idea here is that because Mozilla implements its own
 * toolkit via XUL, we can (and do) get away with having practically
 * no "toolkit" logic ourselves.  Basically everything operates on
 * Allegro "bitmaps" (the BITMAP structure); this is the actual buffer
 * that we render onto.  One nice feature about Allegro is that it
 * supports sub-bitmaps, that is, a bitmap that is actualy mapped onto
 * another bitmap.  This allows us to "nest" a bitmap inside a larger
 * bitmap, although only one level deep (not recursively).  We will
 * use this heavily, because it is close enough to the notion of
 * parent/child windows in a real toolkit that we can get by.
 *
 * While nsIWidget Windows can have multiple children we get around
 *   that issue by holding the parent bitmap of the top level Widget
 *   as the parent bitmap and not our direct Widget parent's bitmap.
 *   This way all sub_bitmaps are subs of the top level bitmap and 
 *   there is only one level of indirection.
 */

NS_IMETHODIMP
membufWindow::Create( nsIWidget        *aParent,
                      const nsRect     &aRect,
                      EVENT_CALLBACK   aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell      *aAppShell,
                      nsIToolkit       *aToolkit,
                      nsWidgetInitData *aInitData )
{
    printf("membufWindow[%x]::Create(1): aParentWidget:[%x]\n",this,aParent);
    // Call base constructor and set up the bitmap

    // no parent if we're a dialog or toplevel window
    nsIWidget *baseParent = aInitData &&
                 (aInitData->mWindowType == eWindowType_dialog ||
                  aInitData->mWindowType == eWindowType_toplevel) ?
                  nsnull : aParent;

    BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
               aAppShell, aToolkit, aInitData);

    mBounds = aRect;

    mParent = baseParent;

    createBitmap();

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::Create( nsNativeWidget aParent,
                      const nsRect     &aRect,
                      EVENT_CALLBACK   aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell      *aAppShell,
                      nsIToolkit       *aToolkit,
                      nsWidgetInitData *aInitData )
{
    printf("membufWindow[%x]::Create(2): nativeWindow[%x]\n",this,aParent);
    // Call base constructor and set up the bitmap given a "native"
    // parent widget (which is a parent bitmap).

    BaseCreate(nsnull, aRect, aHandleEventFunction, aContext,
               aAppShell, aToolkit, aInitData);

    mBounds = aRect;

    // If a bitmap was sent in it becomes the Parent Bitmap of this
    //   window. 
    if(mBounds.width > 0 && mBounds.height > 0) {
        // we were asked to make a non-emtpy window.
        // one creation method passes in -1 so check for > 0
        if((int)aParent > 0) 
            mParentBitmap = (BITMAP*)aParent;

        if(mParentBitmap) {
            // there is a parent bitmap so lets make a subbitmap
            if(!is_sub_bitmap(mParentBitmap)) {
                // create a subbitmap from the parent bitmap
                mBitmap = create_sub_bitmap(mParentBitmap,
                                            mBounds.x,
                                            mBounds.y,
                                            mBounds.width,
                                            mBounds.height);
#ifdef MEMBUF_DEBUG
                printf("[>> creating subbitmap %x (%i %i) with parent %x\n", mBitmap, aRect.width, aRect.height, aParent);
#endif
                bitmap_refcount = new int;
                *bitmap_refcount = 1;
            } else {
                // Hmmm... the parent passed in was created as a sub, that
                //   won't work with Allegro. Can't create a subbitmap of
                //   a subbitmap.
#ifdef MEMBUF_DEBUG
                printf(">>> Yoiks! I was asked to create a bitmap from native widget that was a sub-bitmap.  I don't know how to do that.\n");
#endif
            }
        } else {
            // there isn't a parent bitmap so we'll create a regular
            //   bitmap and set it as our mBitmap. This should really
            //   only happen once per browser window/tab
            mBitmap = create_bitmap_ex(32, mBounds.width, mBounds.height);
#ifdef MEMBUF_DEBUG
            printf("[>> creating non-sub bitmap %x (%i %i)\n", mBitmap, aRect.width, aRect.height);
#endif
            bitmap_refcount = new int;
            *bitmap_refcount = 1;
        }
#ifdef MEMBUF_DEBUG
        printf(">>> did create bitmap %x <<]\n", mBitmap);
#endif
    }

    return NS_OK;
}

void membufWindow::calculateAbsolutePosition(int& total_offset_x, int& total_offset_y)
{
    total_offset_x = mBounds.x;
    total_offset_y = mBounds.y;

    for(nsIWidget* par = mParent; par->GetParent(); par = par->GetParent()) {
        nsRect r;
        par->GetBounds(r);
        total_offset_x += r.x;
        total_offset_y += r.y;
    }
}

void membufWindow::createBitmap()
{
    printf("membufWindow[%x]::createBitmap()\n",this);
    // Standard logic to create a bitmap.  We don't do anything if
    // asked to create a 0x0 bitmap (I think Allegro will crash in
    // this case) If this window has a parent, we fetch the toplevel
    // bitmap (recall that allegro bitmaps can only be nested one
    // level deep), compute the correct offset, and create the
    // sub-bitmap.  If there is no parent, create a new 32bpp bitmap
    // with the desired dimensions.

    // bitmap_refcount is a slightly hackish reference count made
    // available to the calling application so as to avoid the widget
    // deleting the bitmap at an inconvenient time

    if(mBounds.width > 0 && mBounds.height > 0) {
        // we were asked to create a non-empty bitmap
        if(mParent) {
            int total_offset_x = 0;
            int total_offset_y = 0;

            calculateAbsolutePosition(total_offset_x, total_offset_y);
            mParentBitmap = (BITMAP*)mParent->GetNativeData(
                                              MEMBUF_NATIVE_BITMAP_PARENT);
#ifdef MEMBUF_DEBUG
            printf("absolute position of subbitmap is %i %i\n", total_offset_x, total_offset_y);
#endif
            printf(" BITMAP retrieved mParentBitmap: %x\n", mParentBitmap);

            mBitmap = create_sub_bitmap(mParentBitmap,
                                        total_offset_x, total_offset_y,
                                        mBounds.width, mBounds.height);
#ifdef MEMBUF_DEBUG
            printf("[>> creating subbbitmap %x (%i %i) with parent %x\n", mBitmap, mBounds.width, mBounds.height, mParentBitmap);
#endif
            printf("SUB_BITMAP created!!!  mBitmap: %x\n", mBitmap);
            bitmap_refcount = new int;
            *bitmap_refcount = 1;
        } else {
            if (mParentBitmap) {
                // need to special case the top-level widget. In order to
                //   be able to pass in a bitmap that is the ultimate
                //   top-level bitmap. In this case there is no parent
                //   widget but we have the ParentBitmap set due to having
                //   it passed in by the nsWebBrowser.

                // just point to the parent
                mBitmap = mParentBitmap;
                // increment the refcount so that if we resize the mBitmap
                //   we don't actually destroy the mParentBitmap.
                *bitmap_refcount++;
            } else {
                mBitmap = create_bitmap_ex(32, mBounds.width, mBounds.height);
#ifdef MEMBUF_DEBUG
                printf("[>> creating bitmap %x (%i %i)\n", mBitmap, mBounds.width, mBounds.height);
#endif
                bitmap_refcount = new int;
                *bitmap_refcount = 1;
                printf("Non-sub BITMAP created!!!  mBitmap: %x\n", mBitmap);
            }
        }
#ifdef MEMBUF_DEBUG
        printf(">>> did create bitmap %x <<]\n", mBitmap);
#endif
    } else {
#ifdef MEMBUF_DEBUG
        printf("[>> created (0 0) window <<]\n");
#endif
        printf("XXXjgaunt - asked to create 0x0 BITMAP mBounds is 0.\n");
        mBitmap = 0;
        bitmap_refcount = 0;
    }
}

NS_IMETHODIMP
membufWindow::Destroy(void)
{
    printf("membufWindow[%x]::Destroy()\n",this);
    nsBaseWidget::Destroy();
    mParent = nsnull;
    mEventCallback = nsnull;

    destroy_bitmap(mBitmap);

#ifdef MEMBUF_DEBUG
    printf("Destroyed window\n");
#endif

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::SetParent(nsIWidget* aNewParent)
{
  printf("membufWindow::SetParent()\n");
  printf("    **************** PROBABLY NOT GOOD *******************\n");
#ifdef MEMBUF_DEBUG
    printf("<><> Parent was changed for window with bitmap %x (parent bitmap %x)\n", mBitmap, mParentBitmap);
#endif
    mParent = aNewParent;

    return NS_OK;
}

nsIWidget*
membufWindow::GetParent(void)
{
    nsIWidget *ret;
    ret = mParent;
    NS_IF_ADDREF(ret);
    return ret;
}

// User interaction actually works fairly well if you paint the membuf
// window on screen and accept input -- quite a testament to the
// strength of XUL as a standalone toolkit.  Accordingly, methods
// involving visibility and repaints like Show(), Invalidate() etc are
// actually correct for interactive use and not just offline rendering.

NS_IMETHODIMP
membufWindow::Show(PRBool aState)
{
    printf("membufWindow[%x]::Show(%d)\n",this,aState);
    // invalidate ourself if we are visible, otherwise walk
    //   up to the toplevel parent and tell it to invalidate
    mIsVisible = aState;
    if(aState)
        Invalidate(PR_FALSE);
    else {
        nsIWidget* par = this;
        while(par->GetParent())
            par = par->GetParent();
        if(par == this) {
            printf("  we are the toplevel parent - falling through\n");
            //return NS_OK;
        }
        else
            par->Invalidate(PR_FALSE);
    }

#ifdef MEMBUF_DEBUG
    printf("Set window show state is %i\n", aState);
#endif

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::SetModal(PRBool aModal)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::IsVisible(PRBool & aState)
{
    aState = mIsVisible;
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::ConstrainPosition(PRBool aAllowSlop,
                              PRInt32 *aX,
                              PRInt32 *aY)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::Move(PRInt32 aX, PRInt32 aY)
{
    return Resize(aX, aY, mBounds.width, mBounds.height, 0);
}

NS_IMETHODIMP
membufWindow::Resize(PRInt32 aWidth,
                   PRInt32 aHeight,
                   PRBool  aRepaint)
{
    return Resize(mBounds.x, mBounds.y, aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP
membufWindow::Resize(PRInt32 aX,
                     PRInt32 aY,
                     PRInt32 aWidth,
                     PRInt32 aHeight,
                     PRBool aRepaint)
{
// Reszing the window requires deleting the old bitmap and creating a
// new one of the desired size.
   printf("membufWindow[%x]::Resize(x:%d,y:%d,w:%d,h:%d,repaint:%d)\n",this,aX,aY,aWidth,aHeight,aRepaint);

    if((!mParent ||
        (void*)mParent->GetNativeData(MEMBUF_NATIVE_BITMAP_PARENT) ==
        (void*)mParentBitmap)
       && mBounds.x == aX
       && mBounds.y == aY
       && mBounds.width == aWidth
       && mBounds.height == aHeight)
    {
        // don't resize the toplevel window (!mParent)
        // don't resize if the mParentBitmap is the same as the mParent's
        //          parent Bitmap (the toplevel bitmap)
        // don't resize if there is no change in size
        // asked to resize to the current size -- no need for that!
        printf("  falling through resize()\n");
        return NS_OK;
    }

    mBounds.x = aX;
    mBounds.y = aY;
    mBounds.width = aWidth;
    mBounds.height = aHeight;

#ifdef MEMBUF_DEBUG
    printf("(// resizing bitmap (was %x) to size (%i, %i)\n", mBitmap, aWidth, aHeight);
    PR_ASSERT(aHeight < 100000);
#endif

    BITMAP* oldbitmap = mBitmap;
    int* oldbitmap_refcount = bitmap_refcount;
    createBitmap();
    printf("membufWindow::Resize() - back from createBitmap\n");

    for(nsIWidget* wid = GetFirstChild();
        wid != 0;
        wid = wid->GetNextSibling())
    {
        nsRect r;
        wid->GetBounds(r);
#ifdef MEMBUF_DEBUG
        printf("// has child widget with native %x\n", wid->GetNativeData(MEMBUF_NATIVE_BITMAP));
#endif
        wid->Resize(r.x, r.y, r.width, r.height, 0);
#ifdef MEMBUF_DEBUG
        printf("// did resize on child, now %x\n", wid->GetNativeData(MEMBUF_NATIVE_BITMAP));
#endif
    }
#ifdef MEMBUF_DEBUG
    printf("//)\n");
#endif

    if(oldbitmap && mBitmap != oldbitmap && oldbitmap_refcount) {
        (*oldbitmap_refcount)--;
        if((*oldbitmap_refcount) == 0) {
            printf("XXXjgaunt - DESTROYING A BITMAP IN RESIZE!!!\n");
            printf("   - mBitmap       [%x]\n", mBitmap);
            printf("   - mParentBitmap [%x]\n", mParentBitmap);
            printf("   - oldBitmap     [%x]\n", oldbitmap);
            printf("   - membufWindow  [%x]\n", this);
            printf("XXXjgaunt - DESTROYING A BITMAP IN RESIZE!!!\n");
            destroy_bitmap(oldbitmap);
            delete oldbitmap_refcount;
        }
    }

#if 0
    nsSizeEvent sevent;
    nsRect sevent_windowSize(0, 0, aWidth, aHeight);
    sevent.message = NS_SIZE;
    sevent.widget = this;
    sevent.eventStructType = NS_SIZE_EVENT;
    sevent.windowSize = &sevent_windowSize;
    sevent.point.x = 0;
    sevent.point.y = 0;
    sevent.mWinWidth = aWidth;
    sevent.mWinHeight = aHeight;
    sevent.time = PR_Now();

    nsEventStatus stat;
    DispatchEvent(&sevent, stat);
#endif

    if (aRepaint) 
      Update();

/*
    nsIWidget* par = this;
    while(par->GetParent()) par = par->GetParent();
    par->Update();
*/

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                          nsIWidget *aWidget, PRBool aActivate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::Enable(PRBool aState)
{
    mEnabled = aState;
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::IsEnabled(PRBool *aState)
{
    *aState = mEnabled;
    return NS_OK;
}

// Focus (mostly) works as well.

NS_IMETHODIMP
membufWindow::SetFocus(PRBool aRaise)
{
    if(!hasFocus) {
#ifdef MEMBUF_DEBUG
        printf("Set Focus %x\n", this);
#endif
        DispatchGotFocusEvent();
    }

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::LoseFocus()
{
    if(hasFocus) {
#ifdef MEMBUF_DEBUG
        printf("lost Focus %x\n", this);
#endif
        DispatchLostFocusEvent();
    }

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::GetBounds(nsRect &aRect)
{
    aRect = mBounds;
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::GetScreenBounds(nsRect &aRect)
{
    return GetBounds(aRect);
}

NS_IMETHODIMP
membufWindow::GetClientBounds(nsRect &aRect)
{
    return GetBounds(aRect);
}

NS_IMETHODIMP
membufWindow::GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight)
{
    aWidth = aHeight = 0;
    return NS_OK;
}

nsIFontMetrics* membufWindow::GetFont(void)
{
    return nsnull;
}

NS_IMETHODIMP
membufWindow::SetFont(const nsFont &aFont)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsCursor membufWindow::GetCursor(void)
{
    return mCursor;
}

NS_IMETHODIMP
membufWindow::Validate()
{
    mUpdateRegion->SetTo(0, 0, 0, 0);
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::Invalidate(PRBool aIsSynchronous)
{
    Invalidate(mBounds, aIsSynchronous);

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
    //printf("mbWin[%x]::Inval(w:%d,h:%d,s:%d)",
    //       this,aRect.width, aRect.height, aIsSynchronous);
    mUpdateRegion->Union(aRect.x, aRect.y, aRect.width, aRect.height);

    if (aIsSynchronous)
      Update();
    else
      QueueUpdate();

    //InvalidateHelper(aIsSynchronous);

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous)
{
    //printf("membufWindow[%x]::InvalidateRegion(reg, synch:%d)\n",
    //       this,aIsSynchronous);
    mUpdateRegion->Union(*aRegion);

    if (aIsSynchronous)
      Update();
    else
      QueueUpdate();

    //InvalidateHelper(aIsSynchronous);

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::Update()
{
    //printf("mbWin[%x]::Upd():",this);

    if(!mUpdateRegion->IsEmpty()) {
        printf("membufWindow[%x]::Update() - dirty: -- sending paint event\n",
                   this);

        nsCOMPtr<nsIRegion> updateRegion = mUpdateRegion;
        mUpdateRegion = do_CreateInstance(kRegionCID);
        if (mUpdateRegion) {
            mUpdateRegion->Init();
            mUpdateRegion->SetTo(0, 0, 0, 0);
        }
                                                                                
        DoPaint(updateRegion);

        // check to see if our children need updating, 
        InvalidateChildren(updateRegion, mUpdateQueued);
        mUpdateQueued = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::SetColorMap(nsColorMap *aColorMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
membufWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
// Implements efficient scrolling.  Viewport is moved over by the
// desired amount, and the edge region is invalidated.

    if(aDx == 0 && aDy == 0) return NS_OK;

    if(PR_ABS(aDx) > mBounds.width || PR_ABS(aDy) > mBounds.height) {
        Invalidate(PR_TRUE);
        return NS_OK;
    }

    printf("scrolling by %i %i\n", aDx, aDy);

    if(aDx >= 0) {
        if(aDy > 0) {
            int src_y = mBounds.height - aDy - 1;
            int dest_y = mBounds.height - 1;

            for(; src_y >= 0; src_y--, dest_y--) {
                int src_x = mBounds.width - aDx - 1;
                int dest_x = mBounds.width - 1;

                for(; src_x >= 0; src_x--, dest_x--) {
                    mBitmap->line[dest_y][dest_x*4] = mBitmap->line[src_y][src_x*4];
                    mBitmap->line[dest_y][dest_x*4+1] = mBitmap->line[src_y][src_x*4+1];
                    mBitmap->line[dest_y][dest_x*4+2] = mBitmap->line[src_y][src_x*4+2];
                    mBitmap->line[dest_y][dest_x*4+3] = mBitmap->line[src_y][src_x*4+3];
                }
            }
            if(aDy) Invalidate(nsRect(0, 0, mBounds.width, aDy), PR_TRUE);
            if(aDx) Invalidate(nsRect(0, 0, aDx, mBounds.height), PR_TRUE);
        } else {
            int src_y = -aDy;
            int dest_y = 0;

            for(; src_y < mBounds.height; src_y++, dest_y++) {
                int src_x = mBounds.width - aDx - 1;
                int dest_x = mBounds.width - 1;

                for(; src_x >= 0; src_x--, dest_x--) {
                    mBitmap->line[dest_y][dest_x*4] = mBitmap->line[src_y][src_x*4];
                    mBitmap->line[dest_y][dest_x*4+1] = mBitmap->line[src_y][src_x*4+1];
                    mBitmap->line[dest_y][dest_x*4+2] = mBitmap->line[src_y][src_x*4+2];
                    mBitmap->line[dest_y][dest_x*4+3] = mBitmap->line[src_y][src_x*4+3];
                }
            }
            if(aDy) Invalidate(nsRect(0, mBounds.height + aDy, mBounds.width, -aDy), PR_TRUE);
            if(aDx) Invalidate(nsRect(0, 0, aDx, mBounds.height), PR_TRUE);
        }
    } else {
        if(aDy > 0) {
            int src_y = mBounds.height - aDy - 1;
            int dest_y = mBounds.height - 1;

            for(; src_y >= 0; src_y--, dest_y--) {
                int src_x = -aDx;
                int dest_x = 0;

                for(; src_x < mBounds.width; src_x++, dest_x++) {
                    mBitmap->line[dest_y][dest_x*4] = mBitmap->line[src_y][src_x*4];
                    mBitmap->line[dest_y][dest_x*4+1] = mBitmap->line[src_y][src_x*4+1];
                    mBitmap->line[dest_y][dest_x*4+2] = mBitmap->line[src_y][src_x*4+2];
                    mBitmap->line[dest_y][dest_x*4+3] = mBitmap->line[src_y][src_x*4+3];
                }
            }
            if(aDy) Invalidate(nsRect(0, 0, mBounds.width, aDy), PR_TRUE);
            if(aDx) Invalidate(nsRect(mBounds.width + aDx, 0, -aDx, mBounds.height), PR_TRUE);
        } else {
            int src_y = -aDy;
            int dest_y = 0;

            for(; src_y < mBounds.height; src_y++, dest_y++) {
                int src_x = -aDx;
                int dest_x = 0;

                for(; src_x < mBounds.width; src_x++, dest_x++) {
                    mBitmap->line[dest_y][dest_x*4] = mBitmap->line[src_y][src_x*4];
                    mBitmap->line[dest_y][dest_x*4+1] = mBitmap->line[src_y][src_x*4+1];
                    mBitmap->line[dest_y][dest_x*4+2] = mBitmap->line[src_y][src_x*4+2];
                    mBitmap->line[dest_y][dest_x*4+3] = mBitmap->line[src_y][src_x*4+3];
                }
            }
            if(aDy) Invalidate(nsRect(0, mBounds.height + aDy, mBounds.width, -aDy), PR_TRUE);
            if(aDx) Invalidate(nsRect(mBounds.width + aDx, 0, -aDx, mBounds.height), PR_TRUE);
        }
    }

    //mDirty = true;

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::ScrollWidgets(PRInt32 aDx, PRInt32 aDy)
{
    return Scroll(aDx, aDy, nsnull);
}

NS_IMETHODIMP
membufWindow::ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy)
{
    return Scroll(aDx, aDy, nsnull);
}

// This is the hook by which the application can access the bitmap for
// this window, the topmost bitmap (the parent window if this is a
// sub-bitmap), the reference count (so that if a Resize() occurs the
// application doesn't find the bitmap it is reading from suddenly
// deleted) and a dirty flag which can be used by the application to
// efficiently check whether the underlying bitmap has changed and
// needs to be refreshed (if for example the application copies the
// bitmap buffer itself).

void* membufWindow::GetNativeData(PRUint32 aDataType)
{
    //printf("membufWindow[%x]::GetNativeData()",this);
    switch(aDataType) {
    case NS_NATIVE_WINDOW:
        /*
        if(mParentBitmap) {
          printf("  -NS_NAT_WIN: ret mParentBitmap:[%x]\n", mParentBitmap);
          return mParentBitmap;
        }
        else {
          printf("  -NS_NAT_WIN: ret mBitmap:[%x]\n", mBitmap);
          return mBitmap;
        }
        */
        //printf(" - NS_NAT_WIN: ret mBitmap:[%x]\n", mBitmap);
        return mBitmap;

    case NS_NATIVE_GRAPHIC:
        //printf(" - NS_NAT_GRAPHIC\n");
        if(mParentBitmap) return mParentBitmap;
        else return mBitmap;

    case NS_NATIVE_COLORMAP:
        return 0;

    case NS_NATIVE_WIDGET:
        /*
        if(mParentBitmap) {
          //printf("  -NS_NAT_WIDGET: ret mParentBitmap:[%x]\n", mParentBitmap);
          return mParentBitmap;
        }
        else {
          //printf("  -NS_NAT_WIDGET: ret mBitmap:[%x]\n", mBitmap);
          return mBitmap;
        }
        */
        //printf(" - NS_NAT_WIDGET: ret mBitmap:[%x]\n", mBitmap);
        return mBitmap;

    case NS_NATIVE_DISPLAY:
        return 0;

    case NS_NATIVE_REGION:
        return 0;

    case NS_NATIVE_OFFSETX:
        if(mBitmap) return &(mBitmap->x_ofs);
        else return 0;

    case NS_NATIVE_OFFSETY:
        if(mBitmap) return &(mBitmap->y_ofs);
        else return 0;

    case NS_NATIVE_PLUGIN_PORT:
        return 0;

    case NS_NATIVE_SCREEN:
        return 0;

    case MEMBUF_NATIVE_BITMAP:
        // take out visibility check for fun
        //if (mIsVisible) {
          if (mBitmap) {
            //printf(" - MB_NAT_BITMAP: ret mBitmap: %x\n", mBitmap);
            return mBitmap;
          }
          //printf(" - MB_NAT_BITMAP - null mBitmap !!!\n");
          return 0;
        /*
        }
        else {
          //printf(" - MB_NAT_BITMAP -- not visible\n");
          return 0;
        }
        */

    case MEMBUF_NATIVE_BITMAP_PARENT:
        if(mParentBitmap) {
          //printf(" - MB_NAT_PAR: ret mParentBitmap:[%x]\n", mParentBitmap);
          return mParentBitmap;
        }
        else {
          //printf(" - MB_NAT_PAR: ret mBitmap:[%x]\n", mBitmap);
          return mBitmap;
        }

    case MEMBUF_NATIVE_BITMAP_DIRTY_FLAG: {
        //printf("XXXjgaunt - MEMBUF_NATIVE_BITMAP_DIRTY_FLAG\n");
        nsIWidget* par = this;
        // walk up to the parent
        while (par->GetParent())
          par = par->GetParent();
        if (par == this) {
          if (mUpdateRegion->IsEmpty())
            mDirty = false;
          else
            mDirty = true;
          return &mDirty;
        }
        else
          return par->GetNativeData(MEMBUF_NATIVE_BITMAP_DIRTY_FLAG);
    }
    case MEMBUF_NATIVE_BITMAP_REFCOUNT:
        //printf("XXXjgaunt - MEMBUF_NATIVE_BITMAP_REFCOUNT\n");
        return bitmap_refcount;

    default:
        return 0;
    }
}
void membufWindow::FreeNativeData(void *data, PRUint32 aDataType)
{
    printf("membufWindow[%x]::FreeNativeData()\n",this);
}

NS_IMETHODIMP
membufWindow::SetTitle(const nsAString& aTitle)
{
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::SetIcon(const nsAString& anIconSpec)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
membufWindow::ShowMenuBar(PRBool aShow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::BeginResizingChildren(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::EndResizingChildren(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
    printf("XXXjgaunt - calling GetPrefferedSize\n");
    // XXXjgaunt these need to be implementation specific get set by the app
    aWidth = 800;
    aHeight = 600;
    return NS_OK;
}

NS_IMETHODIMP
membufWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
    printf("XXXjgaunt - calling SetPrefferedSize\n");
    // XXXjgaunt see comment above
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsIWidget* findWidgetAt(nsIWidget* start, int x, int y)
{
    for(nsIWidget* wid = start->GetFirstChild();
        wid != 0;
        wid = wid->GetNextSibling())
    {
        nsRect r;
        wid->GetBounds(r);

        if(x >= r.x && y >= r.y && x <= (r.x + r.width) && y <= (r.y + r.height))
        {
            return findWidgetAt(wid, x - r.x, y - r.y);
        }
    }

    return start;
}

NS_IMETHODIMP
membufWindow::DispatchEvent(nsGUIEvent* aEvent, nsEventStatus & aStatus)
{
    //printf("membufWindow[%x]::DispatchEvent()\n",this);
    aStatus = nsEventStatus_eIgnore;

    if(aEvent->eventStructType == NS_MOUSE_EVENT) {
        //nsMouseEvent* me = (nsMouseEvent*)aEvent;

        if(aEvent->message == NS_MOUSE_LEFT_CLICK) {
            if(!hasFocus) {
                DispatchActivateEvent();
                SetFocus(PR_TRUE);
            }
        }

        /*
        nsIWidget* wid = findWidgetAt(this, me->point.x, me->point.y);
        nsRect r;
        wid->GetBounds(r);
        //printf("inside widget %x with bounds %i %i %i %i\n", wid, r.x, r.y, r.width, r.height);
        */
    }

    // hold a widget reference while we dispatch this event
    NS_ADDREF(aEvent->widget);

    // send it to the standard callback
    if (mEventCallback)
        aStatus = (* mEventCallback)(aEvent);

    // dispatch to event listener if event was not consumed
    if ((aStatus != nsEventStatus_eIgnore) && mEventListener)
        aStatus = mEventListener->ProcessEvent(*aEvent);

    NS_IF_RELEASE(aEvent->widget);

    return NS_OK;
}

NS_IMETHODIMP
membufWindow::EnableDragDrop(PRBool aEnable)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

void
membufWindow::ConvertToDeviceCoordinates(nscoord &aX,nscoord &aY)
{
}

NS_IMETHODIMP
membufWindow::CaptureMouse(PRBool aCapture)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::GetWindowClass(char *aClass)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::SetWindowClass(char *aClass)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::ModalEventFilter(PRBool aRealEvent, void *aEvent, PRBool *aForWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
membufWindow::GetAttention(PRInt32 aCycleCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////
//// membufWindow protected

struct AsyncUpdateEvent
{
    PLEvent e;
    membufWindow* window;

    static void* handle(AsyncUpdateEvent*);
    static void destroy(AsyncUpdateEvent*);
};

void* AsyncUpdateEvent::handle(AsyncUpdateEvent* e)
{
    //printf("AsyncUpdateEvent::handle() -- calling Update()\n");
    e->window->Update();
    return 0;
}

void AsyncUpdateEvent::destroy(AsyncUpdateEvent* e)
{
    delete e;
}

/*
nsresult membufWindow::InvalidateHelper(PRBool aIsSynchronous)
{ 
*/

nsresult
membufWindow::QueueUpdate()
{
    // if we haven't already posted an event into the queue
    if(!mUpdateQueued) {
        nsresult rv;

        // Copy the dirty region to update.
        nsCOMPtr<nsIRegion> updateRegion(do_CreateInstance(kRegionCID));
        updateRegion->SetTo(*mUpdateRegion.get());

        // stick it in the event queue
        nsCOMPtr<nsIEventQueueService> eventQService =
                                  do_GetService(kEventQueueServiceCID, &rv);

        if (NS_FAILED(rv)) {
            NS_WARNING("Could not obtain event queue service");
            return rv;
        }

        nsCOMPtr<nsIEventQueue> eventQueue;
        rv = eventQService->GetThreadEventQueue(NS_CURRENT_THREAD,
                                                getter_AddRefs(eventQueue));
        if (NS_FAILED(rv)) {
            NS_WARNING("Could not get the thread event queue");
            return rv;
        }

        AsyncUpdateEvent* aue = new AsyncUpdateEvent;
        PL_InitEvent((PLEvent*)aue,
                     0,
                     (PLHandleEventProc)AsyncUpdateEvent::handle,
                     (PLDestroyEventProc)AsyncUpdateEvent::destroy);
        aue->window = this;
        eventQueue->PostEvent((PLEvent*)aue);
        mUpdateQueued = PR_TRUE;
    }
    return NS_OK;
}

nsresult
membufWindow::InvalidateChildren(nsIRegion *aRegion, PRBool aIsSync)
{
    nsIWidget *child = GetFirstChild();
    for( child; child != 0; child = child->GetNextSibling() ) {
        PRBool vis;
        child->IsVisible(vis);
        if (vis) {
            nsRect rec;
            child->GetBounds(rec);
            if(aRegion->ContainsRect(rec.x, rec.y, rec.width, rec.height)) {
                printf("  - invalidating child[%x]\n", child);
                nsRect bbox;
                aRegion->GetBoundingBox(&bbox.x,
                                        &bbox.y,
                                        &bbox.width,
                                        &bbox.height);
                child->Invalidate(bbox, aIsSync);
            }
        }
    }
    return NS_OK;
}

nsresult
membufWindow::DoPaint(nsIRegion *aPaintRegion)
{
    printf("membufWindow[%x]::DoPaint()\n",this);
    nsCOMPtr<nsIRenderingContext> rc = getter_AddRefs(GetRenderingContext());
    if (!rc)
        return NS_ERROR_FAILURE;

    nsRect boundsRect;
    aPaintRegion->GetBoundingBox(&boundsRect.x,
                                 &boundsRect.y,
                                 &boundsRect.width,
                                 &boundsRect.height);
                                                                                
    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    event.renderingContext = rc;
    event.point.x = boundsRect.x;
    event.point.y = boundsRect.y;
    event.rect = &boundsRect;
    event.widget = this;
#ifdef WIN32
    event.time = ::GetMessageTime();
#else
    event.time = PR_Now();
#endif

    // Dispatch the event to our callback (the view)
    nsEventStatus stat;
    DispatchEvent(&event, stat);

    return NS_OK;
}

nsresult
membufWindow::Paint(nsIRenderingContext& aRenderingContext,
                    const nsRect& aBounds)
{
    printf("XXXjgaunt - PAINTING!!!! ****************************\n");
    printf("membufWindow[%x]::Paint()\n",this);
    // Create a paint event
    nsPaintEvent* event = new nsPaintEvent(PR_TRUE, NS_PAINT, this);
#ifdef WIN32
    event->time = ::GetMessageTime();
#else
    event->time = PR_Now();
#endif
    nsRect boundsRect;
    mUpdateRegion->GetBoundingBox(&boundsRect.x, &boundsRect.y, &boundsRect.width, &boundsRect.height);

    event->message = NS_PAINT;
    event->eventStructType = NS_PAINT_EVENT;
    event->point.x = aBounds.x;
    event->point.y = aBounds.y;
    event->rect = const_cast<nsRect*>(&aBounds);
    event->region = nsnull;
    event->widget = this;
    NS_ADDREF(event->widget);

    event->renderingContext = &aRenderingContext;

    // Dispatch the event to our callback (the view)
    nsEventStatus stat;
    DispatchEvent(event, stat);
    NS_RELEASE(event->renderingContext);
    delete event;

    return NS_OK;
}


NS_IMETHODIMP membufWindow::AddMouseListener(nsIMouseListener * aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP membufWindow::AddEventListener(nsIEventListener * aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP membufWindow::AddMenuListener(nsIMenuListener * aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

void membufWindow::InitGUIEvent(nsGUIEvent &aEvent, PRUint32 aMsg)
{
    memset(&aEvent, 0, sizeof(nsGUIEvent));
    aEvent.eventStructType = NS_GUI_EVENT;
    aEvent.message = aMsg;
    aEvent.widget = NS_STATIC_CAST(nsIWidget *, this);
}

void membufWindow::DispatchGotFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_GOTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void membufWindow::DispatchLostFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_LOSTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void membufWindow::DispatchActivateEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_ACTIVATE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}
