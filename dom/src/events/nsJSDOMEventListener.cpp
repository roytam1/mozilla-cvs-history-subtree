/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsJSDOMEventListener.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsIDOMMutationEvent.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptGlobalObject.h"
#include "nsJSUtils.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

static NS_DEFINE_IID(kIScriptEventListenerIID, NS_ISCRIPTEVENTLISTENER_IID);
static NS_DEFINE_IID(kIDOMEventListenerIID, NS_IDOMEVENTLISTENER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

/*
 * nsJSDOMEventListener implementation
 */
nsJSDOMEventListener::nsJSDOMEventListener(nsIScriptContext* aContext,
                                           JSObject *aTarget,
                                           JSObject *aHandler)
  : mContext(aContext),
    mTarget(aTarget),
    mHandler(aHandler)
{
  NS_INIT_REFCNT();

  JSContext *cx = (JSContext *)aContext->GetNativeContext();

  ::JS_AddNamedRoot(cx, &mHandler, "nsJSDOMEventListener.mHandler");
}

nsJSDOMEventListener::~nsJSDOMEventListener() 
{
  JSContext *cx = (JSContext *)mContext->GetNativeContext();

  ::JS_RemoveRoot(cx, &mHandler);
}

nsresult
nsJSDOMEventListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (! aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIDOMEventListenerIID)) {
    *aInstancePtr = (void*)(nsIDOMEventListener*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kIScriptEventListenerIID)) {
    *aInstancePtr = (void*)(nsIScriptEventListener*)this;
    AddRef();
    return NS_OK;
  }
  if (aIID.Equals(kISupportsIID)) {
    *aInstancePtr = (void*)(nsISupports*)(nsIDOMEventListener*)this;
    AddRef();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsJSDOMEventListener)

NS_IMPL_RELEASE(nsJSDOMEventListener)

NS_IMETHODIMP
nsJSDOMEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  JSContext *cx = (JSContext *)mContext->GetNativeContext();

  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  rv = xpc->WrapNative(cx, ::JS_GetGlobalObject(cx), aEvent,
                       NS_GET_IID(nsIDOMEvent), getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* e = nsnull; // XPConnect-wrapped property value.

  rv = holder->GetJSObject(&e);
  NS_ENSURE_SUCCESS(rv, rv);

  jsval argv[1];
  argv[0] = OBJECT_TO_JSVAL(e);
  PRBool jsBoolResult;

  rv = mContext->CallEventHandler(mTarget, mHandler, 1, argv, &jsBoolResult,
                                  PR_FALSE);

  if (NS_FAILED(rv)) {
    return rv;
  }

  return jsBoolResult ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJSDOMEventListener::CheckIfEqual(nsIScriptEventListener *aListener,
                                   PRBool *aResult)
{
  *aResult = PR_FALSE;

  JSObject *otherTarget, *otherHandler;

  if (NS_SUCCEEDED(aListener->GetInternals((void**)&otherTarget, (void**)&otherHandler))) {
    if (otherTarget == mTarget && otherHandler == mHandler) {
      *aResult = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsJSDOMEventListener::GetInternals(void** aTarget, void** aHandler)
{
  NS_ENSURE_ARG_POINTER(aTarget);
  NS_ENSURE_ARG_POINTER(aHandler);

  *aTarget = (void*)mTarget;
  *aHandler = (void*)mHandler;
  return NS_OK;
}

/*
 * Factory functions
 */

extern nsresult
NS_NewScriptEventListener(nsIDOMEventListener ** aInstancePtrResult,
                          nsIScriptContext *aContext,
                          void* aTarget,
                          void *aHandler)
{
  NS_PRECONDITION(aContext != nsnull, "null ptr");
  if (!aContext)
    return NS_ERROR_NULL_POINTER;

  nsJSDOMEventListener* it =
    new nsJSDOMEventListener(aContext,
                             NS_REINTERPRET_CAST(JSObject*, aTarget),
                             NS_REINTERPRET_CAST(JSObject*, aHandler));

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kIDOMEventListenerIID,
                            (void **)aInstancePtrResult);   
}

