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

#include "nsSVGRect.h"
#include "prdtoa.h"
#include "nsSVGValue.h"

////////////////////////////////////////////////////////////////////////
// nsSVGRect 'letter' class

class nsSVGRectLetter : public nsIDOMSVGRect,
                        public nsSVGValue
{
public:
  static nsresult Create(nsIDOMSVGRect** result,
                         float x=0.0f, float y=0.0f,
                         float w=0.0f, float h=0.0f);
protected:
  nsSVGRectLetter(float x, float y, float w, float h);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGRect interface:
  NS_DECL_NSIDOMSVGRECT

  // nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);
  
  
protected:
  float mX, mY, mWidth, mHeight;
};

//----------------------------------------------------------------------
// implementation:

nsresult
nsSVGRectLetter::Create(nsIDOMSVGRect** result,
                        float x, float y, float w, float h)
{
  *result = (nsIDOMSVGRect*) new nsSVGRectLetter(x,y,w,h);
  if(!*result) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*result);
  return NS_OK;
}


nsSVGRectLetter::nsSVGRectLetter(float x, float y, float w, float h)
    : mX(x), mY(y), mWidth(w), mHeight(h)
{
  NS_INIT_ISUPPORTS();
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ISUPPORTS2(nsSVGRectLetter, nsIDOMSVGRect, nsISVGValue);

//----------------------------------------------------------------------
// nsISVGValue methods:

NS_IMETHODIMP
nsSVGRectLetter::SetValueString(const nsAReadableString& aValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGRectLetter::GetValueString(nsAWritableString& aValue)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------
// nsIDOMSVGRect methods:

/* attribute float x; */
NS_IMETHODIMP nsSVGRectLetter::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRectLetter::SetX(float aX)
{
  WillModify();
  mX = aX;
  DidModify();
  return NS_OK;
}

/* attribute float y; */
NS_IMETHODIMP nsSVGRectLetter::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRectLetter::SetY(float aY)
{
  WillModify();
  mY = aY;
  DidModify();
  return NS_OK;
}

/* attribute float width; */
NS_IMETHODIMP nsSVGRectLetter::GetWidth(float *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRectLetter::SetWidth(float aWidth)
{
  WillModify();
  mWidth = aWidth;
  DidModify();
  return NS_OK;
}

/* attribute float height; */
NS_IMETHODIMP nsSVGRectLetter::GetHeight(float *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRectLetter::SetHeight(float aHeight)
{
  WillModify();
  mHeight = aHeight;
  DidModify();
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// nsSVGRect 'envelope' class

class nsSVGRectEnvelope : public nsIDOMSVGRect,
                          public nsSVGValue
{
public:
  static nsresult Create(nsIDOMSVGRect** result,
                         nsIDOMSVGRect* prototype,
                         nsIDOMSVGRect* body=nsnull);
protected:
  nsSVGRectEnvelope(nsIDOMSVGRect* prototype, nsIDOMSVGRect* body);
  virtual ~nsSVGRectEnvelope();
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGRect interface:
  NS_DECL_NSIDOMSVGRECT

  // nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);
  
  
protected:
  void EnsureBody();
  nsIDOMSVGRect* Delegate() { return mBody ? mBody.get() : mPrototype.get(); }
  
  nsCOMPtr<nsIDOMSVGRect> mPrototype;
  nsCOMPtr<nsIDOMSVGRect> mBody;
};

//----------------------------------------------------------------------
// implementation:

nsresult
nsSVGRectEnvelope::Create(nsIDOMSVGRect** result,
                          nsIDOMSVGRect* prototype,
                          nsIDOMSVGRect* body)
{
  *result = (nsIDOMSVGRect*) new nsSVGRectEnvelope(prototype, body);
  if(!*result) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*result);
  return NS_OK;
}

nsSVGRectEnvelope::nsSVGRectEnvelope(nsIDOMSVGRect* prototype,
                                     nsIDOMSVGRect* body)
    : mPrototype(prototype), mBody(body)
{
  NS_INIT_ISUPPORTS();
  NS_ASSERTION(mPrototype, "need prototype");
}

nsSVGRectEnvelope::~nsSVGRectEnvelope()
{
//   if (mBody) {
//     nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBody);
//     if (val)
//       val->RemoveObserver(this);
//   }
}

void nsSVGRectEnvelope::EnsureBody()
{
  if (mBody) return;

  nsSVGRectLetter::Create(getter_AddRefs(mBody));
  NS_ASSERTION(mBody, "couldn't create letter");
//   nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBody);
//   if (val)
//     val->AddObserver(this);
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ISUPPORTS2(nsSVGRectEnvelope, nsIDOMSVGRect, nsISVGValue);

//----------------------------------------------------------------------
// nsISVGValue methods:

NS_IMETHODIMP
nsSVGRectEnvelope::SetValueString(const nsAReadableString& aValue)
{
  EnsureBody();
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBody);
  NS_ASSERTION(val, "missing interface on letter");

  return val->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGRectEnvelope::GetValueString(nsAWritableString& aValue)
{
  nsCOMPtr<nsISVGValue> val = do_QueryInterface( Delegate() );
  NS_ASSERTION(val, "missing interface on letter");
  
  return val->GetValueString(aValue);
}

//----------------------------------------------------------------------
// nsIDOMSVGRect methods:

/* attribute float x; */
NS_IMETHODIMP nsSVGRectEnvelope::GetX(float *aX)
{
  return Delegate()->GetX(aX);
}
NS_IMETHODIMP nsSVGRectEnvelope::SetX(float aX)
{
  WillModify();
  EnsureBody();
  nsresult rv =  mBody->SetX(aX);
  DidModify();
  return rv;
}

/* attribute float y; */
NS_IMETHODIMP nsSVGRectEnvelope::GetY(float *aY)
{
  return Delegate()->GetY(aY);
}
NS_IMETHODIMP nsSVGRectEnvelope::SetY(float aY)
{
  WillModify();
  EnsureBody();
  nsresult rv = mBody->SetY(aY);
  DidModify();
  return rv;
}

/* attribute float width; */
NS_IMETHODIMP nsSVGRectEnvelope::GetWidth(float *aWidth)
{
  return Delegate()->GetWidth(aWidth);
}
NS_IMETHODIMP nsSVGRectEnvelope::SetWidth(float aWidth)
{
  WillModify();
  EnsureBody();
  nsresult rv = mBody->SetWidth(aWidth);
  DidModify();
  return rv;
}

/* attribute float height; */
NS_IMETHODIMP nsSVGRectEnvelope::GetHeight(float *aHeight)
{
  return Delegate()->GetHeight(aHeight);
}
NS_IMETHODIMP nsSVGRectEnvelope::SetHeight(float aHeight)
{
  WillModify();
  EnsureBody();
  nsresult rv = mBody->SetHeight(aHeight);
  DidModify();
  return rv;
}


////////////////////////////////////////////////////////////////////////
// Exported creation functions:

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, float x, float y,
              float width, float height)
{
  return nsSVGRectLetter::Create(result, x, y, width, height);
}

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, nsIDOMSVGRect* prototype)
{
  return nsSVGRectEnvelope::Create(result, prototype);
}
