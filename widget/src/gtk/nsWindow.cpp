/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include <gtk/gtk.h>

#include <gdk/gdkx.h>
#include <gtk/gtkprivate.h>

#include <X11/Xatom.h>   // For XA_STRING

#include "nsWindow.h"
#include "nsWidgetsCID.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"
#include "nsIMenuBar.h"
#include "nsToolkit.h"
#include "nsGtkEventHandler.h"
#include "nsIAppShell.h"
#include "nsClipboard.h"
#include "nsIRollupListener.h"

#include "nsGtkUtils.h" // for nsGtkUtils::gdk_window_flash()

#include "stdio.h"

//-------------------------------------------------------------------------
//
// nsWindow constructor
//
//-------------------------------------------------------------------------
nsWindow::nsWindow() 
{
  NS_INIT_REFCNT();
  mFontMetrics = nsnull;
  mShell = nsnull;
  mResized = PR_FALSE;
  mVisible = PR_FALSE;
  mDisplayed = PR_FALSE;
  mLowerLeft = PR_FALSE;
  mWindowType = eWindowType_child;
  mBorderStyle = eBorderStyle_default;
  mIsDestroyingWindow = PR_FALSE;
  mOnDestroyCalled = PR_FALSE;
  mFont = nsnull;
  
  mMenuBar = nsnull;
  mMozArea = 0;
  mSuperWin = 0;
}

//-------------------------------------------------------------------------
//
// nsWindow destructor
//
//-------------------------------------------------------------------------
nsWindow::~nsWindow()
{
#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("nsWindow::~nsWindow:%p\n", this);
#endif
  mIsDestroyingWindow = PR_TRUE;
  if ((nsnull != mShell) || (nsnull != mSuperWin)) {
    Destroy();
  }
  NS_IF_RELEASE(mMenuBar);
}

PRBool nsWindow::IsChild() const
{
  return PR_FALSE;
}

NS_IMETHODIMP nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  gint x;
  gint y;

  if (mSuperWin)
  {
    if (mSuperWin->shell_window)
    {
      gdk_window_get_origin(mSuperWin->shell_window, &x, &x);
      aNewRect.x = x + aOldRect.x;
      aNewRect.y = y + aOldRect.y;
    }
    else
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::Destroy()
{
#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("nsWindow::Destroy:%p: isDestroyingWindow=%s widget=%p shell=%p parent=%p\n",
         this, mIsDestroyingWindow ? "yes" : "no", mWidget, mShell, mParent);
#endif
  NS_IF_RELEASE(mMenuBar);

  // Call base class first... we need to ensure that upper management
  // knows about the close so that if this is the main application
  // window, for example, the application will exit as it should.

  if (mIsDestroyingWindow == PR_TRUE) {
    nsBaseWidget::Destroy();
    if (PR_FALSE == mOnDestroyCalled) {
        nsWidget::OnDestroy();
    }

    if (mShell) {
    	if (GTK_IS_WIDGET(mShell))
     		gtk_widget_destroy(mShell);
    	mShell = nsnull;
    }
    if (mSuperWin) {
      gdk_superwin_destroy(mSuperWin);
      mSuperWin = 0;
    }
  }

  return NS_OK;
}

void
nsWindow::OnDestroySignal(GtkWidget* aGtkWidget)
{
  nsWidget::OnDestroySignal(aGtkWidget);
  if (aGtkWidget == mShell) {
    mShell = nsnull;
  }
}

gint handle_delete_event(GtkWidget *w, GdkEventAny *e, nsWindow *win)
{
  NS_ADDREF(win);
  win->SetIsDestroying( PR_TRUE );
  win->Destroy();
  NS_RELEASE(win);
  return TRUE;
}



