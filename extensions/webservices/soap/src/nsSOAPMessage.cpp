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

#include "nsSOAPUtils.h"
#include "nsSOAPMessage.h"
#include "nsCRT.h"
#include "jsapi.h"
#include "nsISOAPParameter.h"
#include "nsISOAPType.h"
#include "nsISOAPMarshaller.h"
#include "nsISOAPUnmarshaller.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

/////////////////////////////////////////////
//
//
/////////////////////////////////////////////
  
nsSOAPMessage::nsSOAPMessage()
 : mDefaultTypes(do_GetService(NS_SOAPDEFAULTTYPEREGISTRY_CONTRACTID))
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
}

nsSOAPMessage::~nsSOAPMessage()
{
}

NS_IMPL_ISUPPORTS2(nsSOAPMessage, 
                   nsISOAPMessage, 
                   nsISecurityCheckedComponent)

/* attribute nsIDOMDocument message; */
NS_IMETHODIMP nsSOAPMessage::GetMessage(nsIDOMDocument * *aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  *aMessage = mMessage;
  NS_IF_ADDREF(*aMessage);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMessage(nsIDOMDocument * aMessage)
{
  mMessage = aMessage;
  return NS_OK;
}

/* readonly attribute nsIDOMElement envelope; */
NS_IMETHODIMP nsSOAPMessage::GetEnvelope(nsIDOMElement * *aEnvelope)
{
  NS_ENSURE_ARG_POINTER(aEnvelope);

  if (mMessage) {
    nsCOMPtr<nsIDOMElement> root;
    mMessage->GetDocumentElement(getter_AddRefs(root));
    if (root) {
      nsAutoString name, namespaceURI;
      root->GetLocalName(name);
      root->GetNamespaceURI(namespaceURI);
      if (name.Equals(nsSOAPUtils::kEnvelopeTagName)
        && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
      {
        *aEnvelope = root;
        NS_ADDREF(*aEnvelope);
        return NS_OK;
      }
    } 

  }
  *aEnvelope = nsnull;
  return NS_OK;
}

/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPMessage::GetHeader(nsIDOMElement * *aHeader)
{
  NS_ENSURE_ARG_POINTER(aHeader);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kHeaderTagName, 
      aHeader);
  }
  else {
    *aHeader = nsnull;
  }
  return NS_OK;
}

/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPMessage::GetBody(nsIDOMElement * *aBody)
{
  NS_ENSURE_ARG_POINTER(aBody);
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  if (env) {
    nsSOAPUtils::GetSpecificChildElement(env, 
      nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kBodyTagName, 
      aBody);
  }
  else {
    *aBody = nsnull;
  }
  return NS_OK;
}

