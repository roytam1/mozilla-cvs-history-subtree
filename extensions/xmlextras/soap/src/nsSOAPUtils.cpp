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
#include "nsIDOMText.h"
#include "nsCOMPtr.h"
#include "nsIJSContextStack.h"
#include "nsISOAPParameter.h"
#include "nsSOAPParameter.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsXPIDLString.h"
#include "nsISupportsArray.h"
#include "nsISOAPJSValue.h"
#include "nsIXPConnect.h"

const nsString nsSOAPUtils::kSOAPEnvURI(NS_LITERAL_STRING("http://schemas.xmlsoap.org/soap/envelope/"));
const nsString nsSOAPUtils::kSOAPEncodingURI(NS_LITERAL_STRING("http://schemas.xmlsoap.org/soap/encoding/"));
const nsString nsSOAPUtils::kSOAPEnvPrefix(NS_LITERAL_STRING("SOAP-ENV"));
const nsString nsSOAPUtils::kSOAPEncodingPrefix(NS_LITERAL_STRING("SOAP-ENC"));
const nsString nsSOAPUtils::kXSIURI(NS_LITERAL_STRING("http://www.w3.org/1999/XMLSchema-instance"));
const nsString nsSOAPUtils::kXSDURI(NS_LITERAL_STRING("http://www.w3.org/1999/XMLSchema"));
const nsString nsSOAPUtils::kXSIPrefix(NS_LITERAL_STRING("xsi"));
const nsString nsSOAPUtils::kXSDPrefix(NS_LITERAL_STRING("xsd"));
const nsString nsSOAPUtils::kEncodingStyleAttribute(NS_LITERAL_STRING("encodingStyle"));
const nsString nsSOAPUtils::kEnvelopeTagName(NS_LITERAL_STRING("Envelope"));
const nsString nsSOAPUtils::kHeaderTagName(NS_LITERAL_STRING("Header"));
const nsString nsSOAPUtils::kBodyTagName(NS_LITERAL_STRING("Body"));
const nsString nsSOAPUtils::kFaultTagName(NS_LITERAL_STRING("Fault"));
const nsString nsSOAPUtils::kFaultCodeTagName(NS_LITERAL_STRING("faultcode"));
const nsString nsSOAPUtils::kFaultStringTagName(NS_LITERAL_STRING("faultstring"));
const nsString nsSOAPUtils::kFaultActorTagName(NS_LITERAL_STRING("faultactor"));
const nsString nsSOAPUtils::kFaultDetailTagName(NS_LITERAL_STRING("detail"));

const nsString nsSOAPUtils::kSOAPCallType(NS_LITERAL_STRING("#nsSOAPUtils::kSOAPCallType"));
const nsString nsSOAPUtils::kEmpty(NS_LITERAL_STRING(""));

const nsString nsSOAPUtils::kWStringType(NS_LITERAL_STRING("#DOMString"));
const nsString nsSOAPUtils::kPRBoolType(NS_LITERAL_STRING("#boolean"));
const nsString nsSOAPUtils::kDoubleType(NS_LITERAL_STRING("#double"));
const nsString nsSOAPUtils::kFloatType(NS_LITERAL_STRING("#float"));
const nsString nsSOAPUtils::kPRInt64Type(NS_LITERAL_STRING("#long"));
const nsString nsSOAPUtils::kPRInt32Type(NS_LITERAL_STRING("#int"));
const nsString nsSOAPUtils::kPRInt16Type(NS_LITERAL_STRING("#short"));
const nsString nsSOAPUtils::kCharType(NS_LITERAL_STRING("#byte"));
const nsString nsSOAPUtils::kArrayType(NS_LITERAL_STRING("#array"));
const nsString nsSOAPUtils::kJSObjectTypePrefix(NS_LITERAL_STRING("#js#"));
const nsString nsSOAPUtils::kTypeSeparator(NS_LITERAL_STRING("#"));
const nsString nsSOAPUtils::kIIDObjectTypePrefix(NS_LITERAL_STRING("#iid#"));
const nsString nsSOAPUtils::kNullType(NS_LITERAL_STRING("#null"));
const nsString nsSOAPUtils::kVoidType(NS_LITERAL_STRING("#void"));

void 
nsSOAPUtils::GetSpecificChildElement(
  nsIDOMElement *aParent, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  GetFirstChildElement(aParent, getter_AddRefs(sibling));
  if (sibling)
  {
    GetSpecificSiblingElement(sibling,
      aNamespace, aType, aElement);
  }
}