NS_IMETHODIMP nsWindow::PreCreateWidget(nsWidgetInitData *aInitData)
{
  if (nsnull != aInitData) {
    SetWindowType(aInitData->mWindowType);
    SetBorderStyle(aInitData->mBorderStyle);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}


gint nsWindow::ConvertBorderStyles(nsBorderStyle bs)
{
  gint w = 0;

  if (bs == eBorderStyle_default)
    return -1;

  if (bs & eBorderStyle_all)
    w |= GDK_DECOR_ALL;
  if (bs & eBorderStyle_border)
    w |= GDK_DECOR_BORDER;
  if (bs & eBorderStyle_resizeh)
    w |= GDK_DECOR_RESIZEH;
  if (bs & eBorderStyle_title)
    w |= GDK_DECOR_TITLE;
  if (bs & eBorderStyle_menu)
    w |= GDK_DECOR_MENU;
  if (bs & eBorderStyle_minimize)
    w |= GDK_DECOR_MINIMIZE;
  if (bs & eBorderStyle_maximize)
    w |= GDK_DECOR_MAXIMIZE;
  if (bs & eBorderStyle_close)
    printf("we don't handle eBorderStyle_close yet... please fix me\n");

  return w;
}

//-------------------------------------------------------------------------
//
// Create the native widget
//
//-------------------------------------------------------------------------
NS_METHOD nsWindow::CreateNative(GtkObject *parentWidget)
{
  GdkSuperWin *superwin = 0;

  if (parentWidget) {
    if (GDK_IS_SUPERWIN(parentWidget))
      superwin = GDK_SUPERWIN(parentWidget);
    else
      g_print("warning: attempted to CreateNative() width a non-superwin parent\n");
  }

  switch(mWindowType)
  {
  case eWindowType_dialog:
    mIsToplevel = PR_TRUE;

    mShell = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_window_set_policy(GTK_WINDOW(mShell), PR_TRUE, PR_TRUE, PR_FALSE);
    gtk_widget_set_app_paintable(mShell, PR_TRUE);
    InstallRealizeSignal(mShell);

    // create the mozarea.  this will be the single child of the
    // toplevel window
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "delete_event",
                       GTK_SIGNAL_FUNC(handle_delete_event),
                       this);
    break;

  case eWindowType_popup:
    mIsToplevel = PR_TRUE;
    mShell = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(mShell, PR_TRUE);
    // create the mozarea.  this will be the single child of the
    // toplevel window
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
    break;

  case eWindowType_toplevel:
    mIsToplevel = PR_TRUE;
    mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_app_paintable(mShell, PR_TRUE);
    gtk_window_set_policy(GTK_WINDOW(mShell), PR_TRUE, PR_TRUE, PR_FALSE);
    InstallRealizeSignal(mShell);
    // create the mozarea.  this will be the single child of the
    // toplevel window
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;

    gtk_signal_connect(GTK_OBJECT(mShell),
                       "delete_event",
                       GTK_SIGNAL_FUNC(handle_delete_event),
                       this);
    gtk_signal_connect_after(GTK_OBJECT(mShell),
                             "size_allocate",
                             GTK_SIGNAL_FUNC(handle_size_allocate),
                             this);
    break;

  case eWindowType_child:
    if (superwin)
      mSuperWin = gdk_superwin_new(superwin->bin_window,
                                   mBounds.x, mBounds.y,
                                   mBounds.width, mBounds.height);
    else
      g_print("warning: attempted to CreateNative() without a superwin parent\n");
    break;

  default:
    break;
  }
  
  gdk_window_set_events(mSuperWin->bin_window, 
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_EXPOSURE_MASK |
                        GDK_FOCUS_CHANGE_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_POINTER_MOTION_HINT_MASK);

  gtk_object_set_data (GTK_OBJECT (mSuperWin), "nsWindow", this);
  gdk_window_set_user_data (mSuperWin->bin_window, (gpointer)mSuperWin);

  /* XXX fix this later */
#if 0  
  if (mIsToplevel)
  {
   if (parentWidget)
    {
      GtkWidget *tlw = gtk_widget_get_toplevel(parentWidget);
      if (GTK_IS_WINDOW(tlw))
      {
        gtk_window_set_transient_for(GTK_WINDOW(mShell), GTK_WINDOW(tlw));
      }
    }
  }
#endif

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Initialize all the Callbacks
//
//-------------------------------------------------------------------------
void nsWindow::InitCallbacks(char * aName)
{
  gdk_superwin_set_event_funcs(mSuperWin, handle_xlib_shell_event,
                               handle_xlib_bin_event, this, NULL);
}

//-------------------------------------------------------------------------
//
// Return some native data according to aDataType
//
//-------------------------------------------------------------------------
void * nsWindow::GetNativeData(PRUint32 aDataType)
{
  if (aDataType == NS_NATIVE_WINDOW) {
    // The GTK layout widget uses a clip window to do scrolling.
    // All the action happens on that window - called the 'bin_window'
    if (mSuperWin)
      return (void *) mSuperWin->bin_window;
  }
  else if (aDataType == NS_NATIVE_WIDGET) {
    return (void *)mSuperWin;
  }
  return nsWidget::GetNativeData(aDataType);
}

NS_IMETHODIMP nsWindow::GetAbsoluteBounds(nsRect &aRect)
{
  gint x;
  gint y;

#ifdef DEBUG_pavlov
  g_print("nsWidget::GetAbsoluteBounds\n");
#endif
  if (mSuperWin)
  {
    gdk_window_get_origin(mSuperWin->shell_window, &x, &y);
    aRect.x = x;
    aRect.y = y;
#ifdef DEBUG_pavlov
    g_print("  x = %i, y = %i\n", x, y);
#endif
  }
  else
    return NS_ERROR_FAILURE;
  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Scroll the bits of a window
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  if (mSuperWin) {
    gdk_superwin_scroll(mSuperWin, aDx, aDy);
  }
  return NS_OK;
}


NS_IMETHODIMP nsWindow::SetTitle(const nsString& aTitle)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

  gtk_window_set_title(GTK_WINDOW(mShell), nsAutoCString(aTitle));

  return NS_OK;
}


