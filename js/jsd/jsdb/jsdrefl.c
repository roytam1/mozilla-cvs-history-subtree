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
* Reflects JSD api into JavaScript 
*/                                 

#include "jsdbpriv.h"

/***************************************************************************/
/* Non-const Properties */

static const char _str_ThreadStateHandle[]  = "ThreadStateHandle";
static const char _str_InterruptSet[]       = "InterruptSet";
static const char _str_Evaluating[]         = "Evaluating";
static const char _str_DebuggerDepth[]      = "DebuggerDepth";
static const char _str_ReturnExpression[]   = "ReturnExpression";

static jsval _valFalse = JSVAL_FALSE;
static jsval _valTrue  = JSVAL_TRUE;
static jsval _valNull  = JSVAL_NULL;


enum jsd_prop_ids
{
    JSD_PROP_ID_THREADSTATE_HANDLE  = -1,
    JSD_PROP_ID_INTERRUPT_SET       = -2,
    JSD_PROP_ID_EVALUATING          = -3,
    JSD_PROP_ID_DEBUGGER_DEPTH      = -4,
    JSD_PROP_ID_RETURN_EXPRESSION   = -5,
    COMPLETLY_IGNORED_ID            = 0 /* just to avoid tracking the comma */
};

static JSPropertySpec jsd_properties[] = {
    {_str_ThreadStateHandle, JSD_PROP_ID_THREADSTATE_HANDLE, JSPROP_ENUMERATE|JSPROP_PERMANENT},
    {_str_InterruptSet,      JSD_PROP_ID_INTERRUPT_SET,      JSPROP_ENUMERATE|JSPROP_PERMANENT},
    {_str_Evaluating,        JSD_PROP_ID_EVALUATING,         JSPROP_ENUMERATE|JSPROP_PERMANENT},
    {_str_DebuggerDepth,     JSD_PROP_ID_DEBUGGER_DEPTH,     JSPROP_ENUMERATE|JSPROP_PERMANENT},
    {_str_ReturnExpression,  JSD_PROP_ID_RETURN_EXPRESSION,  JSPROP_ENUMERATE|JSPROP_PERMANENT},
    {0}                                                                        
};

static JSBool
_defineNonConstProperties(JSDB_Data* data)
{
    JSContext* cx = data->cxDebugger;
    JSObject*  ob = data->jsdOb;
    jsval val;
    JSBool ignored;

    if(!JS_DefineProperties(cx, ob, jsd_properties))
        return JS_FALSE;

    /* set initial state of some of the properties */

    if(!JS_SetProperty(cx, ob, _str_ThreadStateHandle, &_valNull))
        return JS_FALSE;

    if(!JS_SetProperty(cx, ob, _str_InterruptSet, &_valFalse))
        return JS_FALSE;

    if(!JS_SetProperty(cx, ob, _str_Evaluating, &_valFalse))
        return JS_FALSE;

    val = INT_TO_JSVAL(data->debuggerDepth);
    if(!JS_SetProperty(cx, ob, _str_DebuggerDepth, &val))
        return JS_FALSE;
    if(!JS_SetPropertyAttributes(cx, ob, _str_DebuggerDepth,
                                 JSPROP_READONLY | 
                                 JSPROP_ENUMERATE | 
                                 JSPROP_PERMANENT,
                                 &ignored))
        return JS_FALSE;

    if(!JS_SetProperty(cx, ob, _str_ReturnExpression, &_valNull))
        return JS_FALSE;

    return JS_TRUE;
}

JSBool
jsdb_SetThreadState(JSDB_Data* data, JSDThreadState* jsdthreadstate)
{
    jsval val;

    data->jsdthreadstate = jsdthreadstate;
    if(jsdthreadstate)
        val = P2H_THREADSTATE(data->cxDebugger, jsdthreadstate);
    else
        val = JSVAL_NULL;

    return JS_SetProperty(data->cxDebugger, data->jsdOb,
                          _str_ThreadStateHandle, &val);
}

