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
 * JavaScript Debugging support - 'High Level' functions
 */

#include "jsd.h"

/***************************************************************************/

/* XXX not 'static' because of old Mac CodeWarrior bug */ 
JSCList _jsd_context_list = JS_INIT_STATIC_CLIST(&_jsd_context_list);

/* these are used to connect JSD_SetUserCallbacks() with JSD_DebuggerOn() */
static JSD_UserCallbacks _callbacks;
static void*             _user = NULL; 
static JSRuntime*        _jsrt = NULL;

#ifdef JSD_HAS_DANGEROUS_THREAD
static void* _dangerousThread = NULL;
#endif

#ifdef JSD_THREADSAFE
void* _jsd_global_lock = NULL;
#endif

#ifdef DEBUG
void JSD_ASSERT_VALID_CONTEXT(JSDContext* jsdc)
{
    JS_ASSERT(jsdc->inited);
    JS_ASSERT(jsdc->jsrt);
    JS_ASSERT(jsdc->jscontexts);
    JS_ASSERT(jsdc->dumbContext);
    JS_ASSERT(jsdc->glob);
}
#endif

JS_STATIC_DLL_CALLBACK(JSHashNumber)
_hash_root(const void *key)
{
    return ((JSHashNumber) key) >> 2; /* help lame MSVC1.5 on Win16 */
}

