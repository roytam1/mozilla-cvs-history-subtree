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
#ifndef nsBaseWidget_h__
#define nsBaseWidget_h__

#include "nsRect.h"
#include "nsIWidget.h"
#include "nsIGenericWindow.h"
#include "nsIWindow.h"
#include "nsIEnumerator.h"
#include "nsIMouseListener.h"
#include "nsIEventListener.h"
#include "nsIMenuListener.h"
#include "nsIToolkit.h"
#include "nsStringUtil.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsISupportsArray.h"
#include "nsCOMPtr.h"

/**
 * Common widget implementation used as base class for native
 * or crossplatform implementations of Widgets. 
 * All cross-platform behavior that all widgets need to implement 
 * should be placed in this class. 
 * (Note: widget implementations are not required to use this
 * class, but it gives them a head start.)
 */

class nsBaseWidget : public nsIWidget, public nsIGenericWindow
{

public:
  nsBaseWidget();
  virtual ~nsBaseWidget();
  
  NS_DECL_ISUPPORTS
  
  /* the end of this file has the methods seperated into new and old */

  NS_IMETHOD              PreCreateWidget(nsWidgetInitData *aWidgetInitData) { return NS_OK;}
  
  // nsIWidget interface
  NS_IMETHOD              CaptureMouse(PRBool aCapture);
  NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
  NS_IMETHOD              Destroy();

  NS_IMETHOD              SetZIndex(PRInt32 aZIndex);
  NS_IMETHOD              GetZIndex(PRInt32* aZIndex);

  NS_IMETHOD              SetModal(void); 
  NS_IMETHOD              SetWindowType(nsWindowType aWindowType);
  NS_IMETHOD              SetBorderStyle(nsBorderStyle aBorderStyle); 
  NS_IMETHOD              AddMouseListener(nsIMouseListener * aListener);
  NS_IMETHOD              AddEventListener(nsIEventListener * aListener);
  NS_IMETHOD              AddMenuListener(nsIMenuListener * aListener);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetBounds(nsRect &aRect);
  NS_IMETHOD              GetBoundsAppUnits(nsRect &aRect, float aAppUnits);
  NS_IMETHOD              GetClientBounds(nsRect &aRect);
  NS_IMETHOD              GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight);
  NS_IMETHOD              ScrollRect(nsRect &aRect, PRInt32 aDx, PRInt32 aDy);
#ifdef LOSER
  NS_IMETHOD              SetVerticalScrollbar(nsIWidget * aScrollbar);
#endif
  NS_IMETHOD              EnableDragDrop(PRBool aEnable);
  virtual void            ConvertToDeviceCoordinates(nscoord  &aX,nscoord &aY) {}
  virtual void            FreeNativeData(void * data, PRUint32 aDataType) {}//~~~

protected:

  virtual void            DrawScaledRect(nsIRenderingContext& aRenderingContext,
                                         const nsRect & aRect,
                                         float aScale,
                                         float aAppUnits);
  virtual void            DrawScaledLine(nsIRenderingContext& aRenderingContext, 
                                         nscoord aSX, nscoord aSY, nscoord aEX, nscoord aEY, 
                                         float   aScale, float aAppUnits, PRBool aIsHorz);
  virtual void            OnDestroy();
  virtual void            BaseCreate(nsIWidget *aParent,
                                     const nsRect &aRect,
                                     EVENT_CALLBACK aHandleEventFunction,
                                     nsIDeviceContext *aContext,
                                     nsIAppShell *aAppShell,
                                     nsIToolkit *aToolkit,
                                     nsWidgetInitData *aInitData);

protected: 
  void*             mClientData;
  EVENT_CALLBACK    mEventCallback;
  nsIDeviceContext  *mContext;
  nsIAppShell       *mAppShell;
  nsIToolkit        *mToolkit;
  nsIMouseListener  *mMouseListener;
  nsIEventListener  *mEventListener;
  nsIMenuListener   *mMenuListener;
  nscolor           mBackground;
  nscolor           mForeground;
  nsCursor          mCursor;
  nsWindowType      mWindowType;
  nsBorderStyle     mBorderStyle;
  PRBool            mIsShiftDown;
  PRBool            mIsControlDown;
  PRBool            mIsAltDown;
  PRBool            mIsDestroying;
  PRBool            mOnDestroyCalled;
  nsRect            mBounds;
#ifdef LOSER
  nsIWidget        *mVScrollbar;
#endif
  PRInt32           mZIndex;

    // keep the list of children
  nsCOMPtr<nsISupportsArray> mChildren;
    
  class Enumerator : public nsIBidirectionalEnumerator {
  public:
    NS_DECL_ISUPPORTS

    Enumerator(nsBaseWidget & inParent);
    virtual ~Enumerator();

    NS_DECL_NSIENUMERATOR
    NS_DECL_NSIBIDIRECTIONALENUMERATOR

  private:
    PRUint32       mCurrentPosition;
    nsBaseWidget& mParent;
  };
  friend class Enumerator;

    // Enumeration of the methods which are accessable on the "main GUI thread"
    // via the CallMethod(...) mechanism...
    // see nsSwitchToUIThread
  enum {
    CREATE       = 0x0101,
    CREATE_NATIVE,
    DESTROY, 
    SET_FOCUS,
    SET_CURSOR,
    CREATE_HACK
  };

