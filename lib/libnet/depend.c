/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *
 * This file provides stubs for all external APIs used by Netlib
 * which are not client specific (ie. MOZILLA_CLIENT).
 *
 */

/* #define USE_JS_STUBS */
#include "depend.h"

#include "nspr.h"
#include "net.h"
#include "pwcacapi.h"
#include "prefapi.h"
#include "xp_file.h"

#include "secnav.h"

#include "xp_reg.h"
#include "libi18n.h"
#include "libevent.h"
#include "mkgeturl.h"

/*
 *---------------------------------------------------------------------------
 * These protocol initialization routines are stubs for the protocols which
 * could be cleanly removed...  (see makefile.win)
 *---------------------------------------------------------------------------
 */

MODULE_PRIVATE void
NET_InitGopherProtocol(void)
{
}

MODULE_PRIVATE void
NET_InitFTPProtocol(void)
{
}

MODULE_PRIVATE void
NET_InitMochaProtocol(void)
{
}

MODULE_PRIVATE void
NET_InitRemoteProtocol(void)
{
}

MODULE_PRIVATE void
NET_InitDataURLProtocol(void)
{
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmisc/glhist.c
 *---------------------------------------------------------------------------
 */

PUBLIC void
GH_UpdateGlobalHistory(URL_Struct * URL_s)
{
    MOZ_FUNCTION_STUB;
}


/* clear the global history list
 */
PUBLIC void
GH_ClearGlobalHistory(void)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmisc/shist.c
 *---------------------------------------------------------------------------
 */


PUBLIC History_entry *
SHIST_GetCurrent(History * hist)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libpwcac/pwcacapi.c
 *---------------------------------------------------------------------------
 */

/* returns value for a given name
 */
char *
PC_FindInNameValueArray(PCNameValueArray *array, char *name)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* returns a PCNameValueArray from serialized char data.
 *
 * returns NULL on error
 */
PUBLIC PCNameValueArray *
PC_CharToNameValueArray(char *data, int32 len)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PUBLIC int
PC_PromptUsernameAndPassword(MWContext *context,
                             char *prompt,
                             char **username,
                             char **password,
                             XP_Bool *remember,
                             XP_Bool is_secure)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


PUBLIC char *
PC_PromptPassword(MWContext *context,
							 char *prompt,
							 XP_Bool *remember,
							 XP_Bool is_secure)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* takes a key string as input and returns a name value array
 *
 * A module name is also passed in to guarentee that a key from
 * another module is never returned by an accidental key match.
 */
PUBLIC PCNameValueArray *
PC_CheckForStoredPasswordArray(char *module, char *key)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/* stores a name value array in the password database
 * returns 0 on success
 */
PUBLIC int
PC_StorePasswordNameValueArray(char *module, char *key, PCNameValueArray *array)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/* adds to end of name value array 
 *
 * Possible to add duplicate names with this
 */
PUBLIC int
PC_AddToNameValueArray(PCNameValueArray *array, char *name, char *value)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


PUBLIC PCNameValueArray *
PC_NewNameValueArray()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PUBLIC void
PC_FreeNameValueArray(PCNameValueArray *array)
{
    MOZ_FUNCTION_STUB;
}



/* returns 0 on success else -1 
 */
PUBLIC int
PC_DeleteStoredPassword(char *module, char *key)
{
    MOZ_FUNCTION_STUB;
    return -1;
}


/* returns 0 on success -1 on error */
PUBLIC int
PC_RegisterDataInterpretFunc(char *module, PCDataInterpretFunc *func)
{
    MOZ_FUNCTION_STUB;
    return -1;
}

/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/fegui.cpp
 *---------------------------------------------------------------------------
 */

/*
//
// Open a file with the given name
// If a special file type is provided we might need to get the name
//  out of the preferences list
//
*/
PUBLIC XP_File 
XP_FileOpen(const char * name, XP_FileType type, const XP_FilePerm perm)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
//
// Return 0 on success, -1 on failure.  
//
*/
PUBLIC int 
XP_FileRemove(const char * name, XP_FileType type)
{
    MOZ_FUNCTION_STUB;
    return -1;
}

/*
//
// Mimic unix stat call
// Return -1 on error
//
*/
PUBLIC int 
XP_Stat(const char * name, XP_StatStruct * info, XP_FileType type)
{
    MOZ_FUNCTION_STUB;
    return -1;
}

/*
// The caller is responsible for XP_FREE()ing the return string
*/
PUBLIC char *
WH_FileName (const char *name, XP_FileType type)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


char *
WH_TempName(XP_FileType type, const char * prefix)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PUBLIC XP_Dir 
XP_OpenDir(const char * name, XP_FileType type)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}

