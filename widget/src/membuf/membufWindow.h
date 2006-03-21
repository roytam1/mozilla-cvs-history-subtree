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

#ifndef _MEMBUFWINDOW_H_
#define _MEMBUFWINDOW_H_

#include "nsBaseWidget.h"
#include "nsGUIEvent.h"
#include "nsRect.h"

// Allegro defines non-ISO-C++ compliant ZERO_SIZED_ARRAY
#include "membufAllegroDefines.h"
#include <allegro.h>

class membufWindow : public nsBaseWidget
{
public:
    membufWindow();
    virtual ~membufWindow();

    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD Create(nsIWidget        *aParent,
                      const nsRect     &aRect,
                      EVENT_CALLBACK   aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell      *aAppShell = nsnull,
                      nsIToolkit       *aToolkit = nsnull,
                      nsWidgetInitData *aInitData = nsnull);

    NS_IMETHOD Create(nsNativeWidget   aParent,
                      const nsRect     &aRect,
                      EVENT_CALLBACK   aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell      *aAppShell = nsnull,
                      nsIToolkit       *aToolkit = nsnull,
                      nsWidgetInitData *aInitData = nsnull);

    NS_IMETHOD Destroy(void);
    NS_IMETHOD SetParent(nsIWidget* aNewParent);
    virtual nsIWidget* GetParent(void);

    NS_IMETHOD Show(PRBool aState);
    NS_IMETHOD SetModal(PRBool aModal);
    NS_IMETHOD IsVisible(PRBool & aState);
    NS_IMETHOD ConstrainPosition(PRBool aAllowSlop,
                                 PRInt32 *aX,
                                 PRInt32 *aY);
    NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);

    NS_IMETHOD Resize(PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint);
    NS_IMETHOD Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint);

    NS_IMETHOD PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                           nsIWidget *aWidget, PRBool aActivate);

    NS_IMETHOD Enable(PRBool aState);
    NS_IMETHOD IsEnabled(PRBool *aState);

    NS_IMETHOD SetFocus(PRBool aRaise = PR_FALSE);
    NS_IMETHOD LoseFocus();

    NS_IMETHOD GetBounds(nsRect &aRect);
    NS_IMETHOD GetScreenBounds(nsRect &aRect);
    NS_IMETHOD GetClientBounds(nsRect &aRect);

    NS_IMETHOD GetBorderSize(PRInt32 &aWidth, PRInt32 &aHeight);

    virtual nsIFontMetrics* GetFont(void);
    NS_IMETHOD SetFont(const nsFont &aFont);

    virtual nsCursor GetCursor(void);

    NS_IMETHOD Validate();
    NS_IMETHOD Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
    NS_IMETHOD InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous);
    NS_IMETHOD Update();

    NS_IMETHOD SetColorMap(nsColorMap *aColorMap);

    NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
    NS_IMETHOD ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
    NS_IMETHOD ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy);

    virtual void* GetNativeData(PRUint32 aDataType);
    virtual void FreeNativeData(void * data, PRUint32 aDataType);

    NS_IMETHOD SetTitle(const nsAString& aTitle);

    NS_IMETHOD SetIcon(const nsAString& anIconSpec);

    NS_IMETHOD SetMenuBar(nsIMenuBar * aMenuBar);
    NS_IMETHOD ShowMenuBar(PRBool aShow);

    NS_IMETHOD WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
    NS_IMETHOD ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);

    NS_IMETHOD BeginResizingChildren(void);
    NS_IMETHOD EndResizingChildren(void);

    NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
    NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);

    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);

    NS_IMETHOD EnableDragDrop(PRBool aEnable);
    virtual void  ConvertToDeviceCoordinates(nscoord &aX,nscoord &aY);

    NS_IMETHOD CaptureMouse(PRBool aCapture);

    NS_IMETHOD GetWindowClass(char *aClass);
    NS_IMETHOD SetWindowClass(char *aClass);

    NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener,
                                   PRBool aDoCapture,
                                   PRBool aConsumeRollupEvent);

    NS_IMETHOD ModalEventFilter(PRBool aRealEvent,
                                void *aEvent,
                                PRBool *aForWindow);

    NS_IMETHOD GetAttention(PRInt32 aCycleCount);

    NS_IMETHOD AddMouseListener(nsIMouseListener * aListener);
    NS_IMETHOD AddEventListener(nsIEventListener * aListener);
    NS_IMETHOD AddMenuListener(nsIMenuListener * aListener);

protected:
    nsresult DoPaint(nsIRegion *aPaintRegion);
    nsresult Paint(nsIRenderingContext& aRenderingContext,
                   const nsRect& aBounds);
    nsresult InvalidateHelper(PRBool aIsSynchronous);
    nsresult InvalidateChildren(nsIRegion *aUpdateRegion, PRBool aIsSync);
    nsresult QueueUpdate();

    void calculateAbsolutePosition(int& total_offset_x, int& total_offset_y);

    void InitGUIEvent(nsGUIEvent &aEvent, PRUint32 aMsg);
    void DispatchGotFocusEvent();
    void DispatchLostFocusEvent();
    void DispatchActivateEvent(void);

    virtual void createBitmap();

    nsCOMPtr<nsIWidget> mParent;
    PRBool mEnabled;
    PRBool mIsVisible;
    PRBool mUpdateQueued;

private:
    static bool sNeed_install_allegro;
    BITMAP* mParentBitmap;
    BITMAP* mBitmap;
    static int sWindowCounter;
    int mCounter;
    bool mDirty;
    bool hasFocus;
    int* bitmap_refcount;
    nsCOMPtr<nsIRegion> mUpdateRegion;

};

#endif
