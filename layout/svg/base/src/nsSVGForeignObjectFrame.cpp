/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *          Alex Fritze <alex.fritze@crocodile-clips.com>
 *
 */

#include "nsBlockFrame.h"
#include "nsIDOMSVGGElement.h"
#include "nsIPresContext.h"
#include "nsISVGFrame.h"
#include "nsSVGRenderingContext.h"
#include "nsWeakReference.h"
#include "nsISVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsIDOMSVGTransformable.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGForeignObjectElem.h"

typedef nsBlockFrame nsSVGForeignObjectFrameBase;

class nsSVGForeignObjectFrame : public nsSVGForeignObjectFrameBase,
                                public nsISVGFrame,
                                public nsISVGValueObserver,
                                public nsSupportsWeakReference
{
  friend nsresult
  NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame);
protected:
  nsSVGForeignObjectFrame();
  virtual ~nsSVGForeignObjectFrame();
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

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
  // nsISVGFrame interface:
  NS_IMETHOD Paint(nsSVGRenderingContext* renderingContext);
  NS_IMETHOD InvalidateRegion(ArtUta* uta, PRBool bRedraw);
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit);  
  NS_IMETHOD NotifyCTMChanged();
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD IsRedrawSuspended(PRBool* isSuspended);
  
protected:
  // implementation helpers:
  float GetPxPerTwips();
  float GetTwipsPerPx();
  
  nsIPresShell* mPresShell; // XXX is a non-owning ref ok?

  nsCOMPtr<nsIDOMSVGLength> mX;
  nsCOMPtr<nsIDOMSVGLength> mY;
  nsCOMPtr<nsIDOMSVGLength> mWidth;
  nsCOMPtr<nsIDOMSVGLength> mHeight;
};

//----------------------------------------------------------------------
// Implementation

nsresult
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsIFrame** aNewFrame)
{
  *aNewFrame = nsnull;
  
  nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject = do_QueryInterface(aContent);
  if (!foreignObject) {
#ifdef DEBUG
    printf("warning: trying to contruct an SVGForeignObjectFrame for a content element that doesn't support the right interfaces\n");
#endif
    return NS_ERROR_FAILURE;
  }
  
  nsSVGForeignObjectFrame* it = new (aPresShell) nsSVGForeignObjectFrame;
  if (nsnull == it)
    return NS_ERROR_OUT_OF_MEMORY;

  *aNewFrame = it;

  // XXX is this ok?
  it->mPresShell = aPresShell;

  return NS_OK;
}

nsSVGForeignObjectFrame::nsSVGForeignObjectFrame()
{
  printf("nsSVGForeignObjectFrame CTOR\n");
}

nsSVGForeignObjectFrame::~nsSVGForeignObjectFrame()
{
  printf("~nsSVGForeignObjectFrame\n");
//   nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(mContent);
//   NS_ASSERTION(transformable, "wrong content element");
//   nsCOMPtr<nsIDOMSVGAnimatedTransformList> transforms;
//   transformable->GetTransform(getter_AddRefs(transforms));
//   nsCOMPtr<nsISVGValue> value = do_QueryInterface(transforms);
//   NS_ASSERTION(value, "interface not found");
//   if (value)
//     value->RemoveObserver(this);
  nsCOMPtr<nsISVGValue> value;
  if (mX && (value = do_QueryInterface(mX)))
      value->RemoveObserver(this);
  if (mY && (value = do_QueryInterface(mY)))
      value->RemoveObserver(this);
  if (mWidth && (value = do_QueryInterface(mWidth)))
      value->RemoveObserver(this);
  if (mHeight && (value = do_QueryInterface(mHeight)))
      value->RemoveObserver(this);
}

