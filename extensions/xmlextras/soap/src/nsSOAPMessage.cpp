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
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
}

nsSOAPMessage::~nsSOAPMessage()
{
}

NS_IMPL_ISUPPORTS2(nsSOAPMessage, nsISOAPMessage, nsISecurityCheckedComponent)

/* attribute nsIDOMDocument message; */
NS_IMETHODIMP nsSOAPMessage::GetMessage(nsIDOMDocument * *aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  *aMessage = mMessage;
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetMessage(nsIDOMDocument * aMessage)
{
  mMessage = aMessage;
  return NS_OK;
}

static const nsString SOAPNS(NS_LITERAL_STRING("http://schemas.xmlsoap.org/soap/envelope/"));

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
      if (name.Equals(NS_LITERAL_STRING("Envelope"))
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

static nsresult GetSOAPElementOf(nsIDOMElement *aParent, const char* aType, nsIDOMElement * *aElement)
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
        && name.Equals(NS_LITERAL_STRING(aType))
        && namespaceURI.Equals(SOAPNS))
      {
        return element->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      }
    }
  }
  *aElement = nsnull;
  return NS_OK;
}

/* readonly attribute nsIDOMElement header; */
NS_IMETHODIMP nsSOAPMessage::GetHeader(nsIDOMElement * *aHeader)
{
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  return GetSOAPElementOf(env, "Header", aHeader);
}

