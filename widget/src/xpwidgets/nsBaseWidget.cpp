/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsBaseWidget.h"
#include "nsIAppShell.h"
#include "nsIDeviceContext.h"
#include "nsCOMPtr.h"
#include "nsIMenuListener.h"
#include "nsIEnumerator.h"
#include "nsGfxCIID.h"
#include "nsWidgetsCID.h"

#ifdef NS_DEBUG
#include "nsIServiceManager.h"
#include "nsIPref.h"
#endif

#ifdef NOISY_WIDGET_LEAKS
static PRInt32 gNumWidgets;
#endif

// nsBaseWidget
NS_IMPL_ISUPPORTS2(nsBaseWidget, nsIWidget, nsIGenericWindow)

// nsBaseWidget::Enumerator
NS_IMPL_ISUPPORTS2(nsBaseWidget::Enumerator, nsIBidirectionalEnumerator, nsIEnumerator)


//-------------------------------------------------------------------------
//
// nsBaseWidget constructor
//
//-------------------------------------------------------------------------

nsBaseWidget::nsBaseWidget()
:	mClientData(nsnull)
,	mEventCallback(nsnull)
,	mContext(nsnull)
,	mAppShell(nsnull)
,	mToolkit(nsnull)
,	mMouseListener(nsnull)
,	mEventListener(nsnull)
,	mMenuListener(nsnull)
,	mCursor(eCursor_standard)
  //,	mBorderStyle(eBorderStyle_none)
,	mIsShiftDown(PR_FALSE)
,	mIsControlDown(PR_FALSE)
,	mIsAltDown(PR_FALSE)
,	mIsDestroying(PR_FALSE)
,	mOnDestroyCalled(PR_FALSE)
,	mBounds(0,0,0,0)
#ifdef LOSER
,	mVScrollbar(nsnull)
#endif
,   mZIndex(0)
{
#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets++;
  printf("WIDGETS+ = %d\n", gNumWidgets);
#endif

    NS_NewISupportsArray(getter_AddRefs(mChildren));
    
    NS_INIT_REFCNT();
}


//-------------------------------------------------------------------------
//
// nsBaseWidget destructor
//
//-------------------------------------------------------------------------
nsBaseWidget::~nsBaseWidget()
{
#ifdef NOISY_WIDGET_LEAKS
  gNumWidgets--;
  printf("WIDGETS- = %d\n", gNumWidgets);
#endif

	NS_IF_RELEASE(mMenuListener);
#ifdef LOSER
	NS_IF_RELEASE(mVScrollbar);
#endif
	NS_IF_RELEASE(mToolkit);
	NS_IF_RELEASE(mContext);
}


//-------------------------------------------------------------------------
//
// Basic create.
//
//-------------------------------------------------------------------------
void nsBaseWidget::BaseCreate(nsIWidget *aParent,
                              const nsRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell *aAppShell,
                              nsIToolkit *aToolkit,
                              nsWidgetInitData *aInitData)
{
  if (nsnull == mToolkit) {
    if (nsnull != aToolkit) {
      mToolkit = (nsIToolkit*)aToolkit;
      NS_ADDREF(mToolkit);
    }
    else {
      if (nsnull != aParent) {
        mToolkit = (nsIToolkit*)(aParent->GetToolkit()); // the call AddRef's, we don't have to
      }
      // it's some top level window with no toolkit passed in.
      // Create a default toolkit with the current thread
#if !defined(USE_TLS_FOR_TOOLKIT)
      else {
        static NS_DEFINE_CID(kToolkitCID, NS_TOOLKIT_CID);
        
        nsresult res;
        res = nsComponentManager::CreateInstance(kToolkitCID, nsnull,
                                                 NS_GET_IID(nsIToolkit), (void **)&mToolkit);
        if (NS_OK != res)
          NS_ASSERTION(PR_FALSE, "Can not create a toolkit in nsBaseWidget::Create");
        if (mToolkit)
          mToolkit->Init(PR_GetCurrentThread());
      }
#else /* USE_TLS_FOR_TOOLKIT */
      else {
        nsresult rv;

        rv = NS_GetCurrentToolkit(&mToolkit);
      }
#endif /* USE_TLS_FOR_TOOLKIT */
    }
    
  }
  
  mAppShell = aAppShell;
  NS_IF_ADDREF(mAppShell);
  
  // save the event callback function
  mEventCallback = aHandleEventFunction;
  
  // keep a reference to the device context
  if (aContext) {
    mContext = aContext;
    NS_ADDREF(mContext);
  }
  else {
    nsresult  res;
    
    static NS_DEFINE_CID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);
    
    res = nsComponentManager::CreateInstance(kDeviceContextCID, nsnull,
                                             NS_GET_IID(nsIDeviceContext), (void **)&mContext);

    if (NS_OK == res)
      mContext->Init(nsnull);
  }

  if (nsnull != aInitData) {
    PreCreateWidget(aInitData);
  }

  if (aParent) {
    aParent->AddChild(this);
  }
}

NS_IMETHODIMP nsBaseWidget::CaptureMouse(PRBool aCapture)
{
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous)
{
  return NS_ERROR_FAILURE;
}

//-------------------------------------------------------------------------
//
// Accessor functions to get/set the client data
//
//-------------------------------------------------------------------------