nsresult nsSVGForeignObjectFrame::Init()
{
  nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject = do_QueryInterface(mContent);
  NS_ASSERTION(foreignObject, "wrong content element");
  
  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    foreignObject->GetX(getter_AddRefs(length));
    length->GetAnimVal(getter_AddRefs(mX));
    NS_ASSERTION(mX, "no x");
    if (!mX) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mX);
    if (value)
      value->AddObserver(this);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    foreignObject->GetY(getter_AddRefs(length));
    length->GetAnimVal(getter_AddRefs(mY));
    NS_ASSERTION(mY, "no y");
    if (!mY) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mY);
    if (value)
      value->AddObserver(this);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    foreignObject->GetWidth(getter_AddRefs(length));
    length->GetAnimVal(getter_AddRefs(mWidth));
    NS_ASSERTION(mWidth, "no width");
    if (!mWidth) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mWidth);
    if (value)
      value->AddObserver(this);
  }

  {
    nsCOMPtr<nsIDOMSVGAnimatedLength> length;
    foreignObject->GetHeight(getter_AddRefs(length));
    length->GetAnimVal(getter_AddRefs(mHeight));
    NS_ASSERTION(mHeight, "no height");
    if (!mHeight) return NS_ERROR_FAILURE;
    nsCOMPtr<nsISVGValue> value = do_QueryInterface(mHeight);
    if (value)
      value->AddObserver(this);
  }
  
// XXX 
//   nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(mContent);
//   NS_ASSERTION(transformable, "wrong content element");
//   nsCOMPtr<nsIDOMSVGAnimatedTransformList> transforms;
//   transformable->GetTransform(getter_AddRefs(transforms));
//   nsCOMPtr<nsISVGValue> value = do_QueryInterface(transforms);
//   NS_ASSERTION(value, "interface not found");
//   if (value)
//     value->AddObserver(this);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_INTERFACE_MAP_BEGIN(nsSVGForeignObjectFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
NS_INTERFACE_MAP_END_INHERITING(nsSVGForeignObjectFrameBase)


