/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*                         
* Private Headers for JSDB 
*/                         

#ifndef jsdbpriv_h___
#define jsdbpriv_h___

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" /* Added by JSIFY */
#include "jsprf.h"
#include "jsdbgapi.h"
#include "jsdb.h"

/***************************************************************************/

typedef struct JSDB_Data
{
    JSDContext*     jsdcTarget;
    JSRuntime*      rtTarget;
    JSRuntime*      rtDebugger;
    JSContext*      cxDebugger;
    JSObject*       globDebugger;
    JSObject*       jsdOb;
    jsval           jsScriptHook;
    jsval           jsExecutionHook;
    jsval           jsErrorReporterHook;
    JSDThreadState* jsdthreadstate;
    int             debuggerDepth;

} JSDB_Data;

extern JSBool
jsdb_ReflectJSD(JSDB_Data* data);

extern jsval
jsdb_PointerToNewHandleVal(JSContext *cx, void* ptr);

extern void*
jsdb_HandleValToPointer(JSContext *cx, jsval val);

extern JSBool
jsdb_SetThreadState(JSDB_Data* data, JSDThreadState* jsdthreadstate);

extern uintN JS_DLL_CALLBACK
jsdb_ExecHookHandler(JSDContext*     jsdc, 
                     JSDThreadState* jsdthreadstate,
                     uintN           type,
                     void*           callerdata,
                     jsval*          rval);

extern JSBool 
jsdb_EvalReturnExpression(JSDB_Data* data, jsval* rval);

#endif /* jsdbpriv_h___ */