NS_IMETHODIMP nsBaseWidget::GetClientData(void*& aClientData)
{
  aClientData = mClientData;
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::SetClientData(void* aClientData)
{
  mClientData = aClientData;
  return NS_OK;
}

//-------------------------------------------------------------------------
//
// Close this nsBaseWidget
//
//-------------------------------------------------------------------------
NS_METHOD nsBaseWidget::Destroy()
{
  // disconnect from the parent
  nsIWidget *parent = GetParent();
  if (parent) {
    parent->RemoveChild(this);
    NS_RELEASE(parent);
  }
#ifdef LOSER
	NS_IF_RELEASE(mVScrollbar);
#endif
  // disconnect listeners.
  NS_IF_RELEASE(mMouseListener);
  NS_IF_RELEASE(mEventListener);
  NS_IF_RELEASE(mMenuListener);

  return NS_OK;
}


//-------------------------------------------------------------------------
//
// Get this nsBaseWidget parent
//
//-------------------------------------------------------------------------
nsIWidget* nsBaseWidget::GetParent(void)
{
  return nsnull;
}

//-------------------------------------------------------------------------
//
// Get this nsBaseWidget's list of children
//
//-------------------------------------------------------------------------
nsIEnumerator* nsBaseWidget::GetChildren()
{
  nsIEnumerator* children = nsnull;

  PRUint32 itemCount = 0;
  mChildren->Count(&itemCount);
  if ( itemCount ) {
    children = new Enumerator(*this);
    NS_IF_ADDREF(children);
  }
  return children;
}

//-------------------------------------------------------------------------
//
// Sets widget's position within its parent's child list.
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsBaseWidget::SetZIndex(PRInt32 aZIndex)
{
	mZIndex = aZIndex;

	// reorder this child in its parent's list.
	nsBaseWidget* parent = NS_STATIC_CAST(nsBaseWidget*, GetParent());
	if (nsnull != parent) {
		parent->mChildren->RemoveElement(NS_STATIC_CAST(nsIWidget*,this));
		PRUint32 childCount, index;
		if (NS_SUCCEEDED(parent->mChildren->Count(&childCount))) {
			for (index = 0; index < childCount; index++) {
				nsCOMPtr<nsIWidget> childWidget;
				if (NS_SUCCEEDED(parent->mChildren->QueryElementAt(index, NS_GET_IID(nsIWidget), (void**)getter_AddRefs(childWidget)))) {
					PRInt32 childZIndex;
					if (NS_SUCCEEDED(childWidget->GetZIndex(&childZIndex))) {
						if (aZIndex < childZIndex) {
							parent->mChildren->InsertElementAt(NS_STATIC_CAST(nsIWidget*,this), index);
							break;
						}
					}
				}
			}
			// were we added to the list?
			if (index == childCount) {
				parent->mChildren->AppendElement(NS_STATIC_CAST(nsIWidget*,this));
			}
		}
		NS_RELEASE(parent);
	}
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Gets widget's position within its parent's child list.
//
//-------------------------------------------------------------------------
NS_IMETHODIMP nsBaseWidget::GetZIndex(PRInt32* aZIndex)
{
	*aZIndex = mZIndex;
	return NS_OK;
}

//-------------------------------------------------------------------------
//
// Get the foreground color
//
//-------------------------------------------------------------------------
nscolor nsBaseWidget::GetForegroundColor(void)
{
  return mForeground;
}

//-------------------------------------------------------------------------
//
// Get the background color
//
//-------------------------------------------------------------------------
nscolor nsBaseWidget::GetBackgroundColor(void)
{
  return mBackground;
}

//-------------------------------------------------------------------------
//
// Get this component cursor
//
//-------------------------------------------------------------------------
nsCursor nsBaseWidget::GetCursor()
{
  return mCursor;
}

NS_METHOD nsBaseWidget::SetCursor(nsCursor aCursor)
{
  mCursor = aCursor; 
  return NS_OK;
}
    
//-------------------------------------------------------------------------
//
// Create a rendering context from this nsBaseWidget
//
//-------------------------------------------------------------------------
nsIRenderingContext* nsBaseWidget::GetRenderingContext()
{
  nsIRenderingContext *renderingCtx = NULL;
  nsresult  res;

  static NS_DEFINE_CID(kRenderingContextCID, NS_RENDERING_CONTEXT_CID);

  res = nsComponentManager::CreateInstance(kRenderingContextCID, nsnull,
                                           NS_GET_IID(nsIRenderingContext),
                                           (void **)&renderingCtx);

  if (NS_OK == res)
    renderingCtx->Init(mContext, this);

  NS_ASSERTION(NULL != renderingCtx, "Null rendering context");
  
  return renderingCtx;
}

//-------------------------------------------------------------------------
//
// Return the toolkit this widget was created on
//
//-------------------------------------------------------------------------
nsIToolkit* nsBaseWidget::GetToolkit()
{
  NS_IF_ADDREF(mToolkit);
  return mToolkit;
}


//-------------------------------------------------------------------------
//
// Return the used device context
//
//-------------------------------------------------------------------------
nsIDeviceContext* nsBaseWidget::GetDeviceContext() 
{
  NS_IF_ADDREF(mContext);
  return mContext; 
}

//-------------------------------------------------------------------------
//
// Return the App Shell
//
//-------------------------------------------------------------------------

nsIAppShell *nsBaseWidget::GetAppShell()
{
  NS_IF_ADDREF(mAppShell);
  return mAppShell;
}


//-------------------------------------------------------------------------
//
// Destroy the window
//
//-------------------------------------------------------------------------
void nsBaseWidget::OnDestroy()
{
  // release references to device context, toolkit, and app shell
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mToolkit);
  NS_IF_RELEASE(mAppShell);
}


NS_METHOD nsBaseWidget::SetWindowType(nsWindowType aWindowType) 
{
  mWindowType = aWindowType;
  return NS_OK;
}


NS_METHOD nsBaseWidget::SetBorderStyle(nsBorderStyle aBorderStyle)
{
  mBorderStyle = aBorderStyle;
  return NS_OK;
}


/**
* Processes a mouse pressed event
*
**/
NS_METHOD nsBaseWidget::AddMouseListener(nsIMouseListener * aListener)
{
  NS_PRECONDITION(mMouseListener == nsnull, "Null mouse listener");
  NS_IF_RELEASE(mMouseListener);
  NS_ADDREF(aListener);
  mMouseListener = aListener;
  return NS_OK;
}

/**
* Processes a mouse pressed event
*
**/
NS_METHOD nsBaseWidget::AddEventListener(nsIEventListener * aListener)
{
  NS_PRECONDITION(mEventListener == nsnull, "Null mouse listener");
  NS_IF_RELEASE(mEventListener);
  NS_ADDREF(aListener);
  mEventListener = aListener;
  return NS_OK;
}

/**
* Add a menu listener
* This interface should only be called by the menu services manager
* This will AddRef() the menu listener
* This will Release() a previously set menu listener
*
**/

NS_METHOD nsBaseWidget::AddMenuListener(nsIMenuListener * aListener)
{
  NS_IF_RELEASE(mMenuListener);
  NS_IF_ADDREF(aListener);
  mMenuListener = aListener;
  return NS_OK;
}


/**
* If the implementation of nsWindow supports borders this method MUST be overridden
*
**/
NS_METHOD nsBaseWidget::GetClientBounds(nsRect &aRect)
{
  return GetBounds(aRect);
}

/**
* If the implementation of nsWindow supports borders this method MUST be overridden
*
**/
NS_METHOD nsBaseWidget::GetBounds(nsRect &aRect)
{
  aRect = mBounds;
  return NS_OK;
}


/**
* If the implementation of nsWindow supports borders this method MUST be overridden
*
**/
NS_METHOD nsBaseWidget::GetBoundsAppUnits(nsRect &aRect, float aAppUnits)
{
  aRect = mBounds;
  // Convert to twips
  aRect.x      = nscoord((PRFloat64)aRect.x * aAppUnits);
  aRect.y      = nscoord((PRFloat64)aRect.y * aAppUnits);
  aRect.width  = nscoord((PRFloat64)aRect.width * aAppUnits); 
  aRect.height = nscoord((PRFloat64)aRect.height * aAppUnits);
  return NS_OK;
}

/**
* 
*
**/
NS_METHOD nsBaseWidget::SetBounds(const nsRect &aRect)
{
  mBounds = aRect;

  return NS_OK;
}
 


/**
* Calculates the border width and height  
*
**/
NS_METHOD nsBaseWidget::GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight)
{
  nsRect rectWin;
  nsRect rect;
  GetBounds(rectWin);
  GetClientBounds(rect);

  aWidth  = (rectWin.width - rect.width) / 2;
  aHeight = (rectWin.height - rect.height) / 2;

  return NS_OK;
}


