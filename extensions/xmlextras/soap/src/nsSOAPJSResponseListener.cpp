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

#include "nsSOAPCall.h"
#include "nsSOAPResponse.h"
#include "nsSOAPParameter.h"
#include "nsSOAPUtils.h"
#include "nsCRT.h"
#include "jsapi.h"
#include "nsIDOMParser.h"
#include "nsISOAPParameter.h"
#include "nsISOAPTransport.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIURI.h"
#include "nsNetUtil.h"

static NS_DEFINE_CID(kDOMParserCID, NS_DOMPARSER_CID);

/////////////////////////////////////////////
// This class must be made very simple to construct by JS
//   from a function.  There is a CID and CONTRACTID.
//
/////////////////////////////////////////////

class nsSOAPJSResponseListener : public nsISOAPResponseListener 
{
public:
  nsSOAPJSResponseListener(JSObject* aScopeObj, JSObject* aFunctionObj);
  virtual ~nsSOAPJSResponseListener();
  
  NS_DECL_ISUPPORTS

  // nsISOAPResponseListener
  NS_DECL_NSISOAPRESPONSELISTENER

protected:  
  JSObject* mScopeObj;
  JSObject* mFunctionObj;
};

nsSOAPJSResponseListener::nsSOAPJSResponseListener(JSObject* aScopeObj,
                                                   JSObject* aFunctionObj)
{
  NS_INIT_ISUPPORTS();
  // We don't have to add a GC root for the scope object
  // since we'll go away if it goes away
  mScopeObj = aScopeObj;
  mFunctionObj = aFunctionObj;
  JSContext* cx;
  cx = nsSOAPUtils::GetSafeContext();
  if (cx) {
    JS_AddNamedRoot(cx, &mFunctionObj, "nsSOAPCall");
  }
}

nsSOAPJSResponseListener::~nsSOAPJSResponseListener()
{
  JSContext* cx;
  cx = nsSOAPUtils::GetSafeContext();
  if (cx) {
    JS_RemoveRoot(cx, &mFunctionObj);
  }
}

NS_IMPL_ISUPPORTS1(nsSOAPJSResponseListener,
                   nsISOAPResponseListener)

NS_IMETHODIMP
nsSOAPJSResponseListener::HandleResponse(nsISOAPResponse* aResponse,
                                         PRBool aLast,
                                         PRBool *aCease)
{
  nsresult rv;
  JSContext* cx;
  cx = nsSOAPUtils::GetCurrentContext();
  if (!cx) {
    cx = nsSOAPUtils::GetSafeContext();
  }
  if (cx) {
    nsCOMPtr<nsIXPConnect> xpc =
      do_GetService(nsIXPConnect::GetCID()); 
    if (!xpc) return NS_OK;

    jsval params[2];
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    JSObject* obj;

    // Get the JSObject wrapper for the response
    rv = xpc->WrapNative(cx, mScopeObj,
                         aResponse, NS_GET_IID(nsISOAPResponse),
                         getter_AddRefs(holder));
    if (NS_FAILED(rv)) return NS_OK;

    rv = holder->GetJSObject(&obj);
    if (!obj) return NS_OK;

    params[0] = OBJECT_TO_JSVAL(obj);

    params[1] = BOOLEAN_TO_JSVAL(aLast);

    jsval val;
    rv = JS_CallFunctionValue(cx, mScopeObj, OBJECT_TO_JSVAL(mFunctionObj),
                         2, params, &val);
    *aCease = NS_FAILED(rv) || !JSVAL_IS_BOOLEAN(val) || JSVAL_TO_BOOLEAN(val);
  }
  else {
    *aCease = PR_TRUE;
  }

  return NS_OK;
}

