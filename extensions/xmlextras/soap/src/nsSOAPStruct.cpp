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

#include "nsString.h"
#include "nsSOAPStruct.h"
#include "nsSOAPUtils.h"
#include "nsSupportsArray.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsISOAPParameter.h"
#include "nsSOAPJSValue.h"
#include "jsapi.h"

NS_IMPL_ISUPPORTS3(nsSOAPStruct, nsISOAPStruct, nsIXPCScriptable, nsISecurityCheckedComponent)

nsSOAPStruct::nsSOAPStruct(): mMembers(new nsSupportsHashtable)
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
}

nsSOAPStruct::~nsSOAPStruct()
{
  /* destructor code */
  delete mMembers;
}

class nsSOAPStructEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsISimpleEnumerator methods:
    NS_DECL_NSISIMPLEENUMERATOR

    // nsSOAPStructEnumerator methods:

    nsSOAPStructEnumerator(nsSOAPStruct* aStruct);
    virtual ~nsSOAPStructEnumerator();

protected:
  nsCOMPtr<nsSupportsArray> mMembers;
  PRUint32 mCurrent;
};

PRBool StructEnumFunc(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsISupportsArray* members = NS_STATIC_CAST(nsISupportsArray*,aClosure);
  nsISOAPParameter* parameter = NS_STATIC_CAST(nsISOAPParameter*,aData);
  members->AppendElement(parameter);
  return PR_TRUE;
}

nsSOAPStructEnumerator::nsSOAPStructEnumerator(nsSOAPStruct* aStruct)
  : mMembers(new nsSupportsArray()), mCurrent(0)
{
  NS_INIT_REFCNT();
  aStruct->mMembers->Enumerate(&StructEnumFunc, mMembers);
}

nsSOAPStructEnumerator::~nsSOAPStructEnumerator()
{
}

NS_IMPL_ISUPPORTS1(nsSOAPStructEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP nsSOAPStructEnumerator::GetNext(nsISupports** aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  PRUint32 count;
  mMembers->Count(&count);
  if (mCurrent < count) {
      *aItem = mMembers->ElementAt(mCurrent++);
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsSOAPStructEnumerator::HasMoreElements(PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  PRUint32 count;
  mMembers->Count(&count);
  *aResult = mCurrent < count;
  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator members; */
NS_IMETHODIMP nsSOAPStruct::GetMembers(nsISimpleEnumerator * *aMembers)
{
  NS_ENSURE_ARG_POINTER(aMembers);
  *aMembers = new nsSOAPStructEnumerator(this);
  if (aMembers) {
    NS_ADDREF(*aMembers);
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsSOAPStruct::GetMember(const nsAReadableString& aName, nsISOAPParameter* *aMember)
{
  NS_ENSURE_ARG_POINTER(aMember);
  nsStringKey nameKey(aName);
  *aMember = NS_STATIC_CAST(nsISOAPParameter*, mMembers->Get(&nameKey));
  return NS_OK;
}

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsSOAPStruct
#define XPC_MAP_QUOTED_CLASSNAME   "SOAPStruct"
#define                             XPC_MAP_WANT_SETPROPERTY
#define                             XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_IMETHODIMP 
nsSOAPStruct::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    nsDependentString namestr(name);
    nsStringKey nameKey(namestr);
    nsCOMPtr<nsISOAPParameter> parameter = dont_AddRef(NS_STATIC_CAST(nsISOAPParameter*, mMembers->Get(&nameKey)));
    if (parameter == nsnull)
      return NS_OK;
    nsAutoString type;
    parameter->GetType(type);
    nsCOMPtr<nsISupports> value;
    parameter->GetValue(getter_AddRefs(value));
    return nsSOAPJSValue::ConvertValueToJSVal(cx,
                                        value,
                                        type,
                                        vp);
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsSOAPStruct::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    nsDependentString namestr(name);
    nsStringKey nameKey(namestr);
    nsCOMPtr<nsISupports> value;
    nsAutoString type;
    nsresult rc = nsSOAPJSValue::ConvertJSValToValue(cx,
                                        *vp,
                                        getter_AddRefs(value),
                                        type);
    if (NS_FAILED(rc))
	return rc;
    nsCOMPtr<nsISOAPParameter> parameter = do_CreateInstance(NS_SOAPPARAMETER_CONTRACTID);
    parameter->SetName(namestr);
    parameter->SetValue(value);
    parameter->SetType(type);
    mMembers->Put(&nameKey, parameter);
  }
  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPStruct::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPStruct::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPStruct::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPStruct::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