/**
* Calculates the border width and height  
*
**/
void nsBaseWidget::DrawScaledRect(nsIRenderingContext& aRenderingContext, const nsRect & aRect, float aScale, float aAppUnits)
{
  nsRect rect = aRect;

  float x = (float)rect.x;
  float y = (float)rect.y;
  float w = (float)rect.width;
  float h = (float)rect.height;
  float twoAppUnits = aAppUnits * 2.0f;

  for (int i=0;i<int(aScale);i++) {
    rect.x      = nscoord(x);
    rect.y      = nscoord(y);
    rect.width  = nscoord(w);
    rect.height = nscoord(h);
    aRenderingContext.DrawRect(rect);
    x += aAppUnits; 
    y += aAppUnits;
    w -= twoAppUnits; 
    h -= twoAppUnits;
  }
}

/**
* Calculates the border width and height  
*
**/
void nsBaseWidget::DrawScaledLine(nsIRenderingContext& aRenderingContext, 
                                  nscoord aSX, 
                                  nscoord aSY, 
                                  nscoord aEX, 
                                  nscoord aEY, 
                                  float   aScale, 
                                  float   aAppUnits,
                                  PRBool  aIsHorz)
{
  float sx = (float)aSX;
  float sy = (float)aSY;
  float ex = (float)aEX;
  float ey = (float)aEY;

  for (int i=0;i<int(aScale);i++) {
    aSX = nscoord(sx);
    aSY = nscoord(sy);
    aEX = nscoord(ex);
    aEY = nscoord(ey);
    aRenderingContext.DrawLine(aSX, aSY, aEX, aEY);
    if (aIsHorz) {
      sy += aAppUnits; 
      ey += aAppUnits;
    } else {
      sx += aAppUnits; 
      ex += aAppUnits;
    }
  }
}