void 
nsSOAPUtils::GetSpecificSiblingElement(
  nsIDOMElement *aSibling, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  sibling = aSibling;
  do {
    nsAutoString name, namespaceURI;
    sibling->GetLocalName(name);
    sibling->GetNamespaceURI(namespaceURI);
    if (name.Equals(aType)
      && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
    {
      *aElement = sibling;
      NS_ADDREF(*aElement);
      return;
    }
    nsCOMPtr<nsIDOMElement> temp = sibling;
    GetNextSiblingElement(temp, getter_AddRefs(sibling));
  } while (sibling);
}

void
nsSOAPUtils::GetFirstChildElement(nsIDOMElement* aParent, 
                                  nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  *aElement = nsnull;
  aParent->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      child->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
}

void
nsSOAPUtils::GetNextSiblingElement(nsIDOMElement* aStart, 
                                   nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> sibling;

  *aElement = nsnull;
  GetNextSibling(aStart, getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      sibling->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = sibling;
    GetNextSibling(temp, getter_AddRefs(sibling));
  }
}

void 
nsSOAPUtils::GetElementTextContent(nsIDOMElement* aElement, 
                                   nsAWritableString& aText)
{
  nsCOMPtr<nsIDOMNode> child;
  nsAutoString rtext;
  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::TEXT_NODE == type
        || nsIDOMNode::CDATA_SECTION_NODE == type) {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(child);
      nsAutoString data;
      text->GetData(data);
      rtext.Append(data);
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
  aText.Assign(rtext);
}

PRBool
nsSOAPUtils::HasChildElements(nsIDOMElement* aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      return PR_TRUE;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }

  return PR_FALSE;
}

void
nsSOAPUtils::GetNextSibling(nsIDOMNode* aSibling, nsIDOMNode **aNext)
{
  nsCOMPtr<nsIDOMNode> last;
  nsCOMPtr<nsIDOMNode> current;
  PRUint16 type;

  *aNext = nsnull;
  last = aSibling;

  last->GetNodeType(&type);
  if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
    last->GetFirstChild(getter_AddRefs(current));
    if (!last)
    {
      last->GetNextSibling(getter_AddRefs(current));
    }
  }
  else {
    last->GetNextSibling(getter_AddRefs(current));
  }
  while (!current)
  {
    last->GetParentNode(getter_AddRefs(current));
    current->GetNodeType(&type);
    if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
      last = current;
      last->GetNextSibling(getter_AddRefs(current));
    }
    else {
      current = nsnull;
      break;
    }
  }
  *aNext = current;
  NS_IF_ADDREF(*aNext);
}

void
nsSOAPUtils::GetInheritedEncodingStyle(nsIDOMElement* aEntry, 
                                       nsAWritableString & aEncodingStyle)
{
  nsCOMPtr<nsIDOMNode> node = aEntry;

  while (node) {
    nsAutoString value;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
    if (element) {
      element->GetAttributeNS(nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kEncodingStyleAttribute,
                              value);
      if (value.Length() > 0) {
        aEncodingStyle.Assign(value);
        return;
      }
    }
    nsCOMPtr<nsIDOMNode> temp = node;
    temp->GetParentNode(getter_AddRefs(node));
  }
  aEncodingStyle.Assign(kSOAPEncodingURI);
}

JSContext*
nsSOAPUtils::GetSafeContext()
{
  // Get the "safe" JSContext: our JSContext of last resort
  nsresult rv;
  NS_WITH_SERVICE(nsIJSContextStack, stack, "@mozilla.org/js/xpc/ContextStack;1", 
                  &rv);
  if (NS_FAILED(rv))
    return nsnull;
  nsCOMPtr<nsIThreadJSContextStack> tcs = do_QueryInterface(stack);
  JSContext* cx;
  if (NS_FAILED(tcs->GetSafeJSContext(&cx))) {
    return nsnull;
  }
  return cx;
}
 
JSContext*
nsSOAPUtils::GetCurrentContext()
{
  // Get JSContext from stack.
  nsresult rv;
  NS_WITH_SERVICE(nsIJSContextStack, stack, "@mozilla.org/js/xpc/ContextStack;1", 
                  &rv);
  if (NS_FAILED(rv))
    return nsnull;
  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)))
    return nsnull;
  return cx;
}

