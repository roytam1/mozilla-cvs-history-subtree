/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is 
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
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
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

//#include "nsHTMLContainerFrame.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsISVGSVGElement.h"
#include "nsIPresContext.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGLength.h"
#include "nsISVGContainerFrame.h"
#include "nsISVGChildFrame.h"
#include "nsISVGOuterSVGFrame.h"
#include "nsISVGRendererRenderContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsWeakReference.h"
#include "nsISVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsHTMLParts.h"
#include "nsReflowPath.h"
#include "nsISVGRenderer.h"
#include "nsISVGRendererRegion.h"
#include "nsIServiceManager.h"
#include "nsISVGRectangleSink.h"
#include "nsISVGValueUtils.h"
#include "nsISVGViewportRect.h"
#include "nsISVGViewportAxis.h"
#include "nsIDOMSVGNumber.h"

////////////////////////////////////////////////////////////////////////
// VMRectInvalidator: helper class for invalidating rects on the viewmanager.
// used in nsSVGOuterSVGFrame::InvalidateRegion

class VMRectInvalidator : public nsISVGRectangleSink
{  
protected:
  friend already_AddRefed<nsISVGRectangleSink> CreateVMRectInvalidator(nsIViewManager* vm,
                                                                       nsIView* view,
                                                                       int twipsPerPx);
  VMRectInvalidator(nsIViewManager* vm, nsIView* view, int twipsPerPx); 

public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGRectangleSink interface:
  NS_DECL_NSISVGRECTANGLESINK
private:
  nsCOMPtr<nsIViewManager> mViewManager;
  nsIView* mView;
  int mTwipsPerPx;
};

//----------------------------------------------------------------------
// Implementation:

VMRectInvalidator::VMRectInvalidator(nsIViewManager* vm, nsIView* view,
                                     int twipsPerPx)
    : mViewManager(vm), mView(view), mTwipsPerPx(twipsPerPx)
{
  NS_INIT_ISUPPORTS();
}

already_AddRefed<nsISVGRectangleSink>
CreateVMRectInvalidator(nsIViewManager* vm, nsIView* view, int twipsPerPx)
{
  nsISVGRectangleSink* retval = new VMRectInvalidator(vm, view, twipsPerPx);
  NS_IF_ADDREF(retval);
  return retval;
}


//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(VMRectInvalidator)
NS_IMPL_RELEASE(VMRectInvalidator)

NS_INTERFACE_MAP_BEGIN(VMRectInvalidator)
  NS_INTERFACE_MAP_ENTRY(nsISVGRectangleSink)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsISVGRectangleSink methods:

/* void sinkRectangle (in float x, in float y, in float width, in float height); */
NS_IMETHODIMP
VMRectInvalidator::SinkRectangle(float x, float y, float width, float height)
{
  nsRect rect((nscoord)(x*mTwipsPerPx), (nscoord)(y*mTwipsPerPx),
              (nscoord)(width*mTwipsPerPx), (nscoord)(height*mTwipsPerPx));
  mViewManager->UpdateView(mView, rect, NS_VMREFRESH_NO_SYNC);
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// nsSVGOuterSVGFrame class

//typedef nsHTMLContainerFrame nsSVGOuterSVGFrameBase;
typedef nsContainerFrame nsSVGOuterSVGFrameBase;

class nsSVGOuterSVGFrame : public nsSVGOuterSVGFrameBase,
                           public nsISVGOuterSVGFrame,
                           public nsISVGContainerFrame,
                           public nsISVGValueObserver,
                           public nsSupportsWeakReference
{
  friend nsresult
  NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
protected:
  nsSVGOuterSVGFrame();
  virtual ~nsSVGOuterSVGFrame();
  nsresult Init();
  
   // nsISupports interface:
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }  
public:
  // nsIFrame:
  NS_IMETHOD Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow);
  
  NS_IMETHOD Reflow(nsIPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD  DidReflow(nsIPresContext*   aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus aStatus);


  NS_IMETHOD  AppendFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsIPresContext* aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  NS_IMETHOD  ReplaceFrame(nsIPresContext* aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIAtom*        aListName,
                           nsIFrame*       aOldFrame,
                           nsIFrame*       aNewFrame);

  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType,
                               PRInt32         aHint);