/**
* Paints default border (XXX - this should be done by CSS)
*
**/
NS_METHOD nsBaseWidget::Paint(nsIRenderingContext& aRenderingContext,
                              const nsRect&        aDirtyRect)
{
  nsRect rect;
  float  appUnits;
  float  scale;
  nsIDeviceContext * context;
  aRenderingContext.GetDeviceContext(context);

  context->GetCanonicalPixelScale(scale);
  context->GetDevUnitsToAppUnits(appUnits);

  GetBoundsAppUnits(rect, appUnits);
  aRenderingContext.SetColor(NS_RGB(0,0,0));

  DrawScaledRect(aRenderingContext, rect, scale, appUnits);

  NS_RELEASE(context);
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::ScrollRect(nsRect &aRect, PRInt32 aDx, PRInt32 aDy)
{
  return NS_ERROR_FAILURE;
}

#ifdef LOSER
NS_METHOD nsBaseWidget::SetVerticalScrollbar(nsIWidget * aWidget)
{
  NS_IF_RELEASE(mVScrollbar);
  mVScrollbar = aWidget;
  NS_IF_ADDREF(mVScrollbar);
  return NS_OK;
}
#endif

NS_METHOD nsBaseWidget::EnableDragDrop(PRBool aEnable)
{
  return NS_OK;
}

NS_METHOD nsBaseWidget::SetModal(void)
{
  return NS_ERROR_FAILURE;
}

#ifdef NS_DEBUG
//////////////////////////////////////////////////////////////
//
// Convert a GUI event message code to a string.
// Makes it a lot easier to debug events.
//
// See gtk/nsWidget.cpp and windows/nsWindow.cpp
// for a DebugPrintEvent() function that uses
// this.
//
//////////////////////////////////////////////////////////////
/* static */ nsAutoString
nsBaseWidget::debug_GuiEventToString(nsGUIEvent * aGuiEvent)
{
  NS_ASSERTION(nsnull != aGuiEvent,"cmon, null gui event.");

  nsAutoString eventName = "UNKNOWN";

#define _ASSIGN_eventName(_value,_name)\
case _value: eventName = _name ; break

  switch(aGuiEvent->message)
  {
    _ASSIGN_eventName(NS_BLUR_CONTENT,"NS_BLUR_CONTENT");
    _ASSIGN_eventName(NS_CONTROL_CHANGE,"NS_CONTROL_CHANGE");
    _ASSIGN_eventName(NS_CREATE,"NS_CREATE");
    _ASSIGN_eventName(NS_DESTROY,"NS_DESTROY");
    _ASSIGN_eventName(NS_DRAGDROP_GESTURE,"NS_DND_GESTURE");
    _ASSIGN_eventName(NS_DRAGDROP_DROP,"NS_DND_DROP");
    _ASSIGN_eventName(NS_DRAGDROP_ENTER,"NS_DND_ENTER");
    _ASSIGN_eventName(NS_DRAGDROP_EXIT,"NS_DND_EXIT");
    _ASSIGN_eventName(NS_DRAGDROP_OVER,"NS_DND_OVER");
    _ASSIGN_eventName(NS_FOCUS_CONTENT,"NS_FOCUS_CONTENT");
    _ASSIGN_eventName(NS_FORM_SELECTED,"NS_FORM_SELECTED");
    _ASSIGN_eventName(NS_FORM_CHANGE,"NS_FORM_CHANGE");
    _ASSIGN_eventName(NS_FORM_INPUT,"NS_FORM_INPUT");
    _ASSIGN_eventName(NS_FORM_RESET,"NS_FORM_RESET");
    _ASSIGN_eventName(NS_FORM_SUBMIT,"NS_FORM_SUBMIT");
    _ASSIGN_eventName(NS_GOTFOCUS,"NS_GOTFOCUS");
    _ASSIGN_eventName(NS_IMAGE_ABORT,"NS_IMAGE_ABORT");
    _ASSIGN_eventName(NS_IMAGE_ERROR,"NS_IMAGE_ERROR");
    _ASSIGN_eventName(NS_IMAGE_LOAD,"NS_IMAGE_LOAD");
    _ASSIGN_eventName(NS_KEY_DOWN,"NS_KEY_DOWN");
    _ASSIGN_eventName(NS_KEY_PRESS,"NS_KEY_PRESS");
    _ASSIGN_eventName(NS_KEY_UP,"NS_KEY_UP");
    _ASSIGN_eventName(NS_LOSTFOCUS,"NS_LOSTFOCUS");
    _ASSIGN_eventName(NS_MENU_SELECTED,"NS_MENU_SELECTED");
    _ASSIGN_eventName(NS_MOUSE_ENTER,"NS_MOUSE_ENTER");
    _ASSIGN_eventName(NS_MOUSE_EXIT,"NS_MOUSE_EXIT");
    _ASSIGN_eventName(NS_MOUSE_LEFT_BUTTON_DOWN,"NS_MOUSE_LEFT_BTN_DOWN");
    _ASSIGN_eventName(NS_MOUSE_LEFT_BUTTON_UP,"NS_MOUSE_LEFT_BTN_UP");
    _ASSIGN_eventName(NS_MOUSE_LEFT_CLICK,"NS_MOUSE_LEFT_CLICK");
    _ASSIGN_eventName(NS_MOUSE_LEFT_DOUBLECLICK,"NS_MOUSE_LEFT_DBLCLICK");
    _ASSIGN_eventName(NS_MOUSE_MIDDLE_BUTTON_DOWN,"NS_MOUSE_MIDDLE_BTN_DOWN");
    _ASSIGN_eventName(NS_MOUSE_MIDDLE_BUTTON_UP,"NS_MOUSE_MIDDLE_BTN_UP");
    _ASSIGN_eventName(NS_MOUSE_MIDDLE_CLICK,"NS_MOUSE_MIDDLE_CLICK");
    _ASSIGN_eventName(NS_MOUSE_MIDDLE_DOUBLECLICK,"NS_MOUSE_MIDDLE_DBLCLICK");
    _ASSIGN_eventName(NS_MOUSE_MOVE,"NS_MOUSE_MOVE");
    _ASSIGN_eventName(NS_MOUSE_RIGHT_BUTTON_DOWN,"NS_MOUSE_RIGHT_BTN_DOWN");
    _ASSIGN_eventName(NS_MOUSE_RIGHT_BUTTON_UP,"NS_MOUSE_RIGHT_BTN_UP");
    _ASSIGN_eventName(NS_MOUSE_RIGHT_CLICK,"NS_MOUSE_RIGHT_CLICK");
    _ASSIGN_eventName(NS_MOUSE_RIGHT_DOUBLECLICK,"NS_MOUSE_RIGHT_DBLCLICK");
    _ASSIGN_eventName(NS_MOVE,"NS_MOVE");
    _ASSIGN_eventName(NS_PAGE_LOAD,"NS_PAGE_LOAD");
    _ASSIGN_eventName(NS_PAGE_UNLOAD,"NS_PAGE_UNLOAD");
    _ASSIGN_eventName(NS_PAINT,"NS_PAINT");
    _ASSIGN_eventName(NS_MENU_CREATE,"NS_MENU_CREATE");
    _ASSIGN_eventName(NS_MENU_DESTROY,"NS_MENU_DESTROY");
    _ASSIGN_eventName(NS_MENU_ACTION, "NS_MENU_ACTION");
    _ASSIGN_eventName(NS_XUL_BROADCAST, "NS_XUL_BROADCAST");
    _ASSIGN_eventName(NS_XUL_COMMAND_UPDATE, "NS_XUL_COMMAND_UPDATE");
    _ASSIGN_eventName(NS_SCROLLBAR_LINE_NEXT,"NS_SB_LINE_NEXT");
    _ASSIGN_eventName(NS_SCROLLBAR_LINE_PREV,"NS_SB_LINE_PREV");
    _ASSIGN_eventName(NS_SCROLLBAR_PAGE_NEXT,"NS_SB_PAGE_NEXT");
    _ASSIGN_eventName(NS_SCROLLBAR_PAGE_PREV,"NS_SB_PAGE_PREV");
    _ASSIGN_eventName(NS_SCROLLBAR_POS,"NS_SB_POS");
    _ASSIGN_eventName(NS_SIZE,"NS_SIZE");

#undef _ASSIGN_eventName

  default: 
    {
      char buf[32];
      
      sprintf(buf,"UNKNOWN: %d",aGuiEvent->message);
      
      eventName = buf;
    }
    break;
  }
  
  return nsAutoString(eventName);
}
//////////////////////////////////////////////////////////////
//
// The idea here is to get the prefs service once and cache it.
// The reason being that this code gets called from OnPaint() 
// which we dont want to slow down.
//
// So, gPrefs will leak when the beast shutdowns.
//
// debug_CleanupCrapSoThatBruceAndPurifyAreHappy() can be called from
// the widget dll unloading hook to cleanup...
//
// But then again, its only debug code, so...
//
//////////////////////////////////////////////////////////////
static nsIPref * gPrefs = nsnull;

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);

