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

#include "nsSOAPParameter.h"
#include "nsSOAPJSValue.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsISupportsArray.h"
#include "nsISOAPJSValue.h"

NS_IMPL_ISUPPORTS2(nsSOAPJSValue, nsISOAPJSValue, nsISOAPStruct)

nsSOAPJSValue::nsSOAPJSValue()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsSOAPJSValue::~nsSOAPJSValue()
{
  /* destructor code */
  if (mContext)
    JS_RemoveRoot(mContext, mObject);
}

/* attribute JSContextPtr ctx; */
NS_IMETHODIMP nsSOAPJSValue::GetContext(JSContext * *aContext)
{
  *aContext = mContext;
  return NS_OK;
}

NS_IMETHODIMP nsSOAPJSValue::SetContext(JSContext * aContext)
{
  mObject = nsnull;
  if (mContext)
    JS_RemoveRoot(mContext, mObject);
  mContext = aContext;
  if (mContext)
    JS_AddRoot(mContext, mObject);
  return NS_OK;
}

/* attribute JSObjectPtr value; */
NS_IMETHODIMP nsSOAPJSValue::GetObject(JSObject * *aObject)
{
  if (mObject) {
    *aObject = mObject;
    return NS_OK;
  }
  else {
    *aObject = nsnull;       //  None of these should exist with null
    return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP nsSOAPJSValue::SetObject(JSObject * aObject)
{
  if (!mContext)
    return NS_ERROR_FAILURE;
  mObject = aObject;
  return NS_OK;
}

class nsSOAPJSStructEnumerator : public nsIEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsIEnumerator methods:
    NS_DECL_NSIENUMERATOR

    // nsSOAPJSStructEnumerator methods:

    nsSOAPJSStructEnumerator(JSContext* ctx, JSObject* obj);
    virtual ~nsSOAPJSStructEnumerator();

protected:
  nsCOMPtr<nsISOAPJSValue> mValue;
  nsISOAPStruct* mStruct; // Convenience non-referenced pointer to same value
  JSIdArray *mMembers;
  PRInt32 mCurrent;
};

nsSOAPJSStructEnumerator::nsSOAPJSStructEnumerator(JSContext* ctx, JSObject* obj)
  : mMembers(JS_Enumerate(ctx, obj)), mCurrent(0)
{
  NS_INIT_REFCNT();
  nsSOAPJSValue* value = new nsSOAPJSValue();
  mValue = value;
  mStruct = value;
  value->SetContext(ctx);
  value->SetObject(obj);
}

nsSOAPJSStructEnumerator::~nsSOAPJSStructEnumerator()
{
  if (mMembers) {
    JSContext* context;
    mValue->GetContext(&context);
    JS_DestroyIdArray(context, mMembers);
  }
}

NS_IMPL_ISUPPORTS1(nsSOAPJSStructEnumerator, nsIEnumerator)

NS_IMETHODIMP nsSOAPJSStructEnumerator::First(void)
{
  mCurrent = -1;
  return Next();
}

NS_IMETHODIMP nsSOAPJSStructEnumerator::Next(void)
{
  if (mCurrent >= mMembers->length)
      return NS_ERROR_FAILURE;
  for (;;) {
    mCurrent++;
    if (mCurrent >= mMembers->length)
      return NS_ERROR_FAILURE;
    JSContext* context;
    mValue->GetContext(&context);
    jsval idval;
    if (!JS_IdToValue(context, mMembers->vector[mCurrent], &idval))
      continue;
    JSString* str = JS_ValueToString(context, idval);
    if (str) return NS_OK;
  }
}

NS_IMETHODIMP nsSOAPJSStructEnumerator::CurrentItem(nsISupports **aItem)
{
  *aItem = nsnull;
  if (mCurrent < mMembers->length) {
    jsval idval;
    JSContext* context;
    mValue->GetContext(&context);
    JSObject* object;
    mValue->GetObject(&object);
    if (!JS_IdToValue(context, mMembers->vector[mCurrent], &idval))
      return NS_ERROR_FAILURE;
    JSString* str = JS_ValueToString(context, idval);       
    if (str) {
      nsCOMPtr<nsISOAPParameter> param;
      nsresult rc = mStruct->GetMember(nsLiteralString(JS_GetStringChars(str)), getter_AddRefs(param));
      *aItem = param;
      NS_IF_ADDREF(*aItem);
      return rc;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsSOAPJSStructEnumerator::IsDone(void)
{
  return mCurrent >= mMembers->length ? NS_OK : NS_ERROR_FAILURE;
}

/* readonly attribute nsIEnumerator members; */
NS_IMETHODIMP nsSOAPJSValue::GetMembers(nsIEnumerator * *aMembers)
{
  if (mContext == 0
    || mObject == 0) {
    *aMembers = 0;
    return NS_ERROR_FAILURE;
  }
  *aMembers = new nsSOAPJSStructEnumerator(mContext, mObject);
  if (aMembers) {
    NS_ADDREF(*aMembers);
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

/* void setMember (in nsISOAPParameter aMember); */
NS_IMETHODIMP nsSOAPJSValue::GetMember(const nsAReadableString& name, nsISOAPParameter* *aMember)
{
  jsval val;
  if (JS_GetProperty(mContext, mObject, NS_ConvertUCS2toUTF8(name).get(), &val)) {
    nsCOMPtr<nsISOAPParameter> newparam = new nsSOAPParameter();
    if (!newparam) return NS_ERROR_OUT_OF_MEMORY;
    
    nsresult rv;
    nsAutoString type;
    nsCOMPtr<nsISupports>value;
    rv = nsSOAPJSValue::ConvertJSValToValue(mContext, val, getter_AddRefs(value), type);
    if (NS_FAILED(rv)) return rv;
    rv = newparam->SetAsInterface(NS_GET_IID(nsISOAPJSValue), value);
    if (NS_FAILED(rv)) return rv;
    rv = newparam->SetType(type);
    if (NS_FAILED(rv)) return rv;
    rv = newparam->SetName(name);
    if (NS_FAILED(rv)) return rv;
    *aMember = newparam;
    NS_ADDREF(*aMember);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

/* void setMember (in nsISOAPParameter member); */
NS_IMETHODIMP nsSOAPJSValue::SetMember(nsISOAPParameter *member)
{
  nsAutoString name;
  member->GetName(name);
  nsAutoString type;
  member->GetName(type);
  jsval val;
  nsresult rv = nsSOAPJSValue::ConvertValueToJSVal(mContext, NS_STATIC_CAST(nsISOAPJSValue*, this), type, &val);
  if (NS_FAILED(rv)) return rv;
  return JS_SetProperty(mContext, mObject, NS_ConvertUCS2toUTF8(name).get(), &val);
}

JSContext*
nsSOAPJSValue::GetSafeContext()
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
nsSOAPJSValue::GetCurrentContext()
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
nsSOAPJSValue::ConvertValueToJSVal(JSContext* aContext, 
                                 nsISupports* aValue, 
                                 const nsAReadableString & aType,
                                 jsval* vp)
{

  *vp = JSVAL_NULL;

  if (aType.Equals(nsSOAPUtils::kVoidType)) {

      *vp = JSVAL_VOID;

  } else if (aType.Equals(nsSOAPUtils::kWStringType)) {

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

  } else if (aType.Equals(nsSOAPUtils::kPRBoolType)) {

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

  } else if (aType.Equals(nsSOAPUtils::kDoubleType)) {

      nsCOMPtr<nsISupportsDouble> dub = do_QueryInterface(aValue);
      if (!dub) return NS_ERROR_FAILURE;

      double data;
      dub->GetData(&data);

      double* dataPtr = JS_NewDouble(aContext, (jsdouble)data); 
      *vp = DOUBLE_TO_JSVAL(dataPtr);

  } else if (aType.Equals(nsSOAPUtils::kFloatType)) {

      nsCOMPtr<nsISupportsFloat> flt = do_QueryInterface(aValue);
      if (!flt) return NS_ERROR_FAILURE;

      float data;
      flt->GetData(&data);

      double* dataPtr = JS_NewDouble(aContext, (jsdouble)data); 
      *vp = DOUBLE_TO_JSVAL(dataPtr);

  } else if (aType.Equals(nsSOAPUtils::kPRInt64Type)) {

      // XXX How to express 64-bit values in JavaScript?
      return NS_ERROR_NOT_IMPLEMENTED;

  } else if (aType.Equals(nsSOAPUtils::kPRInt32Type)) {

      nsCOMPtr<nsISupportsPRInt32> isupint32 = do_QueryInterface(aValue);
      if (!isupint32) return NS_ERROR_FAILURE;

      PRInt32 data;
      isupint32->GetData(&data);
      
      *vp = INT_TO_JSVAL(data);
      
  } else if (aType.Equals(nsSOAPUtils::kPRInt16Type)) {

      nsCOMPtr<nsISupportsPRInt16> isupint16 = do_QueryInterface(aValue);
      if (!isupint16) return NS_ERROR_FAILURE;

      PRInt16 data;
      isupint16->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);
      
  } else if (aType.Equals(nsSOAPUtils::kCharType)) {

      nsCOMPtr<nsISupportsChar> isupchar = do_QueryInterface(aValue);
      if (!isupchar) return NS_ERROR_FAILURE;

      char data;
      isupchar->GetData(&data);
      
      *vp = INT_TO_JSVAL((PRInt32)data);

  } else if (aType.Equals(nsSOAPUtils::kArrayType)) {

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

  } else if (nsAutoString(aType).RFind(nsSOAPUtils::kJSObjectTypePrefix, false, 0) >= 0) {
      
      nsCOMPtr<nsISOAPJSValue> jsvalue = do_QueryInterface(aValue);
      if (!jsvalue) return NS_ERROR_FAILURE;

      JSObject* data;
      jsvalue->GetObject(&data);
      
      *vp = OBJECT_TO_JSVAL(data);
  }
  return NS_OK;
}

nsresult 
nsSOAPJSValue::ConvertJSValToValue(JSContext* aContext,
                                 jsval val, 
                                 nsISupports** aValue,
                                 nsAWritableString & aType)
{
  *aValue = nsnull;
  if (JSVAL_IS_NULL(val)) {
    aType = nsSOAPUtils::kNullType;
  }
  else if (JSVAL_IS_VOID(val)) {
    aType = nsSOAPUtils::kVoidType;
  }
  else if (JSVAL_IS_STRING(val)) {
    JSString* jsstr;
    aType = nsSOAPUtils::kWStringType;
    
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
    aType = nsSOAPUtils::kDoubleType;
    
    nsCOMPtr<nsISupportsDouble> dub = do_CreateInstance(NS_SUPPORTS_DOUBLE_CONTRACTID);
    if (!dub) return NS_ERROR_FAILURE;

    dub->SetData((double)(*JSVAL_TO_DOUBLE(val)));
    *aValue = dub;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_INT(val)) {
    aType = nsSOAPUtils::kPRInt32Type;
    
    nsCOMPtr<nsISupportsPRInt32> isupint = do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
    if (!isupint) return NS_ERROR_FAILURE;
    
    isupint->SetData((PRInt32)JSVAL_TO_INT(val));
    *aValue = isupint;
    NS_ADDREF(*aValue);
  }
  else if (JSVAL_IS_BOOLEAN(val)) {
    aType = nsSOAPUtils::kPRBoolType;
    
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
      aType = nsSOAPUtils::kArrayType;

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
      nsCOMPtr<nsISOAPJSValue> value = do_CreateInstance(NS_SOAPJSVALUE_CONTRACTID, &rc);
      if (NS_FAILED(rc)) return rc;

// Look for a constructor name on the current object or a prototype

      for (;;) {
        JSObject* constructor = JS_GetConstructor(aContext, jsobj);
        jsobj = JS_GetPrototype(aContext, jsobj);
        jsval cname;
        if (constructor
          && JS_GetProperty(aContext, constructor, "name", &cname)
          && JSVAL_IS_STRING(cname))
        {
          JSString* jsstr = JSVAL_TO_STRING(cname);
          if (jsstr) {
            PRUnichar* data = NS_REINTERPRET_CAST(PRUnichar*, 
                                            JS_GetStringChars(jsstr));
            if (data) {
              aType = nsSOAPUtils::kJSObjectTypePrefix;
              aType.Append(data);
              jsobj = nsnull;
              break;
            }
          }
        }
        if (!jsobj) {
          aType = nsSOAPUtils::kJSObjectTypePrefix;
          break;
        }
      }

      *aValue = value;
      NS_ADDREF(*aValue);
    }
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

nsresult 
nsSOAPJSValue::ConvertJSArgsToValue(JSContext* aContext,
                                 PRUint32 argc,
                                 jsval* argv,
                                 PRBool list,
                                 nsISupports** aValue,
                                 nsAWritableString & aType)
{
  if (argc > 1
    || list) {
    nsresult rc;
    nsCOMPtr<nsISupportsArray> array = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID, &rc);
    if (NS_FAILED(rc))
      return rc;
    nsAutoString type;
    nsCOMPtr<nsISupports> value;
    PRUint32 count = argc;
    while (argc--) {
      rc = ConvertJSValToValue(aContext, *(argv++), 
        getter_AddRefs(value), type);
      if (NS_FAILED(rc))
        return rc;
      if (count == 1 && aType.Equals(nsSOAPUtils::kArrayType)) {
        *aValue = value;
        NS_IF_ADDREF(*aValue);
        aType = type;
        return NS_OK;
      }
      nsCOMPtr<nsISOAPParameter> newparam = new nsSOAPParameter();
      if (!newparam) return NS_ERROR_OUT_OF_MEMORY;
      newparam->SetAsInterface(NS_GET_IID(nsISOAPJSValue), newparam);
      newparam->SetType(type);
      array->InsertElementAt(newparam, count - argc - 1);
    }
    aType = nsSOAPUtils::kArrayType;
    *aValue = array;
    NS_IF_ADDREF(*aValue);
    return NS_OK;
  }
  else if (argc == 1) {
    return ConvertJSValToValue(aContext, argv[1], aValue, aType);
  }
  return NS_ERROR_FAILURE;
}
