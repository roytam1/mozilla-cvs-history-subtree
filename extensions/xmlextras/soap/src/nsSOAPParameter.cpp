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

#include "nsReadableUtils.h"
#include "nsSOAPParameter.h"
#include "nsSOAPUtils.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"

nsSOAPParameter::nsSOAPParameter()
{
  NS_INIT_ISUPPORTS();
}

nsSOAPParameter::~nsSOAPParameter()
{
}

NS_IMPL_ISUPPORTS4(nsSOAPParameter, 
                   nsISOAPParameter, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable,
                   nsIJSNativeInitializer)

/* attribute DOMString name; */
NS_IMETHODIMP nsSOAPParameter::GetName(nsAWritableString & aName)
{
  NS_ENSURE_ARG_POINTER(&aName);
  aName.Assign(mName);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetName(const nsAReadableString & aName)
{
  mName.Assign(aName);
  return NS_OK;
}

/* attribute DOMString encodingStyleURI; */
NS_IMETHODIMP nsSOAPParameter::GetEncodingStyleURI(nsAWritableString & aEncodingStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingStyleURI);
  aEncodingStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetEncodingStyleURI(const nsAReadableString & aEncodingStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingStyleURI);
  return NS_OK;
}

/* attribute DOMString type; */
NS_IMETHODIMP nsSOAPParameter::GetType(nsAWritableString & aType)
{
  NS_ENSURE_ARG_POINTER(&aType);
  aType.Assign(mType);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPParameter::SetType(const nsAReadableString & aType)
{
  mType.Assign(aType);
  return NS_OK;
}

/* [noscript] readonly attribute nsISupports value; */
NS_IMETHODIMP nsSOAPParameter::GetValue(nsISupports * *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = mValue;
  NS_IF_ADDREF(*aValue);
  return NS_OK;
}

/* void setAsWString (in DOMString aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsWString(const nsAReadableString & aValue)
{
    mType.Assign(nsSOAPUtils::kWStringType);
    nsCOMPtr<nsISupportsWString> value = do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID);
    value->SetData(ToNewUnicode(aValue));
    mValue = value;
    return NS_OK;
}

/* void setAsBoolean (in PRBool aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsBoolean(PRBool aValue)
{
    mType.Assign(nsSOAPUtils::kPRBoolType);
    nsCOMPtr<nsISupportsPRBool> value = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsDouble (in double aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsDouble(double aValue)
{
    mType.Assign(nsSOAPUtils::kDoubleType);
    nsCOMPtr<nsISupportsDouble> value = do_CreateInstance(NS_SUPPORTS_DOUBLE_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsFloat (in float aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsFloat(float aValue)
{
    mType.Assign(nsSOAPUtils::kFloatType);
    nsCOMPtr<nsISupportsFloat> value = do_CreateInstance(NS_SUPPORTS_FLOAT_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsLong (in PRInt64 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsLong(PRInt64 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt64Type);
    nsCOMPtr<nsISupportsPRInt64> value = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsInt (in PRInt32 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsInt(PRInt32 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt32Type);
    nsCOMPtr<nsISupportsPRInt32> value = do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsShort (in PRInt16 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsShort(PRInt16 aValue)
{
    mType.Assign(nsSOAPUtils::kPRInt16Type);
    nsCOMPtr<nsISupportsPRInt16> value = do_CreateInstance(NS_SUPPORTS_PRINT16_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsByte (in PRUint8 aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsByte(PRUint8 aValue)
{
    mType.Assign(nsSOAPUtils::kCharType);
    nsCOMPtr<nsISupportsChar> value = do_CreateInstance(NS_SUPPORTS_CHAR_CONTRACTID);
    value->SetData(aValue);
    mValue = value;
    return NS_OK;
}

/* void setAsArray (in nsISupportsArray aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsArray(nsISupportsArray *aValue)
{
    mType.Assign(nsSOAPUtils::kArrayType);
    mValue = NS_REINTERPRET_CAST(nsISupports*, aValue);
    return NS_OK;
}

/* void setAsInterface (in nsIIDRef aIID, in nsISupports aValue); */
NS_IMETHODIMP nsSOAPParameter::SetAsInterface(const nsIID & aIID, nsISupports *aValue)
{
    mType.Assign(nsSOAPUtils::kIIDObjectType);
    mType.Append(nsSOAPUtils::kTypeSeparator);
    mValue = aValue;
    return NS_OK;
}

#if 0
/* [noscript] void setValueAndType (in nsISupports value, in long type); */
NS_IMETHODIMP nsSOAPParameter::SetValueAndType(nsISupports *value, PRInt32 type)
{
  mValue = value;
  mType = type;
  mJSValue = nsnull;
  return NS_OK;
}

/* [noscript] void getValueAndType (out nsISupports value, out long type); */
NS_IMETHODIMP nsSOAPParameter::GetValueAndType(nsISupports **value, PRInt32 *type)
{
  NS_ENSURE_ARG_POINTER(value);
  NS_ENSURE_ARG_POINTER(type);

  *value = mValue;
  NS_IF_ADDREF(*value);
  *type = mType;

  return NS_OK;
}

/* attribute nsISupports value; */
// We can't use this attribute for script calls without a variant type
// in xpidl. For now, use nsIXPCScriptable to implement.
NS_IMETHODIMP nsSOAPParameter::GetValue(nsISupports * *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);

  *aValue = mValue;
  NS_IF_ADDREF(*aValue);

  return NS_OK;
}