//  NS_IMETHOD SetView(nsIPresContext* aPresContext, nsIView* aView);

  NS_IMETHOD  GetFrameForPoint(nsIPresContext* aPresContext,
                               const nsPoint& aPoint, 
                               nsFramePaintLayer aWhichLayer,
                               nsIFrame**     aFrame);

  
  NS_IMETHOD  Paint(nsIPresContext* aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect& aDirtyRect,
                    nsFramePaintLayer aWhichLayer,
                    PRUint32 aFlags = 0);

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
  // nsISVGOuterSVGFrame interface:
  NS_IMETHOD InvalidateRegion(nsISVGRendererRegion* region, PRBool bRedraw);
  NS_IMETHOD IsRedrawSuspended(PRBool* isSuspended);
  NS_IMETHOD SuspendRedraw();
  NS_IMETHOD UnsuspendRedraw();
  NS_IMETHOD GetRenderer(nsISVGRenderer**renderer);
  NS_IMETHOD GetPresContext(nsIPresContext**presContext);

  // nsISVGContainerFrame interface:
  NS_IMETHOD_(nsISVGOuterSVGFrame*) GetOuterSVGFrame();
  
protected:
  // implementation helpers:
  void InitiateReflow();
  
  float GetPxPerTwips();
  float GetTwipsPerPx();

  void AddAsWidthHeightObserver();
  void RemoveAsWidthHeightObserver();

  void CalculateAvailableSpace(nsRect *maxRect, nsRect *preferredRect,
                               nsIPresContext* aPresContext,
                               const nsHTMLReflowState& aReflowState);
  nsresult SetViewportDimensions(nsISVGViewportRect* vp, float width, float height);
  nsresult SetViewportScale(nsISVGViewportRect* vp, nsIPresContext *context);
  
//  nsIView* mView;
  nsIPresShell* mPresShell; // XXX is a non-owning ref ok?
  PRUint32 mRedrawSuspendCount;
  PRBool mNeedsReflow;
  PRBool mViewportInitialized;
  nsCOMPtr<nsISVGRenderer> mRenderer;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;
  
  nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(aContent);
  if (!svgElement) {
#ifdef DEBUG
    printf("warning: trying to construct an SVGOuterSVGFrame for a content element that doesn't support the right interfaces\n");
#endif
    return NS_ERROR_FAILURE;
  }

  nsSVGOuterSVGFrame* it = new (aPresShell) nsSVGOuterSVGFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;

  // XXX is this ok?
  it->mPresShell = aPresShell;
  
  return NS_OK;
}

nsSVGOuterSVGFrame::nsSVGOuterSVGFrame()
    : mRedrawSuspendCount(0),
      mNeedsReflow(PR_FALSE),
      mViewportInitialized(PR_FALSE)
{
}

nsSVGOuterSVGFrame::~nsSVGOuterSVGFrame()
{
#ifdef DEBUG
//  printf("~nsSVGOuterSVGFrame %p\n", this);
#endif
  RemoveAsWidthHeightObserver();
}

