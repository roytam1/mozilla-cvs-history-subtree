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
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsSOAPResponse::SetRespondingTo(nsISOAPMessage * aRespondingTo)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute boolean generatedFault; */
NS_IMETHODIMP nsSOAPResponse::GetGeneratedFault(PRBool *aGeneratedFault)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsISOAPFault fault; */
NS_IMETHODIMP nsSOAPResponse::GetFault(nsISOAPFault * *aFault)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsISOAPParameter returnValue; */
NS_IMETHODIMP nsSOAPResponse::GetReturnValue(nsISOAPParameter * *aReturnValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
#if 0

unmarshalling needs work

/* boolean generatedFault (); */
NS_IMETHODIMP nsSOAPResponse::GeneratedFault(PRBool *_retval)
{
  NS_ENSURE_ARG(_retval);
  if (mFaultElement) {
    *_retval = PR_TRUE;
  }
  else {
    *_retval = PR_FALSE;
  }
  return NS_OK;
}

/* readonly attribute nsISOAPFault fault; */
NS_IMETHODIMP nsSOAPResponse::GetFault(nsISOAPFault * *aFault)
{
  NS_ENSURE_ARG_POINTER(aFault);
  *aFault = nsnull;
  if (mFaultElement) {
    nsSOAPFault* fault = new nsSOAPFault(mFaultElement);
    if (!fault) return NS_ERROR_OUT_OF_MEMORY;

    return fault->QueryInterface(NS_GET_IID(nsISOAPFault), (void**)aFault);
  }
  return NS_OK;
}

/* readonly attribute nsISOAPParameter returnValue; */
NS_IMETHODIMP nsSOAPResponse::GetReturnValue(nsISOAPParameter * *aReturnValue)
{
  NS_ENSURE_ARG_POINTER(aReturnValue);
  nsresult rv;

  *aReturnValue = nsnull;

  if (mResultElement) {
    // The first child element is assumed to be the returned
    // value
    nsCOMPtr<nsIDOMElement> value;
    nsSOAPUtils::GetFirstChildElement(mResultElement, getter_AddRefs(value));

    // Get the inherited encoding style starting from the
    // value
    char* encodingStyle;
    nsSOAPUtils::GetInheritedEncodingStyle(value, 
                                           &encodingStyle);
    
    if (!encodingStyle) {
      encodingStyle = nsCRT::strdup(nsSOAPUtils::kSOAPEncodingURI);
    }
    
    // Find the corresponding encoder
    nsCAutoString encoderContractid;
    encoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
    encoderContractid.Append(encodingStyle);

    nsCOMPtr<nsISOAPEncoder> encoder = do_CreateInstance(encoderContractid);
    if (!encoder) {
      nsMemory::Free(encodingStyle);
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    // Convert the result element to a parameter
    nsCOMPtr<nsISOAPParameter> param;
    rv = encoder->ElementToParameter(value,
                                     encodingStyle,
                                     nsISOAPParameter::PARAMETER_TYPE_UNKNOWN,
                                     getter_AddRefs(param));
    nsMemory::Free(encodingStyle);
    if (NS_FAILED(rv)) return rv;

    *aReturnValue = param;
    NS_IF_ADDREF(*aReturnValue);
  }

  return NS_OK;
}

#endif

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