static JSClass global_class = {
    "JSDGlobal", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

static JSBool
_validateUserCallbacks(JSD_UserCallbacks* callbacks)
{
    return !callbacks ||
           (callbacks->size && callbacks->size <= sizeof(JSD_UserCallbacks));
}    

static JSDContext*
_newJSDContext(JSRuntime*         jsrt, 
               JSD_UserCallbacks* callbacks, 
               void*              user)
{
    JSDContext* jsdc = NULL;

    if( ! jsrt )
        return NULL;

    if( ! _validateUserCallbacks(callbacks) )
        return NULL;

    jsdc = (JSDContext*) calloc(1, sizeof(JSDContext));
    if( ! jsdc )
        goto label_newJSDContext_failure;

    if( ! JSD_INIT_LOCKS(jsdc) )
        goto label_newJSDContext_failure;

    JS_INIT_CLIST(&jsdc->links);

    jsdc->jsrt = jsrt;

    if( callbacks )
        memcpy(&jsdc->userCallbacks, callbacks, callbacks->size);
    
    jsdc->user = user;

#ifdef JSD_HAS_DANGEROUS_THREAD
    jsdc->dangerousThread = _dangerousThread;
#endif

    JS_INIT_CLIST(&jsdc->threadsStates);
    JS_INIT_CLIST(&jsdc->scripts);
    JS_INIT_CLIST(&jsdc->sources);
    JS_INIT_CLIST(&jsdc->removedSources);

    jsdc->sourceAlterCount = 1;

    jsdc->jscontexts = JS_NewHashTable(256, _hash_root,
                                       JS_CompareValues, JS_CompareValues,
                                       NULL, NULL);
    if( ! jsdc->jscontexts )
        goto label_newJSDContext_failure;

    if( ! jsd_CreateAtomTable(jsdc) )
        goto label_newJSDContext_failure;

    if( ! jsd_InitObjectManager(jsdc) )
        goto label_newJSDContext_failure;

    jsdc->dumbContext = JS_NewContext(jsdc->jsrt, 256);
    if( ! jsdc->dumbContext )
        goto label_newJSDContext_failure;

    jsdc->glob = JS_NewObject(jsdc->dumbContext, &global_class, NULL, NULL);
    if( ! jsdc->glob )
        goto label_newJSDContext_failure;

    if( ! JS_InitStandardClasses(jsdc->dumbContext, jsdc->glob) )
        goto label_newJSDContext_failure;

    jsdc->inited = JS_TRUE;

    JSD_LOCK();
    JS_INSERT_LINK(&jsdc->links, &_jsd_context_list);
    JSD_UNLOCK();

    return jsdc;

label_newJSDContext_failure:
    jsd_DestroyObjectManager(jsdc);
    jsd_DestroyAtomTable(jsdc);
    if( jsdc->jscontexts )
        JS_HashTableDestroy(jsdc->jscontexts);
    if( jsdc )
        free(jsdc);
    return NULL;
}

JS_STATIC_DLL_CALLBACK(intN)
_hash_entry_zapper(JSHashEntry *he, intN i, void *arg)
{
    if(he->value)
        free(he->value);
    he->value = NULL;
    he->key   = NULL;
    return HT_ENUMERATE_NEXT;
}

static void
_destroyJSDContext(JSDContext* jsdc)
{
    JSD_ASSERT_VALID_CONTEXT(jsdc);

    JSD_LOCK();
    JS_REMOVE_LINK(&jsdc->links);
    JSD_UNLOCK();

    jsd_DestroyObjectManager(jsdc);
    jsd_DestroyAtomTable(jsdc);

    if( jsdc->jscontexts )
    {
        JS_HashTableEnumerateEntries(jsdc->jscontexts, _hash_entry_zapper, NULL);
        JS_HashTableDestroy(jsdc->jscontexts);
        jsdc->jscontexts = NULL;
    }

    jsdc->inited = JS_FALSE;

    /*
    * We should free jsdc here, but we let it leak in case there are any 
    * asynchronous hooks calling into the system using it as a handle
    *
    * XXX we also leak the locks
    */

}

/***************************************************************************/

JSDContext*
jsd_DebuggerOnForUser(JSRuntime*         jsrt, 
                      JSD_UserCallbacks* callbacks, 
                      void*              user)
{
    JSDContext* jsdc;
    JSContext* iter = NULL;
    JSContext* cx;

    jsdc = _newJSDContext(jsrt, callbacks, user);
    if( ! jsdc )
        return NULL;

    /* set hooks here */
    JS_SetNewScriptHookProc(jsdc->jsrt, jsd_NewScriptHookProc, jsdc);
    JS_SetDestroyScriptHookProc(jsdc->jsrt, jsd_DestroyScriptHookProc, jsdc);
    JS_SetDebuggerHandler(jsdc->jsrt, jsd_DebuggerHandler, jsdc);
    JS_SetExecuteHook(jsdc->jsrt, jsd_InterpreterHook, jsdc);
    JS_SetCallHook(jsdc->jsrt, jsd_InterpreterHook, jsdc);
    JS_SetObjectHook(jsdc->jsrt, jsd_ObjectHook, jsdc);
#ifdef LIVEWIRE
    LWDBG_SetNewScriptHookProc(jsd_NewScriptHookProc, jsdc);
#endif

    /* enumerate contexts for JSRuntime and add them to our table */
    while( NULL != (cx = JS_ContextIterator(jsdc->jsrt, &iter)) )
        jsd_JSContextUsed(jsdc, cx);

    if( jsdc->userCallbacks.setContext )
        jsdc->userCallbacks.setContext(jsdc, jsdc->user);
    
    return jsdc;
}

JSDContext*
jsd_DebuggerOn(void)
{
    JS_ASSERT(_jsrt);
    JS_ASSERT(_validateUserCallbacks(&_callbacks));
    return jsd_DebuggerOnForUser(_jsrt, &_callbacks, _user);
}

void
jsd_DebuggerOff(JSDContext* jsdc)
{
    /* clear hooks here */
    JS_SetNewScriptHookProc(jsdc->jsrt, NULL, NULL);
    JS_SetDestroyScriptHookProc(jsdc->jsrt, NULL, NULL);
    JS_SetDebuggerHandler(jsdc->jsrt, NULL, NULL);
    JS_SetExecuteHook(jsdc->jsrt, NULL, NULL);
    JS_SetCallHook(jsdc->jsrt, NULL, NULL);
    JS_SetObjectHook(jsdc->jsrt, NULL, NULL);
#ifdef LIVEWIRE
    LWDBG_SetNewScriptHookProc(NULL,NULL);
#endif

    /* clean up */
    jsd_DestroyAllJSDScripts(jsdc);
    jsd_DestroyAllSources(jsdc);
    
    _destroyJSDContext(jsdc);

    if( jsdc->userCallbacks.setContext )
        jsdc->userCallbacks.setContext(NULL, jsdc->user);
}

void
jsd_SetUserCallbacks(JSRuntime* jsrt, JSD_UserCallbacks* callbacks, void* user)
{
    _jsrt = jsrt;
    _user = user;

#ifdef JSD_HAS_DANGEROUS_THREAD
    _dangerousThread = JSD_CURRENT_THREAD();
#endif

    if( callbacks )
        memcpy(&_callbacks, callbacks, sizeof(JSD_UserCallbacks));
    else
        memset(&_callbacks, 0 , sizeof(JSD_UserCallbacks));
}

JSDContext*
jsd_JSDContextForJSContext(JSContext* context)
{
    JSDContext* iter;
    JSDContext* jsdc = NULL;
    JSRuntime*  runtime = JS_GetRuntime(context);

    JSD_LOCK();
    for( iter = (JSDContext*)_jsd_context_list.next;
         iter != (JSDContext*)&_jsd_context_list;
         iter = (JSDContext*)iter->links.next )
    {
        /* if( JS_HashTableLookup(iter->jscontexts, context) ) */
        if( runtime == iter->jsrt )
        {
            jsdc = iter;
            break;
        }
    }
    JSD_UNLOCK();
    return jsdc;
}    

static JSDContextWrapper*
_jsd_JSDContextWrapperForJSContext(JSDContext* jsdc, JSContext* context)
{
    JSDContextWrapper* w = NULL;
    JSD_LOCK();
    w = (JSDContextWrapper*) JS_HashTableLookup(jsdc->jscontexts, context);
    JSD_UNLOCK();
    return w;
}

JS_STATIC_DLL_CALLBACK(void)
jsd_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    JSDContextWrapper* wrapper;
    JSDContext* jsdc;
    uintN action = JSD_ERROR_REPORTER_PASS_ALONG;
    JSD_ErrorReporter errorReporter;
    void*             errorReporterData;

    jsdc = jsd_JSDContextForJSContext(cx);
    if( ! jsdc )
    {
        JS_ASSERT(0);
        return;
    }

    wrapper = _jsd_JSDContextWrapperForJSContext(jsdc, cx);
    if( ! wrapper )
    {
        JS_ASSERT(0);
        return;
    }

    /* local in case hook gets cleared on another thread */
    JSD_LOCK();
    errorReporter     = jsdc->errorReporter;
    errorReporterData = jsdc->errorReporterData;
    JSD_UNLOCK();

    if( errorReporter && ! JSD_IS_DANGEROUS_THREAD(jsdc) )
        action = errorReporter(jsdc, cx, message, report, 
                               errorReporterData);

    switch(action)
    {
        case JSD_ERROR_REPORTER_PASS_ALONG:
            if( wrapper->originalErrorReporter )
                wrapper->originalErrorReporter(cx, message, report);
            break;
        case JSD_ERROR_REPORTER_RETURN:    
            break;
        case JSD_ERROR_REPORTER_DEBUG:
        {
            jsval rval;
            JSD_ExecutionHookProc   hook;
            void*                   hookData;

            /* local in case hook gets cleared on another thread */
            JSD_LOCK();
            hook = jsdc->debugBreakHook;
            hookData = jsdc->debugBreakHookData;
            JSD_UNLOCK();

            jsd_CallExecutionHook(jsdc, cx, JSD_HOOK_DEBUG_REQUESTED,
                                  hook, hookData, &rval);
            break;
        }
        default:
            JS_ASSERT(0);
            break;
    }
}