/***************************************************************************/
/*
*  System to store JSD_xxx handles in jsvals. This supports tracking the
*  handle's type - both for debugging and for automatic 'dropping' of
*  reference counted handle types (e.g. JSDValue).
*/

typedef struct JSDBHandle
{
    void* ptr;
    JSDBHandleType type;
} JSDBHandle;

static const char _str_HandleNameString[]   = "handle_name";

JS_STATIC_DLL_CALLBACK(void)
handle_finalize(JSContext *cx, JSObject *obj)
{
    JSDBHandle* p;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    p = (JSDBHandle*) JS_GetPrivate(cx, obj);
    if(p)
    {
        switch(p->type)
        {
          case JSDB_VALUE:
            JSD_DropValue(data->jsdcTarget, (JSDValue*) p->ptr);
            break;
          case JSDB_PROPERTY:
            JSD_DropProperty(data->jsdcTarget, (JSDProperty*) p->ptr);
            break;
          default:
            break;
        }
        free(p);
    }
    else
        JS_ASSERT(0);
}    

JSClass jsdb_HandleClass = {
    "JSDHandle",
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   handle_finalize
};

JS_STATIC_DLL_CALLBACK(JSBool)
handle_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if(!JS_InstanceOf(cx, obj, &jsdb_HandleClass, argv) ||
       !JS_GetProperty(cx, obj, _str_HandleNameString, rval))
    {
        JS_ASSERT(0);
        return JS_FALSE;
    }
    return JS_TRUE;
}    

static JSFunctionSpec handle_methods[] = {
    {"toString",   handle_toString,        0},
    {0}
};

void*
jsdb_HandleValToPointer(JSContext *cx, jsval val, JSDBHandleType type)
{
    JSDBHandle* p;
    JSObject *obj;

    if(!JSVAL_IS_OBJECT(val) ||
       !(obj = JSVAL_TO_OBJECT(val)) ||
       !JS_InstanceOf(cx, obj, &jsdb_HandleClass, NULL) ||
       !(p = (JSDBHandle*) JS_GetPrivate(cx, obj)))
    {
        JS_ASSERT(0);
        return NULL;
    }
    JS_ASSERT(p->ptr);
    JS_ASSERT(p->type == type);
    return p->ptr;
}

jsval
jsdb_PointerToNewHandleVal(JSContext *cx, void* ptr, JSDBHandleType type)
{
    JSDBHandle* p;
    JSObject* obj;
    char* name;
    char* type_name;
    JSString* name_str;

    if(!ptr || !(p = (JSDBHandle*)malloc(sizeof(JSDBHandle))))
    {
        JS_ASSERT(0);
        return JSVAL_NULL;
    }
    JS_ASSERT(!(((int)p) & 1));    /* must be 2-byte-aligned */

    p->ptr = ptr;
    p->type = type;

    if(!(obj = JS_NewObject(cx, &jsdb_HandleClass, NULL, NULL)) ||
       !JS_SetPrivate(cx, obj, p))
    {
        JS_ASSERT(0);
        return JSVAL_NULL;
    }

    switch(type)
    {
      case JSDB_GENERIC:
        type_name = "GENERIC";
        break;
      case JSDB_CONTEXT:
        type_name = "CONTEXT";
        break;
      case JSDB_SCRIPT:
        type_name = "SCRIPT";
        break;
      case JSDB_SOURCETEXT:
        type_name = "SOURCETEXT";
        break;
      case JSDB_THREADSTATE:
        type_name = "THREADSTATE";
        break;
      case JSDB_STACKFRAMEINFO:
        type_name = "STACKFRAMEINFO";
        break;
      case JSDB_VALUE:
        type_name = "VALUE";
        break;
      case JSDB_PROPERTY:
        type_name = "PROPERTY";
        break;
      case JSDB_OBJECT:
        type_name = "OBJECT";
        break;
      default:
        JS_ASSERT(0);
        type_name = "BOGUS";
        break;
    }

    name = JS_smprintf("%s:%x", type_name, ptr);
    if(!name || 
       !(name_str = JS_NewString(cx, name, strlen(name))) ||
       !JS_DefineProperty(cx, obj,
                          _str_HandleNameString,
                          STRING_TO_JSVAL(name_str),
                          NULL, NULL,
                          JSPROP_READONLY|JSPROP_PERMANENT))
    {
        JS_ASSERT(0);
        return JSVAL_NULL;
    }
    return OBJECT_TO_JSVAL(obj);
}