#ifdef NS_DEBUG
protected:
  static nsAutoString debug_GuiEventToString(nsGUIEvent * aGuiEvent);
  static PRBool debug_WantPaintFlashing();

  static void debug_DumpInvalidate(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   const nsRect *        aRect,
                                   PRBool                aIsSynchronous,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

  static void debug_DumpEvent(FILE *                aFileOut,
                              nsIWidget *           aWidget,
                              nsGUIEvent *          aGuiEvent,
                              const nsCAutoString & aWidgetName,
                              PRInt32               aWindowID);
  
  static void debug_DumpPaintEvent(FILE *                aFileOut,
                                   nsIWidget *           aWidget,
                                   nsPaintEvent *        aPaintEvent,
                                   const nsCAutoString & aWidgetName,
                                   PRInt32               aWindowID);

public:
  static void debug_CleanupCrapSoThatBruceAndPurifyAreHappy();
#endif











public:
  /* new methods */
  NS_IMETHOD InitWidget(nsIAppShell      *aAppShell,
                        nsIToolkit       *aToolkit,
                        nsIDeviceContext *aContext,
                        EVENT_CALLBACK   aEventFunction);
  NS_IMETHOD InitWindow(nativeWindow aParentNativeWindow,
                        nsIWidget *aParentWidget,
                        PRInt32 aX, PRInt32 aY,
                        PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD Create();

  NS_IMETHOD SetVisibility(PRBool aShow);
  NS_IMETHOD GetVisibility(PRBool *aState);
  NS_IMETHOD SetPosition(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD SetSize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD SetPositionAndSize(PRInt32 aX,
                                PRInt32 aY,
                                PRInt32 aWidth,
                                PRInt32 aHeight,
                                PRBool  aRepaint);
  NS_IMETHOD GetForegroundColor(nscolor *aColor);
  NS_IMETHOD SetForegroundColor(nscolor aColor);
  NS_IMETHOD GetBackgroundColor(nscolor *aColor);
  NS_IMETHOD SetBackgroundColor(nscolor aColor);
  NS_IMETHOD SetFont(nsFont *aFont);
  NS_IMETHOD GetFont(nsFont **aFont);
  NS_IMETHOD GetCursor(nsCursor *aCursor);
  NS_IMETHOD SetCursor(nsCursor aCursor);
  NS_IMETHOD InvalidateRect(const nsRect *aRect, PRBool aIsSynchronous);
  NS_IMETHOD GetPreferredSize(PRInt32 *aWidth, PRInt32 *aHeight);
  NS_IMETHOD GetParentWidget(nsIWidget **aWidget);
  NS_IMETHOD GetChildren(nsIEnumerator **aChildren);
  NS_IMETHOD GetNativeData(PRUint32 aDataType, void **aData);
  NS_IMETHOD GetDeviceContext(nsIDeviceContext **aDeviceContext);
  NS_IMETHOD GetAppShell(nsIAppShell **aAppShell);
  NS_IMETHOD GetToolkit(nsIToolkit **aToolkit);
  NS_IMETHOD GetClientData(void **aData);
  NS_IMETHOD Paint(nsIRenderingContext *, const nsRect *);
  NS_IMETHOD ConvertToDeviceCoordinates(nscoord *, nscoord *);
  NS_IMETHOD SetFocus() { };

public:
  /* old methods that have the same names as new ones */
  NS_IMETHOD              GetClientData(void*& aClientData);
  NS_IMETHOD              SetClientData(void* aClientData);
  virtual nsIEnumerator*  GetChildren();
  NS_IMETHOD              AddChild(nsIWidget* aChild);
  NS_IMETHOD              RemoveChild(nsIWidget* aChild); 
  virtual nscolor         GetForegroundColor(void);
  virtual nscolor         GetBackgroundColor(void);
  virtual nsCursor        GetCursor();
  NS_IMETHOD              Paint(nsIRenderingContext& aRenderingContext, const nsRect& aDirtyRect);
  virtual nsIWidget*      GetParent(void);

  /* base widget does not really support these... they are here only to avoid shadowing problems */
  NS_IMETHOD              SetFont(const nsFont &) = 0;
  virtual nsIFontMetrics *GetFont() = 0;
  NS_IMETHOD              GetPreferredSize(PRInt32 &, PRInt32 &) = 0;
  virtual void *          GetNativeData(unsigned int) = 0;
  virtual nsIRenderingContext* GetRenderingContext();
  virtual nsIDeviceContext* GetDeviceContext();
  virtual nsIAppShell *   GetAppShell();
  virtual nsIToolkit*     GetToolkit();



  NS_IMETHOD Create(nsIWidget        *aParent,
                    const nsRect     &aRect,
                    EVENT_CALLBACK   aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell      *aAppShell = nsnull,
                    nsIToolkit       *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull) = 0;
  NS_IMETHOD Create(nsNativeWidget aParent,
                    const nsRect     &aRect,
                    EVENT_CALLBACK   aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell      *aAppShell = nsnull,
                    nsIToolkit       *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull) = 0;
};

#endif // nsBaseWidget_h__
