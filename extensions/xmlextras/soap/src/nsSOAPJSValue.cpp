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
    rv = nsSOAPUtils::ConvertJSValToValue(mContext, val, getter_AddRefs(value), type);
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
  nsresult rv = nsSOAPUtils::ConvertValueToJSVal(mContext, NS_STATIC_CAST(nsISOAPJSValue*, this), type, &val);
  if (NS_FAILED(rv)) return rv;
  return JS_SetProperty(mContext, mObject, NS_ConvertUCS2toUTF8(name).get(), &val);
}