JSBool _initHandleSystem(JSDB_Data* data)
{
    return (JSBool) JS_InitClass(data->cxDebugger, data->globDebugger, 
                                 NULL, &jsdb_HandleClass, NULL, 0,
                                 NULL, handle_methods, NULL, NULL);
}    

/***************************************************************************/

JSBool 
jsdb_EvalReturnExpression(JSDB_Data* data, jsval* rval)
{
    JSDStackFrameInfo* frame;
    jsval expressionVal;
    JSString* expressionString;
    JSContext* cx = data->cxDebugger;
    JSBool result = JS_FALSE;

    frame = JSD_GetStackFrame(data->jsdcTarget, data->jsdthreadstate);
    if(!frame)
        return JS_FALSE;

    if(!JS_GetProperty(cx, data->jsdOb, _str_ReturnExpression, &expressionVal))
        return JS_FALSE;

    if(!(expressionString = JS_ValueToString(cx, expressionVal)))
        return JS_FALSE;

    JS_SetProperty(cx, data->jsdOb, _str_Evaluating, &_valTrue);
    if(JSD_EvaluateScriptInStackFrame(data->jsdcTarget,
                                      data->jsdthreadstate,
                                      frame,
                                      JS_GetStringBytes(expressionString),
                                      JS_GetStringLength(expressionString),
                                      "ReturnExpressionEval", 1, rval))
    {
        result = JS_TRUE;
    }
    JS_SetProperty(cx, data->jsdOb, _str_Evaluating, &_valFalse);
    JS_SetProperty(cx, data->jsdOb, _str_ReturnExpression, &_valNull);
    return result;
}

/***************************************************************************/
/* High Level calls */

JS_STATIC_DLL_CALLBACK(JSBool)
GetMajorVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = INT_TO_JSVAL(JSD_GetMajorVersion());
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetMinorVersion(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = INT_TO_JSVAL(JSD_GetMinorVersion());
    return JS_TRUE;
}

/***************************************************************************/
/* Script functions */

JS_STATIC_DLL_CALLBACK(JSBool)
LockScriptSubsystem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);
    JSD_LockScriptSubsystem(data->jsdcTarget);
    *rval = JSVAL_TRUE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
UnlockScriptSubsystem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);
    JSD_UnlockScriptSubsystem(data->jsdcTarget);
    *rval = JSVAL_TRUE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
IterateScripts(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *iterp = NULL;
    JSDScript *script;
    JSFunction *fun;
    jsval argv0;
    int count = 0;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || NULL ==(fun = JS_ValueToFunction(cx, argv[0])))
    {
        JS_ReportError(cx, "IterateScripts requires a function param");
        return JS_FALSE;
    }

    /* We pass along any additional args, so patch and leverage argv */
    argv0 = argv[0];
    while(NULL != (script = JSD_IterateScripts(data->jsdcTarget, &iterp)))
    {
        jsval retval;
        argv[0] = P2H_SCRIPT(cx, script);

        JS_CallFunction(cx, NULL, fun, argc, argv, &retval);
        count++ ;
    }
    argv[0] = argv0;

    *rval = INT_TO_JSVAL(count);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