static nsIPref *
_GetPrefService()
{
  if (!gPrefs)
  {
    nsresult rv = nsServiceManager::GetService(kPrefCID, 
                                               NS_GET_IID(nsIPref),
                                               (nsISupports**) &gPrefs);
    
    NS_ASSERTION(NS_SUCCEEDED(rv),"Could not get prefs service.");
    NS_ASSERTION(nsnull != gPrefs,"Prefs services is null.");
  }

  return gPrefs;
}
//////////////////////////////////////////////////////////////
static PRBool
_GetBoolPref(const char * aPrefName)
{
  NS_ASSERTION(nsnull != aPrefName,"cmon, pref name is null.");

  PRBool value = PR_FALSE;

  nsIPref * prefs = _GetPrefService();

  if (prefs)
    prefs->GetBoolPref(aPrefName,&value);

  return value;
}
//////////////////////////////////////////////////////////////
static PRInt32
_GetPrintCount()
{
  static PRInt32 sCount = 0;
  
  return ++sCount;
}
//////////////////////////////////////////////////////////////
/* static */ PRBool
nsBaseWidget::debug_WantPaintFlashing()
{
  return _GetBoolPref("nglayout.debug.paint_flashing");
}
//////////////////////////////////////////////////////////////
/* static */ void
nsBaseWidget::debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              nsGUIEvent *          aGuiEvent,
                              const nsCAutoString & aWidgetName,
                              PRInt32               aWindowID)
{
  // NS_PAINT is handled by debug_DumpPaintEvent()
  if (aGuiEvent->message == NS_PAINT)
    return;

  if (aGuiEvent->message == NS_MOUSE_MOVE)
  {
    if (!_GetBoolPref("nglayout.debug.motion_event_dumping"))
      return;
  }
  
  if (aGuiEvent->message == NS_MOUSE_ENTER || 
      aGuiEvent->message == NS_MOUSE_EXIT)
  {
    if (!_GetBoolPref("nglayout.debug.crossing_event_dumping"))
      return;
  }

  if (!_GetBoolPref("nglayout.debug.event_dumping"))
    return;
  
  fprintf(aFileOut,
          "%4d %-26s widget=%-8p name=%-12s id=%-8p pos=%d,%d\n",
          _GetPrintCount(),
          (const char *) nsCAutoString(debug_GuiEventToString(aGuiEvent)),
          (void *) aWidget,
          (const char *) aWidgetName,
          (void *) (aWindowID ? aWindowID : 0x0),
          aGuiEvent->point.x,
          aGuiEvent->point.y);
}
//////////////////////////////////////////////////////////////
/* static */ void
nsBaseWidget::debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   nsPaintEvent *        aPaintEvent,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID)
{
  NS_ASSERTION(nsnull != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nsnull != aWidget,"cmon, the widget is null");
  NS_ASSERTION(nsnull != aPaintEvent,"cmon, the paint event is null");

  if (!_GetBoolPref("nglayout.debug.paint_dumping"))
    return;
  
  fprintf(aFileOut,
          "%4d PAINT      widget=%p name=%-12s id=%-8p rect=", 
          _GetPrintCount(),
          (void *) aWidget,
          (const char *) aWidgetName,
          (void *) aWindowID);
  
  if (aPaintEvent->rect) 
  {
    fprintf(aFileOut,
            "%3d,%-3d %3d,%-3d",
            aPaintEvent->rect->x, 
            aPaintEvent->rect->y,
            aPaintEvent->rect->width, 
            aPaintEvent->rect->height);
  }
  else
  {
    fprintf(aFileOut,"none");
  }
  
  fprintf(aFileOut,"\n");
}
//////////////////////////////////////////////////////////////
/* static */ void
nsBaseWidget::debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsRect *        aRect,
                                   PRBool                aIsSynchronous,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID)
{
  if (!_GetBoolPref("nglayout.debug.invalidate_dumping"))
    return;

  NS_ASSERTION(nsnull != aFileOut,"cmon, null output FILE");
  NS_ASSERTION(nsnull != aWidget,"cmon, the widget is null");

  fprintf(aFileOut,
          "%4d Invalidate widget=%p name=%-12s id=%-8p",
          _GetPrintCount(),
          (void *) aWidget,
          (const char *) aWidgetName,
          (void *) aWindowID);

  if (aRect) 
  {
    fprintf(aFileOut,
            " rect=%3d,%-3d %3d,%-3d",
            aRect->x, 
            aRect->y,
            aRect->width, 
            aRect->height);
  }
  else
  {
    fprintf(aFileOut,
            " rect=%-15s",
            "none");
  }

  fprintf(aFileOut,
          " sync=%s",
          (const char *) (aIsSynchronous ? "yes" : "no "));
  
  fprintf(aFileOut,"\n");
}
//////////////////////////////////////////////////////////////
/* static */ void
nsBaseWidget::debug_CleanupCrapSoThatBruceAndPurifyAreHappy()
{
  if (gPrefs)
  {
    nsServiceManager::ReleaseService(kPrefCID, gPrefs);

    gPrefs = nsnull;
  }
}
//////////////////////////////////////////////////////////////

