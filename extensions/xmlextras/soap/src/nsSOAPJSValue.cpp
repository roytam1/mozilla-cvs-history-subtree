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

#include "nsSOAPJSValue.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"

NS_IMPL_ISUPPORTS1(nsSOAPJSValue, nsISOAPJSValue)

nsSOAPJSValue::nsSOAPJSValue()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsSOAPJSValue::~nsSOAPJSValue()
{
  /* destructor code */
}

/* attribute JSObjectPtr value; */
NS_IMETHODIMP nsSOAPJSValue::GetValue(JSObject * *aValue)
{
  if (mJSValue) {
    *aValue = mJSValue;
    return NS_OK;
  }
  else {
    *aValue = nsnull;       //  None of these should exist with null
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP nsSOAPJSValue::SetValue(JSObject * aValue)
{
  mJSValue = aValue;
  return NS_OK;
}