IsActiveScript(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "IsActiveScript requires script handle");
        return JS_FALSE;
    }
    *rval = JSD_IsActiveScript(data->jsdcTarget, jsdscript)
                ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptFilename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetScriptFilename requires script handle");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        printf("%d\n", (int)jsdscript);
        JS_ReportError(cx, "GetScriptFilename passed inactive script");
        return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,
                JSD_GetScriptFilename(data->jsdcTarget, jsdscript)));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptFunctionName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetScriptFunctionName requires script handle");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetScriptFunctionName passed inactive script");
        return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,
                JSD_GetScriptFunctionName(data->jsdcTarget, jsdscript)));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptBaseLineNumber(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetScriptBaseLineNumber requires script handle");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetScriptBaseLineNumber passed inactive script");
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(JSD_GetScriptBaseLineNumber(data->jsdcTarget, jsdscript));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptLineExtent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetScriptLineExtent requires script handle");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetScriptLineExtent passed inactive script");
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(JSD_GetScriptLineExtent(data->jsdcTarget, jsdscript));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
SetScriptHook(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval oldHook;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 ||
       !(JSVAL_IS_NULL(argv[0]) ||
         NULL != JS_ValueToFunction(cx, argv[0])))
    {
        JS_ReportError(cx, "SetScriptHook requires a function or null param");
        return JS_FALSE;
    }

    oldHook = data->jsScriptHook;
    data->jsScriptHook = argv[0];

    *rval = oldHook;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptHook(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    *rval = data->jsScriptHook;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetClosestPC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 line;
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 2 || !(jsdscript = H2P_SCRIPT(cx, argv[0])) ||
       ! JS_ValueToInt32(cx, argv[1], &line))
    {
        JS_ReportError(cx, "GetClosestPC requires script handle and a line number");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetClosestPC passed inactive script");
        return JS_FALSE;
    }

    *rval = P2H_GENERIC(cx, (void*) 
                        JSD_GetClosestPC(data->jsdcTarget, jsdscript, line));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetClosestLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 2 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetClosestLine requires script handle and a line number");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetClosestLine passed inactive script");
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(JSD_GetClosestLine(data->jsdcTarget, jsdscript,
                                 (jsuword) H2P_GENERIC(cx, argv[1])));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
SetExecutionHook(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval oldHook;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 ||
       !(JSVAL_IS_NULL(argv[0]) ||
         NULL != JS_ValueToFunction(cx, argv[0])))
    {
        JS_ReportError(cx, "SetExecutionHook requires a function or null param");
        return JS_FALSE;
    }

    oldHook = data->jsExecutionHook;
    data->jsExecutionHook = argv[0];

    *rval = oldHook;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetExecutionHook(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    *rval = data->jsExecutionHook;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
SendInterrupt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val = JSVAL_TRUE;
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);


    success = JSD_SetInterruptHook(data->jsdcTarget, jsdb_ExecHookHandler, data);
    if(success)
        success = JS_SetProperty(data->cxDebugger, data->jsdOb,
                                 _str_InterruptSet, &val);
    return success;
}

JS_STATIC_DLL_CALLBACK(JSBool)
ClearInterrupt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval val = JSVAL_FALSE;
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    success = JSD_ClearInterruptHook(data->jsdcTarget);
    if(success)
        success = JS_SetProperty(data->cxDebugger, data->jsdOb,
                                 _str_InterruptSet, &val);
    return success;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetCountOfStackFrames(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    *rval = INT_TO_JSVAL(JSD_GetCountOfStackFrames(data->jsdcTarget,
                                                   data->jsdthreadstate));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetStackFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    *rval = P2H_STACKFRAMEINFO(cx, JSD_GetStackFrame(data->jsdcTarget,
                                                     data->jsdthreadstate));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetCallingStackFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDStackFrameInfo* jsdframe;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdframe = H2P_STACKFRAMEINFO(cx, argv[0])))
    {
        JS_ReportError(cx, "GetCallingStackFrame requires stackframe handle");
        return JS_FALSE;
    }

    *rval = P2H_STACKFRAMEINFO(cx, JSD_GetCallingStackFrame(data->jsdcTarget,
                                                            data->jsdthreadstate,
                                                            jsdframe));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetScriptForStackFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDStackFrameInfo* jsdframe;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdframe = H2P_STACKFRAMEINFO(cx, argv[0])))
    {
        JS_ReportError(cx, "GetScriptForStackFrame requires stackframe handle");
        return JS_FALSE;
    }

    *rval = P2H_SCRIPT(cx, JSD_GetScriptForStackFrame(data->jsdcTarget,
                                                      data->jsdthreadstate,
                                                      jsdframe));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetPCForStackFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDStackFrameInfo* jsdframe;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdframe = H2P_STACKFRAMEINFO(cx, argv[0])))
    {
        JS_ReportError(cx, "GetPCForStackFrame requires stackframe handle");
        return JS_FALSE;
    }

    *rval = P2H_GENERIC(cx, (void*) JSD_GetPCForStackFrame(data->jsdcTarget,
                                                           data->jsdthreadstate,
                                                           jsdframe));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
