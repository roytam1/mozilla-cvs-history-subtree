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

#ifndef nsView_h___
#define nsView_h___

#include "nsISupports.h"
#include "nsIView.h"
#include "nsRect.h"
#include "nsCRT.h"
#include "nsIFactory.h"
#include "nsIViewObserver.h"

class nsIWindow;

//mmptemp

class nsIPresContext;
class nsIViewManager;


#include "nsGUIEvent.h"
#include "nsIGUIEventListener.h"

class nsGUIEventListener : public nsIGUIEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGUIEVENTLISTENER

  nsGUIEventListener() {} ;
  virtual ~nsGUIEventListener() {};
};

class nsView : public nsIView
{
public:
  nsView();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports
  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  // nsIView
  NS_IMETHOD  Init(nsIViewManager* aManager,
      						 const nsRect &aBounds,
                   const nsIView *aParent,
      						 nsViewVisibility aVisibilityFlag = nsViewVisibility_kShow);

  NS_IMETHOD  Destroy();
  NS_IMETHOD  GetViewManager(nsIViewManager *&aViewMgr) const;
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsRect& rect,
                    PRUint32 aPaintFlags, PRBool &aResult);
  NS_IMETHOD  Paint(nsIRenderingContext& rc, const nsIRegion& region,
                    PRUint32 aPaintFlags, PRBool &aResult);
  NS_IMETHOD  HandleEvent(nsGUIEvent *event, 
                          PRUint32 aEventFlags,
                          nsEventStatus* aStatus,
                          PRBool aForceHandle,
                          PRBool& aHandled);
  NS_IMETHOD  SetPosition(gfx_coord x, gfx_coord y);
  NS_IMETHOD  GetPosition(gfx_coord *x, gfx_coord *y) const;
  NS_IMETHOD  SetDimensions(gfx_coord width, gfx_coord height, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  GetDimensions(gfx_coord *width, gfx_coord *height) const;
  NS_IMETHOD  SetBounds(const nsRect &aBounds, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  SetBounds(gfx_coord aX, gfx_coord aY, gfx_coord aWidth, gfx_coord aHeight, PRBool aPaint = PR_TRUE);
  NS_IMETHOD  GetBounds(nsRect &aBounds) const;
  NS_IMETHOD  SetChildClip(gfx_coord aX, gfx_coord aY, gfx_coord aWidth, gfx_coord aHeight);
  NS_IMETHOD  GetChildClip(gfx_coord *aLeft, gfx_coord *aTop, gfx_coord *aRight, gfx_coord *aBottom) const;
  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);
  NS_IMETHOD  GetVisibility(nsViewVisibility &aVisibility) const;
  NS_IMETHOD  SetZParent(nsIView *aZParent);
  NS_IMETHOD  GetZParent(nsIView *&aZParent) const;
  NS_IMETHOD  SetZIndex(PRInt32 aZIndex);
  NS_IMETHOD  GetZIndex(PRInt32 &aZIndex) const;
  NS_IMETHOD  SetAutoZIndex(PRBool aAutoZIndex);
  NS_IMETHOD  GetAutoZIndex(PRBool &aAutoZIndex) const;
  NS_IMETHOD  SetFloating(PRBool aFloatingView);
  NS_IMETHOD  GetFloating(PRBool &aFloatingView) const;
  NS_IMETHOD  SetParent(nsIView *aParent);
  NS_IMETHOD  GetParent(nsIView *&aParent) const;
  NS_IMETHOD  GetNextSibling(nsIView *&aNextSibling) const;
  NS_IMETHOD  SetNextSibling(nsIView* aNextSibling);
  NS_IMETHOD  InsertChild(nsIView *child, nsIView *sibling);
  NS_IMETHOD  RemoveChild(nsIView *child);
  NS_IMETHOD  GetChildCount(PRInt32 &aCount) const;
  NS_IMETHOD  GetChild(PRInt32 index, nsIView*& aChild) const;
  NS_IMETHOD  SetTransform(nsTransform2D &aXForm);
  NS_IMETHOD  GetTransform(nsTransform2D &aXForm) const;
  NS_IMETHOD  SetOpacity(float opacity);
  NS_IMETHOD  GetOpacity(float &aOpacity) const;
  NS_IMETHOD  HasTransparency(PRBool &aTransparent) const;
  NS_IMETHOD  SetContentTransparency(PRBool aTransparent);
  NS_IMETHOD  SetClientData(void *aData);
  NS_IMETHOD  GetClientData(void *&aData) const;
  NS_IMETHOD  GetOffsetFromWidget(gfx_coord *aDx, gfx_coord *aDy, nsIWindow *aWidget);
  NS_IMETHOD  GetDirtyRegion(nsIRegion*& aRegion) const;
  NS_IMETHOD  CreateWidget(const char *contractid);
  NS_IMETHOD  SetWidget(nsIWindow *aWidget);
  NS_IMETHOD  GetWidget(nsIWindow **aWidget) const;
  NS_IMETHOD  HasWidget(PRBool *aHasWidget) const;
  NS_IMETHOD  List(FILE* out = stdout, PRInt32 aIndent = 0) const;
  NS_IMETHOD  SetViewFlags(PRUint32 aFlags);
  NS_IMETHOD  ClearViewFlags(PRUint32 aFlags);
  NS_IMETHOD  GetViewFlags(PRUint32 *aFlags) const;
  NS_IMETHOD  GetScratchPoint(nsPoint **aPoint);
  NS_IMETHOD  SetCompositorFlags(PRUint32 aFlags);
  NS_IMETHOD  GetCompositorFlags(PRUint32 *aFlags);
  NS_IMETHOD  GetExtents(nsRect *aExtents);
  NS_IMETHOD  GetClippedRect(nsRect& aClippedRect, PRBool& aIsClipped, PRBool& aEmpty) const;


  // XXX Temporary for Bug #19416
  NS_IMETHOD IgnoreSetPosition(PRBool aShouldIgnore);

  NS_IMETHOD SynchWidgetSizePosition();


  // Helper function to get the view that's associated with a widget
  static nsIView*  GetViewFor(nsIWindow* aWidget);

   // Helper function to determine if the view instance is the root view
  PRBool IsRoot();

   // Helper function to determine if the view point is inside of a view
  PRBool PointIsInside(nsIView& aView, gfx_coord x, gfx_coord y) const;

protected:
  virtual ~nsView();
  //
  virtual nsresult LoadWidget(const nsCID &aClassIID);

protected:
  nsIViewManager    *mViewManager;
  nsIView           *mParent;
  nsIWindow         *mWindow;

  nsIView           *mZParent;

  //XXX should there be pointers to last child so backward walking is fast?
  nsIView           *mNextSibling;
  nsIView           *mFirstChild;
  void              *mClientData;
  PRInt32           mZindex;
  nsViewVisibility  mVis;
  PRInt32           mNumKids;
  nsRect            mBounds;
  nsViewClip        mChildClip;
  nsTransform2D     *mXForm;
  float             mOpacity;
  PRUint32          mVFlags;
  nsIRegion*        mDirtyRegion;
  nsPoint           mScratchPoint;
  PRUint32			mCompositorFlags;

  // Bug #19416
  PRBool            mShouldIgnoreSetPosition;

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);
};

#endif
