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
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */

#include "nsSVGPoint.h"
#include "nsIDOMSVGMatrix.h"

nsresult
nsSVGPoint::Create(float x, float y, nsIDOMSVGPoint** aResult)
{
  *aResult = (nsIDOMSVGPoint*) new nsSVGPoint(x, y);
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsSVGPoint::nsSVGPoint(float x, float y)
    : mX(x), mY(y)
{
  NS_INIT_ISUPPORTS();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGPoint)
NS_IMPL_RELEASE(nsSVGPoint)

NS_INTERFACE_MAP_BEGIN(nsSVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPoint)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// nsIDOMSVGPoint methods:

/* attribute float x; */
NS_IMETHODIMP nsSVGPoint::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPoint::SetX(float aX)
{
  WillModify();
  mX = aX;
  DidModify();
  
  return NS_OK;
}

/* attribute float y; */
NS_IMETHODIMP nsSVGPoint::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPoint::SetY(float aY)
{
  WillModify();
  mY = aY;
  DidModify();
  
  return NS_OK;
}

/* nsIDOMSVGPoint matrixTransform (in nsIDOMSVGMatrix matrix); */
NS_IMETHODIMP nsSVGPoint::MatrixTransform(nsIDOMSVGMatrix *matrix, nsIDOMSVGPoint **_retval)
{
  if (!matrix) return NS_ERROR_FAILURE;
  
  float a, b, c, d, e, f;
  matrix->GetA(&a);
  matrix->GetB(&b);
  matrix->GetC(&c);
  matrix->GetD(&d);
  matrix->GetE(&e);
  matrix->GetF(&f);
  
  return Create( a*mX + c*mY + e, b*mX + d*mY + f, _retval);
}

//----------------------------------------------------------------------
// nsISVGValue methods:
NS_IMETHODIMP
nsSVGPoint::SetValueString(const nsAReadableString& aValue)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGPoint::GetValueString(nsAWritableString& aValue)
{
  NS_NOTYETIMPLEMENTED("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}