EvaluateScriptInStackFrame(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    static char default_filename[] = "jsdb_show";
    JSDStackFrameInfo* jsdframe;
    jsval foreignAnswerVal;
    JSString* foreignAnswerString;
    JSString* textJSString;
    char* filename;
    int32 lineno;
    JSBool retVal = JS_FALSE;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdframe = H2P_STACKFRAMEINFO(cx, argv[0])))
    {
        JS_ReportError(cx, "EvaluateScriptInStackFrame requires stackframe handle");
        return JS_FALSE;
    }

    if(argc < 2 || !(textJSString = JS_ValueToString(cx, argv[1])))
    {
        JS_ReportError(cx, "EvaluateScriptInStackFrame requires source text as a second param");
        return JS_FALSE;
    }

    if(argc < 3)
        filename = default_filename;
    else
    {
        JSString* filenameJSString;
        if(!(filenameJSString = JS_ValueToString(cx, argv[2])))
        {
            JS_ReportError(cx, "EvaluateScriptInStackFrame passed non-string filename as 3rd param");
            return JS_FALSE;
        }
        filename = JS_GetStringBytes(filenameJSString);
    }

    if(argc < 4)
        lineno = 1;
    else
    {
        if(!JS_ValueToInt32(cx, argv[3], &lineno))
        {
            JS_ReportError(cx, "EvaluateScriptInStackFrame passed non-int lineno as 4th param");
            return JS_FALSE;
        }
    }


    JS_SetProperty(cx, data->jsdOb, _str_Evaluating, &_valTrue);

    if(JSD_EvaluateScriptInStackFrame(data->jsdcTarget,
                                      data->jsdthreadstate,
                                      jsdframe,
                                      JS_GetStringBytes(textJSString),
                                      JS_GetStringLength(textJSString),
                                      filename, lineno, &foreignAnswerVal))
    {
        if(NULL != (foreignAnswerString =
            JSD_ValToStringInStackFrame(data->jsdcTarget,
                                        data->jsdthreadstate,
                                        jsdframe,
                                        foreignAnswerVal)))
        {
            *rval = STRING_TO_JSVAL(
                        JS_NewStringCopyZ(cx,
                                      JS_GetStringBytes(foreignAnswerString)));
            retVal = JS_TRUE;
        }

    }

    JS_SetProperty(cx, data->jsdOb, _str_Evaluating, &_valFalse);

    return retVal;
}

JS_STATIC_DLL_CALLBACK(JSBool)
SetTrap(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    jsuword pc;
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "SetTrap requires script handle as first arg");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "SetTrap passed inactive script");
        return JS_FALSE;
    }

    if(argc < 2 || 0 == (pc = (jsuword) H2P_GENERIC(cx, argv[1])))
    {
        JS_ReportError(cx, "SetTrap requires pc handle as second arg");
        return JS_FALSE;
    }

    success = JSD_SetExecutionHook(data->jsdcTarget, jsdscript, pc,
                                   jsdb_ExecHookHandler, data);
    *rval = success ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
ClearTrap(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    jsuword pc;
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "ClearTrap requires script handle as first arg");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "ClearTrap passed inactive script");
        return JS_FALSE;
    }

    if(argc < 2 || 0 == (pc = (jsuword) H2P_GENERIC(cx, argv[1])))
    {
        JS_ReportError(cx, "ClearTrap requires pc handle as second arg");
        return JS_FALSE;
    }

    success = JSD_ClearExecutionHook(data->jsdcTarget, jsdscript, pc);
    *rval = success ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
