/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#ifndef jslog_h___
#define jslog_h___

#include "jstypes.h"

JS_BEGIN_EXTERN_C

/*
/* Removed by JSIFY: ** prlog.h -- Declare interfaces to NSPR's Logging service */
#include "jsutil.h" /* Added by JSIFY */
**
** NSPR provides a logging service that is used by NSPR itself and is
** available to client programs.
**
** To use the service from a client program, you should create a
** PRLogModuleInfo structure by calling JS_NewLogModule(). After
** creating the LogModule, you can write to the log using the JS_LOG()
** macro.
**
** Initialization of the log service is handled by NSPR initialization.
**
** At execution time, you must enable the log service. To enable the
** log service, set the environment variable: NSPR_LOG_MODULES
** variable.
**
** NSPR_LOG_MODULES variable has the form:
**
**     <moduleName>:<value>[, <moduleName>:<value>]*
**
** Where:
**  <moduleName> is the name passed to JS_NewLogModule().
**  <value> is a numeric constant, e.g. 5. This value is the maximum
** value of a log event, enumerated by PRLogModuleLevel, that you want
** written to the log.
** 
** For example: to record all events of greater value than or equal to
** JS_LOG_ERROR for a LogModule names "gizmo", say:
** 
** set NSPR_LOG_MODULES=gizmo:2
** 
** Note that you must specify the numeric value of JS_LOG_ERROR.
** 
** Special LogModule names are provided for controlling NSPR's log
** service at execution time. These controls should be set in the
** NSPR_LOG_MODULES environment variable at execution time to affect
** NSPR's log service for your application.
** 
** The special LogModule "all" enables all LogModules. To enable all
** LogModule calls to JS_LOG(), say:
** 
** set NSPR_LOG_MODULES=all:5
** 
** The special LogModule name "sync" tells the NSPR log service to do
** unbuffered logging.
** 
** The special LogModule name "buffsize:<size>" tells NSPR to set the
** log buffer to <size>.
**
** The environment variable NSPR_LOG_FILE specifies the log file to use
** unless the default of "stderr" is acceptable.
**
** To put log messages in your programs, use the JS_LOG macro:
**
**     JS_LOG(<module>, <level>, (<printfString>, <args>*));
**
** Where <module> is the address of a PRLogModuleInfo structure, and
** <level> is one of the levels defined by the enumeration:
** PRLogModuleLevel. <args> is a printf() style of argument list. That
** is: (fmtstring, ...).
**
** Example:
** 
** main() {
**    JSIntn one = 1;
**    PRLogModuleInfo * myLm = JS_NewLogModule("gizmo");
**    JS_LOG( myLm, JS_LOG_ALWAYS, ("Log this! %d\n", one)); 
**    return; 
** }
** 
** Note the use of printf() style arguments as the third agrument(s) to
** JS_LOG().
** 
** After compiling and linking you application, set the environment:
** 
** SET NSPR_LOGMODULES=gizmo:5
** SET NSPR_LOG_FILE=logfile.txt
** 
** When you execute your application, the string "Log this! 1" will be
** written to the file "logfile.txt".
** 
** Note to NSPR engineers: a number of PRLogModuleInfo structures are
** defined and initialized in prinit.c. See this module for ideas on
** what to log where.
** 
*/

typedef enum PRLogModuleLevel {
    JS_LOG_NONE = 0,                /* nothing */
    JS_LOG_ALWAYS = 1,              /* always printed */
    JS_LOG_ERROR = 2,               /* error messages */
    JS_LOG_WARNING = 3,             /* warning messages */
    JS_LOG_DEBUG = 4,               /* debug messages */

    JS_LOG_NOTICE = JS_LOG_DEBUG,   /* notice messages */
    JS_LOG_WARN = JS_LOG_WARNING,   /* warning messages */
    JS_LOG_MIN = JS_LOG_DEBUG,      /* minimal debugging messages */
    JS_LOG_MAX = JS_LOG_DEBUG       /* maximal debugging messages */
} PRLogModuleLevel;

/*
** One of these structures is created for each module that uses logging.
**    "name" is the name of the module
**    "level" is the debugging level selected for that module
*/
typedef struct PRLogModuleInfo {
    const char *name;
    PRLogModuleLevel level;
    struct PRLogModuleInfo *next;
} PRLogModuleInfo;

/*
** Create a new log module.
*/
EXTERN(PRLogModuleInfo*) JS_NewLogModule(const char *name);

/*
** Set the file to use for logging. Returns JS_FALSE if the file cannot
** be created
*/
EXTERN(JSBool) JS_SetLogFile(const char *name);

/*
** Set the size of the logging buffer. If "buffer_size" is zero then the
** logging becomes "synchronous" (or unbuffered).
*/
EXTERN(void) JS_SetLogBuffering(JSIntn buffer_size);

/*
** Print a string to the log. "fmt" is a PR_snprintf format type. All
** messages printed to the log are preceeded by the name of the thread
** and a time stamp. Also, the routine provides a missing newline if one
** is not provided.
*/
EXTERN(void) JS_LogPrint(const char *fmt, ...);

/*
** Flush the log to its file.
*/
EXTERN(void) JS_LogFlush(void);

/*
** Windoze 16 can't support a large static string space for all of the
** various debugging strings so logging is not enabled for it.
*/
#if (defined(DEBUG) || defined(FORCE_PR_LOG)) && !defined(WIN16)
#define JS_LOGGING 1

#define JS_LOG_TEST(_module,_level) \
    ((_module)->level >= (_level))

/*
** Log something.
**    "module" is the address of a PRLogModuleInfo structure
**    "level" is the desired logging level
**    "args" is a variable length list of arguments to print, in the following
**       format:  ("printf style format string", ...)
*/
#define JS_LOG(_module,_level,_args)     \
    JS_BEGIN_MACRO             \
      if (JS_LOG_TEST(_module,_level)) { \
      JS_LogPrint _args;         \
      }                     \
    JS_END_MACRO

#else /* (defined(DEBUG) || defined(FORCE_PR_LOG)) && !defined(WIN16) */

#undef JS_LOGGING
#define JS_LOG_TEST(module,level) 0
#define JS_LOG(module,level,args)

#endif /* (defined(DEBUG) || defined(FORCE_PR_LOG)) && !defined(WIN16) */

#ifndef NO_NSPR_10_SUPPORT

#ifdef JS_LOGGING
#define JS_LOG_BEGIN    JS_LOG
#define JS_LOG_END      JS_LOG
#define JS_LOG_DEFINE   JS_NewLogModule
#else
#define JS_LOG_BEGIN(module,level,args)
#define JS_LOG_END(module,level,args)
#define JS_LOG_DEFINE(_name)    NULL
#endif /* JS_LOGGING */

#endif /* NO_NSPR_10_SUPPORT */

#ifdef DEBUG

EXTERN(void) JS_Assert(const char *s, const char *file, JSIntn ln);
#define JS_ASSERT(_expr) \
    ((_expr)?((void)0):JS_Assert(# _expr,__FILE__,__LINE__))

#define JS_NOT_REACHED(_reasonStr) \
    JS_Assert(_reasonStr,__FILE__,__LINE__)

#else

#define JS_ASSERT(expr) ((void) 0)
#define JS_NOT_REACHED(reasonStr)

#endif /* defined(DEBUG) */

JS_END_EXTERN_C

#endif /* jslog_h___ */