// Just give the window a default icon, Mozilla.
#include "mozicon50.xpm"
nsresult nsWindow::SetIcon()
{
  static GdkPixmap *w_pixmap = nsnull;
  static GdkBitmap *w_mask   = nsnull;
  GtkStyle         *w_style;

  /* XXX need to fix this */
#if 0
  w_style = gtk_widget_get_style (mShell);

  if (!w_pixmap) {
    w_pixmap =
      gdk_pixmap_create_from_xpm_d (mShell->window,
				    &w_mask,
				    &w_style->bg[GTK_STATE_NORMAL],
				    mozicon50_xpm);
  }
  
  return SetIcon(w_pixmap, w_mask);
#endif
  return NS_OK;
}


// Set the iconify icon for the window.
nsresult nsWindow::SetIcon(GdkPixmap *pixmap, 
                           GdkBitmap *mask)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

  gdk_window_set_icon(mShell->window, (GdkWindow*)nsnull, pixmap, mask);

  return NS_OK;
}

#define CAPS_LOCK_IS_ON \
(nsGtkUtils::gdk_keyboard_get_modifiers() & GDK_LOCK_MASK)

#define WANT_PAINT_FLASHING \
(debug_WantPaintFlashing() && CAPS_LOCK_IS_ON)

/**
 * Processes an Expose Event
 *
 **/
PRBool nsWindow::OnExpose(nsPaintEvent &event)
{
  nsresult result ;

  // call the event callback
  if (mEventCallback) 
  {
    event.renderingContext = nsnull;

    // expose.. we didn't get an Invalidate, so we should up the count here
    mUpdateArea->Union(event.rect->x, event.rect->y, event.rect->width, event.rect->height);


    //    NS_ADDREF(mUpdateArea);
    //    event.region = mUpdateArea;


    //    printf("\n\n");
    PRInt32 x, y, w, h;
    mUpdateArea->GetBoundingBox(&x,&y,&w,&h);
    //    printf("should be painting x = %i , y = %i , w = %i , h = %i\n", x, y, w, h);
    //    printf("\n\n");
    event.rect->x = x;
    event.rect->y = y;
    event.rect->width = w;
    event.rect->height = h;

    if (event.rect->width == 0 || event.rect->height == 0)
    {
      //      printf("ignoring paint for 0x0\n");
      return NS_OK;
    }

    // print out stuff here incase the event got dropped on the floor above
#ifdef NS_DEBUG
    if (CAPS_LOCK_IS_ON)
    {
      debug_DumpPaintEvent(stdout,
                           this,
                           &event,
                           debug_GetName(GTK_OBJECT(mWidget)),
                           (PRInt32) debug_GetRenderXID(GTK_OBJECT(mWidget)));
    }
#endif // NS_DEBUG

    event.renderingContext = GetRenderingContext();
    if (event.renderingContext)
    {
      PRBool rv;

      event.renderingContext->SetClipRegion(NS_STATIC_CAST(const nsIRegion &, *mUpdateArea),
                                            nsClipCombine_kReplace, rv);

      result = DispatchWindowEvent(&event);
      NS_RELEASE(event.renderingContext);
      //      NS_RELEASE(mUpdateArea);
    }


    mUpdateArea->Subtract(event.rect->x, event.rect->y, event.rect->width, event.rect->height);

#ifdef NS_DEBUG
    if (WANT_PAINT_FLASHING)
    {
      GdkWindow *    gw = GetRenderWindow(GTK_OBJECT(mWidget));
      
      if (gw)
      {
        GdkRectangle   ar;
        GdkRectangle * area = (GdkRectangle*) NULL;
        
        if (event.rect)
        {
          ar.x = event.rect->x;
          ar.y = event.rect->y;
          
          ar.width = event.rect->width;
          ar.height = event.rect->height;
          
          area = &ar;
        }
        
        nsGtkUtils::gdk_window_flash(gw,1,100000,area);
      }
    }
#endif // NS_DEBUG
  }
  return result;
}