ClearAllTrapsForScript(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDScript *jsdscript;
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdscript = H2P_SCRIPT(cx, argv[0])))
    {
        JS_ReportError(cx, "ClearAllTrapsForScript script handle as first arg");
        return JS_FALSE;
    }
    if(! JSD_IsActiveScript(data->jsdcTarget, jsdscript))
    {
        JS_ReportError(cx, "GetClosestLine passed inactive script");
        return JS_FALSE;
    }

    success = JSD_ClearAllExecutionHooksForScript(data->jsdcTarget, jsdscript);
    *rval = success ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
ClearAllTraps(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool success;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    success = JSD_ClearAllExecutionHooks(data->jsdcTarget);
    *rval = success ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
SetErrorReporterHook(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval oldHook;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 ||
       !(JSVAL_IS_NULL(argv[0]) ||
         NULL != JS_ValueToFunction(cx, argv[0])))
    {
        JS_ReportError(cx, "SetErrorReporterHook requires a function or null param");
        return JS_FALSE;
    }

    oldHook = data->jsErrorReporterHook;
    data->jsErrorReporterHook = argv[0];

    *rval = oldHook;
    return JS_TRUE;
}

/***************************************************************************/
/* Source Text functions */

JS_STATIC_DLL_CALLBACK(JSBool)
LockSourceTextSubsystem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);
    JSD_LockSourceTextSubsystem(data->jsdcTarget);
    *rval = JSVAL_TRUE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
UnlockSourceTextSubsystem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);
    JSD_UnlockSourceTextSubsystem(data->jsdcTarget);
    *rval = JSVAL_TRUE;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
IterateSources(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDSourceText *iterp = NULL;
    JSDSourceText *jsdsrc;
    JSFunction *fun;
    jsval argv0;
    int count = 0;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || NULL ==(fun = JS_ValueToFunction(cx, argv[0])))
    {
        JS_ReportError(cx, "IterateSources requires a function param");
        return JS_FALSE;
    }

    /* We pass along any additional args, so patch and leverage argv */
    argv0 = argv[0];
    while(NULL != (jsdsrc = JSD_IterateSources(data->jsdcTarget, &iterp)))
    {
        jsval retval;
        argv[0] = P2H_SOURCETEXT(cx, jsdsrc);

        JS_CallFunction(cx, NULL, fun, argc, argv, &retval);
        count++ ;
    }
    argv[0] = argv0;

    *rval = INT_TO_JSVAL(count);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
