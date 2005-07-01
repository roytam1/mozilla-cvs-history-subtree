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

#include "JSFunctions.h"
#include "nsCOMPtr.h"
#include "nsIJSContextStack.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#ifdef MOZ_JSCODELIB
#include "mozIJSCodeLib.h"
#include "nsString.h"
#endif

const char kJSContextStackContractID[] = "@mozilla.org/js/xpc/ContextStack;1";

////////////////////////////////////////////////////////////////////////
// JS functions shared between codelib and loader

JSBool JS_DLL_CALLBACK
JSDump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!argc)
        return JS_TRUE;
    
    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    char *bytes = JS_GetStringBytes(str);
    bytes = nsCRT::strdup(bytes);

#ifdef XP_MAC
    for (char *c = bytes; *c; c++)
        if (*c == '\r')
            *c = '\n';
#endif
    fputs(bytes, stderr);
    nsMemory::Free(bytes);
    return JS_TRUE;
}

JSBool JS_DLL_CALLBACK
JSDebug(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef DEBUG
    return JSDump(cx, obj, argc, argv, rval);
#else
    return JS_TRUE;
#endif
}

#ifdef MOZ_JSCODELIB
JSBool JS_DLL_CALLBACK
JSImportModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    char *url;
    JSObject *targetObj = nsnull;
    if (!JS_ConvertArguments(cx, argc, argv, "s/o", &url, &targetObj)) {
        return JS_FALSE;
    }

    if (!targetObj) {
        // Our targetObject is the caller's global object. Find it by
        // walking the calling object's parent chain.
        targetObj = obj;
        JSObject *parent = nsnull;
        while ((parent = JS_GetParent(cx, targetObj)))
            targetObj = parent;
    }

    nsCOMPtr<mozIJSCodeLib> codelib = do_GetService(MOZ_JSCODELIB_CONTRACTID);
    if (!codelib)
        return JS_FALSE;
    
    return NS_SUCCEEDED(codelib->ImportModuleToJSObject(nsDependentCString(url), targetObj));
}
#endif // MOZ_JSCODELIB


////////////////////////////////////////////////////////////////////////
// JSContextHelper

JSContextHelper::JSContextHelper(JSContext *cx)
    : mContext(cx), mContextThread(0)
{
    mContextThread = JS_GetContextThread(mContext);
    if (mContextThread) {
        JS_BeginRequest(mContext);
    } 
}

JSContextHelper::~JSContextHelper()
{
    JS_ClearNewbornRoots(mContext);
    if (mContextThread)
        JS_EndRequest(mContext);
}        
