/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributors:
 *   Mike Shaver <shaver@zeroknowledge.com>
 *   John Bandhauer <jband@netscape.com>
 *   IBM Corp.
 *   Robert Ginda <rginda@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "JSAutoContext.h"
#include "nsCOMPtr.h"
#include "nsIJSContextStack.h"
#include "nsIServiceManager.h"

const char kJSContextStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";

JSAutoContext::JSAutoContext()
    : mContext(nsnull), mError(NS_OK), mPopNeeded(JS_FALSE), mContextThread(0)
{
    nsCOMPtr<nsIThreadJSContextStack> cxstack = 
        do_GetService(kJSContextStackContractID, &mError);
    
    if (NS_SUCCEEDED(mError)) {
        mError = cxstack->GetSafeJSContext(&mContext);
        if (NS_SUCCEEDED(mError) && mContext) {
            mError = cxstack->Push(mContext);
            if (NS_SUCCEEDED(mError)) {
                mPopNeeded = JS_TRUE;   
            } 
        } 
    }
    
    if (mContext) {
        mContextThread = JS_GetContextThread(mContext);
        if (mContextThread) {
            JS_BeginRequest(mContext);
        } 
    } else {
        if (NS_SUCCEEDED(mError)) {
            mError = NS_ERROR_FAILURE;
        }
    }
}

JSAutoContext::~JSAutoContext()
{
    if (mContext && mContextThread) {
        JS_ClearNewbornRoots(mContext);
        JS_EndRequest(mContext);
    }

    if (mPopNeeded) {
        nsCOMPtr<nsIThreadJSContextStack> cxstack = 
            do_GetService(kJSContextStackContractID);
        if (cxstack) {
            JSContext* cx;
            nsresult rv = cxstack->Pop(&cx);
            NS_ASSERTION(NS_SUCCEEDED(rv) && cx == mContext, "push/pop mismatch");
        }        
    }        
}        