/**
 * Processes an Draw Event
 *
 **/
PRBool nsWindow::OnDraw(nsPaintEvent &event)
{
  nsresult result ;

  // call the event callback
  if (mEventCallback) 
  {
    event.renderingContext = nsnull;

#ifdef NS_DEBUG
    if (CAPS_LOCK_IS_ON)
    {
      debug_DumpPaintEvent(stdout,
                           this,
                           &event,
                           debug_GetName(GTK_OBJECT(mWidget)),
                           (PRInt32) debug_GetRenderXID(GTK_OBJECT(mWidget)));
    }
#endif // NS_DEBUG


    //    NS_ADDREF(mUpdateArea);
    //    event.region = mUpdateArea;

    //    printf("\n\n");
    PRInt32 x, y, w, h;
    mUpdateArea->GetBoundingBox(&x,&y,&w,&h);
    //    printf("should be painting x = %i , y = %i , w = %i , h = %i\n", x, y, w, h);
    //    printf("\n\n");
    event.rect->x = x;
    event.rect->y = y;
    event.rect->width = w;
    event.rect->height = h;

    if (event.rect->width == 0 || event.rect->height == 0)
    {
      //      printf("ignoring paint for 0x0\n");
      return NS_OK;
    }


    event.renderingContext = GetRenderingContext();
    if (event.renderingContext)
    {
      PRBool rv;

      event.renderingContext->SetClipRegion(NS_STATIC_CAST(const nsIRegion &, *mUpdateArea),
                                            nsClipCombine_kReplace, rv);

      result = DispatchWindowEvent(&event);
      NS_RELEASE(event.renderingContext);
      //      NS_RELEASE(mUpdateArea);
    }


    mUpdateArea->Subtract(event.rect->x, event.rect->y, event.rect->width, event.rect->height);

#ifdef NS_DEBUG
    if (WANT_PAINT_FLASHING)
    {
      GdkWindow *    gw = GetRenderWindow(GTK_OBJECT(mWidget));
      
      if (gw)
      {
        GdkRectangle   ar;
        GdkRectangle * area = (GdkRectangle*) NULL;
        
        if (event.rect)
        {
          ar.x = event.rect->x;
          ar.y = event.rect->y;
          
          ar.width = event.rect->width;
          ar.height = event.rect->height;
          
          area = &ar;
        }
        
        nsGtkUtils::gdk_window_flash(gw,1,100000,area);
      }
    }
#endif // NS_DEBUG
  }
  return result;
}


NS_IMETHODIMP nsWindow::BeginResizingChildren(void)
{
  //  gtk_layout_freeze(GTK_LAYOUT(mWidget));
  return NS_OK;
}

NS_IMETHODIMP nsWindow::EndResizingChildren(void)
{
  //  gtk_layout_thaw(GTK_LAYOUT(mWidget));
  return NS_OK;
}

PRBool nsWindow::OnKey(nsKeyEvent &aEvent)
{
  if (mEventCallback) {
    return DispatchWindowEvent(&aEvent);
  }
  return PR_FALSE;
}

PRBool nsWindow::OnScroll(nsScrollbarEvent &aEvent, PRUint32 cPos)
{
  return PR_FALSE;
}

//-------------------------------------------------------------------------
//
// Hide or show this component
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  if (!mSuperWin)
    return NS_OK; // Will be null durring printing

  printf("called nsWindow::Show\n");

  mShown = bState;


  // show
  if (bState)
  {
    // show mWidget
    gdk_window_show(mSuperWin->shell_window);
    gdk_window_show(mSuperWin->bin_window);

    // are we a toplevel window?
    if (mIsToplevel && mShell)
    {
#if 0
      printf("nsWindow::Show %s (%p) bState = %i, mWindowType = %i\n",
             (const char *) debug_GetName(mWidget),
             this,
             bState, mWindowType);
#endif
      gtk_widget_show(mMozArea);
      gtk_widget_show(mShell);
    }
  }
  // hide
  else
  {
    // hide toplevel first so that things don't disapear from the screen one by one

    // are we a toplevel window?
    if (mIsToplevel && mShell)
    {
      gtk_widget_hide(mShell);
      gtk_widget_hide(mMozArea);
      gtk_widget_unmap(mShell);
    } 

    gdk_window_hide(mSuperWin->shell_window);
    // XXX owen commented this out.
    //gdk_window_hide(mSuperWin->bin_window);
  }

  mShown = bState;

  return NS_OK;
}