#endif // NS_DEBUG






















//-------------------------------------------------------------------------
//
// Constructor
//
//-------------------------------------------------------------------------

nsBaseWidget::Enumerator::Enumerator(nsBaseWidget & inParent)
  : mCurrentPosition(0), mParent(inParent)
{
  NS_INIT_REFCNT();
}


//-------------------------------------------------------------------------
//
// Destructor
//
//-------------------------------------------------------------------------
nsBaseWidget::Enumerator::~Enumerator()
{   
}


//enumerator interfaces
NS_IMETHODIMP
nsBaseWidget::Enumerator::Next()
{
  PRUint32 itemCount = 0;
  mParent.mChildren->Count(&itemCount);
  if (mCurrentPosition < itemCount - 1 )
    mCurrentPosition ++;
  else
    return NS_ERROR_FAILURE;
  return NS_OK;
}


 
NS_IMETHODIMP
nsBaseWidget::Enumerator::Prev()
{
  if (mCurrentPosition > 0 )
    mCurrentPosition --;
  else
    return NS_ERROR_FAILURE;
  return NS_OK;
}



NS_IMETHODIMP
nsBaseWidget::Enumerator::CurrentItem(nsISupports **aItem)
{
  if (!aItem)
    return NS_ERROR_NULL_POINTER;

  PRUint32 itemCount = 0;
  mParent.mChildren->Count(&itemCount);
  if ( mCurrentPosition < itemCount ) {
    nsISupports* widget = mParent.mChildren->ElementAt(mCurrentPosition);
//  NS_IF_ADDREF(widget);		already addref'd in nsSupportsArray::ElementAt()
    *aItem = widget;
  }
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}



