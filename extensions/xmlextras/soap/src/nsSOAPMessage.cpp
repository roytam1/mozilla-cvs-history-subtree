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

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIXPConnect.h"
#include "nsSOAPUtils.h"
#include "nsSOAPMessage.h"
#include "nsSOAPParameter.h"
#include "nsSOAPEncodingRegistry.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "nsSOAPJSValue.h"
#include "nsIDOMDocument.h"
#include "nsIDOMParser.h"
#include "nsIDOMElement.h"
#include "nsIDOMNamedNodeMap.h"

static NS_DEFINE_CID(kDOMParserCID, NS_DOMPARSER_CID);
/////////////////////////////////////////////
//
//
/////////////////////////////////////////////
  
nsSOAPMessage::nsSOAPMessage(): mEncodings(new nsSOAPEncodingRegistry())
{
  NS_INIT_ISUPPORTS();
  mStatus = 0;
}

nsSOAPMessage::~nsSOAPMessage()
{
}

NS_IMPL_ISUPPORTS3(nsSOAPMessage, 
                   nsISOAPMessage, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable)

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
NS_IMETHODIMP nsSOAPMessage::GetEncodingStyleURI(nsAWritableString & aEncodingsStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingsStyleURI);
  aEncodingsStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetEncodingStyleURI(const nsAReadableString & aEncodingsStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingsStyleURI);
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

NS_NAMED_LITERAL_STRING(kEmptySOAPDocStr, "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\">"
"<SOAP-ENV:Header>"
"</SOAP-ENV:Header>"
"<SOAP-ENV:Body>"
"</SOAP-ENV:Body>"
"</SOAP-ENV:Envelope>");

//  The default encoders assume that the above declarations are in place, without bothering to check.