void 
jsd_JSContextUsed(JSDContext* jsdc, JSContext* context)
{
    JSDContextWrapper* wrapper;
    JSHashEntry*       he;

    wrapper = _jsd_JSDContextWrapperForJSContext(jsdc, context);
    if( wrapper )
    {
        /* error reporters are sometimes overwritten by other code... */
        JSErrorReporter oldrep = JS_SetErrorReporter(context, jsd_ErrorReporter);
        if( jsd_ErrorReporter != oldrep )
            wrapper->originalErrorReporter = oldrep;
        return;
    }

    /* else... */
    wrapper = (JSDContextWrapper*) calloc(1,sizeof(JSDContextWrapper));
    if( ! wrapper )
        return;

    JSD_LOCK();
    he = JS_HashTableAdd(jsdc->jscontexts, context, wrapper);
    JSD_UNLOCK();
    if( ! he )
    {
        free(wrapper);
        return;
    }

    wrapper->context = context;
    wrapper->jsdc    = jsdc;

    /* add our error reporter */
    wrapper->originalErrorReporter = JS_SetErrorReporter(context, jsd_ErrorReporter);

    /* add our printer */
    /* add our loader */
}

JSBool
jsd_SetErrorReporter(JSDContext*       jsdc, 
                     JSD_ErrorReporter reporter, 
                     void*             callerdata)
{
    JSD_LOCK();
    jsdc->errorReporter = reporter;
    jsdc->errorReporterData = callerdata;
    JSD_UNLOCK();
    return JS_TRUE;
}

JSBool
jsd_GetErrorReporter(JSDContext*        jsdc, 
                     JSD_ErrorReporter* reporter, 
                     void**             callerdata)
{
    JSD_LOCK();
    if( reporter )
        *reporter = jsdc->errorReporter;
    if( callerdata )
        *callerdata = jsdc->errorReporterData;
    JSD_UNLOCK();
    return JS_TRUE;
}