NS_IMETHODIMP
nsBaseWidget::Enumerator::First()
{
  PRUint32 itemCount = 0;
  mParent.mChildren->Count(&itemCount);
  if ( itemCount ) {
    mCurrentPosition = 0;
    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}



NS_IMETHODIMP
nsBaseWidget::Enumerator::Last()
{
  PRUint32 itemCount = 0;
  mParent.mChildren->Count(&itemCount);
  if ( itemCount ) {
    mCurrentPosition = itemCount - 1;
    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;

  return NS_OK;
}



NS_IMETHODIMP
nsBaseWidget::Enumerator::IsDone()
{
  PRUint32 itemCount = 0;
  mParent.mChildren->Count(&itemCount);

  if ((mCurrentPosition == itemCount-1) || (itemCount == 0) ){ //empty lists always return done
    return NS_OK;
  }
  else {
    return NS_ENUMERATOR_FALSE;
  }
  return NS_OK;
}















































#if 0

  /* void initWidget (in nsIAppShell aAppShell, in nsIToolkit aToolkit, in nsIDeviceContext aContext, in EVENT_CALLBACK aEventFunction); */
  NS_IMETHOD InitWidget(nsIAppShell *aAppShell, nsIToolkit *aToolkit, nsIDeviceContext * aContext, EVENT_CALLBACK aEventFunction) = 0;

  /* voidStar getNativeData (in PRUint32 aDataType); */
  NS_IMETHOD GetNativeData(PRUint32 aDataType, void * *_retval) = 0;

  /* void move (in PRInt32 aX, in PRInt32 aY); */
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY) = 0;

  /* void resize (in PRInt32 aWidth, in PRInt32 aHeight, in PRBool aRepaint); */
  NS_IMETHOD Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint) = 0;

  /* void moveResize (in PRInt32 aX, in PRInt32 aY, in PRInt32 aWidth, in PRInt32 aHeight, in PRBool aRepaint); */
  NS_IMETHOD MoveResize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint) = 0;

  /* void enable (in PRBool aState); */
  NS_IMETHOD Enable(PRBool aState) = 0;

  /* void setFocus (); */
  NS_IMETHOD SetFocus(void) = 0;

  /* void getBounds (in nsRectRef aRect); */
  NS_IMETHOD GetBounds(nsRect & aRect) = 0;

  /* void getClientBounds (in nsRectRef aRect); */
  NS_IMETHOD GetClientBounds(nsRect & aRect) = 0;

  /* void getPreferredSize (out PRInt32 aWidth, out PRInt32 aHeight); */
  NS_IMETHOD GetPreferredSize(PRInt32 *aWidth, PRInt32 *aHeight) = 0;

  /* void setPreferredSize (in PRInt32 aWidth, in PRInt32 aHeight); */
  NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight) = 0;

  /* void invalidate (in PRBool aIsSynchronous); */
  NS_IMETHOD Invalidate(PRBool aIsSynchronous) = 0;

  /* void invalidateRect ([const] in nsRect aRect, in PRBool aIsSynchronous); */
  NS_IMETHOD InvalidateRect(const nsRect * aRect, PRBool aIsSynchronous) = 0;

  /* void invalidateRegion ([const] in nsIRegion aRegion, in PRBool aIsSynchronous); */
  NS_IMETHOD InvalidateRegion(const nsIRegion * aRegion, PRBool aIsSynchronous) = 0;

  /* void update (); */
  NS_IMETHOD Update(void) = 0;

  /* void widgetToScreen ([const] in nsRect aOldRect, out nsRect aNewRect); */
  NS_IMETHOD WidgetToScreen(const nsRect * aOldRect, nsRect * *aNewRect) = 0;

  /* void screenToWidget ([const] in nsRect aOldRect, out nsRect aNewRect); */
  NS_IMETHOD ScreenToWidget(const nsRect * aOldRect, nsRect * *aNewRect) = 0;

  /* void convertToDeviceCoordinates (inout nscoord aX, inout nscoord aY); */
  NS_IMETHOD ConvertToDeviceCoordinates(nscoord *aX, nscoord *aY) = 0;

  /* void paint (in nsIRenderingContext aRenderingContext, [const] in nsRect aDirtyRect); */
  NS_IMETHOD Paint(nsIRenderingContext * aRenderingContext, const nsRect * aDirtyRect) = 0;

  /* void enableDragDrop (in PRBool aEnable); */
  NS_IMETHOD EnableDragDrop(PRBool aEnable) = 0;

  /* void captureMouse (in PRBool aCapture); */
  NS_IMETHOD CaptureMouse(PRBool aCapture) = 0;

  /* void captureRollupEvents (in nsIRollupListener aListener, in PRBool aDoCapture, in PRBool aConsumeRollupEvent); */
  NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent) = 0;

  /* void addMouseListener (in nsIMouseListener aListener); */
  NS_IMETHOD AddMouseListener(nsIMouseListener * aListener) = 0;

  /* void addEventListener (in nsIEventListener aListener); */
  NS_IMETHOD AddEventListener(nsIEventListener * aListener) = 0;

  /* void addMenuListener (in nsIMenuListener aListener); */
  NS_IMETHOD AddMenuListener(nsIMenuListener * aListener) = 0;

  /* void dispatchEvent (in nsGUIEvent event, in nsEventStatusRef aStatus); */
  NS_IMETHOD DispatchEvent(nsGUIEvent * event, nsEventStatus & aStatus) = 0;

  /* void addChild (in nsIWidget aChild); */
  NS_IMETHOD AddChild(nsIWidget *aChild) = 0;

  /* void removeChild (in nsIWidget aChild); */
  NS_IMETHOD RemoveChild(nsIWidget *aChild) = 0;

  /* readonly attribute nsIDeviceContext deviceContext; */
  NS_IMETHOD GetDeviceContext(nsIDeviceContext * *aDeviceContext) = 0;

  /* readonly attribute nsIAppShell appShell; */
  NS_IMETHOD GetAppShell(nsIAppShell * *aAppShell) = 0;

  /* readonly attribute nsIToolkit toolkit; */
  NS_IMETHOD GetToolkit(nsIToolkit * *aToolkit) = 0;

  /* [noscript] readonly attribute EVENT_CALLBACK eventFunction; */
  NS_IMETHOD GetEventFunction(EVENT_CALLBACK *aEventFunction) = 0;

  /* attribute PRInt32 zIndex; */
  NS_IMETHOD GetZIndex(PRInt32 *aZIndex) = 0;
  NS_IMETHOD SetZIndex(PRInt32 aZIndex) = 0;

  /* attribute nscolor foregroundColor; */
  NS_IMETHOD GetForegroundColor(nscolor *aForegroundColor) = 0;
  NS_IMETHOD SetForegroundColor(nscolor aForegroundColor) = 0;

  /* attribute nscolor backgroundColor; */
  NS_IMETHOD GetBackgroundColor(nscolor *aBackgroundColor) = 0;
  NS_IMETHOD SetBackgroundColor(nscolor aBackgroundColor) = 0;

  /* attribute nsFont font; */
  NS_IMETHOD GetFont(nsFont * *aFont) = 0;
  NS_IMETHOD SetFont(nsFont * aFont) = 0;

  /* attribute nsCursor cursor; */
  NS_IMETHOD GetCursor(nsCursor *aCursor) = 0;
  NS_IMETHOD SetCursor(nsCursor aCursor) = 0;

  /* attribute nsColorMap colorMap; */
  NS_IMETHOD GetColorMap(nsColorMap * *aColorMap) = 0;
  NS_IMETHOD SetColorMap(nsColorMap * aColorMap) = 0;

  /* attribute voidStar clientData; */
  NS_IMETHOD GetClientData(void * *aClientData) = 0;
  NS_IMETHOD SetClientData(void * aClientData) = 0;

  /* readonly attribute nsIEnumerator children; */
  NS_IMETHOD GetChildren(nsIEnumerator * *aChildren) = 0;



  /* [noscript] void initWindow (in nativeWindow parentNativeWindow, in nsIWidget parentWidget, in long x, in long y, in long cx, in long cy); */
  NS_IMETHOD InitWindow(nativeWindow parentNativeWindow, nsIWidget *parentWidget, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy) = 0;

  /* void create (); */
  NS_IMETHOD Create(void) = 0;

  /* void destroy (); */
  NS_IMETHOD Destroy(void) = 0;

  /* void setPosition (in long x, in long y); */
  NS_IMETHOD SetPosition(PRInt32 x, PRInt32 y) = 0;

  /* void getPosition (out long x, out long y); */
  NS_IMETHOD GetPosition(PRInt32 *x, PRInt32 *y) = 0;

  /* void setSize (in long cx, in long cy, in boolean fRepaint); */
  NS_IMETHOD SetSize(PRInt32 cx, PRInt32 cy, PRBool fRepaint) = 0;

  /* void getSize (out long cx, out long cy); */
  NS_IMETHOD GetSize(PRInt32 *cx, PRInt32 *cy) = 0;

  /* void setPositionAndSize (in long x, in long y, in long cx, in long cy, in boolean fRepaint); */
  NS_IMETHOD SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy, PRBool fRepaint) = 0;

  /* void repaint (in boolean force); */
  NS_IMETHOD Repaint(PRBool force) = 0;

  /* attribute nsIWidget parentWidget; */
  NS_IMETHOD GetParentWidget(nsIWidget * *aParentWidget) = 0;
  NS_IMETHOD SetParentWidget(nsIWidget * aParentWidget) = 0;

  /* attribute nativeWindow parentNativeWindow; */
  NS_IMETHOD GetParentNativeWindow(nativeWindow *aParentNativeWindow) = 0;
  NS_IMETHOD SetParentNativeWindow(nativeWindow aParentNativeWindow) = 0;

  /* attribute boolean visibility; */
  NS_IMETHOD GetVisibility(PRBool *aVisibility) = 0;
  NS_IMETHOD SetVisibility(PRBool aVisibility) = 0;

  /* readonly attribute nsIWidget mainWidget; */
  NS_IMETHOD GetMainWidget(nsIWidget * *aMainWidget) = 0;

  /* void setFocus (); */
  NS_IMETHOD SetFocus(void) = 0;

  /* attribute wstring title; */
  NS_IMETHOD GetTitle(PRUnichar * *aTitle) = 0;
  NS_IMETHOD SetTitle(const PRUnichar * aTitle) = 0;