/* attribute DOMString actionURI; */
NS_IMETHODIMP nsSOAPMessage::GetActionURI(nsAWritableString & aActionURI)
{
  NS_ENSURE_ARG_POINTER(&aActionURI);
  aActionURI.Assign(mActionURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetActionURI(const nsAReadableString & aActionURI)
{
  mActionURI.Assign(aActionURI);
  return NS_OK;
}

/* attribute DOMString encodingStyleURI; */
NS_IMETHODIMP nsSOAPMessage::GetEncodingStyleURI(nsAWritableString & aEncodingStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingStyleURI);
  aEncodingStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetEncodingStyleURI(const nsAReadableString & aEncodingStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingStyleURI);
  return NS_OK;
}

/* attribute DOMString methodName; */
NS_IMETHODIMP nsSOAPMessage::GetMethodName(nsAWritableString & aMethodName)
{
  NS_ENSURE_ARG_POINTER(&aMethodName);
  aMethodName.Assign(mMethodName);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMethodName(const nsAReadableString & aMethodName)
{
  mMethodName.Assign(aMethodName);
  return NS_OK;
}

/* attribute DOMString targetObjectURI; */
NS_IMETHODIMP nsSOAPMessage::GetTargetObjectURI(nsAWritableString & aTargetObjectURI)
{
  NS_ENSURE_ARG_POINTER(&aTargetObjectURI);
  aTargetObjectURI.Assign(mTargetObjectURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetTargetObjectURI(const nsAReadableString & aTargetObjectURI)
{
  mTargetObjectURI.Assign(aTargetObjectURI);
  return NS_OK;
}

/* void marshallParameters (in nsISupportsArray SOAPParameters); */
NS_IMETHODIMP nsSOAPMessage::MarshallParameters(nsISupportsArray *SOAPParameters)
{
  nsCOMPtr<nsISupports> ignore;
  return Marshall(nsSOAPUtils::kEmpty, nsSOAPUtils::kEmpty, nsSOAPUtils::kSOAPCallType, SOAPParameters, getter_AddRefs(ignore));
}

/* nsISupportsArray unmarshallParameters (); */
NS_IMETHODIMP nsSOAPMessage::UnmarshallParameters(nsISupportsArray **_retval)
{
  nsCOMPtr<nsISupports> result;
  nsAutoString name;
  nsresult rv = Unmarshall(name, nsSOAPUtils::kEmpty, nsSOAPUtils::kEmpty, nsSOAPUtils::kSOAPCallType, nsnull, getter_AddRefs(result));

  if (NS_SUCCEEDED(rv))
    return rv;

  return result->QueryInterface(NS_GET_IID(nsISupportsArray), (void**)_retval);
}

/* readonly attribute unsigned long status; */
NS_IMETHODIMP nsSOAPMessage::GetStatus(PRUint32 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

/* attribute nsISupportsArray protocol; */
NS_IMETHODIMP nsSOAPMessage::GetProtocol(nsISupportsArray * *aProtocol)
{
  NS_ENSURE_ARG_POINTER(aProtocol);
  *aProtocol = mProtocol;
  NS_IF_ADDREF(*aProtocol);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetProtocol(nsISupportsArray * aProtocol)
{
  mProtocol = aProtocol;
  return NS_OK;
}

/* attribute nsISOAPTypeRegistry types; */
NS_IMETHODIMP nsSOAPMessage::GetTypes(nsISOAPTypeRegistry * *aTypes)
{
  NS_ENSURE_ARG_POINTER(aTypes);
  *aTypes = mTypes;
  NS_IF_ADDREF(*aTypes);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetTypes(nsISOAPTypeRegistry * aTypes)
{
  mTypes = aTypes;
  return NS_OK;
}

/* nsISupports marshall (in DOMString aName, in DOMString aEncodingStyleURI, in DOMString aType, in nsISupports aSource); */
NS_IMETHODIMP nsSOAPMessage::Marshall(const nsAReadableString & aName, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aType, nsISupports *aSource, nsISupports **_retval)
{
  nsresult rc;
  nsCOMPtr<nsISOAPType> type;
  rc = mTypes->QueryByParameterType(aEncodingStyleURI, aType, getter_AddRefs(type));
  if (!type)
  {
    rc = mDefaultTypes->QueryByParameterType(aEncodingStyleURI, aType, getter_AddRefs(type));
  }
  if (!type)
  {
    nsAutoString namespaceURI, name;
    nsCOMPtr<nsISOAPMarshaller> marshaller;
    nsCOMPtr<nsISupports> configuration;
    rc = type->GetElementLocalname(name);
    rc = type->GetElementNamespace(namespaceURI);
    rc = type->GetMarshallConfiguration(getter_AddRefs(configuration));
    rc = type->GetMarshaller(getter_AddRefs(marshaller));
    if (marshaller)
    {
      return marshaller->Marshall(aName, aEncodingStyleURI, aType, namespaceURI, name, aSource, this, configuration, _retval);
    }
  }
  return NS_ERROR_FAILURE;
}

/* nsISupports unmarshall (out DOMString aName, in DOMString aEncodingStyleURI, in DOMString aNamespace, in DOMString aLocalname, in nsISupports aSource); */
NS_IMETHODIMP nsSOAPMessage::Unmarshall(nsAWritableString & aName, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aNamespace, const nsAReadableString & aLocalname, nsISupports *aSource, nsISupports **_retval)
{
  nsresult rc;
  nsCOMPtr<nsISOAPType> type;
  rc = mTypes->QueryByElementType(aEncodingStyleURI, aNamespace, aLocalname, getter_AddRefs(type));
  if (!type)
  {
    rc = mDefaultTypes->QueryByElementType(aEncodingStyleURI, aNamespace, aLocalname, getter_AddRefs(type));
  }
  if (!type)
  {
    nsAutoString ptype;
    nsCOMPtr<nsISOAPUnmarshaller> unmarshaller;
    nsCOMPtr<nsISupports> configuration;
    rc = type->GetParameterType(ptype);
    rc = type->GetUnmarshallConfiguration(getter_AddRefs(configuration));
    rc = type->GetUnmarshaller(getter_AddRefs(unmarshaller));
    if (unmarshaller)
    {
      return unmarshaller->Unmarshall(aName, aEncodingStyleURI, ptype, aNamespace, aLocalname, aSource, this, configuration, _retval);
    }
  }
  return NS_ERROR_FAILURE;
}

static const char*kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPMessage::CanCreateWrapper(const nsIID * iid, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPMessage::CanCallMethod(const nsIID * iid, const PRUnichar*methodName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanGetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPMessage::CanSetProperty(const nsIID * iid, const PRUnichar*propertyName, char**_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPMessage))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
