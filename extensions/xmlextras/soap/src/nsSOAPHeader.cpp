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
#include "nsReadableUtils.h"
#include "nsSOAPHeader.h"
#include "nsSOAPJSValue.h"
#include "nsSOAPUtils.h"
#include "nsIXPConnect.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"

nsSOAPHeader::nsSOAPHeader()
{
  NS_INIT_ISUPPORTS();
}

nsSOAPHeader::~nsSOAPHeader()
{
}

NS_IMPL_ISUPPORTS4(nsSOAPHeader, 
                   nsISOAPHeader, 
                   nsISecurityCheckedComponent,
                   nsIXPCScriptable,
                   nsIJSNativeInitializer)

/* attribute AString name; */
NS_IMETHODIMP nsSOAPHeader::GetActorURI(nsAWritableString & aActorURI)
{
  NS_ENSURE_ARG_POINTER(&aActorURI);
  aActorURI.Assign(mActorURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPHeader::SetActorURI(const nsAReadableString & aActorURI)
{
  mActorURI.Assign(aActorURI);
  return NS_OK;
}

NS_IMETHODIMP nsSOAPHeader::GetParameters(nsISupportsArray * *aParameters)
{
  NS_ENSURE_ARG_POINTER(aParameters);
  *aParameters = mParameters;
  NS_IF_ADDREF(*aParameters);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPHeader::SetParameters(nsISupportsArray *aParameters)
{
    mParameters = aParameters;
    return NS_OK;
}

NS_IMETHODIMP 
nsSOAPHeader::Initialize(JSContext *cx, JSObject *obj, 
                            PRUint32 argc, jsval *argv)
{
  if (argc > 0) {
    JSString* actorstr = JS_ValueToString(cx, argv[0]);
    if (actorstr) {
      SetActorURI(nsString(NS_REINTERPRET_CAST(PRUnichar*, JS_GetStringChars(actorstr))));
    }
    if (argc > 1) {
      nsCOMPtr<nsISupports> parameters;
      nsAutoString type;
      nsresult rc = nsSOAPJSValue::ConvertJSArgsToValue(cx,
                                        argc - 1,
                                        argv + 1,
                                        PR_TRUE,
                                        getter_AddRefs(parameters),
                                        type);
      if (NS_FAILED(rc))
        return rc;
      mParameters = do_QueryInterface(parameters);
      if (mParameters == nsnull)
	return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           nsSOAPHeader
#define XPC_MAP_QUOTED_CLASSNAME   "SOAPHeader"
#define                             XPC_MAP_WANT_SETPROPERTY
#define                             XPC_MAP_WANT_GETPROPERTY
#define XPC_MAP_FLAGS       nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY   | \
                            nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_IMETHODIMP 
nsSOAPHeader::GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    if (NS_LITERAL_STRING("parameters").Equals(name)) {
      return nsSOAPJSValue::ConvertValueToJSVal(cx,
                                        mParameters,
					nsSOAPUtils::kArrayType,
                                        vp);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsSOAPHeader::SetProperty(nsIXPConnectWrappedNative *wrapper, JSContext *cx,
                             JSObject *obj, jsval id, jsval *vp,
                             PRBool *_retval)
{
  if (JSVAL_IS_STRING(id)) {
    JSString* str = JSVAL_TO_STRING(id);
    const PRUnichar* name = NS_REINTERPRET_CAST(const PRUnichar *,
                                                JS_GetStringChars(str));
    if (NS_LITERAL_STRING("parameters").Equals(name)) {
      nsCOMPtr<nsISupports> value;
      nsAutoString type;
      return nsSOAPJSValue::ConvertJSValToValue(cx,
                                        *vp,
                                        getter_AddRefs(value),
                                        type);
    }
  }
  return NS_OK;
}


static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPHeader::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPHeader))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodActorURI); */
NS_IMETHODIMP 
nsSOAPHeader::CanCallMethod(const nsIID * iid, const PRUnichar *methodActorURI, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPHeader))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyActorURI); */
NS_IMETHODIMP 
nsSOAPHeader::CanGetProperty(const nsIID * iid, const PRUnichar *propertyActorURI, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPHeader))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyActorURI); */
NS_IMETHODIMP 
nsSOAPHeader::CanSetProperty(const nsIID * iid, const PRUnichar *propertyActorURI, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPHeader))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