//-------------------------------------------------------------------------
//
// grab mouse events for this widget
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsWindow::CaptureMouse(PRBool aCapture)
{
  /* XXX fix grabbing later */
#if 0
  GtkWidget *grabWidget;

  if (mIsToplevel && mShell)
    grabWidget = mShell;
  else
    grabWidget = mWidget;

  if (aCapture)
  {
    printf("grabbing widget\n");
    GdkCursor *cursor = gdk_cursor_new (GDK_ARROW);
    gdk_pointer_grab (GTK_WIDGET(grabWidget)->window, PR_TRUE,(GdkEventMask)
                      (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                       GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                       GDK_POINTER_MOTION_MASK),
                      (GdkWindow*) NULL, cursor, GDK_CURRENT_TIME);
    gdk_cursor_destroy(cursor);
    gtk_grab_add(grabWidget);
  }
  else
  {
    printf("ungrabbing widget\n");
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    gtk_grab_remove(grabWidget);
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent)
{
#ifdef DEBUG_pavlov
  printf("nsWindow::CaptureRollupEvents() this = %p , doCapture = %i\n", this, aDoCapture);
#endif
  GtkWidget *grabWidget;

  grabWidget = mWidget;
  // XXX we need a visible widget!!

  if (aDoCapture)
  {
#ifdef DEBUG_pavlov
    printf("grabbing widget\n");
#endif
    GdkCursor *cursor = gdk_cursor_new (GDK_ARROW);
    if (!mSuperWin) {
#ifdef DEBUG_blizzard
      printf("no superWin for this widget");
#endif
    }
    else
    {
      int ret = gdk_pointer_grab (GDK_SUPERWIN(mSuperWin)->bin_window, PR_TRUE,(GdkEventMask)
                                  (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                   GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                                   GDK_POINTER_MOTION_MASK),
                                  (GdkWindow*)NULL, cursor, GDK_CURRENT_TIME);
#ifdef DEBUG_blizzard
      printf("pointer grab returned %i\n", ret);
#endif
      gdk_cursor_destroy(cursor);
    }
  }
  else
  {
#ifdef DEBUG_blizzard
    printf("ungrabbing widget\n");
#endif
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    //    gtk_grab_remove(grabWidget);
  }

  if (aDoCapture) {
    //    gtk_grab_add(mWidget);
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupConsumeRollupEvent = PR_TRUE;
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);
  } else {
    //    gtk_grab_remove(mWidget);
    NS_IF_RELEASE(gRollupListener);
    //gRollupListener = nsnull;
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}


NS_IMETHODIMP nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
#if 0
  printf("nsWindow::Move %s (%p) to %d %d\n",
         (const char *) debug_GetName(mWidget),
         this,
         aX, aY);
#endif
  printf("nsWindow::Move to %d %d\n",
         aX, aY);
  // not implimented for toplevel windows
  if (mIsToplevel && mShell)
  {
    // do it the way it should be done period.
    if (!mParent)
    {
      if (mWindowType != eWindowType_toplevel)
        gtk_widget_set_uposition(mShell, aX, aY);
    }
    else
    {
      // *VERY* stupid hack to make gfx combo boxes work
      nsRect oldrect, newrect;
      oldrect.x = aX;
      oldrect.y = aY;
      mParent->WidgetToScreen(oldrect, newrect);
      gtk_widget_set_uposition(mShell, newrect.x, newrect.y);
    }
  }
  else if (mSuperWin)
  {
    gdk_window_move(mSuperWin->shell_window, aX, aY);
  }

  return NS_OK;
}