//----------------------------------------------------------------------
// nsIFrame methods
NS_IMETHODIMP
nsSVGForeignObjectFrame::Init(nsIPresContext*  aPresContext,
                  nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIStyleContext* aContext,
                  nsIFrame*        aPrevInFlow)
{
  nsresult rv;
  rv = nsSVGForeignObjectFrameBase::Init(aPresContext, aContent, aParent,
                             aContext, aPrevInFlow);

  Init();

  return rv;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::Reflow(nsIPresContext*          aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  float twipsPerPx = GetTwipsPerPx();

  
  NS_ENSURE_TRUE(mX && mY && mWidth && mHeight, NS_ERROR_FAILURE);

  float x, y, width, height;
  mX->GetValue(&x);
  mY->GetValue(&y);
  mWidth->GetValue(&width);
  mHeight->GetValue(&height);

  // move ourselves to (x,y):
  MoveTo(aPresContext, (nscoord) (x*twipsPerPx), (nscoord) (y*twipsPerPx));
  // XXX: if we have a view, move that 
  
  // create a new reflow state, setting our max size to (width,height):
  nsSize availableSpace((nscoord)(width*twipsPerPx), (nscoord)(height*twipsPerPx));
  nsHTMLReflowState sizedReflowState(aPresContext,
                                     aReflowState,
                                     this,
                                     availableSpace);
  
  // leverage our base class' reflow function to do all the work:
  return nsSVGForeignObjectFrameBase::Reflow(aPresContext, aDesiredSize, sizedReflowState, aStatus);
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods:

NS_IMETHODIMP
nsSVGForeignObjectFrame::WillModifySVGObservable(nsISVGValue* observable)
{
  return NS_OK;
}


NS_IMETHODIMP
nsSVGForeignObjectFrame::DidModifySVGObservable (nsISVGValue* observable)
{
  
//   nsIFrame* kid = mFrames.FirstChild();
//   while (kid) {
//     nsISVGFrame* SVGFrame=0;
//     kid->QueryInterface(NS_GET_IID(nsISVGFrame),(void**)&SVGFrame);
//     if (SVGFrame)
//       SVGFrame->NotifyCTMChanged();
//     kid->GetNextSibling(&kid);
//   }

  // we have either moved or resized -> reflow XXX
  
  return NS_OK;
}


//----------------------------------------------------------------------
// nsISVGFrame methods

NS_IMETHODIMP
nsSVGForeignObjectFrame::Paint(nsSVGRenderingContext* renderingContext)
{
  nsIRenderingContext* ctx = renderingContext->LockMozRenderingContext();
  nsRect dirtyRect = renderingContext->GetDirtyRectTwips();

  ctx->Translate(mRect.x, mRect.y);
  dirtyRect.x -= mRect.x;
  dirtyRect.y -= mRect.y;

  nsSVGForeignObjectFrameBase::Paint(renderingContext->GetPresContext(),
                                     *ctx,
                                     dirtyRect,
                                     NS_FRAME_PAINT_LAYER_BACKGROUND);

  nsSVGForeignObjectFrameBase::Paint(renderingContext->GetPresContext(),
                                     *ctx,
                                     dirtyRect,
                                     NS_FRAME_PAINT_LAYER_FLOATERS);

  nsSVGForeignObjectFrameBase::Paint(renderingContext->GetPresContext(),
                                     *ctx,
                                     dirtyRect,
                                     NS_FRAME_PAINT_LAYER_FOREGROUND);

  ctx->Translate(-mRect.x, -mRect.y);

  renderingContext->UnlockMozRenderingContext();
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::InvalidateRegion(ArtUta* uta, PRBool bRedraw)
{
  if (!uta && !bRedraw) return NS_OK;
  
  if (!mParent) {
    if (uta)
      art_free(uta);
    return NS_OK;
  }

  nsCOMPtr<nsISVGFrame> SVGFrame = do_QueryInterface(mParent);
  if (!SVGFrame) {
    if (uta)
      art_free(uta);
    return NS_OK;
  }

  return SVGFrame->InvalidateRegion(uta, bRedraw);
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::GetFrameForPoint(float x, float y, nsIFrame** hit)
{
  *hit = nsnull;

  NS_ASSERTION(mPresShell, "need presshell");
  if (!mPresShell) return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIPresContext> presContext;
  mPresShell->GetPresContext(getter_AddRefs(presContext));

  nsPoint p( (nscoord)(x*GetTwipsPerPx()),
             (nscoord)(y*GetTwipsPerPx()));

  nsresult rv;

  rv = nsSVGForeignObjectFrameBase::GetFrameForPoint(presContext, p,
                                                     NS_FRAME_PAINT_LAYER_FOREGROUND, hit);
  if (NS_SUCCEEDED(rv) && *hit) return rv;

  rv = nsSVGForeignObjectFrameBase::GetFrameForPoint(presContext, p,
                                                     NS_FRAME_PAINT_LAYER_FLOATERS, hit);
  if (NS_SUCCEEDED(rv) && *hit) return rv;

  return nsSVGForeignObjectFrameBase::GetFrameForPoint(presContext, p,
                                                       NS_FRAME_PAINT_LAYER_BACKGROUND, hit);
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyCTMChanged()
{
//  NS_NOTYETIMPLEMENTED("write me!");
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawSuspended()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawUnsuspended()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::IsRedrawSuspended(PRBool* isSuspended)
{
  nsCOMPtr<nsISVGFrame> SVGFrame = do_QueryInterface(mParent);
  if (!SVGFrame) {
    *isSuspended = PR_FALSE;
    return NS_OK;
  }

  return SVGFrame->IsRedrawSuspended(isSuspended);  
}


//----------------------------------------------------------------------
// Implementation helpers

float nsSVGForeignObjectFrame::GetPxPerTwips()
{
  float val = GetTwipsPerPx();
  
  NS_ASSERTION(val!=0.0f, "invalid px/twips");  
  if (val == 0.0) val = 1e-20f;
  
  return 1.0f/val;
}

float nsSVGForeignObjectFrame::GetTwipsPerPx()
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
