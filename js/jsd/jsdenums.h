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
 * JavaScript Debugging support - enums shared by jsdebug.h and jsdxpcom.h apis
 */

#ifndef jsdenums_h___
#define jsdenums_h___

/* these coorespond to netscape.jsdebug.SourceTextItem.java values -
*  change in both places if anywhere
*/

typedef enum JSDSourceStatus
{
    JSD_SOURCE_INITED       = 0, /* initialized, but contains no source yet */
    JSD_SOURCE_PARTIAL      = 1, /* some source loaded, more expected */
    JSD_SOURCE_COMPLETED    = 2, /* all source finished loading */
    JSD_SOURCE_ABORTED      = 3, /* user aborted loading, some may be loaded */
    JSD_SOURCE_FAILED       = 4, /* loading failed, some may be loaded */
    JSD_SOURCE_CLEARED      = 5  /* text has been cleared by debugger */
} JSDSourceStatus;

/* possible 'type' params for JSD_ExecutionHookProc */
typedef enum JSDHookType
{
    JSD_HOOK_INTERRUPTED      = 0,
    JSD_HOOK_BREAKPOINT       = 1,
    JSD_HOOK_DEBUG_REQUESTED  = 2,
    JSD_HOOK_DEBUGGER_KEYWORD = 3,
    JSD_HOOK_THROW            = 4
} JSDHookType;

/* legal return values for JSD_ExecutionHookProc */
typedef enum JSDHookResult
{
    JSD_HOOK_RETURN_HOOK_ERROR     = 0,
    JSD_HOOK_RETURN_CONTINUE       = 1,
    JSD_HOOK_RETURN_ABORT          = 2,
    JSD_HOOK_RETURN_RET_WITH_VAL   = 3,
    JSD_HOOK_RETURN_THROW_WITH_VAL = 4,
    JSD_HOOK_RETURN_CONTINUE_THROW = 5
} JSDHookResult;

/* legal return values for JSD_ErrorReporter */
typedef enum JSDErrorReporterResult
{
    JSD_ERROR_REPORTER_PASS_ALONG   = 0, /* pass along to regular reporter */
    JSD_ERROR_REPORTER_RETURN       = 1, /* don't pass to error reporter */
    JSD_ERROR_REPORTER_DEBUG        = 2, /* force call to DebugBreakHook */
    JSD_ERROR_REPORTER_CLEAR_RETURN = 3  /* clear exception and don't pass */
} JSDErrorReporterResult;

/* possible or'd together bitflags returned by JSD_GetPropertyFlags
 *
 * XXX these must stay the same as the JSPD_ flags in jsdbgapi.h
 */
typedef enum JSDPropertyFlags
{
    JSDPD_ENUMERATE  = 0x01, /* visible to for/in loop */
    JSDPD_READONLY   = 0x02, /* assignment is error */
    JSDPD_PERMANENT  = 0x04, /* property cannot be deleted */
    JSDPD_ALIAS      = 0x08, /* property has an alias id */
    JSDPD_ARGUMENT   = 0x10, /* argument to function */
    JSDPD_VARIABLE   = 0x20, /* local variable in function */
/* this is not one of the JSPD_ flags in jsdbgapi.h  - careful not to overlap*/
    JSDPD_HINTED     = 0x800 /* found via explicit lookup */
} JSDPropertyFlags;


#endif /* jsdenums_h___ */


