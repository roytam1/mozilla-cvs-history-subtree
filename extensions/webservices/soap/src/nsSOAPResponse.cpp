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

#include "nsSOAPResponse.h"
#include "nsSOAPUtils.h"
#include "nsSOAPFault.h"
#include "nsISOAPEncoder.h"
#include "jsapi.h"
#include "nsISOAPParameter.h"
#include "nsIComponentManager.h"
#include "nsMemory.h"


nsSOAPResponse::nsSOAPResponse()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsSOAPResponse::~nsSOAPResponse()
{
  /* destructor code */
}

NS_IMPL_ISUPPORTS3(nsSOAPResponse, nsISOAPMessage, nsISOAPResponse, nsISecurityCheckedComponent)

/* attribute nsISOAPMessage respondingTo; */
NS_IMETHODIMP nsSOAPResponse::GetRespondingTo(nsISOAPMessage * *aRespondingTo)
{
  NS_ENSURE_ARG_POINTER(aRespondingTo);
  *aRespondingTo = mRespondingTo;
  NS_IF_ADDREF(*aRespondingTo);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPResponse::SetRespondingTo(nsISOAPMessage * aRespondingTo)
{
  mRespondingTo = aRespondingTo;
  return NS_OK;
}

/* readonly attribute boolean generatedFault; */
NS_IMETHODIMP nsSOAPResponse::GetGeneratedFault(PRBool *aGeneratedFault)
{
  nsCOMPtr<nsISOAPFault> fault;
  GetFault(getter_AddRefs(fault));
  *aGeneratedFault = fault != 0;
  return NS_OK;
}

static const nsString SOAPFault(NS_LITERAL_STRING("Fault"));
/* readonly attribute nsISOAPFault fault; */
NS_IMETHODIMP nsSOAPResponse::GetFault(nsISOAPFault * *aFault)
{
  nsCOMPtr<nsIDOMElement> body;
  GetBody(getter_AddRefs(body));
  nsresult rc = GetSOAPElementOf(body, SOAPFault, getter_AddRefs(body));
  if (NS_FAILED(rc))
    return rc;
  if (body) {
    *aFault = new nsSOAPFault(body);
    if (!*aFault)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    *aFault = nsnull;
  }
  return NS_OK;
}

/* readonly attribute nsISOAPParameter returnValue; */
NS_IMETHODIMP nsSOAPResponse::GetReturnValue(nsISOAPParameter * *aReturnValue)
{
  nsCOMPtr<nsISupportsArray> params;
  nsresult rc = UnmarshallParameters(getter_AddRefs(params));
  if (NS_FAILED(rc))
    return rc;
  if (params)
  {
    nsCOMPtr<nsISupports> retval = dont_AddRef(params->ElementAt(0));
    return retval->QueryInterface(NS_GET_IID(nsISOAPParameter), (void**)aReturnValue);
  }
  else
  {
    *aReturnValue = nsnull;
  }
  return nsnull;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPResponse::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPResponse::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPResponse::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPResponse::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPResponse))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
