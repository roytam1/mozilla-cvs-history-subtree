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

#include "nsSVGAnimatedRect.h"
#include "nsSVGRect.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"

////////////////////////////////////////////////////////////////////////
// nsSVGAnimatedRect

class nsSVGAnimatedRect : public nsIDOMSVGAnimatedRect,
                          public nsSVGValue,
                          public nsISVGValueObserver,
                          public nsSupportsWeakReference
{  
protected:
  friend nsresult NS_NewSVGAnimatedRect(nsIDOMSVGAnimatedRect** result,
                                        nsIDOMSVGRect* baseVal);

  nsSVGAnimatedRect();
  ~nsSVGAnimatedRect();
  void Init(nsIDOMSVGRect* baseVal);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGAnimatedRect interface:
  NS_DECL_NSIDOMSVGANIMATEDRECT

  // remainder of nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);
  
  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
protected:
  nsCOMPtr<nsIDOMSVGRect> mBaseVal;
};


//----------------------------------------------------------------------
// Implementation

nsSVGAnimatedRect::nsSVGAnimatedRect()
{
  NS_INIT_ISUPPORTS();
}

nsSVGAnimatedRect::~nsSVGAnimatedRect()
{
  if (!mBaseVal) return;
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedRect::Init(nsIDOMSVGRect* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->AddObserver(this);
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ISUPPORTS4(nsSVGAnimatedRect,
                   nsISVGValue,
                   nsIDOMSVGAnimatedRect,
                   nsISupportsWeakReference,
                   nsISVGValueObserver);

//----------------------------------------------------------------------
// nsISVGValue methods:

NS_IMETHODIMP
nsSVGAnimatedRect::SetValueString(const nsAReadableString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedRect::GetValueString(nsAWritableString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}

//----------------------------------------------------------------------
// nsIDOMSVGAnimatedRect methods:

/* readonly attribute nsIDOMSVGRect baseVal; */
NS_IMETHODIMP
nsSVGAnimatedRect::GetBaseVal(nsIDOMSVGRect * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGRect animVal; */
NS_IMETHODIMP
nsSVGAnimatedRect::GetAnimVal(nsIDOMSVGRect * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods

NS_IMETHODIMP
nsSVGAnimatedRect::WillModifySVGObservable(nsISVGValue* observable)
{
  WillModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedRect::DidModifySVGObservable (nsISVGValue* observable)
{
  DidModify();
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Exported creation functions:

nsresult
NS_NewSVGAnimatedRect(nsIDOMSVGAnimatedRect** result,
                      nsIDOMSVGRect* baseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedRect* animatedRect = new nsSVGAnimatedRect();
  if(!animatedRect) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedRect);

  animatedRect->Init(baseVal);
  
  *result = (nsIDOMSVGAnimatedRect*) animatedRect;
  
  return NS_OK;
}

