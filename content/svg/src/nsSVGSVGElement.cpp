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

#include "nsSVGElement.h"
#include "nsSVGAtoms.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGFitToViewBox.h"
#include "nsSVGAnimatedLength.h"
#include "nsSVGLength.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIPresContext.h"
#include "nsSVGRect.h"
#include "nsSVGAnimatedRect.h"
#include "nsSVGMatrix.h"
#include "nsSVGPoint.h"
#include "nsSVGTransform.h"
#include "nsIDOMEventTarget.h"
#include "nsIViewManager.h"

#include "nsIFrame.h"
#include "nsISVGFrame.h" //XXX

class nsSVGSVGElement : public nsSVGElement,
                        public nsIDOMSVGSVGElement,
                        public nsIDOMSVGFitToViewBox
{
protected:
  friend nsresult NS_NewSVGSVGElement(nsIContent **aResult,
                                      nsINodeInfo *aNodeInfo);
  nsSVGSVGElement();
  virtual ~nsSVGSVGElement();
  virtual nsresult Init();
  
public:
  // interfaces:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSVGELEMENT
  NS_DECL_NSIDOMSVGFITTOVIEWBOX
  
  // xxx I wish we could use virtual inheritance
  NS_FORWARD_NSIDOMNODE_NO_CLONENODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

protected:
  nsCOMPtr<nsIDOMSVGAnimatedLength> mWidth;
  nsCOMPtr<nsIDOMSVGAnimatedLength> mHeight;
  nsCOMPtr<nsIDOMSVGRect>           mViewport;
  nsCOMPtr<nsIDOMSVGAnimatedRect>   mViewBox;
  PRInt32 mRedrawSuspendCount;
};


nsresult NS_NewSVGSVGElement(nsIContent **aResult, nsINodeInfo *aNodeInfo)
{
  *aResult = nsnull;
  nsSVGSVGElement* it = new nsSVGSVGElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsXMLElement*,it)->Init(aNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }
  
  *aResult = NS_STATIC_CAST(nsIContent *, it);

  return NS_OK;
}

//----------------------------------------------------------------------
// XPConnect interface list
NS_CLASSINFO_MAP_BEGIN(SVGSVGElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMEventTarget)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMSVGSVGElement)
  NS_CLASSINFO_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_CLASSINFO_MAP_ENTRY_FUNCTION(GetSVGElementIIDs)
NS_CLASSINFO_MAP_END

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ADDREF_INHERITED(nsSVGSVGElement,nsSVGElement)
NS_IMPL_RELEASE_INHERITED(nsSVGSVGElement,nsSVGElement)

NS_INTERFACE_MAP_BEGIN(nsSVGSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSVGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGElement)

//----------------------------------------------------------------------
// Implementation

nsSVGSVGElement::nsSVGSVGElement()
    : mRedrawSuspendCount(0)
{
}

nsSVGSVGElement::~nsSVGSVGElement()
{
}

  
nsresult
nsSVGSVGElement::Init()
{
  nsresult rv;
  rv = nsSVGElement::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  // nsIDOMSVGSVGElement attributes ------:
  
  // DOM property: width ,  #IMPLIED attrib: width
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eXDirection,
                         100.0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mWidth), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::width, mWidth);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  // DOM property: height , #IMPLIED attrib: height
  {
    nsCOMPtr<nsIDOMSVGLength> length;
    rv = NS_NewSVGLength(getter_AddRefs(length),
                         (nsSVGElement*)this, eYDirection,
                         100.0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLength(getter_AddRefs(mHeight), length);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::height, mHeight);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  // readonly (XXX) DOM property: viewport
  {
    rv = NS_NewSVGRect(getter_AddRefs(mViewport));
    NS_ENSURE_SUCCESS(rv,rv);
  }
  // nsIDOMSVGFitToViewBox attributes ------:
  
  // DOM property: viewBox , #IMPLIED attrib: viewBox
  {
    nsCOMPtr<nsIDOMSVGRect> envelopeRect;
    rv = NS_NewSVGRect(getter_AddRefs(envelopeRect), mViewport);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedRect(getter_AddRefs(mViewBox), envelopeRect);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = mAttributes->AddMappedSVGValue(nsSVGAtoms::viewBox, mViewBox);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

//----------------------------------------------------------------------
// nsIDOMNode methods

NS_IMETHODIMP
nsSVGSVGElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  *aReturn = nsnull;
  nsSVGSVGElement* it = new nsSVGSVGElement();

  if (!it) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);

  nsresult rv = NS_STATIC_CAST(nsXMLElement*,it)->Init(mNodeInfo);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = it->Init();

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  rv = CopyNode(it, aDeep);

  if (NS_FAILED(rv)) {
    it->Release();
    return rv;
  }

  return it->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)aReturn);
}