nsresult 
nsSOAPUtils::ConvertValueToJSVal(JSContext* aContext, 
                                 nsISupports* aValue, 
                                 nsAReadableString & aType,
                                 jsval* vp)
{

  *vp = JSVAL_NULL;

  if (aType.Equals(kVoidType)) {

      *vp = JSVAL_VOID;

  } else if (aType.Equals(kWStringType)) {

      nsCOMPtr<nsISupportsWString> wstr = do_QueryInterface(aValue);
      if (!wstr) return NS_ERROR_FAILURE;
      
      nsXPIDLString data;
      wstr->GetData(getter_Copies(data));
      
      if (data) {
        JSString* jsstr = JS_NewUCStringCopyZ(aContext,
                                             NS_REINTERPRET_CAST(const jschar*, (const PRUnichar*)data));
        if (jsstr) {
          *vp = STRING_TO_JSVAL(jsstr);
        }
      }

  } else if (aType.Equals(kPRBoolType)) {

      nsCOMPtr<nsISupportsPRBool> prb = do_QueryInterface(aValue);
      if (!prb) return NS_ERROR_FAILURE;

      PRBool data;
      prb->GetData(&data);

      if (data) {
        *vp = JSVAL_TRUE;
      }
      else {
        *vp = JSVAL_FALSE;
      }

  } else if (aType.Equals(kDoubleType)) {

      nsCOMPtr<nsISupportsDouble> dub = do_QueryInterface(aValue);
      if (!dub) return NS_ERROR_FAILURE;

      double data;
      dub->GetData(&data);

      double* dataPtr = JS_NewDouble(aContext, (jsdouble)data); 
      *vp = DOUBLE_TO_JSVAL(dataPtr);

  } else if (aType.Equals(kFloatType)) {

      nsCOMPtr<nsISupportsFloat> flt = do_QueryInterface(aValue);
      if (!flt) return NS_ERROR_FAILURE;

      float data;
      flt->GetData(&data);

      double* dataPtr = JS_NewDouble(aContext, (jsdouble)data); 
      *vp = DOUBLE_TO_JSVAL(dataPtr);

  } else if (aType.Equals(kPRInt64Type)) {

      // XXX How to express 64-bit values in JavaScript?
      return NS_ERROR_NOT_IMPLEMENTED;

  } else if (aType.Equals(kPRInt32Type)) {

      nsCOMPtr<nsISupportsPRInt32> isupint32 = do_QueryInterface(aValue);
      if (!isupint32) return NS_ERROR_FAILURE;

      PRInt32 data;
      isupint32->GetData(&data);
      
      *vp = INT_TO_JSVAL(data);
      
  } else if (aType.Equals(kPRInt16Type)) {

      nsCOMPtr<nsISupportsPRInt16> isupint16 = do_QueryInterface(aValue);
      if (!isupint16) return NS_ERROR_FAILURE;

      PRInt16 data;
      isupint16->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);
      
  } else if (aType.Equals(kCharType)) {

      nsCOMPtr<nsISupportsChar> isupchar = do_QueryInterface(aValue);
      if (!isupchar) return NS_ERROR_FAILURE;

      char data;
      isupchar->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);

  } else if (aType.Equals(kArrayType)) {

      nsCOMPtr<nsISupportsArray> array = do_QueryInterface(aValue);
      if (!array) return NS_ERROR_FAILURE;

      JSObject* arrayobj = JS_NewArrayObject(aContext, 0, nsnull);
      if (!arrayobj) return NS_ERROR_FAILURE;

      PRUint32 index, count;
      array->Count(&count);

      for (index = 0; index < count; index++) {
        nsCOMPtr<nsISupports> isup = getter_AddRefs(array->ElementAt(index));
        nsCOMPtr<nsISOAPParameter> param = do_QueryInterface(isup);
        if (!param) return NS_ERROR_FAILURE;

        nsCOMPtr<nsISupports> paramVal;
        nsAutoString paramType;

        param->GetType(paramType);
        param->GetValue(getter_AddRefs(paramVal));
        jsval val;
        nsresult rv = ConvertValueToJSVal(aContext, paramVal,
                                          paramType, &val);
        if (NS_FAILED(rv)) return rv;

        JS_SetElement(aContext, arrayobj, (jsint)index, &val);
      }

      *vp = OBJECT_TO_JSVAL(arrayobj);

  } else if (nsAutoString(aType).RFind(kJSObjectTypePrefix, false, 0) >= 0) {
      
      nsCOMPtr<nsISOAPJSValue> jsvalue = do_QueryInterface(aValue);
      if (!jsvalue) return NS_ERROR_FAILURE;

      JSObject* data;
      jsvalue->GetValue(&data);
      
      *vp = OBJECT_TO_JSVAL(data);
  }
  return NS_OK;
}