/*
//
// Close the directory
//
*/

#ifdef XP_PC
PUBLIC void 
XP_CloseDir(XP_Dir dir)
{
    MOZ_FUNCTION_STUB;
}

PUBLIC XP_DirEntryStruct * 
XP_ReadDir(XP_Dir dir)
{                                         
    MOZ_FUNCTION_STUB;
    return NULL;
}
#endif



PUBLIC void *
FE_AboutData (const char *which,
              char **data_ret, int32 *length_ret, char **content_type_ret)
{
    MOZ_FUNCTION_STUB;

    *data_ret = NULL;
    *length_ret = 0;
    *content_type_ret = NULL;

    return NULL;
}


PUBLIC void
FE_FreeAboutData (void * data, const char* which2)
{
    MOZ_FUNCTION_STUB;
}


PUBLIC void 
FE_Trace(const char * msg)
{
    /* This used to be just TRACE(msg), but that's a mistake -- 
     * if msg happens to have a '%' in it, TRACE will
     * interpret the '%' as the beginning of an argument to be
     * printf'd and look up the stack and try to
     * interpret random data as the argument. At a minimum this
     * leads to corrupt trace info. Usually it causes bus errors.
     */
	printf("%s", msg);
/*  MOZ_FUNCTION_STUB; */
}

/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/fenet.cpp
 *---------------------------------------------------------------------------
 */

int FE_AsyncDNSLookup(MWContext *context, char * host_port, PRHostEnt ** hoststruct_ptr_ptr, PRFileDesc *socket)
{
    MOZ_FUNCTION_STUB;
    return -1;
}

/*
// INTL_ResourceCharSet(void)
//
*/
char *INTL_ResourceCharSet(void)
{
    MOZ_FUNCTION_STUB;

    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/cfe.cpp
 *---------------------------------------------------------------------------
 */

void FE_Alert(MWContext *pContext, const char *pMsg)
{
    MOZ_FUNCTION_STUB;
}


int32 FE_GetContextID(MWContext *pContext)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


XP_Bool FE_IsNetcasterInstalled(void) 
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}

/*
//  A Url has changed context.
//  We need to mark it in the new context if it has ncapi_data (which we use
//  to track such things under windows).
*/
void FE_UrlChangedContext(URL_Struct *pUrl, MWContext *pOldContext, MWContext *pNewContext)
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/urlecho.cpp
 *---------------------------------------------------------------------------
 */
void FE_URLEcho(URL_Struct *pURL, int iStatus, MWContext *pContext)  {
/*
//  Purpose:  Echo the URL to all appropriately registered applications that are monitoring such URL traffic.
//  Arguments:  pURL  The URL which is being loaded.
//        iStatus The status of the load.
//        pContext  The context in which the load occurred.
//  Returns:  void
//  Comments: Well, sometimes there isn't a window in which the load occurred, what to do then?
//        Just don't report.
//  Revision History:
//    01-18-95  created GAB
//
*/
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/edview2.cpp
 *---------------------------------------------------------------------------
 */
/*
// Note: This is used by Navigator's HTP UPLOAD as well as Composer's file saving
//  DON'T ASSUME ANY EDITOR FUNCTIONALITY!
// Dialog to give feedback and allow canceling, overwrite protection
//   when downloading remote files 
//
*/
void FE_SaveDialogCreate( MWContext *pMWContext, int iFileCount, ED_SaveDialogType saveType  )
{
    MOZ_FUNCTION_STUB;
}


void FE_SaveDialogSetFilename( MWContext *pMWContext, char *pFilename )
{
    MOZ_FUNCTION_STUB;
}


void FE_SaveDialogDestroy( MWContext *pMWContext, int status, char *pFileURL )
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/compmapi.cpp
 *---------------------------------------------------------------------------
 */
void FE_AlternateCompose(
    char * from, char * reply_to, char * to, char * cc, char * bcc,
    char * fcc, char * newsgroups, char * followup_to,
    char * organization, char * subject, char * references,
    char * other_random_headers, char * priority,
    char * attachment, char * newspost_url, char * body)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/nethelp.cpp
 *---------------------------------------------------------------------------
 */
MWContext *FE_GetNetHelpContext()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}

