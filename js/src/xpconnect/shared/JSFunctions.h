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

#ifndef __JS_FUNCTIONS_H__
#define __JS_FUNCTIONS_H__

#include "jsapi.h"
#include "nscore.h"

////////////////////////////////////////////////////////////////////////
// JS functions shared by codelib and loader

JSBool JS_DLL_CALLBACK
JSDump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

JSBool JS_DLL_CALLBACK
JSDebug(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

#ifdef MOZ_JSCODELIB
JSBool JS_DLL_CALLBACK
JSImportModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
#endif // MOZ_JSCODELIB

////////////////////////////////////////////////////////////////////////
// JSContextHelper

class JSContextHelper
{
public:
    JSContextHelper(JSContext* cx);
    ~JSContextHelper();

    operator JSContext*() const {return mContext;}

    JSContextHelper(); // not implemnted
private:
    JSContext* mContext;
    intN       mContextThread; 
};

////////////////////////////////////////////////////////////////////////
// JSAutoErrorReporterSetter

class JSAutoErrorReporterSetter
{
public:
    JSAutoErrorReporterSetter(JSContext* cx, JSErrorReporter reporter)
        {mContext = cx; mOldReporter = JS_SetErrorReporter(cx, reporter);}
    ~JSAutoErrorReporterSetter()
        {JS_SetErrorReporter(mContext, mOldReporter);} 
    JSAutoErrorReporterSetter(); // not implemented
private:
    JSContext* mContext;
    JSErrorReporter mOldReporter;
};


#endif // __JS_FUNCTIONS_H__