nsresult 
nsSOAPUtils::ConvertJSValToValue(JSContext* aContext,
                                 jsval val, 
                                 nsISupports** aValue,
                                 nsAWritableString & aType)
{
  *aValue = nsnull;
  if (JSVAL_IS_NULL(val)) {
    aType = kNullType;
  }
  else if (JSVAL_IS_VOID(val)) {
    aType = kVoidType;
  }
  else if (JSVAL_IS_STRING(val)) {
    JSString* jsstr;
    aType = kWStringType;
    
    jsstr = JSVAL_TO_STRING(val);
    if (jsstr) {
      nsCOMPtr<nsISupportsWString> wstr = do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID);
      if (!wstr) return NS_ERROR_FAILURE;

      PRUnichar* data = NS_REINTERPRET_CAST(PRUnichar*, 
                                            JS_GetStringChars(jsstr));
      if (data) {
        wstr->SetData(data);
      }
      *aValue = wstr;
      NS_ADDREF(*aValue);
    }
  }
  else if (JSVAL_IS_DOUBLE(val)) {
    aType = kDoubleType;
    
    nsCOMPtr<nsISupportsDouble> dub = do_CreateInstance(NS_SUPPORTS_DOUBLE_CONTRACTID);
    if (!dub) return NS_ERROR_FAILURE;

    dub->SetData((double)(*JSVAL_TO_DOUBLE(val)));
    *aValue = dub;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_INT(val)) {
    aType = kPRInt32Type;
    
    nsCOMPtr<nsISupportsPRInt32> isupint = do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
    if (!isupint) return NS_ERROR_FAILURE;
    
    isupint->SetData((PRInt32)JSVAL_TO_INT(val));
    *aValue = isupint;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_BOOLEAN(val)) {
    aType = kPRBoolType;
    
    nsCOMPtr<nsISupportsPRBool> isupbool = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
    if (!isupbool) return NS_ERROR_FAILURE;

    isupbool->SetData((PRBool)JSVAL_TO_BOOLEAN(val));
    *aValue = isupbool;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_OBJECT(val)) {
    JSObject* jsobj = JSVAL_TO_OBJECT(val);
    nsresult rc;
    if (JS_IsArrayObject(aContext, jsobj)) {
      aType = kArrayType;

      NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rc);
      if (NS_FAILED(rc)) return NS_ERROR_FAILURE;

      PRUint32 count, index;
      count = JS_GetArrayLength(aContext, jsobj, &count);

  //  Is there a way to preallocate the size?
      nsCOMPtr<nsISupportsArray> value = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rc);
      if (!value) return NS_ERROR_FAILURE;

      for (index = 0; index < count; index++) {

        nsCOMPtr<nsISOAPParameter> param;
        param = nsnull;
        jsval val;
        if (!JS_GetElement(aContext, jsobj, (jsint)index, &val))
          return NS_ERROR_FAILURE;

        // First check if it's a parameter
        if (JSVAL_IS_OBJECT(val)) {
          JSObject* paramobj;
          paramobj = JSVAL_TO_OBJECT(val);
    
          // Check if it's a wrapped native
          nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
          xpc->GetWrappedNativeOfJSObject(aContext, paramobj, getter_AddRefs(wrapper));
          
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
          param = new nsSOAPParameter();
          if (!param) return NS_ERROR_OUT_OF_MEMORY;
          nsCOMPtr<nsISupports> xpcval;
          nsAutoString type;
          
          rc = ConvertJSValToValue(aContext, val, getter_AddRefs(xpcval), type);
          if (NS_FAILED(rc)) 
            return rc;

          param->SetAsInterface(NS_GET_IID(nsISOAPJSValue), xpcval);
          param->SetType(type);
        }
        value->InsertElementAt(param, index);
      }

      *aValue = value;
      NS_ADDREF(*aValue);
    }
    else {
      aType = kJSObjectTypePrefix;
      nsCOMPtr<nsISOAPJSValue> value = do_CreateInstance(NS_SOAPJSVALUE_CONTRACTID, &rc);
      if (NS_FAILED(rc)) return rc;

      value->SetValue(jsobj);
      *aValue = value;
      NS_ADDREF(*aValue);
    }
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

