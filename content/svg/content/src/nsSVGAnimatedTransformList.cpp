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

#include "nsSVGAnimatedTransformList.h"
#include "nsSVGTransformList.h"
#include "nsSVGValue.h"
#include "nsWeakReference.h"

////////////////////////////////////////////////////////////////////////
// nsSVGAnimatedTransformList

class nsSVGAnimatedTransformList : public nsIDOMSVGAnimatedTransformList,
                                   public nsSVGValue,
                                   public nsISVGValueObserver,
                                   public nsSupportsWeakReference
{  
protected:
  friend nsresult
  NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                                 nsIDOMSVGTransformList* baseVal);

  nsSVGAnimatedTransformList();
  ~nsSVGAnimatedTransformList();
  void Init(nsIDOMSVGTransformList* baseVal);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGAnimatedTransformList interface:
  NS_DECL_NSIDOMSVGANIMATEDTRANSFORMLIST

  // remainder of nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);
  
  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference
  
protected:
  nsCOMPtr<nsIDOMSVGTransformList> mBaseVal;
};


//----------------------------------------------------------------------
// Implementation

nsSVGAnimatedTransformList::nsSVGAnimatedTransformList()
{
  NS_INIT_ISUPPORTS();
}

nsSVGAnimatedTransformList::~nsSVGAnimatedTransformList()
{
  if (!mBaseVal) return;
    nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->RemoveObserver(this);
}

void
nsSVGAnimatedTransformList::Init(nsIDOMSVGTransformList* baseVal)
{
  mBaseVal = baseVal;
  if (!mBaseVal) return;
  nsCOMPtr<nsISVGValue> val = do_QueryInterface(mBaseVal);
  if (!val) return;
  val->AddObserver(this);
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGAnimatedTransformList)
NS_IMPL_RELEASE(nsSVGAnimatedTransformList)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsISVGValueObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedTransformList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END


//----------------------------------------------------------------------
// nsISVGValue methods:

NS_IMETHODIMP
nsSVGAnimatedTransformList::SetValueString(const nsAReadableString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->SetValueString(aValue);
}

NS_IMETHODIMP
nsSVGAnimatedTransformList::GetValueString(nsAWritableString& aValue)
{
  nsCOMPtr<nsISVGValue> value = do_QueryInterface(mBaseVal);
  return value->GetValueString(aValue);
}

//----------------------------------------------------------------------
// nsIDOMSVGAnimatedTransformList methods:

/* readonly attribute nsIDOMSVGTransformList baseVal; */
NS_IMETHODIMP
nsSVGAnimatedTransformList::GetBaseVal(nsIDOMSVGTransformList * *aBaseVal)
{
  *aBaseVal = mBaseVal;
  NS_ADDREF(*aBaseVal);
  return NS_OK;
}

/* readonly attribute nsIDOMSVGTransformList animVal; */
NS_IMETHODIMP
nsSVGAnimatedTransformList::GetAnimVal(nsIDOMSVGTransformList * *aAnimVal)
{
  *aAnimVal = mBaseVal;
  NS_ADDREF(*aAnimVal);
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISVGValueObserver methods

NS_IMETHODIMP
nsSVGAnimatedTransformList::WillModifySVGObservable(nsISVGValue* observable)
{
  WillModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedTransformList::DidModifySVGObservable (nsISVGValue* observable)
{
  DidModify();
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Exported creation functions:

nsresult
NS_NewSVGAnimatedTransformList(nsIDOMSVGAnimatedTransformList** result,
                      nsIDOMSVGTransformList* baseVal)
{
  *result = nsnull;
  
  nsSVGAnimatedTransformList* animatedTransformList = new nsSVGAnimatedTransformList();
  if(!animatedTransformList) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedTransformList);

  animatedTransformList->Init(baseVal);
  
  *result = (nsIDOMSVGAnimatedTransformList*) animatedTransformList;
  
  return NS_OK;
}