/*
// Called from mkhelp.c to get the standard location of the NetHelp folder as a URL
*/
char * FE_GetNetHelpDir()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/femess.cpp
 *---------------------------------------------------------------------------
 */
int FE_GetURL(MWContext *pContext, URL_Struct *pUrl)
{
    MOZ_FUNCTION_STUB;

    return -1;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/fmabstra.cpp
 *---------------------------------------------------------------------------
 */
void FE_RaiseWindow(MWContext *pContext)
{
    MOZ_FUNCTION_STUB;
}




/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/fegrid.cpp
 *---------------------------------------------------------------------------
 */
/*
//  Create a new window.
//  If pChrome is NULL, do a FE_MakeBlankWindow....
//  pChrome specifies the attributes of a window.
//  If you use this call, Toolbar information will not be saved in the preferences.
*/
MWContext *FE_MakeNewWindow(MWContext *pOldContext, URL_Struct *pUrl, char *pContextName, Chrome *pChrome)
{
    MOZ_FUNCTION_STUB;

    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/cmd/winfe/timer.cpp
 *---------------------------------------------------------------------------
 */
/* this function should register a function that will
 * be called after the specified interval of time has
 * elapsed.  This function should return an id 
 * that can be passed to FE_ClearTimeout to cancel 
 * the Timeout request.
 *
 * A) Timeouts never fail to trigger, and
 * B) Timeouts don't trigger *before* their nominal timestamp expires, and
 * C) Timeouts trigger in the same ordering as their timestamps
 *
 * After the function has been called it is unregistered
 * and will not be called again unless re-registered.
 *
 * func:    The function to be invoked upon expiration of
 *          the Timeout interval 
 * closure: Data to be passed as the only argument to "func"
 * msecs:   The number of milli-seconds in the interval
 */
PUBLIC void * 
FE_SetTimeout(TimeoutCallbackFunction func, void * closure, uint32 msecs)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/et_moz.c
 *---------------------------------------------------------------------------
 */

JSBool
ET_PostMessageBox(MWContext* context, char* szMessage, JSBool bConfirm)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/et_mocha.c
 *---------------------------------------------------------------------------
 */
/*
 * A mocha stream from netlib has compeleted, eveluate the contents
 *   and pass them up our stream.  We will take ownership of the 
 *   buf argument and are responsible for freeing it
 */
void
ET_MochaStreamComplete(MWContext * pContext, void * buf, int len, 
                       char *content_type, Bool isUnicode)
{
    MOZ_FUNCTION_STUB;
}


/*
 * A mocha stream from netlib has aborted
 */
void
ET_MochaStreamAbort(MWContext * context, int status)
{
    MOZ_FUNCTION_STUB;
}


/*
 * Evaluate the given script.  I'm sure this is going to need a
 *   callback or compeletion routine
 */
void
ET_EvaluateScript(MWContext * pContext, char * buffer, ETEvalStuff * stuff,
                  ETEvalAckFunc fn)
{
    MOZ_FUNCTION_STUB;
}


void
ET_SetDecoderStream(MWContext * pContext, NET_StreamClass *stream,
                    URL_Struct *url_struct, JSBool free_stream_on_close)
{
    MOZ_FUNCTION_STUB;
}


/*
 * Tell the backend about a new load event.
 */
void
ET_SendLoadEvent(MWContext * pContext, int32 type, ETVoidPtrFunc fnClosure,
                 NET_StreamClass *stream, int32 layer_id, Bool resize_reload)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/lm_init.c
 *---------------------------------------------------------------------------
 */

void
LM_PutMochaDecoder(MochaDecoder *decoder)
{
    MOZ_FUNCTION_STUB;
}


MochaDecoder *
LM_GetMochaDecoder(MWContext *context)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 * Release the JSLock
 */
void PR_CALLBACK
LM_UnlockJS()
{
    MOZ_FUNCTION_STUB;
}


/*
 * Try to get the JSLock but just return JS_FALSE if we can't
 *   get it, don't wait since we could deadlock
 */
JSBool PR_CALLBACK
LM_AttemptLockJS(JSLockReleaseFunc fn, void * data)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


JSBool PR_CALLBACK
LM_ClearAttemptLockJS(JSLockReleaseFunc fn, void * data)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/lm_doc.c
 *---------------------------------------------------------------------------
 */

NET_StreamClass *
LM_WysiwygCacheConverter(MWContext *context, URL_Struct *url_struct,
                         const char * wysiwyg_url, const char * base_href)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/lm_win.c
 *---------------------------------------------------------------------------
 */
/*
 * Entry point for front-ends to notify JS code of help events.
 */
void
LM_SendOnHelp(MWContext *context)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libmocha/lm_taint.c
 *---------------------------------------------------------------------------
 */
char lm_unknown_origin_str[] = "[unknown origin]";

JSPrincipals *
LM_NewJSPrincipals(URL_Struct *archive, char *id, const char *codebase)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/util/algrand.c
 *---------------------------------------------------------------------------
 */
SECStatus
RNG_GenerateGlobalRandomBytes(void *data, size_t bytes)
{
    MOZ_FUNCTION_STUB;
    return SECFailure;
}


/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/cert/pcertdb.c
 *---------------------------------------------------------------------------
 */

void
CERT_DestroyCertificate(CERTCertificate *cert)
{
    MOZ_FUNCTION_STUB;
}


CERTCertificate *
CERT_DupCertificate(CERTCertificate *c)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



/*
 *---------------------------------------------------------------------------
 * From ns/security/lib/cert/certdb.c
 *---------------------------------------------------------------------------
 */

CERTCertDBHandle *
CERT_GetDefaultCertDB(void)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}



PRBool
SECNAV_SecurityDialog(MWContext *context, int state)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


/*
 *---------------------------------------------------------------------------
 * From ???
 *---------------------------------------------------------------------------
 */

SECStatus
SECNAV_ComputeFortezzaProxyChallengeResponse(MWContext *context,
					     char *asciiChallenge,
					     char **signature_out,
					     char **clientRan_out,
					     char **certChain_out)
{
    return(SECFailure);
}



#if defined(USE_JS_STUBS)
/*
 *---------------------------------------------------------------------------
 * From ns/js/src/jsapi.c
 *---------------------------------------------------------------------------
 */
PR_IMPLEMENT(JSBool)
JS_PropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    MOZ_FUNCTION_STUB;
    return JS_TRUE;
}