NS_IMETHODIMP
nsSOAPParameter::SetValue(nsISupports* value)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsSOAPParameter::GetValue(JSContext* aContext,
                                        jsval* aValue)
{
  return nsSOAPUtils::ConvertValueToJSVal(aContext,
                                          mValue,
                                          mJSValue,
                                          mType,
                                          aValue);
}

NS_IMETHODIMP nsSOAPParameter::SetValue(JSContext* aContext,
                                        jsval aValue)
{
  return nsSOAPUtils::ConvertJSValToValue(aContext,
                                          aValue,
                                          getter_AddRefs(mValue),
                                          &mJSValue,
                                          &mType);
}

/* [noscript] readonly attribute JSObjectPtr JSValue; */
NS_IMETHODIMP nsSOAPParameter::GetJSValue(JSObject * *aJSValue)
{
  NS_ENSURE_ARG_POINTER(aJSValue);
  *aJSValue = mJSValue;
  
  return NS_OK;
}

#endif

NS_IMETHODIMP 
nsSOAPParameter::Initialize(JSContext *cx, JSObject *obj, 
                            PRUint32 argc, jsval *argv)
{
  if (argc > 0) {
    JSString* namestr = JS_ValueToString(cx, argv[0]);
    if (namestr) {
      SetName(nsString(NS_REINTERPRET_CAST(PRUnichar*, JS_GetStringChars(namestr))));
      if (argc > 1) {
        SetValue(cx, argv[1]);
      }
    }
  }

  return NS_OK;
}

XPC_IMPLEMENT_IGNORE_CREATE(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_GETFLAGS(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_LOOKUPPROPERTY(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_DEFINEPROPERTY(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_GETATTRIBUTES(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_SETATTRIBUTES(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_DELETEPROPERTY(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_DEFAULTVALUE(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_ENUMERATE(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_CHECKACCESS(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_CALL(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_CONSTRUCT(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_HASINSTANCE(nsSOAPParameter)
XPC_IMPLEMENT_IGNORE_FINALIZE(nsSOAPParameter)

NS_IMETHODIMP 
nsSOAPParameter::GetProperty(JSContext *cx, JSObject *obj,
                             jsid id, jsval *vp,
                             nsIXPConnectWrappedNative* wrapper,
                             nsIXPCScriptable* arbitrary,
                             JSBool* retval)
{
  *retval = JS_TRUE;
  jsval val;
  if (JS_IdToValue(cx, id, &val)) {
    if (JSVAL_IS_STRING(val)) {
      JSString* str = JSVAL_TO_STRING(val);
      char* name = JS_GetStringBytes(str);
      if (nsCRT::strcmp(name, "value") == 0) {
        return GetValue(cx, vp);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsSOAPParameter::SetProperty(JSContext *cx, JSObject *obj, jsid id, 
                             jsval *vp, nsIXPConnectWrappedNative* wrapper,
                             nsIXPCScriptable* arbitrary,
                             JSBool* retval)
{
  *retval = JS_TRUE;
  jsval val;
  if (JS_IdToValue(cx, id, &val)) {
    if (JSVAL_IS_STRING(val)) {
      JSString* str = JSVAL_TO_STRING(val);
      char* name = JS_GetStringBytes(str);
      if (nsCRT::strcmp(name, "value") == 0) {
        return SetValue(cx, *vp);
      }
    }
  }
  return NS_OK;
}


static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPParameter::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPParameter::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPParameter::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPParameter::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPParameter))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