/* void encodeParameters (in nsISupportsArray SOAPParameters); */
NS_IMETHODIMP nsSOAPMessage::EncodeParameters(nsISupportsArray *SOAPParameters)
{
  nsresult rv;
  nsCOMPtr<nsIDOMNode> ignored;
  nsCOMPtr<nsIDOMParser> parser = do_CreateInstance(kDOMParserCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsAutoString docstr;
  rv = parser->ParseFromString(kEmptySOAPDocStr.get(), "text/xml", 
		               getter_AddRefs(mMessage));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDOMElement> header;
  nsCOMPtr<nsIDOMElement> body;
  rv = GetHeader(getter_AddRefs(header));
  if (NS_FAILED(rv)) return rv;
  rv = GetBody(getter_AddRefs(body));
  if (NS_FAILED(rv)) return rv;

  if (!mMethodName.IsEmpty()) {  //  Only produce a call element if mMethodName was non-empty
    nsCOMPtr<nsIDOMElement> call;
    rv = mMessage->CreateElementNS(mTargetObjectURI, mMethodName, getter_AddRefs(call));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIDOMNode> ignored;
    rv = body->AppendChild(call, getter_AddRefs(ignored));
    if (NS_FAILED(rv)) return rv;
    body = call;
    nsAutoString prefix;
    rv = nsSOAPUtils::MakeNamespacePrefixFixed(body, mTargetObjectURI, prefix);
    if (NS_FAILED(rv)) return rv;
    if (!prefix.IsEmpty()) {
      rv = body->SetPrefix(prefix);
      if (NS_FAILED(rv)) return rv;
    }
  }

  PRUint32 count;
  rv = SOAPParameters->Count(&count);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISupports> next;
  nsCOMPtr<nsISOAPParameter> param;
  nsCOMPtr<nsIDOMElement> element;
  nsAutoString name;
  PRBool isHeader;
  for (PRUint32 i = 0; i < count; i++) {
    next = dont_AddRef(SOAPParameters->ElementAt(i));
    param = do_QueryInterface(next);
    if (!param) return NS_ERROR_FAILURE;
    rv = param->GetName(name);
    if (NS_FAILED(rv)) return rv;
    rv = param->GetHeader(&isHeader);
    if (NS_FAILED(rv)) return rv;
 //  Need a callback here to get type?
    rv = mEncodings->Encode(mEncodings, mEncodingStyleURI, param, 
		    nsSOAPUtils::kEmpty, name,
		    nsnull,
		    isHeader ? header : body, nsnull);
    if (NS_FAILED(rv)) return rv;
  }
  return NS_OK;
}

/* nsISupportsArray decodeParameters (); */
NS_IMETHODIMP nsSOAPMessage::DecodeParameters(nsISupportsArray **_retval)
{
  nsresult rv;
  nsCOMPtr<nsIDOMElement> header;
  nsCOMPtr<nsIDOMElement> body;
  nsCOMPtr<nsISupportsArray> array = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
  rv = GetHeader(getter_AddRefs(header));
  if (NS_FAILED(rv)) return rv;
  rv = GetBody(getter_AddRefs(body));
  if (NS_FAILED(rv)) return rv;
  if (!body) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMElement> current;

  if (NS_FAILED(rv)) return rv;
  if (!mMethodName.IsEmpty()) {
    nsSOAPUtils::GetFirstChildElement(body, getter_AddRefs(current));
    body = current;
    if (!body) return NS_ERROR_FAILURE;
    rv = current->GetNamespaceURI(mTargetObjectURI);
    if (NS_FAILED(rv)) return rv;
    rv = current->GetLocalName(mMethodName);
    if (NS_FAILED(rv)) return rv;
  }
  nsCOMPtr<nsIDOMElement> next;
  nsCOMPtr<nsIDOMNamedNodeMap> attrs;
  nsCOMPtr<nsIDOMNode> attr;
  nsCOMPtr<nsISOAPParameter> param;
  nsAutoString encoding;
  nsAutoString namespaceURI;
  PRBool isHeader = header != nsnull;
  if (!isHeader)
    header = body;
  PRUint32 count = 0;
  for (;;) {
    nsSOAPUtils::GetFirstChildElement(header, getter_AddRefs(current));
    while (current) {
      rv = current->GetAttributes(getter_AddRefs(attrs));
      if (NS_FAILED(rv)) return rv;
// Get the encoding
      rv = attrs->GetNamedItemNS(nsSOAPUtils::kSOAPEnvURI, 
		                 nsSOAPUtils::kEncodingStyleAttribute, 
				 getter_AddRefs(attr));
      if (NS_FAILED(rv)) return rv;
      if (attr) {
	attr->GetNodeValue(encoding);
      }
      else {
	encoding = mEncodingStyleURI;
      }
 //  Need a callback here to get type.
      rv = mEncodings->Decode(mEncodings, encoding, current, nsnull, nsnull, getter_AddRefs(param));
      if (NS_FAILED(rv)) return rv;
      if (param) {
	rv = param->SetHeader(isHeader);
        if (NS_FAILED(rv)) return rv;
	rv = array->InsertElementAt(param, count++);
        if (NS_FAILED(rv)) return rv;
      }
      nsSOAPUtils::GetNextSiblingElement(current, getter_AddRefs(next));
      current = next;
    }
    if (isHeader)
      header = body;
    else
      break;
  }
  *_retval = array;
  NS_IF_ADDREF(*_retval);
  return NS_OK;
}

/* readonly attribute unsigned long status; */
NS_IMETHODIMP nsSOAPMessage::GetStatus(PRUint32 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

/* attribute nsISupportsArray protocol; */
NS_IMETHODIMP nsSOAPMessage::GetProtocolParameters(nsISupportsArray * *aProtocol)
{
  NS_ENSURE_ARG_POINTER(aProtocol);
  *aProtocol = mProtocol;
  NS_IF_ADDREF(*aProtocol);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetProtocolParameters(nsISupportsArray * aProtocol)
{
  mProtocol = aProtocol;
  return NS_OK;
}

/* attribute nsISOAPEncodingRegistry types; */
NS_IMETHODIMP nsSOAPMessage::GetEncodings(nsISOAPEncodingRegistry * *aEncodings)
{
  NS_ENSURE_ARG_POINTER(aEncodings);
  *aEncodings = mEncodings;
  NS_IF_ADDREF(*aEncodings);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPMessage::SetEncodings(nsISOAPEncodingRegistry * aEncodings)
{
  mEncodings = aEncodings;
  return NS_OK;
}

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsSOAPMessage
#define XPC_MAP_QUOTED_CLASSNAME   "SOAPMessage"
#define                             XPC_MAP_WANT_NEWRESOLVE
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_NAMED_LITERAL_STRING(encodeparameters, "encodeparameters");
NS_NAMED_LITERAL_STRING(decodeparameters, "decodeparameters");

static JSBool PR_CALLBACK
encodeJSParameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  nsCOMPtr<nsISupports> value;
  nsAutoString type;
  nsresult rc = nsSOAPJSValue::ConvertJSArgsToValue(cx, argc, argv, PR_TRUE, getter_AddRefs(value), type);
  if (NS_FAILED(rc))
    return JS_FALSE;

//  Unwrap "this" pointer to call original function

  nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rc); 
  if (NS_FAILED(rc))
    return JS_FALSE;
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  if (wrapper) {
    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    if (native) {
      nsCOMPtr<nsISOAPMessage> message;
      nsCOMPtr<nsISupportsArray> array;
      message = do_QueryInterface(native);
      if (message) {
        array = do_QueryInterface(value);
        nsresult rc = message->EncodeParameters(array);
        if (!NS_FAILED(rc))
          return JS_TRUE;
      }
    }
  }
  return JS_FALSE;
}

static JSBool PR_CALLBACK
DecodeJSParameters(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval)
{
  nsCOMPtr<nsISupportsArray> value;

//  Unwrap "this" pointer to call original function

  nsresult rc;
  nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rc); 
  if (NS_FAILED(rc))
    return JS_FALSE;
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  rc = NS_ERROR_FAILURE;
  if (wrapper) {
    nsCOMPtr<nsISupports> native;
    wrapper->GetNative(getter_AddRefs(native));
    if (native) {
      nsCOMPtr<nsISOAPMessage> message;
      message = do_QueryInterface(native);
      if (message) {
        rc = message->DecodeParameters(getter_AddRefs(value));
      }
    }
  }
  if (NS_FAILED(rc))
    return JS_FALSE;
  rc = nsSOAPJSValue::ConvertValueToJSVal(cx, value, nsSOAPUtils::kArrayType, rval);
  if (NS_FAILED(rc))
    return JS_FALSE;
  return JS_TRUE;
}

NS_IMETHODIMP
nsSOAPMessage::NewResolve(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
  JSObject *obj, jsval id, PRUint32 flags,
  JSObject **objp, PRBool *_retval)
{
  if (!JSVAL_IS_STRING(id)) {
    return NS_OK;
  }

  JSString *str = JSVAL_TO_STRING(id);
  nsDependentString name(JS_GetStringChars(str));
  if (name.Equals(encodeparameters)) {
    JSFunction *f = ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                                        encodeJSParameters, 0, JSPROP_READONLY);
    if (!f) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

  } else if (name.Equals(decodeparameters)) {
    JSFunction *f = ::JS_DefineFunction(cx, obj, ::JS_GetStringBytes(str),
                                        DecodeJSParameters, 0, JSPROP_READONLY);
    if (!f) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
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