FindSourceForURL(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString* jsstr;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsstr = JS_ValueToString(cx, argv[0])))
    {
        JS_ReportError(cx, "FindSourceForURL requires a URL (filename) string");
        return JS_FALSE;
    }
    *rval = P2H_SOURCETEXT(cx, JSD_FindSourceForURL(data->jsdcTarget, 
                                                    JS_GetStringBytes(jsstr)));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetSourceURL(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDSourceText* jsdsrc;
    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdsrc = H2P_SOURCETEXT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetSourceURL requires sourcetext handle");
        return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,
                JSD_GetSourceURL(data->jsdcTarget, jsdsrc)));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetSourceText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDSourceText* jsdsrc;
    const char* ptr;
    int len;

    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdsrc = H2P_SOURCETEXT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetSourceText requires sourcetext handle");
        return JS_FALSE;
    }

    if(JSD_GetSourceText(data->jsdcTarget, jsdsrc, &ptr, &len))
        *rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx, ptr, len));
    else
        *rval = JSVAL_NULL;
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetSourceStatus(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDSourceText* jsdsrc;

    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdsrc = H2P_SOURCETEXT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetSourceStatus requires sourcetext handle");
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(JSD_GetSourceStatus(data->jsdcTarget, jsdsrc));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
GetSourceAlterCount(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSDSourceText* jsdsrc;

    JSDB_Data* data = (JSDB_Data*) JS_GetContextPrivate(cx);
    JS_ASSERT(data);

    if(argc < 1 || !(jsdsrc = H2P_SOURCETEXT(cx, argv[0])))
    {
        JS_ReportError(cx, "GetSourceAlterCount requires sourcetext handle");
        return JS_FALSE;
    }

    *rval = INT_TO_JSVAL(JSD_GetSourceAlterCount(data->jsdcTarget, jsdsrc));
    return JS_TRUE;
}

/***************************************************************************/
static JSFunctionSpec jsd_functions[] = {
/* High Level calls */
    {"GetMajorVersion",             GetMajorVersion,            0, JSPROP_ENUMERATE},
    {"GetMinorVersion",             GetMinorVersion,            0, JSPROP_ENUMERATE},
/* Script functions */
    {"LockScriptSubsystem",         LockScriptSubsystem,        0, JSPROP_ENUMERATE},
    {"UnlockScriptSubsystem",       UnlockScriptSubsystem,      0, JSPROP_ENUMERATE},
    {"IterateScripts",              IterateScripts,             1, JSPROP_ENUMERATE},
    {"IsActiveScript",              IsActiveScript,             1, JSPROP_ENUMERATE},
    {"GetScriptFilename",           GetScriptFilename,          1, JSPROP_ENUMERATE},
    {"GetScriptFunctionName",       GetScriptFunctionName,      1, JSPROP_ENUMERATE},
    {"GetScriptBaseLineNumber",     GetScriptBaseLineNumber,    1, JSPROP_ENUMERATE},
    {"GetScriptLineExtent",         GetScriptLineExtent,        1, JSPROP_ENUMERATE},
    {"SetScriptHook",               SetScriptHook,              1, JSPROP_ENUMERATE},
    {"GetScriptHook",               GetScriptHook,              0, JSPROP_ENUMERATE},
    {"GetClosestPC",                GetClosestPC,               2, JSPROP_ENUMERATE},
    {"GetClosestLine",              GetClosestLine,             2, JSPROP_ENUMERATE},
/* Execution/Interrupt Hook functions */
    {"SetExecutionHook",            SetExecutionHook,           1, JSPROP_ENUMERATE},
    {"GetExecutionHook",            GetExecutionHook,           0, JSPROP_ENUMERATE},
    {"SendInterrupt",               SendInterrupt,              0, JSPROP_ENUMERATE},
    {"ClearInterrupt",              ClearInterrupt,             0, JSPROP_ENUMERATE},
    {"GetCountOfStackFrames",       GetCountOfStackFrames,      0, JSPROP_ENUMERATE},
    {"GetStackFrame",               GetStackFrame,              0, JSPROP_ENUMERATE},
    {"GetCallingStackFrame",        GetCallingStackFrame,       1, JSPROP_ENUMERATE},
    {"GetScriptForStackFrame",      GetScriptForStackFrame,     1, JSPROP_ENUMERATE},
    {"GetPCForStackFrame",          GetPCForStackFrame,         1, JSPROP_ENUMERATE},
    {"EvaluateScriptInStackFrame",  EvaluateScriptInStackFrame, 4, JSPROP_ENUMERATE},
    {"SetTrap",                     SetTrap,                    2, JSPROP_ENUMERATE},
    {"ClearTrap",                   ClearTrap,                  2, JSPROP_ENUMERATE},
    {"ClearAllTrapsForScript",      ClearAllTrapsForScript,     1, JSPROP_ENUMERATE},
    {"ClearAllTraps",               ClearAllTraps,              0, JSPROP_ENUMERATE},
    {"SetErrorReporterHook",        SetErrorReporterHook,       1, JSPROP_ENUMERATE},
/* Source Text functions */
    {"LockSourceTextSubsystem",     LockSourceTextSubsystem,    0, JSPROP_ENUMERATE},
    {"UnlockSourceTextSubsystem",   UnlockSourceTextSubsystem,  0, JSPROP_ENUMERATE},
    {"IterateSources",              IterateSources,             1, JSPROP_ENUMERATE},
    {"FindSourceForURL",            FindSourceForURL,           1, JSPROP_ENUMERATE},
    {"GetSourceURL",                GetSourceURL,               1, JSPROP_ENUMERATE},
    {"GetSourceText",               GetSourceText,              1, JSPROP_ENUMERATE},
    {"GetSourceStatus",             GetSourceStatus,            1, JSPROP_ENUMERATE},
    {"GetSourceAlterCount",         GetSourceAlterCount,        1, JSPROP_ENUMERATE},
    {0}
};

/***************************************************************************/
/***************************************************************************/
/* Constant Properties */
typedef struct ConstProp
{
    char* name;
    int   val;
} ConstProp;

/* these are used for constants defined in jsdebug.h - they must match! */

static ConstProp const_props[] = {
    {"JSD_HOOK_INTERRUPTED",            JSD_HOOK_INTERRUPTED     },
    {"JSD_HOOK_BREAKPOINT",             JSD_HOOK_BREAKPOINT      },
    {"JSD_HOOK_DEBUG_REQUESTED",        JSD_HOOK_DEBUG_REQUESTED },
    {"JSD_HOOK_DEBUGGER_KEYWORD",       JSD_HOOK_DEBUGGER_KEYWORD},

    {"JSD_HOOK_RETURN_HOOK_ERROR",      JSD_HOOK_RETURN_HOOK_ERROR    },
    {"JSD_HOOK_RETURN_CONTINUE",        JSD_HOOK_RETURN_CONTINUE      },
    {"JSD_HOOK_RETURN_ABORT",           JSD_HOOK_RETURN_ABORT         },
    {"JSD_HOOK_RETURN_RET_WITH_VAL",    JSD_HOOK_RETURN_RET_WITH_VAL  },
    {"JSD_HOOK_RETURN_THROW_WITH_VAL",  JSD_HOOK_RETURN_THROW_WITH_VAL},

    {"JSD_SOURCE_INITED",               JSD_SOURCE_INITED   },
    {"JSD_SOURCE_PARTIAL",              JSD_SOURCE_PARTIAL  },
    {"JSD_SOURCE_COMPLETED",            JSD_SOURCE_COMPLETED},
    {"JSD_SOURCE_ABORTED",              JSD_SOURCE_ABORTED  },
    {"JSD_SOURCE_FAILED",               JSD_SOURCE_FAILED   },
    {"JSD_SOURCE_CLEARED",              JSD_SOURCE_CLEARED  },

    {"JSD_ERROR_REPORTER_PASS_ALONG",   JSD_ERROR_REPORTER_PASS_ALONG},
    {"JSD_ERROR_REPORTER_RETURN",       JSD_ERROR_REPORTER_RETURN    },
    {"JSD_ERROR_REPORTER_DEBUG",        JSD_ERROR_REPORTER_DEBUG     },

    {0}
};

JSBool
_defineConstProperties(JSDB_Data* data)
{
    int i;
    for(i = 0; const_props[i].name;i++)
    {
        if(!JS_DefineProperty(data->cxDebugger, data->jsdOb,
                              const_props[i].name,
                              INT_TO_JSVAL(const_props[i].val),
                              NULL, NULL,
                              JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT))
            return JS_FALSE;
    }
    return JS_TRUE;
}
/**************************************/

static JSClass jsd_class = {
    "JSD", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

/***************************************************************************/

JSBool
jsdb_ReflectJSD(JSDB_Data* data)
{
    if(!(data->jsdOb = JS_DefineObject(data->cxDebugger, data->globDebugger,
                   "jsd", &jsd_class, NULL,
                    JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT)))
        return JS_FALSE;

    if(!JS_DefineFunctions(data->cxDebugger, data->jsdOb, jsd_functions))
        return JS_FALSE;

    if (!JS_DefineProperties(data->cxDebugger, data->jsdOb, jsd_properties))
        return JS_FALSE;

    if(!_defineConstProperties(data))
        return JS_FALSE;

    if(!_defineNonConstProperties(data))
        return JS_FALSE;

    if(!_initHandleSystem(data))
        return JS_FALSE;

    return JS_TRUE;
}