#endif







NS_IMETHODIMP nsBaseWidget::SetVisibility(PRBool aShow)
{
  return Show(aShow);
}

NS_IMETHODIMP nsBaseWidget::GetVisibility(PRBool *aState)
{
  return IsVisible(*aState);
}

NS_IMETHODIMP nsBaseWidget::SetPosition(PRInt32 aX, PRInt32 aY)
{
  return Move(aX, aY);
}

NS_IMETHODIMP nsBaseWidget::SetSize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  return Resize(aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP nsBaseWidget::SetPositionAndSize(PRInt32 aX,
                                               PRInt32 aY,
                                               PRInt32 aWidth,
                                               PRInt32 aHeight,
                                               PRBool  aRepaint)
{
  return Resize(aX, aY, aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP nsBaseWidget::GetForegroundColor(nscolor *aColor)
{
  *aColor = GetForegroundColor();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::SetForegroundColor(nscolor aColor)
{
  mForeground = aColor;
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetBackgroundColor(nscolor *aColor)
{
  *aColor = GetBackgroundColor();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::SetBackgroundColor(nscolor aColor)
{
  mBackground = aColor;
  return NS_OK;
}


NS_IMETHODIMP nsBaseWidget::SetFont(nsFont *aFont)
{
  // XXX ugly cast
  return SetFont((const nsFont &)*aFont);
}

NS_IMETHODIMP nsBaseWidget::GetFont(nsFont **aFont)
{
  //  nsCOMPtr<nsIFontMetrics> fontMetrics;
  //  GetFont(getter_AddRefs(fontMetrics));
  //  return fontMetrics.get();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetCursor(nsCursor *aCursor)
{
  *aCursor = GetCursor();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::InvalidateRect(const nsRect *aRect, PRBool aIsSynchronous)
{
  return Invalidate((const nsRect &)*aRect, aIsSynchronous);
}

NS_IMETHODIMP nsBaseWidget::GetPreferredSize(PRInt32 *aWidth, PRInt32 *aHeight)
{
  return GetPreferredSize(*aWidth, *aHeight);
}

NS_IMETHODIMP nsBaseWidget::GetParentWidget(nsIWidget **aWidget)
{
  *aWidget = GetParent();
  return NS_OK;
}


NS_IMETHODIMP nsBaseWidget::AddChild(nsIWidget *aChild)
{
  AddChild(aChild);
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::RemoveChild(nsIWidget *aChild)
{
  RemoveChild(aChild);
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetChildren(nsIEnumerator **aChildren)
{
  *aChildren = GetChildren();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetNativeData(PRUint32 aDataType, void **aData)
{
  *aData = GetNativeData(aDataType);
  return NS_OK;
}
/*
  virtual void nsBaseWidget::FreeNativeData(void *data, PRUint32 aDataType)
  {
  
  }

  nsIRenderingContext* nsBaseWidget::GetRenderingContext()
  {
    nsCOMPtr<nsIRenderingContext> renderingContext;
    GetRenderingContext(getter_AddRefs(renderingContext));
    return renderingContext;
  }
*/

NS_IMETHODIMP nsBaseWidget::GetDeviceContext(nsIDeviceContext **aDeviceContext)
{
  *aDeviceContext = GetDeviceContext();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetAppShell(nsIAppShell **aAppShell)
{
  *aAppShell = GetAppShell();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetToolkit(nsIToolkit **aToolkit)
{
  *aToolkit = GetToolkit();
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::GetClientData(void **aData)
{
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::Paint(nsIRenderingContext *aContext, const nsRect *aRect)
{
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::ConvertToDeviceCoordinates(nscoord *aX, nscoord *aY)
{
  return NS_OK;
}



NS_IMETHODIMP nsBaseWidget::InitWidget(nsIAppShell      *aAppShell,
                                       nsIToolkit       *aToolkit,
                                       nsIDeviceContext *aContext,
                                       EVENT_CALLBACK   aEventFunction)
{
  // this stuff should all get addref'd by BaseCreate()
  mAppShell = aAppShell;
  mToolkit = aToolkit;
  mContext = aContext;
  mEventCallback = aEventFunction;
  return NS_OK;
}


NS_IMETHODIMP nsBaseWidget::InitWindow(nativeWindow aParentNativeWindow,
                         nsIWidget *aParentWidget,
                         PRInt32 aX, PRInt32 aY,
                         PRInt32 aWidth, PRInt32 aHeight)
{
  // this stuff should all get addref'd by BaseCreate();
  //  mParent = aParentWidget;
  mBounds.width = aWidth;
  mBounds.height = aHeight;
  return NS_OK;
}

NS_IMETHODIMP nsBaseWidget::Create()
{
  nsRect rect(0, 0, mBounds.width, mBounds.height);
  //  nsRect(mX, mY, mWidth, mHeight);
  Create((nsIWidget*)nsnull, rect, mEventCallback, mContext, mAppShell, mToolkit, nsnull);
  return NS_OK;
}