//----------------------------------------------------------------------
// nsIDOMSVGSVGElement methods:

/* readonly attribute nsIDOMSVGAnimatedLength x; */
NS_IMETHODIMP
nsSVGSVGElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGAnimatedLength y; */
NS_IMETHODIMP
nsSVGSVGElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGAnimatedLength width; */
NS_IMETHODIMP
nsSVGSVGElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = mWidth;
  NS_ADDREF(*aWidth);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGAnimatedLength height; */
NS_IMETHODIMP
nsSVGSVGElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = mHeight;
  NS_ADDREF(*aHeight);
  return NS_OK;
}

/* attribute DOMString contentScriptType; */
NS_IMETHODIMP
nsSVGSVGElement::GetContentScriptType(nsAWritableString & aContentScriptType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetContentScriptType(const nsAReadableString & aContentScriptType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString contentStyleType; */
NS_IMETHODIMP
nsSVGSVGElement::GetContentStyleType(nsAWritableString & aContentStyleType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetContentStyleType(const nsAReadableString & aContentStyleType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGRect viewport; */
NS_IMETHODIMP
nsSVGSVGElement::GetViewport(nsIDOMSVGRect * *aViewport)
{
  *aViewport = mViewport;
  NS_ADDREF(*aViewport);
  return NS_OK;
}

/* readonly attribute float pixelUnitToMillimeterX; */
NS_IMETHODIMP
nsSVGSVGElement::GetPixelUnitToMillimeterX(float *aPixelUnitToMillimeterX)
{
  // to correctly determine this, the caller would need to pass in the
  // right PresContext...

  *aPixelUnitToMillimeterX = 0.28f; // 90dpi

  if (!mDocument) return NS_OK;
    // Get Presentation shell 0
  nsCOMPtr<nsIPresShell> presShell;
  mDocument->GetShellAt(0, getter_AddRefs(presShell));
  if (!presShell) return NS_OK;
  
  // Get the Presentation Context from the Shell
  nsCOMPtr<nsIPresContext> context;
  presShell->GetPresContext(getter_AddRefs(context));
  if (!context) return NS_OK;

  float TwipsPerPx;
  context->GetScaledPixelsToTwips(&TwipsPerPx);
  *aPixelUnitToMillimeterX = TwipsPerPx / TWIPS_PER_POINT_FLOAT / (72.0f * 0.03937f);
  return NS_OK;
}

/* readonly attribute float pixelUnitToMillimeterY; */
NS_IMETHODIMP
nsSVGSVGElement::GetPixelUnitToMillimeterY(float *aPixelUnitToMillimeterY)
{
  return GetPixelUnitToMillimeterX(aPixelUnitToMillimeterY);
}

/* readonly attribute float screenPixelToMillimeterX; */
NS_IMETHODIMP
nsSVGSVGElement::GetScreenPixelToMillimeterX(float *aScreenPixelToMillimeterX)
{
  // to correctly determine this, the caller would need to pass in the
  // right PresContext...

  *aScreenPixelToMillimeterX = 0.28f; // 90dpi

  if (!mDocument) return NS_OK;
    // Get Presentation shell 0
  nsCOMPtr<nsIPresShell> presShell;
  mDocument->GetShellAt(0, getter_AddRefs(presShell));
  if (!presShell) return NS_OK;
  
  // Get the Presentation Context from the Shell
  nsCOMPtr<nsIPresContext> context;
  presShell->GetPresContext(getter_AddRefs(context));
  if (!context) return NS_OK;

  float TwipsPerPx;
  context->GetPixelsToTwips(&TwipsPerPx);
  *aScreenPixelToMillimeterX = TwipsPerPx / TWIPS_PER_POINT_FLOAT / (72.0f * 0.03937f);
  return NS_OK;
}

/* readonly attribute float screenPixelToMillimeterY; */
NS_IMETHODIMP
nsSVGSVGElement::GetScreenPixelToMillimeterY(float *aScreenPixelToMillimeterY)
{
  return GetScreenPixelToMillimeterX(aScreenPixelToMillimeterY);
}

/* attribute boolean useCurrentView; */
NS_IMETHODIMP
nsSVGSVGElement::GetUseCurrentView(PRBool *aUseCurrentView)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetUseCurrentView(PRBool aUseCurrentView)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGViewSpec currentView; */
NS_IMETHODIMP
nsSVGSVGElement::GetCurrentView(nsIDOMSVGViewSpec * *aCurrentView)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute float currentScale; */
NS_IMETHODIMP
nsSVGSVGElement::GetCurrentScale(float *aCurrentScale)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsSVGSVGElement::SetCurrentScale(float aCurrentScale)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGPoint currentTranslate; */
NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTranslate(nsIDOMSVGPoint * *aCurrentTranslate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long suspendRedraw (in unsigned long max_wait_milliseconds); */
NS_IMETHODIMP
nsSVGSVGElement::SuspendRedraw(PRUint32 max_wait_milliseconds, PRUint32 *_retval)
{
  *_retval = 1;

  if (++mRedrawSuspendCount > 1) 
    return NS_OK;
  
  if (!mDocument) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresShell> presShell;
  mDocument->GetShellAt(0, getter_AddRefs(presShell));
  NS_ASSERTION(presShell, "need presShell to suspend redraw");
  if (!presShell) return NS_ERROR_FAILURE;

  nsIFrame* frame;
  presShell->GetPrimaryFrameFor(NS_STATIC_CAST(nsIStyledContent*, this), &frame);
  NS_ASSERTION(frame, "suspending redraw w/o frame");
  if (frame) {
    nsISVGFrame* svgframe;
    frame->QueryInterface(NS_GET_IID(nsISVGFrame),(void**)&svgframe);
    NS_ASSERTION(svgframe, "wrong frame type");
    if (svgframe) {
      svgframe->NotifyRedrawSuspended();
    }
  }
  
  return NS_OK;
}

/* void unsuspendRedraw (in unsigned long suspend_handle_id); */
NS_IMETHODIMP
nsSVGSVGElement::UnsuspendRedraw(PRUint32 suspend_handle_id)
{
  if (mRedrawSuspendCount == 0) {
    NS_ASSERTION(1==0, "unbalanced suspend/unsuspend calls");
    return NS_ERROR_FAILURE;
  }
                 
  if (mRedrawSuspendCount > 1) {
    --mRedrawSuspendCount;
    return NS_OK;
  }
  
  return UnsuspendRedrawAll();
}

/* void unsuspendRedrawAll (); */
NS_IMETHODIMP
nsSVGSVGElement::UnsuspendRedrawAll()
{
  mRedrawSuspendCount = 0;
  
  if (!mDocument) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIPresShell> presShell;
  mDocument->GetShellAt(0, getter_AddRefs(presShell));
  NS_ASSERTION(presShell, "need presShell to unsuspend redraw");
  if (!presShell) return NS_ERROR_FAILURE;

  nsIFrame* frame;
  presShell->GetPrimaryFrameFor(NS_STATIC_CAST(nsIStyledContent*, this), &frame);
  NS_ASSERTION(frame, "unsuspending redraw w/o frame");
  if (frame) {
    nsISVGFrame* svgframe;
    frame->QueryInterface(NS_GET_IID(nsISVGFrame),(void**)&svgframe);
    NS_ASSERTION(svgframe, "wrong frame type");
    if (svgframe) {
      svgframe->NotifyRedrawUnsuspended();
    }
  }  
  return NS_OK;
}

/* void forceRedraw (); */
NS_IMETHODIMP
nsSVGSVGElement::ForceRedraw()
{
  if (!mDocument) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIPresShell> presShell;
  mDocument->GetShellAt(0, getter_AddRefs(presShell));
  NS_ASSERTION(presShell, "need presShell to unsuspend redraw");
  if (!presShell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIViewManager> vm;
  presShell->GetViewManager(getter_AddRefs(vm));
  NS_ASSERTION(vm, "need viewmanager to unsuspend redraw");
  if (!vm) return NS_ERROR_FAILURE;

  vm->UpdateAllViews(NS_VMREFRESH_IMMEDIATE);

  return NS_OK;
}

/* void pauseAnimations (); */
NS_IMETHODIMP
nsSVGSVGElement::PauseAnimations()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void unpauseAnimations (); */
NS_IMETHODIMP
nsSVGSVGElement::UnpauseAnimations()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean animationsPaused (); */
NS_IMETHODIMP
nsSVGSVGElement::AnimationsPaused(PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* float getCurrentTime (); */
NS_IMETHODIMP
nsSVGSVGElement::GetCurrentTime(float *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setCurrentTime (in float seconds); */
NS_IMETHODIMP
nsSVGSVGElement::SetCurrentTime(float seconds)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMNodeList getIntersectionList (in nsIDOMSVGRect rect, in nsIDOMSVGElement referenceElement); */
NS_IMETHODIMP
nsSVGSVGElement::GetIntersectionList(nsIDOMSVGRect *rect, nsIDOMSVGElement *referenceElement, nsIDOMNodeList **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMNodeList getEnclosureList (in nsIDOMSVGRect rect, in nsIDOMSVGElement referenceElement); */
NS_IMETHODIMP
nsSVGSVGElement::GetEnclosureList(nsIDOMSVGRect *rect, nsIDOMSVGElement *referenceElement, nsIDOMNodeList **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean checkIntersection (in nsIDOMSVGElement element, in nsIDOMSVGRect rect); */
NS_IMETHODIMP
nsSVGSVGElement::CheckIntersection(nsIDOMSVGElement *element, nsIDOMSVGRect *rect, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean checkEnclosure (in nsIDOMSVGElement element, in nsIDOMSVGRect rect); */
NS_IMETHODIMP
nsSVGSVGElement::CheckEnclosure(nsIDOMSVGElement *element, nsIDOMSVGRect *rect, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void deSelectAll (); */
NS_IMETHODIMP
nsSVGSVGElement::DeSelectAll()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGNumber createSVGNumber (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGNumber(nsIDOMSVGNumber **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGLength createSVGLength (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGLength(nsIDOMSVGLength **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGAngle createSVGAngle (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGAngle(nsIDOMSVGAngle **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGPoint createSVGPoint (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGPoint(nsIDOMSVGPoint **_retval)
{
  return nsSVGPoint::Create(0.0f, 0.0f, _retval);
}

/* nsIDOMSVGMatrix createSVGMatrix (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGMatrix(nsIDOMSVGMatrix **_retval)
{
  return nsSVGMatrix::Create(_retval);
}

/* nsIDOMSVGRect createSVGRect (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGRect(nsIDOMSVGRect **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMSVGTransform createSVGTransform (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGTransform(nsIDOMSVGTransform **_retval)
{
  return NS_NewSVGTransform(_retval);
}

/* nsIDOMSVGTransform createSVGTransformFromMatrix (in nsIDOMSVGMatrix matrix); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix, nsIDOMSVGTransform **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* DOMString createSVGString (); */
NS_IMETHODIMP
nsSVGSVGElement::CreateSVGString(nsAWritableString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIDOMElement getElementById (in DOMString elementId); */
NS_IMETHODIMP
nsSVGSVGElement::GetElementById(const nsAReadableString & elementId, nsIDOMElement **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------
// nsIDOMSVGFitToViewBox methods

/* readonly attribute nsIDOMSVGAnimatedRect viewBox; */
NS_IMETHODIMP
nsSVGSVGElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIDOMSVGAnimatedPreserveAspectRatio preserveAspectRatio; */
NS_IMETHODIMP
nsSVGSVGElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio * *aPreserveAspectRatio)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

