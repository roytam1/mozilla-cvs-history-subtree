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

#include "nsSVGValue.h"

////////////////////////////////////////////////////////////////////////
// nsSVGGenericStringValue implementation

class nsSVGGenericStringValue : public nsSVGValue
{
protected:
  friend nsresult
  NS_CreateSVGGenericStringValue(const nsAReadableString& aValue, nsISVGValue** aResult);
  
  nsSVGGenericStringValue(const nsAReadableString& aValue);
  virtual ~nsSVGGenericStringValue();
  
public:
  NS_DECL_ISUPPORTS

  // nsISVGValue interface: 
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);

protected:
  nsString mValue;
};


nsresult
NS_CreateSVGGenericStringValue(const nsAReadableString& aValue,
                               nsISVGValue** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult) return NS_ERROR_NULL_POINTER;
  
  *aResult = (nsISVGValue*) new nsSVGGenericStringValue(aValue);
  if(!*aResult) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsSVGGenericStringValue::nsSVGGenericStringValue(const nsAReadableString& aValue)
{
  NS_INIT_ISUPPORTS();
  mValue = aValue;
}

nsSVGGenericStringValue::~nsSVGGenericStringValue()
{
}


// nsISupports methods:

NS_IMPL_ISUPPORTS1(nsSVGGenericStringValue, nsISVGValue);


// nsISVGValue methods:

NS_IMETHODIMP
nsSVGGenericStringValue::SetValueString(const nsAReadableString& aValue)
{
  WillModify();
  mValue = aValue;
  DidModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGGenericStringValue::GetValueString(nsAWritableString& aValue)
{
  aValue = mValue;
  return NS_OK;
}