NS_IMETHODIMP nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{


#if 0
  printf("nsWindow::Resize %s (%p) to %d %d\n",
         (const char *) debug_GetName(mWidget),
         this,
         aWidth, aHeight);
#endif

  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  if (mSuperWin) {
    // toplevel window?  if so, we should resize it as well.
    if (mIsToplevel && mShell)
    {
      gtk_window_set_default_size(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    gdk_superwin_resize(mSuperWin, aWidth, aHeight);
  }

  /* XXX chris, too. */
#if 0
  // XXX pav
  // call the size allocation handler directly to avoid code duplication
  // note, this could be a problem as this will make layout think that it
  // got the size it requested which could be wrong.
  // but, we don't use many native widgets anymore, so this shouldn't be a problem
  // layout's will size to the size you tell them to, which are the only native widgets
  // we still use after all the xp widgets land
  GtkAllocation alloc;
  alloc.width = aWidth;
  alloc.height = aHeight;
  alloc.x = 0;
  alloc.y = 0;
  handle_size_allocate(mWidget, &alloc, this);

  if (nNeedToShow)
  {
    if (mIsToplevel && mShell)
      gtk_widget_show(mShell);
    else
      gtk_widget_show(mWidget);
  }
#endif
  return NS_OK;
}


NS_IMETHODIMP nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                               PRInt32 aHeight, PRBool aRepaint)
{
  Move(aX,aY);
  // resize can cause a show to happen, so do this last
  Resize(aWidth,aHeight,aRepaint);
  return NS_OK;
}

/* virtual */ void
nsWindow::OnRealize(GtkWidget *aWidget)
{
  if (aWidget == mShell)
  {
    SetIcon();

    // we were just realized, so we better have a window, but we will make sure...
    if (mShell->window)
    {
      // XXX bug 8002
      //    gdk_window_raise(mShell->window);

      gint wmd = ConvertBorderStyles(mBorderStyle);
      if (wmd != -1)
        gdk_window_set_decorations(mShell->window, (GdkWMDecoration)wmd);
    }
  }
}

//////////////////////////////////////////////////////////////////////
//
// Draw signal
// 
//////////////////////////////////////////////////////////////////////
void 
nsWindow::InitDrawEvent(GdkRectangle * aArea,
                        nsPaintEvent & aPaintEvent,
                        PRUint32       aEventType)
{
  aPaintEvent.message = aEventType;
  aPaintEvent.widget  = (nsWidget *) this;

  aPaintEvent.eventStructType = NS_PAINT_EVENT;
  //  aPaintEvent.point.x = 0;
  //  aPaintEvent.point.y = 0;
  aPaintEvent.point.x = aArea->x;
  aPaintEvent.point.y = aArea->y; 
  aPaintEvent.time = GDK_CURRENT_TIME;

  if (aArea != NULL) 
  {
    aPaintEvent.rect = new nsRect(aArea->x, 
							  aArea->y, 
							  aArea->width, 
							  aArea->height);
  }
}
//////////////////////////////////////////////////////////////////////
void 
nsWindow::UninitDrawEvent(GdkRectangle * area,
                          nsPaintEvent & aPaintEvent,
                          PRUint32       aEventType)
{
  if (area != NULL) 
  {
    delete aPaintEvent.rect;
  }

  // While I'd think you should NS_RELEASE(aPaintEvent.widget) here,
  // if you do, it is a NULL pointer.  Not sure where it is getting
  // released.
}
//////////////////////////////////////////////////////////////////////
/* static */ gint
nsWindow::DrawSignal(GtkWidget *    /* aWidget */,
					 GdkRectangle * aArea,
					 gpointer       aData)
{
  nsWindow * window = (nsWindow *) aData;

  NS_ASSERTION(nsnull != window,"window is null");

  return window->OnDrawSignal(aArea);
}
//////////////////////////////////////////////////////////////////////
/* virtual */ gint
nsWindow::OnDrawSignal(GdkRectangle * aArea)
{
  //printf("nsWindow::OnDrawSignal()\n");

  nsPaintEvent pevent;

  InitDrawEvent(aArea, pevent, NS_PAINT);

  nsWindow * win = (nsWindow *) this;

  NS_ADDREF(win);

  win->OnDraw(pevent);

  NS_RELEASE(win);

  UninitDrawEvent(aArea, pevent, NS_PAINT);

  return PR_TRUE;
}

// Add an XATOM property to this window.
// Assuming XA_STRING type.
// Borrowed from xfe classic branch.
void nsWindow::StoreProperty(char *property, unsigned char *data)
{
  
  // This needs to happen before properties start working.
  // Not sure if calling this is ? overkill or not.
  gtk_widget_show_all (mShell);

  // GetRenderWindow(mWidget),
  gdk_property_change (mShell->window,
                       gdk_atom_intern (property, FALSE), /* property */
                       XA_STRING, /* type */
                       8, /* *sizeof(GdkAtom) Format. ? */
                       GDK_PROP_MODE_REPLACE, /* mode */
                       (guchar *)data, /* data */
                       (gint)strlen((char *)data)); /* size of data */
}

void
nsWindow::HandleXlibExposeEvent(XEvent *event)
{
  nsPaintEvent pevent;
  
  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.point.x = event->xexpose.x;
  pevent.point.y = event->xexpose.y;
  pevent.rect = new nsRect(event->xexpose.x, event->xexpose.y,
                           event->xexpose.width, event->xexpose.height);
  /* XXX fix this */
  pevent.time = 0;
  AddRef();
  OnExpose(pevent);
  Release();
  delete pevent.rect;
}

void
nsWindow::HandleXlibButtonEvent(XButtonEvent * aButtonEvent)
{
  nsMouseEvent event;
  
  PRUint32 eventType = 0;
  
  if (aButtonEvent->type == ButtonPress)
    {
      switch(aButtonEvent->button)
        {
        case 1:
          eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
          break;
          
        case 2:
          eventType = NS_MOUSE_MIDDLE_BUTTON_DOWN;
          break;
          
        case 3:
          eventType = NS_MOUSE_RIGHT_BUTTON_DOWN;
          break;
          
        default:
          eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
          break;
        }
    }
  else if (aButtonEvent->type == ButtonRelease)
    {
      switch(aButtonEvent->button)
        {
        case 1:
          eventType = NS_MOUSE_LEFT_BUTTON_UP;
          break;
          
        case 2:
          eventType = NS_MOUSE_MIDDLE_BUTTON_UP;
          break;
          
        case 3:
          eventType = NS_MOUSE_RIGHT_BUTTON_UP;
          break;
          
        default:
          eventType = NS_MOUSE_LEFT_BUTTON_UP;
          break;
        }
    }
  
  event.message = eventType;
  event.widget  = this;
  event.eventStructType = NS_MOUSE_EVENT;
  
  event.point.x = nscoord(aButtonEvent->x);
  event.point.y = nscoord(aButtonEvent->y);
  
  event.isShift = aButtonEvent->state & ShiftMask;
  event.isControl = aButtonEvent->state & ControlMask;
  event.isAlt = aButtonEvent->state & Mod1Mask;
  event.time = aButtonEvent->time;
  event.clickCount = 1;
  
  AddRef();
  
  DispatchMouseEvent(event);
  
  Release();
}

void
nsWindow::HandleXlibMotionNotifyEvent(XMotionEvent * aMotionEvent)
{
  nsMouseEvent event;
  
  event.message = NS_MOUSE_MOVE;
  event.eventStructType = NS_MOUSE_EVENT;
  
  event.point.x = nscoord(aMotionEvent->x);
  event.point.y = nscoord(aMotionEvent->y);
  
  event.widget = this;
  
  AddRef();
  
  DispatchMouseEvent(event);
  
  Release();
}

void
nsWindow::HandleXlibCrossingEvent(XCrossingEvent * aCrossingEvent)
{
  nsMouseEvent event;
  
  if (aCrossingEvent->type == EnterNotify)
    {
      event.message = NS_MOUSE_ENTER;
    }
  else
    {
      event.message = NS_MOUSE_EXIT;
    }
  
  event.widget  = this;
  event.eventStructType = NS_MOUSE_EVENT;
  
  event.point.x = nscoord(aCrossingEvent->x);
  event.point.y = nscoord(aCrossingEvent->y);
  event.time = aCrossingEvent->time;
  
  AddRef();
  
  DispatchMouseEvent(event);
  
  Release();
}


void
nsWindow::HandleXlibConfigureNotifyEvent(XEvent *event)
{
  XEvent    config_event;

  while (XCheckTypedWindowEvent(event->xany.display, 
                                event->xany.window, 
                                ConfigureNotify,
                                &config_event) == True) {
    // make sure that we don't get other types of events.  
    // StructureNotifyMask includes other kinds of events, too.
    gdk_superwin_clear_translate_queue(mSuperWin, event->xany.serial);
    *event = config_event;
    // make sure that if we remove a configure event from the queue
    // that it gets pulled out of the superwin tranlate queue,
    // too.
#if 0
    g_print("Extra ConfigureNotify event for window 0x%lx %d %d %d %d\n",
            event->xconfigure.window,
            event->xconfigure.x, 
            event->xconfigure.y,
            event->xconfigure.width, 
            event->xconfigure.height);
#endif
  }

  gdk_superwin_clear_translate_queue(mSuperWin, event->xany.serial);

  nsSizeEvent sevent;
  sevent.message = NS_SIZE;
  sevent.widget = this;
  sevent.eventStructType = NS_SIZE_EVENT;
  sevent.windowSize = new nsRect (event->xconfigure.x, event->xconfigure.y,
                                  event->xconfigure.width, event->xconfigure.height);
  sevent.point.x = event->xconfigure.x;
  sevent.point.y = event->xconfigure.y;
  sevent.mWinWidth = event->xconfigure.width;
  sevent.mWinHeight = event->xconfigure.height;
  // XXX fix this
  sevent.time = 0;
  AddRef();
  OnResize(sevent);
  Release();
  delete sevent.windowSize;
}

// Return the GtkMozArea that is the nearest parent of this widget
GtkWidget *
nsWindow::GetMozArea()
{
  GdkWindow *parent = mSuperWin->shell_window;
  GtkWidget *widget;

  if (mMozArea == nsnull)
    while (parent)
      {
        gdk_window_get_user_data (parent, (void **)&widget);
        if (widget != nsnull && GTK_IS_MOZAREA (widget))
          {
            mMozArea = widget;
            break;
          }
        parent = gdk_window_get_parent (parent);
        parent = gdk_window_get_parent (parent);
      }
  
  return mMozArea;
}


/* virtual */ GdkWindow *
nsWindow::GetRenderWindow(GtkObject * aGtkObject)
{
  GdkWindow * renderWindow = nsnull;

  if (aGtkObject)
  {
    if (GDK_IS_SUPERWIN(aGtkObject))
    {
      renderWindow = GDK_SUPERWIN(aGtkObject)->bin_window;
    }
  }
  return renderWindow;
}

NS_METHOD
nsWindow::Invalidate(PRBool aIsSynchronous)
{
  nsPaintEvent pevent;
  nsRect       bounds;

  GetBounds(bounds);

  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.point.x = 0;
  pevent.point.y = 0;
  pevent.rect = &bounds;
  /* XXX fix this */
  pevent.time = 0;
  AddRef();
  OnExpose(pevent);
  Release();
  return NS_OK;
}

NS_METHOD
nsWindow::Invalidate(const nsRect &aRect, PRBool aIsSynchronous)
{
  nsPaintEvent pevent;
  nsRect       bounds = aRect;

  pevent.message = NS_PAINT;
  pevent.widget = this;
  pevent.eventStructType = NS_PAINT_EVENT;
  pevent.point.x = 0;
  pevent.point.y = 0;
  pevent.rect = &bounds;
  /* XXX fix this */
  pevent.time = 0;
  AddRef();
  OnExpose(pevent);
  Release();
  return NS_OK;
}

//////////////////////////////////////////////////////////////////////
/* virtual */ void
nsWindow::OnButtonPressSignal(GdkEventButton * aGdkButtonEvent)
{
  nsMouseEvent event;
  PRUint32 eventType = 0;

#ifdef DEBUG_pavlov
  printf("button press\n");
#endif

  if (gRollupWidget && gRollupListener)
  {

    GdkSuperWin *rollupWidget = GDK_SUPERWIN(gRollupWidget->GetNativeData(NS_NATIVE_WIDGET));

    gint x, y;
    gint w, h;
    gdk_window_get_origin(rollupWidget->shell_window, &x, &y);

    gdk_window_get_size(rollupWidget->shell_window, &w, &h);


    if (!(aGdkButtonEvent->x_root > x &&
          aGdkButtonEvent->x_root < x + w &&
          aGdkButtonEvent->y_root > y &&
          aGdkButtonEvent->y_root < y + h))
    {
      gRollupListener->Rollup();
      printf("rolling up\n");
      return;
    }
  }

  // Switch on single, double, triple click.
  switch (aGdkButtonEvent->type) 
  {
    // Single click.
  case GDK_BUTTON_PRESS:
    // Double click.
  case GDK_2BUTTON_PRESS:
    // Triple click.
  case GDK_3BUTTON_PRESS:

    switch (aGdkButtonEvent->button)  // Which button?
    {
    case 1:
      eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
      break;

    case 2:
      eventType = NS_MOUSE_MIDDLE_BUTTON_DOWN;
      break;

    case 3:
      eventType = NS_MOUSE_RIGHT_BUTTON_DOWN;
      break;

      // Single-click default.
    default:
      eventType = NS_MOUSE_LEFT_BUTTON_DOWN;
      break;
    }
    break;

  default:
    break;
  }

  InitMouseEvent(aGdkButtonEvent, event, eventType);

  // Set the button motion target and remeber the widget and root coords
  sButtonMotionTarget = this;

  sButtonMotionRootX = (gint) aGdkButtonEvent->x_root;
  sButtonMotionRootY = (gint) aGdkButtonEvent->y_root;

  sButtonMotionWidgetX = (gint) aGdkButtonEvent->x;
  sButtonMotionWidgetY = (gint) aGdkButtonEvent->y;
  
  AddRef();

  DispatchMouseEvent(event);

  Release();

}

ChildWindow::ChildWindow()
{
}

ChildWindow::~ChildWindow()
{
#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("ChildWindow::~ChildWindow:%p\n", this);
#endif
}

PRBool ChildWindow::IsChild() const
{
  return PR_TRUE;
}

NS_METHOD ChildWindow::Destroy()
{
#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("ChildWindow::Destroy:%p  \n", this);
#endif

  // Skip over baseclass Destroy method which doesn't do what we want;
  // instead make sure widget destroy method gets invoked.
  return nsWidget::Destroy();
}