PR_IMPLEMENT(JSBool)
JS_EnumerateStub(JSContext *cx, JSObject *obj)
{
    MOZ_FUNCTION_STUB;
    return JS_TRUE;
}


PR_IMPLEMENT(JSBool)
JS_ResolveStub(JSContext *cx, JSObject *obj, jsval id)
{
    MOZ_FUNCTION_STUB;
    return JS_TRUE;
}


PR_IMPLEMENT(JSBool)
JS_ConvertStub(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(void)
JS_FinalizeStub(JSContext *cx, JSObject *obj)
{
    MOZ_FUNCTION_STUB;
}


PR_IMPLEMENT(JSBool)
JS_CallFunctionName(JSContext *cx, JSObject *obj, const char *name, uintN argc,
                    jsval *argv, jsval *rval)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


PR_IMPLEMENT(JSBool)
JS_EvaluateScript(JSContext *cx, JSObject *obj,
                  const char *bytes, uintN length,
                  const char *filename, uintN lineno,
                  jsval *rval)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(JSObject *)
JS_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSObject *)
JS_DefineObject(JSContext *cx, JSObject *obj, const char *name, JSClass *clasp,
                JSObject *proto, uintN flags)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSBool)
JS_DefineFunctions(JSContext *cx, JSObject *obj, JSFunctionSpec *fs)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(JSBool)
JS_DefineProperties(JSContext *cx, JSObject *obj, JSPropertySpec *ps)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(JSBool)
JS_AddRoot(JSContext *cx, void *rp)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(JSBool)
JS_RemoveRoot(JSContext *cx, void *rp)
{
    MOZ_FUNCTION_STUB;
    return JS_FALSE;
}


