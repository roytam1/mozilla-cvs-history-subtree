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
  
nsCOMPtr<nsISOAPTypeRegistry> nsSOAPMessage::mDefaultTypes = do_GetService(NS_SOAPDEFAULTTYPEREGISTRY_CONTRACTID);

nsSOAPMessage::nsSOAPMessage()
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
}

nsSOAPMessage::~nsSOAPMessage()
{
}

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

static const nsString SOAPNS(NS_LITERAL_STRING("http://schemas.xmlsoap.org/soap/envelope/"));
static const nsString SOAPEnvelope(NS_LITERAL_STRING("Envelope"));

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
      if (name.Equals(SOAPEnvelope)
        && namespaceURI.Equals(SOAPNS))
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

nsresult nsSOAPMessage::GetSOAPElementOf(nsIDOMElement *aParent, const nsAReadableString& aType, nsIDOMElement * *aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  if (aParent)
  {
    nsCOMPtr<nsIDOMNode> element;
    aParent->GetFirstChild(getter_AddRefs(element));
    while (element)
    {
      PRUint16 type;
      nsAutoString name, namespaceURI;
      element->GetNodeType(&type);
      element->GetLocalName(name);
      element->GetNamespaceURI(namespaceURI);
      if (type == nsIDOMNode::ELEMENT_NODE
        && name.Equals(aType)
        && namespaceURI.Equals(SOAPNS))
      {
        return element->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      }
    }
  }
  *aElement = nsnull;
  return NS_OK;
}

static const nsString SOAPHeader(NS_LITERAL_STRING("Header"));
/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPMessage::GetHeader(nsIDOMElement * *aHeader)
{
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  return GetSOAPElementOf(env, SOAPHeader, aHeader);
}

static const nsString SOAPBody(NS_LITERAL_STRING("Body"));
/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPMessage::GetBody(nsIDOMElement * *aBody)
{
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  return GetSOAPElementOf(env, SOAPBody, aBody);
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

static const nsString SOAPCall(NS_LITERAL_STRING("#SOAPCall"));
static const nsString SOAPNull(NS_LITERAL_STRING(""));
/* void marshallParameters (in nsISupportsArray SOAPParameters); */
NS_IMETHODIMP nsSOAPMessage::MarshallParameters(nsISupportsArray *SOAPParameters)
{
  nsCOMPtr<nsISupports> ignore;
  return Marshall(SOAPNull, SOAPNull, SOAPCall, SOAPParameters, getter_AddRefs(ignore));
}

/* nsISupportsArray unmarshallParameters (); */
NS_IMETHODIMP nsSOAPMessage::UnmarshallParameters(nsISupportsArray **_retval)
{
  nsCOMPtr<nsISupports> result;
  nsAutoString name;
  nsresult rv = Unmarshall(name, SOAPNull, SOAPNull, SOAPCall, nsnull, getter_AddRefs(result));

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
      nsCOMPtr<nsISOAPMessage> message;
      nsresult rc = QueryInterface(NS_GET_IID(nsISOAPMessage), getter_AddRefs(message));
      if (NS_FAILED(rc))
        return rc;
      return marshaller->Marshall(aName, aEncodingStyleURI, aType, namespaceURI, name, aSource, message, configuration, _retval);
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
      nsCOMPtr<nsISOAPMessage> message;
      nsresult rc = QueryInterface(NS_GET_IID(nsISOAPMessage), getter_AddRefs(message));
      if (NS_FAILED(rc))
        return rc;
      return unmarshaller->Unmarshall(aName, aEncodingStyleURI, ptype, aNamespace, aLocalname, aSource, message, configuration, _retval);
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