nsresult nsSVGOuterSVGFrame::Init()
{
  mRenderer = do_CreateInstance(NS_SVG_RENDERER_GDIPLUS_CONTRACTID);
  NS_ASSERTION(mRenderer, "could not get renderer");
  AddAsWidthHeightObserver();
  SuspendRedraw();
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_INTERFACE_MAP_BEGIN(nsSVGOuterSVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGContainerFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGOuterSVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
NS_INTERFACE_MAP_END_INHERITING(nsSVGOuterSVGFrameBase)

//----------------------------------------------------------------------
// nsIFrame methods

NS_IMETHODIMP
nsSVGOuterSVGFrame::Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow)
{
  nsresult rv;
  rv = nsSVGOuterSVGFrameBase::Init(aPresContext, aContent, aParent,
                                    aContext, aPrevInFlow);

  Init();

  return rv;
}


  
//----------------------------------------------------------------------
// reflowing

NS_IMETHODIMP
nsSVGOuterSVGFrame::Reflow(nsIPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  nsresult rv;
  
  // check whether this reflow request is targeted at us or a child
  // frame (e.g. a foreignObject):
  if (aReflowState.reason == eReflowReason_Incremental) {
    nsReflowPath::iterator iter = aReflowState.path->FirstChild();
    nsReflowPath::iterator end = aReflowState.path->EndChildren();

    for ( ; iter != end; ++iter) {
      // The actual target of this reflow is one of our child
      // frames. Since SVG as such doesn't use reflow, this will
      // probably be the child of a <foreignObject>. Some HTML|XUL
      // content frames target reflow events at themselves when they
      // need to be redrawn in response to e.g. a style change. For
      // correct visual updating, we must make sure the reflow
      // reaches its intended target.
        
      // Since it is an svg frame (probably an nsSVGForeignObjectFrame),
      // we might as well pass in our aDesiredSize and aReflowState
      // objects - they are ignored by svg frames:
      nsSize availSpace(0, 0); // XXXwaterson probably wrong!
      nsHTMLReflowState state(aPresContext, aReflowState, *iter, availSpace);
      (*iter)->Reflow (aPresContext,
                       aDesiredSize,
                       state,
                       aStatus);

      // XXX do we really have to return our metrics although we're
      // not affected by the reflow? Is there a way of telling our
      // parent that we don't want anything changed?
      aDesiredSize.width  = mRect.width;
      aDesiredSize.height = mRect.height;
      aDesiredSize.ascent = aDesiredSize.height;
      aDesiredSize.descent = 0;
    }

    if (! aReflowState.path->mReflowCommand) {
      // We're not the target of the incremental reflow, so just bail.
      aStatus = NS_FRAME_COMPLETE;
      return NS_OK;
    }
  }
  
  //  SVG CR 20001102: When the SVG content is embedded inline within
  //  a containing document, and that document is styled using CSS,
  //  then if there are CSS positioning properties specified on the
  //  outermost 'svg' element that are sufficient to establish the
  //  width of the viewport, then these positioning properties
  //  establish the viewport's width; otherwise, the width attribute
  //  on the outermost 'svg' element establishes the viewport's width.
  //  Similarly, if there are CSS positioning properties specified on
  //  the outermost 'svg' element that are sufficient to establish the
  //  height of the viewport, then these positioning properties
  //  establish the viewport's height; otherwise, the height attribute
  //  on the outermost 'svg' element establishes the viewport's
  //  height.
#ifdef DEBUG
  // printf("--- nsSVGOuterSVGFrame(%p)::Reflow(frame:%p,reason:%d) ---\n",this,aReflowState.frame,aReflowState.reason);
#endif
  
  NS_ENSURE_TRUE(mContent, NS_ERROR_FAILURE);

  nsCOMPtr<nsISVGSVGElement> SVGElement = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(SVGElement, NS_ERROR_FAILURE);

  float pxPerTwips = GetPxPerTwips();
  float twipsPerPx = GetTwipsPerPx();

  // The width/height attribs given on the <svg>-element might be
  // percentage values of the parent viewport. We will set the parent
  // viewport dimensions to the available space.

  nsRect maxRect, preferredRect;
  CalculateAvailableSpace(&maxRect, &preferredRect, aPresContext, aReflowState);
  float preferredWidth = preferredRect.width * pxPerTwips;
  float preferredHeight = preferredRect.height * pxPerTwips;
  float maxWidth  = maxRect.width * pxPerTwips;
  float maxHeight = maxRect.height * pxPerTwips;

  SuspendRedraw(); 
  
  // As soon as we set the viewport, the width/height attributes might
  // emit change-notifications. We don't want those right now:
  RemoveAsWidthHeightObserver();

  nsCOMPtr<nsISVGViewportRect> parentViewport;
  SVGElement->GetParentViewportRect(getter_AddRefs(parentViewport));
  NS_ENSURE_TRUE(parentViewport, NS_ERROR_FAILURE);

  nsCOMPtr<nsISVGValue> pvp_value = do_QueryInterface(parentViewport);
  pvp_value->BeginBatchUpdate();
  rv = SetViewportDimensions(parentViewport, preferredWidth, preferredHeight);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = SetViewportScale(parentViewport, aPresContext);
  NS_ENSURE_SUCCESS(rv,rv);
  pvp_value->EndBatchUpdate();
  
#ifdef DEBUG
  // some debug stuff:
//   {
//     nsRect r;
//     aPresContext->GetVisibleArea(r);
//     printf("******* aw: %d, ah: %d visiw: %d, visih: %d\n",
//            aReflowState.availableWidth,
//            aReflowState.availableHeight,
//            r.width, r.height);
//     printf("******* cw: %d, ch: %d \n    cmaxw: %d, cmaxh: %d\n",
//            aReflowState.mComputedWidth,
//            aReflowState.mComputedHeight,
//            aReflowState.mComputedMaxWidth,
//            aReflowState.mComputedMaxHeight);

//     if (aReflowState.parentReflowState) {
//       printf("******* parent aw: %d, parent ah: %d \n",
//              aReflowState.parentReflowState->availableWidth,
//              aReflowState.parentReflowState->availableHeight);
//       printf("******* parent cw: %d, parent ch: %d \n  parent cmaxw: %d, parent cmaxh: %d\n",
//              aReflowState.parentReflowState->mComputedWidth,
//              aReflowState.parentReflowState->mComputedHeight,
//              aReflowState.parentReflowState->mComputedMaxWidth,
//              aReflowState.parentReflowState->mComputedMaxHeight);
//     }
//   }
#endif

  // now that the parent viewport dimensions have been set, the
  // width/height attributes will be valid.
  // Let's work out our desired dimensions.

  float width;
  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    SVGElement->GetWidth(getter_AddRefs(animLength));
    NS_ENSURE_TRUE(animLength, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ENSURE_TRUE(length, NS_ERROR_FAILURE);
    
    length->GetValue(&width);
    if (width > maxWidth)
      width = maxWidth;
    
    aDesiredSize.width = (int)(width*twipsPerPx);
  }

  float height;
  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    SVGElement->GetHeight(getter_AddRefs(animLength));
    NS_ENSURE_TRUE(animLength, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ENSURE_TRUE(length, NS_ERROR_FAILURE);
    
    length->GetValue(&height);
    if (height > maxHeight)
      height = maxHeight;
    
    aDesiredSize.height = (int)(height*twipsPerPx);
  }
  

  aDesiredSize.ascent = aDesiredSize.height;
  aDesiredSize.descent = 0;
  
  // XXX add in CSS borders ??

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  // Set the viewport.
  // XXX. I have a feeling that this code belongs in DidReflow(),
  // using mRect. Bug #118723 prevents us from putting it there.
  nsCOMPtr<nsIDOMSVGRect> domViewport;
  SVGElement->GetViewport(getter_AddRefs(domViewport));
  NS_ENSURE_TRUE(domViewport, NS_ERROR_FAILURE);
  nsCOMPtr<nsISVGViewportRect> viewport = do_QueryInterface(domViewport);
  NS_ENSURE_TRUE(viewport, NS_ERROR_FAILURE);  

  nsCOMPtr<nsISVGValue> vp_value = do_QueryInterface(viewport);
  vp_value->BeginBatchUpdate();
  rv = SetViewportDimensions(viewport, width, height);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = SetViewportScale(viewport, aPresContext);
  NS_ENSURE_SUCCESS(rv,rv);
  vp_value->EndBatchUpdate();
  
  AddAsWidthHeightObserver();
  
  UnsuspendRedraw();

  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::DidReflow(nsIPresContext*   aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus aStatus)
{
  nsresult rv = nsSVGOuterSVGFrameBase::DidReflow(aPresContext,aReflowState,aStatus);

  if (!mViewportInitialized) {
    // it is now
    mViewportInitialized = PR_TRUE;

    // call InitialUpdate() on all frames:
    nsIFrame* kid = mFrames.FirstChild();
    while (kid) {
      nsISVGChildFrame* SVGFrame=nsnull;
      kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
      if (SVGFrame) {
        SVGFrame->InitialUpdate(); 
      }
      kid->GetNextSibling(&kid);
    }
    
    UnsuspendRedraw();
  }
  
  return rv;
}

//----------------------------------------------------------------------
// container methods

NS_IMETHODIMP
nsSVGOuterSVGFrame::AppendFrames(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aFrameList)
{
  // append == insert at end:
  return InsertFrames(aPresContext, aPresShell, aListName,
                      mFrames.LastChild(), aFrameList);  
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::InsertFrames(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aPrevFrame,
                      nsIFrame*       aFrameList)
{
  // memorize last new frame
  nsIFrame* lastNewFrame = nsnull;
  {
    nsFrameList tmpList(aFrameList);
    lastNewFrame = tmpList.LastChild();
  }
  
  // Insert the new frames
  mFrames.InsertFrames(this, aPrevFrame, aFrameList);

  SuspendRedraw();

  // call InitialUpdate() on all new frames:
  nsIFrame* kid = aFrameList;
  nsIFrame* end = nsnull;
  if (lastNewFrame)
    lastNewFrame->GetNextSibling(&end);

  while (kid != end) {
    nsISVGChildFrame* SVGFrame=nsnull;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->InitialUpdate(); 
    }
    kid->GetNextSibling(&kid);
  }

  UnsuspendRedraw();
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::RemoveFrame(nsIPresContext* aPresContext,
                     nsIPresShell&   aPresShell,
                     nsIAtom*        aListName,
                     nsIFrame*       aOldFrame)
{
  // XXX maybe we should invalidate the area covered by the removed frame?
  
  PRBool result = mFrames.DestroyFrame(aPresContext, aOldFrame);
  NS_ASSERTION(result, "didn't find frame to delete");
  return result ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::ReplaceFrame(nsIPresContext* aPresContext,
                      nsIPresShell&   aPresShell,
                      nsIAtom*        aListName,
                      nsIFrame*       aOldFrame,
                      nsIFrame*       aNewFrame)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::AttributeChanged(nsIPresContext* aPresContext,
                                     nsIContent*     aChild,
                                     PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType,
                                     PRInt32         aHint)
{
#ifdef DEBUG
//  {
//    printf("** nsSVGOuterSVGFrame::AttributeChanged(");
//    nsAutoString str;
//    aAttribute->ToString(str);
//    nsCAutoString cstr;
//    cstr.AssignWithConversion(str);
//    printf(cstr.get());
//    printf(", hint:%d)\n",aHint);
//  }
#endif
  return NS_OK;
}


//----------------------------------------------------------------------
//
// NS_IMETHODIMP
// nsSVGOuterSVGFrame::SetView(nsIPresContext* aPresContext, nsIView* aView)
// {
//   mView = aView;
//   return nsSVGOuterSVGFrameBase::SetView(aPresContext, aView);  
// }

nsresult
nsSVGOuterSVGFrame::GetFrameForPoint(nsIPresContext* aPresContext,
                                     const nsPoint& aPoint,
                                     nsFramePaintLayer aWhichLayer,
                                     nsIFrame**     aFrame)
{
  // XXX This algorithm is really bad. Because we only have a
  // singly-linked list we have to test each and every SVG element for
  // a hit. What we really want is a double-linked list.
  

  *aFrame = nsnull;
  if (aWhichLayer != NS_FRAME_PAINT_LAYER_FOREGROUND) return NS_ERROR_FAILURE;
  
  float x = GetPxPerTwips() * ( aPoint.x - mRect.x);
  float y = GetPxPerTwips() * ( aPoint.y - mRect.y);
  
  PRBool inThisFrame = mRect.Contains(aPoint);
  
  if (!inThisFrame) {
    return NS_ERROR_FAILURE;
  }

  *aFrame = this;
  nsIFrame* kid = mFrames.FirstChild();
  nsIFrame* hit = nsnull;
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      nsresult rv = SVGFrame->GetFrameForPoint(x, y, &hit);
      if (NS_SUCCEEDED(rv) && hit) {
        *aFrame = hit;
        // return NS_OK; can't return. we need reverse order but only
        // have a singly linked list...
      }
    }
    kid->GetNextSibling(&kid);
  }
    
  return NS_OK;
}



//----------------------------------------------------------------------
// painting

NS_IMETHODIMP
nsSVGOuterSVGFrame::Paint(nsIPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect,
                          nsFramePaintLayer aWhichLayer,
                          PRUint32 aFlags)
{
  
//    if (NS_FRAME_PAINT_LAYER_BACKGROUND == aWhichLayer) {
//      const nsStyleDisplay* disp = (const nsStyleDisplay*)
//        mStyleContext->GetStyleData(eStyleStruct_Display);
//      if (disp->IsVisible() && mRect.width && mRect.height) {
//        // Paint our background and border
//        const nsStyleBorder* border = (const nsStyleBorder*)
//          mStyleContext->GetStyleData(eStyleStruct_Border);
//        const nsStyleOutline* outline = (const nsStyleOutline*)
//          mStyleContext->GetStyleData(eStyleStruct_Outline);

//        nsRect  rect(0, 0, mRect.width, mRect.height);
// //       nsCSSRendering::PaintBackground(aPresContext, aRenderingContext, this,
// //                                       aDirtyRect, rect, *border, 0, 0);
//        nsCSSRendering::PaintBorder(aPresContext, aRenderingContext, this,
//                                    aDirtyRect, rect, *border, mStyleContext, 0);
//        nsCSSRendering::PaintOutline(aPresContext, aRenderingContext, this,
//                                    aDirtyRect, rect, *border, *outline, mStyleContext, 0);
      
//      }
//    }

  
  if (aWhichLayer != NS_FRAME_PAINT_LAYER_FOREGROUND) return NS_OK;
  if (aDirtyRect.width<=0 || aDirtyRect.height<=0) return NS_OK;

  // initialize Mozilla rendering context
  aRenderingContext.PushState();

  nsRect clipRect(0,0,aDirtyRect.width, aDirtyRect.height);

  PRBool clipState;
  aRenderingContext.SetClipRect(aDirtyRect,nsClipCombine_kIntersect,clipState);

  
  nsCOMPtr<nsISVGRendererRenderContext> SVGCtx;
  mRenderer->CreateRenderContext(&aRenderingContext, aPresContext, getter_AddRefs(SVGCtx));

  SVGCtx->Clear(NS_RGB(255,255,255));

  // paint children:
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame)
      SVGFrame->Paint(SVGCtx);
    kid->GetNextSibling(&kid);
  }
  
  SVGCtx->Flush();

  SVGCtx = nsnull;
  
  aRenderingContext.PopState(clipState);
  
  return NS_OK;
  // see if we have to draw a selection frame around this container
  //return nsFrame::Paint(aPresContext, aRenderingContext, aDirtyRect, aWhichLayer);
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods:

NS_IMETHODIMP
nsSVGOuterSVGFrame::WillModifySVGObservable(nsISVGValue* observable)
{
  return NS_OK;
}


NS_IMETHODIMP
nsSVGOuterSVGFrame::DidModifySVGObservable(nsISVGValue* observable)
{
  mNeedsReflow = PR_TRUE;
  if (mRedrawSuspendCount==0) {
    InitiateReflow();
  }
  
  return NS_OK;
}


//----------------------------------------------------------------------
// nsISVGOuterSVGFrame methods:

NS_IMETHODIMP
nsSVGOuterSVGFrame::InvalidateRegion(nsISVGRendererRegion* region, PRBool bRedraw)
{
//  NS_ASSERTION(mView, "need a view!");
//  if (!mView) return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(mPresShell, NS_ERROR_FAILURE);
  nsCOMPtr<nsIPresContext> presCtx;
  mPresShell->GetPresContext(getter_AddRefs(presCtx));
  NS_ENSURE_TRUE(presCtx, NS_ERROR_FAILURE);
  nsIView* view = nsnull;
  GetView(presCtx, &view);
  if (!view) {
    nsIFrame* frame;
    GetParentWithView(presCtx, &frame);
    if (frame)
      frame->GetView(presCtx, &view);
  }
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
    
  
  if (!region && !bRedraw) return NS_OK;

  nsCOMPtr<nsIViewManager> vm;
  view->GetViewManager(*getter_AddRefs(vm));

  vm->BeginUpdateViewBatch();
  if (region) {
    nsCOMPtr<nsISVGRectangleSink> sink = CreateVMRectInvalidator(vm, view,
                                                                 (int)(GetTwipsPerPx()+0.5f));
    NS_ASSERTION(sink, "could not create rectangle sink for viewmanager");
    if (sink)
      region->GetRectangleScans(sink);
  }
  vm->EndUpdateViewBatch(bRedraw ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_NO_SYNC);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::SuspendRedraw()
{
  if (++mRedrawSuspendCount != 1)
    return NS_OK;

 // get the view manager, so that we can wrap this up in a batch
  // update.
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_FAILURE);
  nsCOMPtr<nsIPresContext> presCtx;
  mPresShell->GetPresContext(getter_AddRefs(presCtx));
  NS_ENSURE_TRUE(presCtx, NS_ERROR_FAILURE);
  nsIView* view = nsnull;
  GetView(presCtx, &view);
  if (!view) {
    nsIFrame* frame;
    GetParentWithView(presCtx, &frame);
    if (frame)
      frame->GetView(presCtx, &view);
  }
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
      
  nsCOMPtr<nsIViewManager> vm;
  view->GetViewManager(*getter_AddRefs(vm));

  vm->BeginUpdateViewBatch();
 
  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawSuspended();
    }
    kid->GetNextSibling(&kid);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::UnsuspendRedraw()
{
  if (--mRedrawSuspendCount > 0)
    return NS_OK;
  
  NS_ASSERTION(mRedrawSuspendCount >=0, "unbalanced suspend count!");
  
  // If we need to reflow, do so before we update any of our
  // children. Reflows are likely to affect the display of children:
  if (mNeedsReflow)
    InitiateReflow();
  
  // get the view manager, so that we can wrap this up in a batch
  // update.
  NS_ENSURE_TRUE(mPresShell, NS_ERROR_FAILURE);
  nsCOMPtr<nsIPresContext> presCtx;
  mPresShell->GetPresContext(getter_AddRefs(presCtx));
  NS_ENSURE_TRUE(presCtx, NS_ERROR_FAILURE);
  nsIView* view = nsnull;
  GetView(presCtx, &view);
  if (!view) {
    nsIFrame* frame;
    GetParentWithView(presCtx, &frame);
    if (frame)
      frame->GetView(presCtx, &view);
  }
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);
      
  nsCOMPtr<nsIViewManager> vm;
  view->GetViewManager(*getter_AddRefs(vm));

  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame=0;
    kid->QueryInterface(NS_GET_IID(nsISVGChildFrame),(void**)&SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
    kid->GetNextSibling(&kid);
  }

  vm->EndUpdateViewBatch(NS_VMREFRESH_IMMEDIATE);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::IsRedrawSuspended(PRBool* isSuspended)
{
  *isSuspended = (mRedrawSuspendCount>0) || !mViewportInitialized;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::GetRenderer(nsISVGRenderer**renderer)
{
  *renderer = mRenderer;
  NS_IF_ADDREF(*renderer);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::GetPresContext(nsIPresContext** presContext)
{
  NS_ASSERTION(mPresShell, "null presshell");
  return mPresShell->GetPresContext(presContext);
}

//----------------------------------------------------------------------
// nsISVGContainerFrame methods:

NS_IMETHODIMP_(nsISVGOuterSVGFrame *)
nsSVGOuterSVGFrame::GetOuterSVGFrame()
{
  return this;
}


//----------------------------------------------------------------------
// Implementation helpers

float nsSVGOuterSVGFrame::GetPxPerTwips()
{
  float val = GetTwipsPerPx();
  
  NS_ASSERTION(val!=0.0f, "invalid px/twips");  
  if (val == 0.0) val = 1e-20f;
  
  return 1.0f/val;
}

float nsSVGOuterSVGFrame::GetTwipsPerPx()
{
  float twipsPerPx=16.0f;
  NS_ASSERTION(mPresShell, "need presshell");
  if (mPresShell) {
    nsCOMPtr<nsIPresContext> presContext;
    mPresShell->GetPresContext(getter_AddRefs(presContext));
    presContext->GetScaledPixelsToTwips(&twipsPerPx);
  }
  return twipsPerPx;
}

void nsSVGOuterSVGFrame::InitiateReflow()
{
  mNeedsReflow = PR_FALSE;
  
  // Generate a reflow command to reflow ourselves
  nsHTMLReflowCommand *reflowCmd;
  NS_NewHTMLReflowCommand(&reflowCmd, this, eReflowType_ReflowDirty);
  if (!reflowCmd) {
    NS_ERROR("error creating reflow command object");
    return;
  }
  
  mPresShell->AppendReflowCommand(reflowCmd);
  mPresShell->FlushPendingNotifications(PR_FALSE);  
}


void nsSVGOuterSVGFrame::AddAsWidthHeightObserver()
{
  nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(mContent);
  NS_ASSERTION(svgElement, "wrong content element");  
 
  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    svgElement->GetWidth(getter_AddRefs(animLength));
    NS_ASSERTION(animLength, "could not get <svg>:width");
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ASSERTION(length, "could not get <svg>:width:animval");
    NS_ADD_SVGVALUE_OBSERVER(length);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    svgElement->GetHeight(getter_AddRefs(animLength));
    NS_ASSERTION(animLength, "could not get <svg>:height");
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ASSERTION(length, "could not get <svg>:height:animval");
    NS_ADD_SVGVALUE_OBSERVER(length);
  }  
}

void nsSVGOuterSVGFrame::RemoveAsWidthHeightObserver()
{
  nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(mContent);
  NS_ASSERTION(svgElement, "wrong content element");  

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    svgElement->GetWidth(getter_AddRefs(animLength));
    NS_ASSERTION(animLength, "could not get <svg>:width");
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ASSERTION(length, "could not get <svg>:width:animval");
    NS_REMOVE_SVGVALUE_OBSERVER(length);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> animLength;
    svgElement->GetHeight(getter_AddRefs(animLength));
    NS_ASSERTION(animLength, "could not get <svg>:height");
    nsCOMPtr<nsIDOMSVGLength> length;
    animLength->GetAnimVal(getter_AddRefs(length));
    NS_ASSERTION(length, "could not get <svg>:height:animval");
    NS_REMOVE_SVGVALUE_OBSERVER(length);
  }  
}

void
nsSVGOuterSVGFrame::CalculateAvailableSpace(nsRect *maxRect,
                                            nsRect *preferredRect,
                                            nsIPresContext* aPresContext,
                                            const nsHTMLReflowState& aReflowState)
{
  aPresContext->GetVisibleArea(*preferredRect);
  
  if (aReflowState.availableWidth != NS_INTRINSICSIZE)
    maxRect->width = aReflowState.availableWidth;
  else if (aReflowState.parentReflowState &&
           aReflowState.parentReflowState->mComputedWidth != NS_INTRINSICSIZE)
    maxRect->width = aReflowState.parentReflowState->mComputedWidth;
  else
    maxRect->width = NS_MAXSIZE;
  
  if (aReflowState.availableHeight != NS_INTRINSICSIZE)
    maxRect->height = aReflowState.availableHeight;    
  else if (aReflowState.parentReflowState &&
           aReflowState.parentReflowState->mComputedHeight != NS_INTRINSICSIZE)
    maxRect->height = aReflowState.parentReflowState->mComputedHeight;
  else
    maxRect->height = NS_MAXSIZE;

  if (preferredRect->width > maxRect->width)
    preferredRect->width = maxRect->width;
  if (preferredRect->height > maxRect->height)
    preferredRect->height = maxRect->height;
}  


nsresult
nsSVGOuterSVGFrame::SetViewportDimensions(nsISVGViewportRect* vp,
                                          float width, float height)
{
  {
    nsCOMPtr<nsISVGViewportAxis> axis;
    vp->GetXAxis(getter_AddRefs(axis));
    NS_ENSURE_TRUE(axis, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMSVGNumber> length;
    axis->GetLength(getter_AddRefs(length));
    length->SetValue(width);
  }

  {
    nsCOMPtr<nsISVGViewportAxis> axis;
    vp->GetYAxis(getter_AddRefs(axis));
    NS_ENSURE_TRUE(axis, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMSVGNumber> length;
    axis->GetLength(getter_AddRefs(length));
    length->SetValue(height);
  }
  return NS_OK;
}  

nsresult
nsSVGOuterSVGFrame::SetViewportScale(nsISVGViewportRect* vp, nsIPresContext *context)
{
  float TwipsPerPx;
  context->GetScaledPixelsToTwips(&TwipsPerPx);
  float mmPerPx = TwipsPerPx / TWIPS_PER_POINT_FLOAT / (72.0f * 0.03937f);

  nsCOMPtr<nsIDOMSVGNumber> scaleX;
  {
    nsCOMPtr<nsISVGViewportAxis> axis;
    vp->GetXAxis(getter_AddRefs(axis));
    NS_ENSURE_TRUE(axis, NS_ERROR_FAILURE);
    axis->GetMillimeterPerPixel(getter_AddRefs(scaleX));
  }

  nsCOMPtr<nsIDOMSVGNumber> scaleY;
  {
    nsCOMPtr<nsISVGViewportAxis> axis;
    vp->GetYAxis(getter_AddRefs(axis));
    NS_ENSURE_TRUE(axis, NS_ERROR_FAILURE);
    axis->GetMillimeterPerPixel(getter_AddRefs(scaleY));
  }

  float old_x, old_y;

  scaleX->GetValue(&old_x);
  scaleY->GetValue(&old_y);

  if (old_x != mmPerPx || old_y != mmPerPx) {
    scaleX->SetValue(mmPerPx);
    scaleY->SetValue(mmPerPx);
  }
  
  return NS_OK;
}