PR_IMPLEMENT(char *)
JS_GetStringBytes(JSString *str)
{
    MOZ_FUNCTION_STUB;
    return "";
}


PR_IMPLEMENT(JSString *)
JS_NewString(JSContext *cx, char *bytes, size_t length)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSString *)
JS_NewStringCopyZ(JSContext *cx, const char *s)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(void *)
JS_GetPrivate(JSContext *cx, JSObject *obj)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSBool)
JS_SetPrivate(JSContext *cx, JSObject *obj, void *data)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


PR_IMPLEMENT(void *)
JS_GetInstancePrivate(JSContext *cx, JSObject *obj, JSClass *clasp,
                      jsval *argv)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSBool)
JS_ValueToBoolean(JSContext *cx, jsval v, JSBool *bp)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


PR_IMPLEMENT(JSBool)
JS_ValueToInt32(JSContext *cx, jsval v, int32 *ip)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


PR_IMPLEMENT(JSString *)
JS_ValueToString(JSContext *cx, jsval v)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSErrorReporter)
JS_SetErrorReporter(JSContext *cx, JSErrorReporter er)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


PR_IMPLEMENT(JSBool)
JS_InitStandardClasses(JSContext *cx, JSObject *obj)
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


PR_IMPLEMENT(void)
JS_GC(JSContext *cx)
{
    MOZ_FUNCTION_STUB;
}

#endif /* !USE_JS_STUBS */

/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_reg.c
 *---------------------------------------------------------------------------
 */


PUBLIC int 
XP_RegExpMatch(char *str, char *xp, Bool case_insensitive) 
{
    MOZ_FUNCTION_STUB;
    return 1;
}