/* readonly attribute nsIDOMElement body; */
NS_IMETHODIMP nsSOAPMessage::GetBody(nsIDOMElement * *aBody)
{
  nsCOMPtr<nsIDOMElement> env;
  GetEnvelope(getter_AddRefs(env));
  return GetSOAPElementOf(env, "Body", aBody);
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
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISupportsArray unmarshallParameters (); */
NS_IMETHODIMP nsSOAPMessage::UnmarshallParameters(nsISupportsArray **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetTypes(nsISOAPTypeRegistry * aTypes)
{
  mTypes = aTypes;
  return NS_OK;
}

#if 0

I kept these messages in the file as a reminder that we need to do marshalling / unmarshalling

PRBool
nsSOAPMessage::HasBodyEntry()
{
  if (!mBodyElement) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMElement> entry;
  nsSOAPUtils::GetFirstChildElement(mBodyElement, getter_AddRefs(entry));
  
  if (entry) {
    return PR_TRUE;
  }
  else {
    return PR_FALSE;
  }
}

nsresult
nsSOAPMessage::CreateBodyEntry(PRBool aNewParameters)
{
  nsresult rv = EnsureDocumentAllocated();
  if (NS_FAILED(rv)) return rv;

  // Create the element that will be the new body entry
  nsCOMPtr<nsIDOMElement> entry;
  nsCOMPtr<nsIDOMNode> dummy;
  
  rv = mEnvelopeDocument->CreateElementNS(NS_ConvertASCIItoUCS2(mTargetObjectURI.get()), 
                                          mMethodName, getter_AddRefs(entry));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  // See if there's an existing body entry (we only worry
  // about the first).
  nsCOMPtr<nsIDOMElement> oldEntry;
  nsSOAPUtils::GetFirstChildElement(mBodyElement, getter_AddRefs(oldEntry));

  // If there is, we're going to replace it, but preserve its
  // children.
  if (oldEntry) {
    // Remove the old entry from the body
    mBodyElement->RemoveChild(oldEntry, getter_AddRefs(dummy));
    
    if (!aNewParameters) {
      // Transfer the children from the old to the new
      nsCOMPtr<nsIDOMNode> child;
      oldEntry->GetFirstChild(getter_AddRefs(child));
      while (child) {
        oldEntry->RemoveChild(child, getter_AddRefs(dummy));
        entry->AppendChild(child, getter_AddRefs(dummy));
        
        nsCOMPtr<nsIDOMNode> temp = child;
        temp->GetNextSibling(getter_AddRefs(child));
      }
    }
  }

  mBodyElement->AppendChild(entry, getter_AddRefs(dummy));

  // If there wasn't an old entry and we have parameters, or we
  // we have new parameters, create the parameter elements.
  if ((!entry && mParameters) || aNewParameters) {
    rv = CreateParameterElements();
    if (NS_FAILED(rv)) return rv;
  }

  return NS_OK;
}

nsresult
nsSOAPMessage::CreateParameterElements()
{
  nsresult rv = EnsureDocumentAllocated();
  if (NS_FAILED(rv)) return rv;

  // Get the body entry that's going to be the parent of
  // the parameter elements. If we got here, there should
  // be one.
  nsCOMPtr<nsIDOMElement> entry;
  nsSOAPUtils::GetFirstChildElement(mBodyElement, getter_AddRefs(entry));
  if (!entry) return NS_ERROR_FAILURE;

  // Get the inherited encoding style starting from the
  // body entry.
  nsXPIDLCString encodingStyle;
  nsSOAPUtils::GetInheritedEncodingStyle(entry, getter_Copies(encodingStyle));

  // Find the corresponding encoder
  nsCAutoString encoderContractid;
  encoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
  encoderContractid.Append(encodingStyle);

  nsCOMPtr<nsISOAPEncoder> encoder = do_CreateInstance(encoderContractid);
  if (!encoder) return NS_ERROR_INVALID_ARG;

  PRUint32 index, count;
  mParameters->Count(&count);

  for(index = 0; index < count; index++) {
    nsCOMPtr<nsISupports> isup = getter_AddRefs(mParameters->ElementAt(index));
    nsCOMPtr<nsISOAPParameter> parameter = do_QueryInterface(isup);
    
    if (parameter) {
      nsCOMPtr<nsISOAPEncoder> paramEncoder = encoder;

      // See if the parameter has its own encoding style
      nsXPIDLCString paramEncoding;
      parameter->GetEncodingStyleURI(getter_Copies(paramEncoding));
      
      // If it does and it's different from the inherited one,
      // find an encoder
      if (paramEncoding && 
          (nsCRT::strcmp(encodingStyle, paramEncoding) != 0)) {
        nsCAutoString paramEncoderContractid;
        paramEncoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
        paramEncoderContractid.Append(paramEncoding);
        
        paramEncoder = do_CreateInstance(paramEncoderContractid);
        if (!paramEncoder) return NS_ERROR_INVALID_ARG;
      }

      // Convert the parameter to an element
      nsCOMPtr<nsIDOMElement> element;
      encoder->ParameterToElement(parameter,
                                  paramEncoding ? paramEncoding : encodingStyle,
                                  mEnvelopeDocument,
                                  getter_AddRefs(element));
      
      // Append the parameter element to the body entry
      nsCOMPtr<nsIDOMNode> dummy;
      entry->AppendChild(element, getter_AddRefs(dummy));
    }
  }
  
  return NS_OK;
}

nsresult
nsSOAPMessage::ClearParameterElements()
{
  nsresult rv = EnsureDocumentAllocated();
  if (NS_FAILED(rv)) return rv;

  // Get the body entry that's the parent of the parameter
  // elements (assuming there is one)
  nsCOMPtr<nsIDOMElement> entry;
  nsSOAPUtils::GetFirstChildElement(mBodyElement, getter_AddRefs(entry));

  if (entry) {
    // Get rid of all the children of the body entry
    nsCOMPtr<nsIDOMNode> child;
    entry->GetFirstChild(getter_AddRefs(child));
    while (child) {
      nsCOMPtr<nsIDOMNode> dummy;
      entry->RemoveChild(child, getter_AddRefs(dummy));
      entry->GetFirstChild(getter_AddRefs(child));
    }
  }
  
  return NS_OK;
}

/* void setParameters (); */
NS_IMETHODIMP nsSOAPMessage::SetParameters()
{
  nsresult rv;

  // Clear out any existing parameters
  if (mParameters) {
    ClearParameterElements();
    mParameters->Clear();
  }
  else {
    rv = NS_NewISupportsArray(getter_AddRefs(mParameters));
    if (!mParameters) return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIXPCNativeMessageContext> cc;
  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = xpc->GetCurrentNativeMessageContext(getter_AddRefs(cc));
  }

  // This should only be called from script
  if (NS_FAILED(rv) || !cc) {
    return NS_ERROR_FAILURE;
  }

  PRUint32 argc;
  rv = cc->GetArgc(&argc);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  
  jsval* argv;
  rv = cc->GetArgvPtr(&argv);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
  
  JSContext* cx;
  rv = cc->GetJSContext(&cx);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  // For each parameter to this method
  PRUint32 index;
  for (index = 0; index < argc; index++) {
    nsCOMPtr<nsISOAPParameter> param;
    jsval val = argv[index];
    
    // First check if it's a parameter
    if (JSVAL_IS_OBJECT(val)) {
      JSObject* paramobj;
      paramobj = JSVAL_TO_OBJECT(val);

      // Check if it's a wrapped native
      nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
      xpc->GetWrappedNativeOfJSObject(cx, paramobj, getter_AddRefs(wrapper));
      
      if (wrapper) {
        // Get the native and see if it's a SOAPParameter
        nsCOMPtr<nsISupports> native;
        wrapper->GetNative(getter_AddRefs(native));
        if (native) {
          param = do_QueryInterface(native);
        }
      }
    }

    // Otherwise create a new parameter with the value
    if (!param) {
      nsSOAPParameter* newparam = new nsSOAPParameter();
      if (!newparam) return NS_ERROR_OUT_OF_MEMORY;
      
      param = (nsISOAPParameter*)newparam;
      rv = newparam->SetValue(cx, val);
      if (NS_FAILED(rv)) return rv;
    }

    mParameters->AppendElement(param);
  }

  if (HasBodyEntry()) {
    return CreateParameterElements();
  }
  else if ((mTargetObjectURI.Length() > 0) && (mMethodName.Length() > 0)) {
    return CreateBodyEntry(PR_TRUE);
  }

  return NS_OK;
}
#endif

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