PUBLIC int 
XP_RegExpValid(char *exp) 
{
    MOZ_FUNCTION_STUB;
    return INVALID_SXP;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_trace.c
 *---------------------------------------------------------------------------
 */

/* Trace with trailing newline */
void XP_Trace (const char* message, ...)
{
    MOZ_FUNCTION_STUB;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xplocale.c
 *---------------------------------------------------------------------------
 */

const char* INTL_ctime(MWContext* context, time_t *date)
{
  static char result[40];	

  MOZ_FUNCTION_STUB;
  *result = '\0';
  return result;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_cntxt.c
 *---------------------------------------------------------------------------
 */

/*
 * Finds a context that should be loaded with the URL, given
 * a name and current (refering) context.
 *
 * If the context returned is not NULL, name is already assigned to context
 * structure. You should load the URL into this context.
 *
 * If you get back a NULL, you should create a new window
 *
 * Both context and current context can be null.
 * Once the grids are in, there should be some kind of a logic that searches
 * siblings first. 
 */

MWContext * XP_FindNamedContextInList(MWContext * context, char *name)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


MWContext*
XP_FindSomeContext()
{
    MOZ_FUNCTION_STUB;
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/xp/xp_time.c
 *---------------------------------------------------------------------------
 */
/* This parses a time/date string into a time_t
   (seconds after "1-Jan-1970 00:00:00 GMT")
   If it can't be parsed, 0 is returned.

   Many formats are handled, including:

     14 Apr 89 03:20:12
     14 Apr 89 03:20 GMT
     Fri, 17 Mar 89 4:01:33
     Fri, 17 Mar 89 4:01 GMT
     Mon Jan 16 16:12 PDT 1989
     Mon Jan 16 16:12 +0130 1989
     6 May 1992 16:41-JST (Wednesday)
     22-AUG-1993 10:59:12.82
     22-AUG-1993 10:59pm
     22-AUG-1993 12:59am
     22-AUG-1993 12:59 PM
     Friday, August 04, 1995 3:54 PM
     06/21/95 04:24:34 PM
     20/06/95 21:07
     95-06-08 19:32:48 EDT

  If the input string doesn't contain a description of the timezone,
  we consult the `default_to_gmt' to decide whether the string should
  be interpreted relative to the local time zone (FALSE) or GMT (TRUE).
  The correct value for this argument depends on what standard specified
  the time string which you are parsing.
 */
time_t
XP_ParseTimeString (const char *string, XP_Bool default_to_gmt)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/plugin/npglue.c
 *---------------------------------------------------------------------------
 */

/* 
 * This exit routine is used for all streams requested by the
 * plug-in: byterange request streams, NPN_GetURL streams, and 
 * NPN_PostURL streams.  NOTE: If the exit routine gets called
 * in the course of a context switch, we must NOT delete the
 * URL_Struct.  Example: FTP post with result from server
 * displayed in new window -- the exit routine will be called
 * when the upload completes, but before the new context to
 * display the result is created, since the display of the
 * results in the new context gets its own completion routine.
 */
void
NPL_URLExit(URL_Struct *urls, int status, MWContext *cx)
{
    MOZ_FUNCTION_STUB;
}



/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/layedit.c
 *---------------------------------------------------------------------------
 */

void LO_SetBaseURL( MWContext *context, char *pURL )
{
    MOZ_FUNCTION_STUB;
}


#if 0
/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/laysel.c
 *---------------------------------------------------------------------------
 */
/*
    get first(last) if current element is NULL.
*/
Bool
LO_getNextTabableElement( MWContext *context, LO_TabFocusData *pCurrentFocus, int forward )
{
    MOZ_FUNCTION_STUB;
    return FALSE;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/layout/laygrid.c
 *---------------------------------------------------------------------------
 */
lo_GridCellRec *
lo_ContextToCell(MWContext *context, Bool reconnect, lo_GridRec **grid_ptr)
{
    MOZ_FUNCTION_STUB;
    return NULL;
}

#endif 

/*
 *---------------------------------------------------------------------------
 * From ns/modules/libfont/src/wfStream.cpp
 *---------------------------------------------------------------------------
 */

char *
/*ARGSUSED*/
NF_AboutFonts(MWContext *context, const char *which)
{
    MOZ_FUNCTION_STUB;

    return NULL;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/csnamefn.c
 *---------------------------------------------------------------------------
 */
int16
INTL_CharSetNameToID(char	*charset)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


/*
 *---------------------------------------------------------------------------
 * From ns/lib/libi18n/ucs2.c
 *---------------------------------------------------------------------------
 */
uint32    INTL_TextToUnicode(
    INTL_Encoding_ID encoding,
    unsigned char*   src,
    uint32           srclen,
    INTL_Unicode*    ustr,
    uint32           ubuflen
)
{
    MOZ_FUNCTION_STUB;
    return 0;
}


uint32    INTL_TextToUnicodeLen(
    INTL_Encoding_ID encoding,
    unsigned char*   src,
    uint32           srclen
)
{
    MOZ_FUNCTION_STUB;
    return 0;
}





/*
 * Random libnet functions...
 */
#ifdef DEBUG
MODULE_PRIVATE void
NET_DisplayStreamInfoAsHTML(ActiveEntry *cur_entry)
{
	MOZ_FUNCTION_STUB;
}
#endif /* DEBUG */



PUBLIC int
NET_ParseNetHelpURL(URL_Struct *URL_s)
{
	MOZ_FUNCTION_STUB;
/*	return MK_OUT_OF_MEMORY; */
	return -207;
}


/* Enable or disable the prefetching, called from NET_SetupPrefs in mkgeturl.c */
PUBLIC void
PRE_Enable(XP_Bool enabled)
{
	MOZ_FUNCTION_STUB;
}



CERTCertificate *
SSL_PeerCertificate(PRFileDesc *fd)
{
	MOZ_FUNCTION_STUB;
    return(NULL);
}


int
SSL_SetSockPeerID(PRFileDesc *fd, char *peerID)
{
	MOZ_FUNCTION_STUB;
    return(0);
}


int
SSL_SecurityStatus(PRFileDesc *fd, int *on, char **cipher,
		   int *keySize, int *secretKeySize,
		   char **issuer, char **subject)
{
	MOZ_FUNCTION_STUB;
    return(0);
}


void
SECNAV_HTTPHead(PRFileDesc *fd)
{
	MOZ_FUNCTION_STUB;
}


void
SECNAV_Posting(PRFileDesc *fd)
{
	MOZ_FUNCTION_STUB;
}


PRBool
CERT_CompareCertsForRedirection(CERTCertificate *c1, CERTCertificate *c2)
{
	MOZ_FUNCTION_STUB;
    return(PR_FALSE);
}


