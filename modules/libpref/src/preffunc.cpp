/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/********************************************************************/
 

#include "jsapi.h"

#include "xp_core.h"
#include "xp_mcom.h"
#include "xp_qsort.h"
#include "xp_file.h"
#include <errno.h>

#include "prefldap.h"
#include "prefapi.h"
#include "prefpriv.h"

#include "cprofile.h"

#if defined(XP_MAC) || defined(XP_UNIX)
#include "fe_proto.h"
#endif
#ifdef XP_WIN
#include "npapi.h"
#include "assert.h"

#define NOT_NULL(X)	X
#ifdef XP_ASSERT
#undef XP_ASSERT
#endif

#define XP_ASSERT(X) assert(X)

#define LINEBREAK "\n"
#endif

#include "sechash.h"
#ifndef NSPR20
#include "prhash.h"
#else
#include "plhash.h"
#include "prenv.h"
#endif
#include "prsystem.h"

JSTaskState *			m_mochaTaskState = NULL;
JSContext *				m_mochaContext = NULL;
JSObject *				m_mochaPrefObject = NULL;
JSObject *			    m_GlobalConfigObject = NULL;

static CProfile             *m_profile = NULL; 
static struct CallbackNode*	m_Callbacks = NULL;
static XP_Bool				m_ErrorOpeningUserPrefs = FALSE;

#ifdef MOZ_LI
static char *				m_lifilename = NULL;
#endif

/* static XP_Bool				m_IsAnyPrefLocked = FALSE; */
/* static PRHashTable*			m_HashTable = NULL; */
static char *               m_SavedLine = NULL;       

#ifdef OSF1
static JSBool pref_HashJSPref(unsigned int argc, jsval *argv, PrefAction action);
#else
JSBool pref_HashJSPref(unsigned int argc, jsval *argv, PrefAction action);
#endif

#define LISTPREF_SEPARATOR      ","


#include "prlink.h"
extern PRLibrary *pref_LoadAutoAdminLib(void);
PRLibrary *m_AutoAdminLib = NULL;

/* -- Prototypes */
int pref_OpenFile(const char* filename, XP_Bool is_error_fatal, XP_Bool verifyHash, XP_Bool bGlobalContext,
                  XP_Bool skipFirstLine);
XP_Bool pref_VerifyLockFile(char* buf, long buflen);

void PREF_SetCallbacksStatus( XP_Bool status );

JSBool PR_CALLBACK pref_BranchCallback(JSContext *cx, JSScript *script);
void pref_ErrorReporter(JSContext *cx, const char *message,JSErrorReport *report);
void pref_Alert(char* msg);

/* -- Platform specific function extern */
extern JSBool pref_InitInitialObjects(void);

#ifdef XP_WIN
extern "C" void WinInitPrefs(void);
#endif

PRIVATE JSClass global_class = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

JSBool PR_CALLBACK pref_NativeDefaultPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeUserPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeLockPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeUnlockPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeSetConfig(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeGetPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeGetLDAPAttr(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeGetenv(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativePutenv(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
#ifdef MOZ_LI
JSBool PR_CALLBACK pref_NativeLILocalPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeLIUserPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
JSBool PR_CALLBACK pref_NativeLIDefPref(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval);
#endif

PRIVATE JSFunctionSpec autoconf_methods[] = {
    { "pref",				pref_NativeDefaultPref,	2 },
    { "defaultPref",		pref_NativeDefaultPref,	2 },
    { "user_pref",			pref_NativeUserPref,	2 },
    { "lockPref",			pref_NativeLockPref,	2 },
    { "unlockPref",			pref_NativeUnlockPref,	1 },
    { "config",				pref_NativeSetConfig,	2 },
    { "getPref",			pref_NativeGetPref,		1 },
    { "getLDAPAttributes",	pref_NativeGetLDAPAttr, 4 },
    { "getenv",				pref_NativeGetenv, 		1 },
    { "putenv",				pref_NativePutenv, 		1 },
#ifdef MOZ_LI
    { "localPref",			pref_NativeLILocalPref,	1 },
    { "localUserPref",		pref_NativeLIUserPref,	2 },
    { "localDefPref",		pref_NativeLIDefPref,	2 },
#else
    { "localDefPref",		pref_NativeDefaultPref,	2 },
#endif
    { NULL,                 NULL,                   0 }
};

PRIVATE JSPropertySpec autoconf_props[] = {
    {0}
};

PRIVATE JSClass autoconf_class = {
    "PrefConfig", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

int pref_OpenFile(const char* filename, XP_Bool is_error_fatal, XP_Bool verifyHash, XP_Bool bGlobalContext,
                  XP_Bool skipFirstLine)
{
	int ok = PREF_ERROR;
	XP_File fp;
	XP_StatStruct stats;
	long fileLength;

	stats.st_size = 0;

#ifdef XP_WIN
	if ( stat(filename, (struct stat *) &stats) == -1)
#else
	if ( XP_Stat(filename, &stats, xpUserPrefs) == -1)
#endif
		return PREF_ERROR;

	fileLength = stats.st_size;
	if (fileLength <= 1)
		return PREF_ERROR;
	fp = fopen(filename, "r");

	if (fp) {	
		char* readBuf = (char *) malloc(fileLength * sizeof(char));
		if (readBuf) {
			fileLength = XP_FileRead(readBuf, fileLength, fp);

			if ( verifyHash && pref_VerifyLockFile(readBuf, fileLength) == FALSE )
			{
				ok = PREF_BAD_LOCKFILE;
			}
			else if ( PREF_EvaluateConfigScript(readBuf, fileLength,
						filename, bGlobalContext, FALSE, skipFirstLine ) == JS_TRUE )
			{
				ok = PREF_NOERROR;
			}
			free(readBuf);
		}
		XP_FileClose(fp);
		
		/* If the user prefs file exists but generates an error,
		   don't clobber the file when we try to save it. */
		if ((!readBuf || ok != PREF_NOERROR) && is_error_fatal)
			m_ErrorOpeningUserPrefs = TRUE;
#ifdef XP_WIN
		if (m_ErrorOpeningUserPrefs && is_error_fatal)
			MessageBox(NULL,"Error in preference file (prefs.js).  Default preferences will be used.","Netscape - Warning", MB_OK);
#endif
	}
	JS_GC(m_mochaContext);
	return (ok);
}

/* Computes the MD5 hash of the given buffer (not including the first line)
   and verifies the first line of the buffer expresses the correct hash in the form:
   // xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx xx
   where each 'xx' is a hex value. */
XP_Bool pref_VerifyLockFile(char* buf, long buflen)
{
	XP_Bool success = FALSE;
	const int obscure_value = 7;
	const long hash_length = 51;		/* len = 48 chars of MD5 + // + EOL */
    unsigned char digest[16];
    char szHash[64];

	/* Unobscure file by subtracting some value from every char. */
	long i;
	for (i = 0; i < buflen; i++) {
		buf[i] -= obscure_value;
	}

    if (buflen >= hash_length) {
    	const unsigned char magic_key[] = "VonGloda5652TX75235ISBN";
	    unsigned char *pStart = (unsigned char*) buf + hash_length;
	    unsigned int len;
	    
	    MD5Context * md5_cxt = MD5_NewContext();
		MD5_Begin(md5_cxt);
		
		/* start with the magic key */
		MD5_Update(md5_cxt, magic_key, sizeof(magic_key));

		MD5_Update(md5_cxt, pStart, buflen - hash_length);
		
		MD5_End(md5_cxt, digest, &len, 16);
		
		MD5_DestroyContext(md5_cxt, PR_TRUE);
		
	    XP_SPRINTF(szHash, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
	        (int)digest[0],(int)digest[1],(int)digest[2],(int)digest[3],
	        (int)digest[4],(int)digest[5],(int)digest[6],(int)digest[7],
	        (int)digest[8],(int)digest[9],(int)digest[10],(int)digest[11],
	        (int)digest[12],(int)digest[13],(int)digest[14],(int)digest[15]);

		success = ( strncmp((const char*) buf + 3, szHash, hash_length - 4) == 0 );
	}
	
	return success;
}

PR_IMPLEMENT(int)
PREF_ReadUserJSFile(char *filename)
{
	int ok = pref_OpenFile(filename, FALSE, FALSE, TRUE, FALSE);

	return ok;
}

#ifdef MOZ_LI
PR_IMPLEMENT(int)
PREF_ReadLIJSFile(char *filename)
{
	int ok;

    if (filename) m_lifilename = strdup(filename);

	ok = pref_OpenFile(filename, FALSE, FALSE, FALSE, FALSE);

	return ok;
}
#endif

PR_IMPLEMENT(void *)
PREF_Init()
{
    JSBool ok = JS_TRUE;
	m_profile = new CProfile();

    if (!m_mochaTaskState)
		m_mochaTaskState = JS_Init((uint32) 0xffffffffL);

    if (!m_mochaContext) {
		m_mochaContext = JS_NewContext(m_mochaTaskState, 8192);  /* ???? What size? */
		if (!m_mochaContext) {
			return NULL;
		}

		JS_SetVersion(m_mochaContext, JSVERSION_1_2);

		m_GlobalConfigObject = JS_NewObject(m_mochaContext, &global_class, NULL, NULL);
		if (!m_GlobalConfigObject) 
		    return NULL;

		if (!JS_InitStandardClasses(m_mochaContext, m_GlobalConfigObject))
		    return NULL;

		JS_SetBranchCallback(m_mochaContext, pref_BranchCallback);
		JS_SetErrorReporter(m_mochaContext, NULL);

		m_mochaPrefObject = JS_DefineObject(m_mochaContext, m_GlobalConfigObject, 
						    "PrefConfig",
						    &autoconf_class, 
						    NULL, 
						    JSPROP_ENUMERATE|JSPROP_READONLY);
		
		if (m_mochaPrefObject) {
		    if (!JS_DefineProperties(m_mochaContext,
					     m_mochaPrefObject,
					     autoconf_props)) {
			return NULL;
		    }

		    if (!JS_DefineFunctions(m_mochaContext,
					    m_mochaPrefObject,
					    autoconf_methods)) {
			return NULL;
		    }

		}

#ifdef XP_WIN
        WinInitPrefs();
#else
		ok = pref_InitInitialObjects();
#endif
	}

    if (!ok) {
        return NULL;
    }

    return m_profile;
}

PR_IMPLEMENT(int)
PREF_GetConfigContext(JSContext **js_context)
{
	if (!js_context) return FALSE;

	*js_context = NULL;
    if (m_mochaContext)
		*js_context = m_mochaContext;

	return TRUE;
}

PR_IMPLEMENT(int)
PREF_GetGlobalConfigObject(JSObject **js_object)
{
	if (!js_object) return FALSE;

	*js_object = NULL;
    if (m_GlobalConfigObject)
		*js_object = m_GlobalConfigObject;

	return TRUE;
}

PR_IMPLEMENT(int)
PREF_GetPrefConfigObject(JSObject **js_object)
{
	if (!js_object) return FALSE;

	*js_object = NULL;
    if (m_mochaPrefObject)
		*js_object = m_mochaPrefObject;

	return TRUE;
}

/* Frees the callback list. */
PR_IMPLEMENT(void)
PREF_Cleanup()
{
	struct CallbackNode* node = m_Callbacks;
	struct CallbackNode* next_node;
	
	while (node) {
		next_node = node->next;
		XP_FREE(node->domain);
		XP_FREE(node);
		node = next_node;
	}
	if (m_mochaContext) JS_DestroyContext(m_mochaContext);
	if (m_mochaTaskState) JS_Finish(m_mochaTaskState);                      
	m_mochaContext = NULL;
	m_mochaTaskState = NULL;
	
    if (m_profile) {
        delete m_profile;
    }
}

PR_IMPLEMENT(int)
PREF_ReadLockFile(const char *filename)
{
	int ok = pref_OpenFile(filename, FALSE, TRUE, TRUE, FALSE);

	return ok;
}

void PREF_SetCallbacksStatus( XP_Bool status )
{
	if (m_profile)
        m_profile->SetCallbacks((PRBool) status);
}

PR_IMPLEMENT(int)
PREF_EvaluateJSBuffer(const char * js_buffer, size_t length)
{
/* old routine that no longer triggers callbacks */
	int ret;

	ret = PREF_QuietEvaluateJSBuffer(js_buffer, length);
	
	return ret;
}

PR_IMPLEMENT(int)
PREF_QuietEvaluateJSBuffer(const char * js_buffer, size_t length)
{
	JSBool ok;
	jsval result;
	
	if (!m_mochaContext || !m_mochaPrefObject)
		return PREF_NOT_INITIALIZED;

	ok = JS_EvaluateScript(m_mochaContext, m_mochaPrefObject,
			js_buffer, length, NULL, 0, &result);
	
	/* Hey, this really returns a JSBool */
	return ok;
}

PR_IMPLEMENT(int)
PREF_QuietEvaluateJSBufferWithGlobalScope(const char * js_buffer, size_t length)
{
	JSBool ok;
	jsval result;
	
	if (!m_mochaContext || !m_GlobalConfigObject)
		return PREF_NOT_INITIALIZED;
	
	ok = JS_EvaluateScript(m_mochaContext, m_GlobalConfigObject,
			js_buffer, length, NULL, 0, &result);
	
	/* Hey, this really returns a JSBool */
	return ok;
}

/*
** External calls
*/
PR_IMPLEMENT(int)
PREF_SetCharPref(const char *pref_name, const char *value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetIntPref(const char *pref_name, int32 value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetBoolPref(const char *pref_name, XP_Bool value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, (PRBool) value);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetBinaryPref(const char *pref_name, void * value, long size)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value, size);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetColorPref(const char *pref_name, uint8 red, uint8 green, uint8 blue)
{
    if (m_profile)
        return m_profile->SetColorPref(pref_name, red, green, blue);
    else
        return PREF_NOT_INITIALIZED;
}

#define MYGetboolVal(rgb)   ((uint8) ((rgb) >> 16))
#define MYGetGValue(rgb)   ((uint8) (((uint16) (rgb)) >> 8)) 
#define MYGetRValue(rgb)   ((uint8) (rgb)) 

PR_IMPLEMENT(int)
PREF_SetColorPrefDWord(const char *pref_name, uint32 colorref)
{
    if (m_profile)
        return m_profile->SetColorPref(pref_name, colorref);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetRectPref(const char *pref_name, int16 left, int16 top, int16 right, int16 bottom)
{
    if (m_profile)
        return m_profile->SetRectPref(pref_name, left, top, right, bottom);
    else
        return PREF_NOT_INITIALIZED;
}

/*
** DEFAULT VERSIONS:  Call internal with (set_default == TRUE)
*/
PR_IMPLEMENT(int)
PREF_SetDefaultCharPref(const char *pref_name,const char *value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}


PR_IMPLEMENT(int)
PREF_SetDefaultIntPref(const char *pref_name,int32 value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetDefaultBoolPref(const char *pref_name,XP_Bool value)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, (PRBool) value, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetDefaultBinaryPref(const char *pref_name,void * value,long size)
{
    if (m_profile)
        return m_profile->SetPref(pref_name, value, size, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetDefaultColorPref(const char *pref_name, uint8 red, uint8 green, uint8 blue)
{
    if (m_profile)
        return m_profile->SetColorPref(pref_name, red, green, blue, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SetDefaultRectPref(const char *pref_name, int16 left, int16 top, int16 right, int16 bottom)
{
    if (m_profile)
        return m_profile->SetRectPref(pref_name, left, top, right, bottom, PREF_SETDEFAULT);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_SavePrefFile()
{
	if (!m_profile)
		return PREF_NOT_INITIALIZED;
	return (m_profile->SavePrefs());
}

#ifdef MOZ_LI 
PR_IMPLEMENT(int)
PREF_SaveLIPrefFile(const char *filename)
{
    if (m_profile)
        return m_profile->SavePrefs("liprefs.js");
    else
        return PREF_NOT_INITIALIZED;
}
#endif

PR_IMPLEMENT(int)
PREF_GetCharPref(const char *pref_name, char * return_buffer, int * length)
{
    if (m_profile)
    	return m_profile->GetCharPref(pref_name, return_buffer, length, FALSE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_CopyCharPref(const char *pref_name, char ** return_buffer)
{
    if (m_profile)
    	return m_profile->CopyCharPref(pref_name, return_buffer, FALSE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetIntPref(const char *pref_name,int32 * return_int)
{
    if (m_profile)
    	return m_profile->GetIntPref(pref_name, return_int, FALSE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetBoolPref(const char *pref_name, XP_Bool * return_value)
{
    if (m_profile)
    	return m_profile->GetBoolPref(pref_name, (PRBool *) return_value, FALSE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetColorPref(const char *pref_name, uint8 *red, uint8 *green, uint8 *blue)
{
    if (m_profile)
    	return m_profile->GetColorPref(pref_name, red, green, blue);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetColorPrefDWord(const char *pref_name, uint32 *colorref)
{
    if (m_profile)
    	return m_profile->GetColorPref(pref_name, colorref);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetRectPref(const char *pref_name, int16 *left, int16 *top, int16 *right, int16 *bottom)
{
    if (m_profile)
        return m_profile->GetRectPref(pref_name, left, top, right, bottom);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetBinaryPref(const char *pref_name, void * return_value, int *size)
{
    if (m_profile)
        return m_profile->GetBinaryPref(pref_name, return_value, size);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_CopyBinaryPref(const char *pref_name, void  ** return_value, int *size)
{
    if (m_profile)
    	return m_profile->CopyBinaryPref(pref_name, return_value, size);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_CopyDefaultBinaryPref(const char *pref_name, void  ** return_value, int *size)
{
    if (m_profile)
    	return m_profile->CopyBinaryPref(pref_name, return_value, size, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

#ifndef XP_MAC
PR_IMPLEMENT(int)
PREF_CopyPathPref(const char *pref_name, char ** return_buffer)
{
	return PREF_CopyCharPref(pref_name, return_buffer);
}

PR_IMPLEMENT(int)
PREF_SetPathPref(const char *pref_name, const char *path, XP_Bool set_default)
{
    if (m_profile)
        return m_profile->SetPathPref(pref_name, path, (set_default ? PREF_SETDEFAULT : PREF_SETUSER));
    else
        return PREF_NOT_INITIALIZED;
}
#endif /* XP_MAC */


PR_IMPLEMENT(int)
PREF_GetDefaultCharPref(const char *pref_name, char * return_buffer, int * length)
{
    if (m_profile)
    	return m_profile->GetCharPref(pref_name, return_buffer, length, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_CopyDefaultCharPref(const char *pref_name, char  ** return_buffer)
{
    if (m_profile)
    	return m_profile->CopyCharPref(pref_name, return_buffer, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetDefaultIntPref(const char *pref_name, int32 * return_int)
{
    if (m_profile)
    	return m_profile->GetIntPref(pref_name, return_int, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetDefaultBoolPref(const char *pref_name, XP_Bool * return_value)
{
    if (m_profile)
    	return m_profile->GetBoolPref(pref_name, (PRBool *) return_value, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetDefaultBinaryPref(const char *pref_name, void * return_value, int * length)
{
#ifdef XP_WIN
	assert( FALSE );
#else
	XP_ASSERT( FALSE );
#endif
	return TRUE;
}

PR_IMPLEMENT(int)
PREF_GetDefaultColorPref(const char *pref_name, uint8 *red, uint8 *green, uint8 *blue)
{
    if (m_profile)
    	return m_profile->GetColorPref(pref_name, red, green, blue, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetDefaultColorPrefDWord(const char *pref_name, uint32 * colorref)
{
    if (m_profile)
    	return m_profile->GetColorPref(pref_name, colorref, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_GetDefaultRectPref(const char *pref_name, int16 *left, int16 *top, int16 *right, int16 *bottom)
{
    if (m_profile)
        return m_profile->GetRectPref(pref_name, left, top, right, bottom, TRUE);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_DeleteBranch(const char *branch_name)
{
    if (m_profile)
        return m_profile->DeleteBranch(branch_name);
    else
        return PREF_NOT_INITIALIZED;
}

PR_IMPLEMENT(int)
PREF_ClearUserPref(const char *pref_name)
{
    if (m_profile)
    	return m_profile->ClearUserPref(pref_name);
    else
        return PREF_NOT_INITIALIZED;
}

#ifdef MOZ_LI 
PR_IMPLEMENT(int)
PREF_ClearLIPref(const char *pref_name)
{
    if (m_profile)
    	return m_profile->ClearLIPref(pref_name);
    else
        return PREF_NOT_INITIALIZED;
}
#endif 


/* Prototype Admin Kit support */
PR_IMPLEMENT(int)
PREF_GetConfigString(const char *obj_name, char * return_buffer, int size,
	int index, const char *field)
{
#ifdef XP_WIN
	assert( FALSE );
#else
	XP_ASSERT( FALSE );
#endif
	return -1;
}

/*
 * Administration Kit support 
 */
PR_IMPLEMENT(int)
PREF_CopyConfigString(const char *obj_name, char **return_buffer)
{
    return PREF_CopyDefaultCharPref(obj_name, return_buffer);
}


PR_IMPLEMENT(int)
PREF_CopyIndexConfigString(const char *obj_name,
	int index, const char *field, char **return_buffer)
{
	int success = PREF_ERROR;
	char* setup_buf = PR_smprintf("%s_%d.%s", obj_name, index, field);

    success = PREF_CopyConfigString(setup_buf, return_buffer);

    XP_FREEIF(setup_buf);
    return success;
}

PR_IMPLEMENT(int)
PREF_GetConfigInt(const char *obj_name, int32 *return_int)
{
    return PREF_GetDefaultIntPref(obj_name, return_int);
}

PR_IMPLEMENT(int)
PREF_GetConfigBool(const char *obj_name, XP_Bool *return_bool)
{
    return PREF_GetDefaultBoolPref(obj_name, return_bool);

}

PR_IMPLEMENT(int)
PREF_GetPrefType(const char *pref_name)
{
    if (m_profile)
        return m_profile->GetPrefType(pref_name);
    else
        return PREF_NOT_INITIALIZED;
}

JSBool PR_CALLBACK pref_NativeDefaultPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return pref_HashJSPref(argc, argv, PREF_SETDEFAULT);
}

#ifdef MOZ_LI 
/* LI_STUFF    here is the hookup with js prefs calls */
JSBool PR_CALLBACK pref_NativeLILocalPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{

    if (argc >= 1 && JSVAL_IS_STRING(argv[0])) {
    	const char *key = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));

        m_profile->SetLILocal(key);
	}

    return JS_TRUE;
}

/* combo li and user pref - save some time */
JSBool PR_CALLBACK pref_NativeLIUserPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return (JSBool)(pref_HashJSPref(argc, argv, PREF_SETUSER) && pref_HashJSPref(argc, argv, PREF_SETLI));
}

/* combo li and default pref - save some time */
JSBool PR_CALLBACK pref_NativeLIDefPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return (JSBool)(pref_HashJSPref(argc, argv, PREF_SETDEFAULT) && pref_HashJSPref(argc, argv, PREF_SETLI));
}
#endif

JSBool PR_CALLBACK pref_NativeUserPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return pref_HashJSPref(argc, argv, PREF_SETUSER);
}

JSBool PR_CALLBACK pref_NativeLockPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return pref_HashJSPref(argc, argv, PREF_LOCK);
}

JSBool PR_CALLBACK pref_NativeUnlockPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
    if (argc >= 1 && JSVAL_IS_STRING(argv[0])) {
    	const char *key = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));

        m_profile->UnlockPref(key);
	}
	return JS_TRUE;
}

JSBool PR_CALLBACK pref_NativeSetConfig
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	return pref_HashJSPref(argc, argv, PREF_SETCONFIG);
}

JSBool PR_CALLBACK pref_NativeGetPref
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	void* value = NULL;
	XP_Bool prefExists = TRUE;
    int     prefType;	

    if (argc >= 1 && JSVAL_IS_STRING(argv[0]))
    {
    	const char *key = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
        prefType = m_profile->GetPrefType(key);

        if (prefType >= PREF_OK) {
    	    if (prefType == PREF_STRING) {
    		    char* str;

                m_profile->CopyCharPref(key, &str);
    		    JSString* jsstr = JS_NewStringCopyZ(cx, str);
    		    *rval = STRING_TO_JSVAL(jsstr);

                XP_FREE(str);
    	    }
    	    else if (prefType == PREF_INT) {
                int32   intVal;

    		    m_profile->GetIntPref(key, &intVal);
                *rval = INT_TO_JSVAL(intVal);
    	    }
    	    else if (prefType == PREF_BOOL) {
                XP_Bool  boolVal;

    		    m_profile->GetBoolPref(key, (PRBool *) &boolVal);
                *rval = BOOLEAN_TO_JSVAL(boolVal);
    	    } else {
                /* Unhandled type */
                XP_ASSERT(FALSE);
            }
        }
    }
	return JS_TRUE;
}
/* -- */

PR_IMPLEMENT(XP_Bool)
PREF_PrefIsLocked(const char *pref_name)
{
    return m_profile->PrefLocked(pref_name);
}

/* if entry begins with the given string, i.e. if string is
  "a"
  and entry is
  "a.b.c" or "a.b"
  then add "a.b" to the list. */
PR_IMPLEMENT(int)
pref_addChild(PRHashEntry *he, int i, void *arg)
{
	PrefChildIter* pcs = (PrefChildIter*) arg;
	if ( XP_STRNCMP((char *) he->key, pcs->parent, strlen(pcs->parent)) == 0 ) {
		char buf[512];
		char* nextdelim;
		unsigned int parentlen = strlen(pcs->parent);
		char* substring;
		int buflen;
		XP_Bool substringBordersSeparator = FALSE;

		strncpy(buf, (char *) he->key, PR_MIN(512, strlen((char *) he->key) + 1));
		nextdelim = buf + parentlen;
		buflen = strlen(buf);
		if (parentlen < strlen(buf)) {
			/* Find the next delimiter if any and truncate the string there */
			nextdelim = strstr(nextdelim, ".");
			if (nextdelim) {
				*nextdelim = '\0';
			}
		}

		substring = strstr(pcs->childList, buf);
		if (substring)
		{
			if (substring[buflen] == '\0' || substring[buflen] == ';')
				substringBordersSeparator = TRUE;
		}

		if (!substring || !substringBordersSeparator) {
			unsigned int newsize = strlen(pcs->childList) + strlen(buf) + 2;
			if (newsize > pcs->bufsize) {
#ifdef XP_WIN16
			return HT_ENUMERATE_STOP;
#else
				pcs->bufsize *= 3;
				pcs->childList = (char*) realloc(pcs->childList, sizeof(char) * pcs->bufsize);
				if (!pcs->childList)
					return HT_ENUMERATE_STOP;
#endif
			}
			XP_STRCAT(pcs->childList, buf);
			XP_STRCAT(pcs->childList, ";");
		}
	}
	return 0;
}

PR_IMPLEMENT(int)
PREF_CreateChildList(const char* parent_node, char **child_list)
{
	return m_profile->CreateChildList(parent_node, child_list);
}

PR_IMPLEMENT(char*)
PREF_NextChild(char *child_list, int *index)
{
	return m_profile->NextChild(child_list, index);
}

/*----------------------------------------------------------------------------------------
*	pref_copyTree
*
*	A recursive function that copies all the prefs in some subtree to
*	another subtree. Either srcPrefix or dstPrefix can be empty strings,
*	but not NULL pointers. Preferences in the destination are created if 
*	they do not already exist; otherwise the old values are replaced.
*
*	Example calls:
*
*		Copy all the prefs to another tree:			pref_copyTree("", "temp", "")
*
*		Copy all the prefs under mail. to newmail.:	pref_copyTree("mail", "newmail", "mail")
*
--------------------------------------------------------------------------------------*/ 
int pref_copyTree(const char *srcPrefix, const char *destPrefix, const char *curSrcBranch)
{
	int		result = PREF_NOERROR;

	char* 	children = NULL;
	
	if ( PREF_CreateChildList(curSrcBranch, &children) == PREF_NOERROR )
	{	
		int 	index = 0;
		int		srcPrefixLen = XP_STRLEN(srcPrefix);
		char* 	child = NULL;
		
		while ( (child = PREF_NextChild(children, &index)) != NULL)
		{
			int		prefType;
			char	*destPrefName = NULL;
			char	*childStart = (srcPrefixLen > 0) ? (child + srcPrefixLen + 1) : child;
			
			XP_ASSERT( XP_STRNCMP(child, curSrcBranch, srcPrefixLen) == 0 );
							
			if (*destPrefix > 0)
				destPrefName = PR_smprintf("%s.%s", destPrefix, childStart);
			else
				destPrefName = PR_smprintf("%s", childStart);
			
			if (!destPrefName)
			{
				result = PREF_OUT_OF_MEMORY;
				break;
			}
			
			if ( ! PREF_PrefIsLocked(destPrefName) )		/* returns true if the prefs exists, and is locked */
			{
				/*	PREF_GetPrefType masks out the other bits of the pref flag, so we only
					every get the values in the switch.
				*/
				prefType = PREF_GetPrefType(child);
				
				switch (prefType)
				{
					case PREF_STRING:
						{
							char	*prefVal = NULL;
							
							result = PREF_CopyCharPref(child, &prefVal);
							if (result == PREF_NOERROR)
								result = PREF_SetCharPref(destPrefName, prefVal);
								
							XP_FREEIF(prefVal);
						}
						break;
					
					case PREF_INT:
							{
							int32 	prefValInt;
							
							result = PREF_GetIntPref(child, &prefValInt);
							if (result == PREF_NOERROR)
								result = PREF_SetIntPref(destPrefName, prefValInt);
						}
						break;
						
					case PREF_BOOL:
						{
							XP_Bool	prefBool;
							
							result = PREF_GetBoolPref(child, &prefBool);
							if (result == PREF_NOERROR)
								result = PREF_SetBoolPref(destPrefName, prefBool);
						}
						break;
					
					case PREF_ERROR:
						/*	this is probably just a branch. Since we can have both
							 a.b and a.b.c as valid prefs, this is OK.
						*/
						break;
						
					default:
						/* we should never get here */
						XP_ASSERT(FALSE);
						break;
				}
				
			}	/* is not locked */
			
			XP_FREEIF(destPrefName);
			
			/* Recurse */
			if (result == PREF_NOERROR || result == PREF_VALUECHANGED)
				result = pref_copyTree(srcPrefix, destPrefix, child);
		}
		
		XP_FREE(children);
	}
	
	return result;
}


PR_IMPLEMENT(int)
PREF_CopyPrefsTree(const char *srcRoot, const char *destRoot)
{
	XP_ASSERT(srcRoot != NULL);
	XP_ASSERT(destRoot != NULL);
	
	return pref_copyTree(srcRoot, destRoot, srcRoot);
}

/* Adds a node to the beginning of the callback list. */
PR_IMPLEMENT(void)
PREF_RegisterCallback(const char *pref_node,
					   PrefChangedFunc callback,
					   void * instance_data)
{
	struct CallbackNode* node = NULL;
	node = (struct CallbackNode*) malloc(sizeof(struct CallbackNode));
	if (node) {
		node->domain = XP_STRDUP(pref_node);
		node->func = callback;
		node->data = instance_data;
		node->next = m_Callbacks;
		m_Callbacks = node;
	}
	return;
}

/* Deletes a node from the callback list. */
PR_IMPLEMENT(int)
PREF_UnregisterCallback(const char *pref_node,
						 PrefChangedFunc callback,
						 void * instance_data)
{
	int result = PREF_ERROR;
	struct CallbackNode* node = m_Callbacks;
	struct CallbackNode* prev_node = NULL;
	
	while (node != NULL)
	{
		if ( strcmp(node->domain, pref_node) == 0 &&
			 node->func == callback &&
			 node->data == instance_data )
		{
			struct CallbackNode* next_node = node->next;
			if (prev_node)
				prev_node->next = next_node;
			else
				m_Callbacks = next_node;
			XP_FREE(node->domain);
			XP_FREE(node);
			node = next_node;
			result = PREF_NOERROR;
		}
		else {
			prev_node = node;
			node = node->next;
		}
	}
	return result;
}

int pref_DoCallback(const char* changed_pref)
{
	int result = PREF_OK;
	struct CallbackNode* node;
	for (node = m_Callbacks; node != NULL; node = node->next)
	{
		if ( XP_STRNCMP(changed_pref, node->domain, strlen(node->domain)) == 0 ) {
			int result2 = (*node->func) (changed_pref, node->data);
			if (result2 != PREF_OK)
				result = result2;
		}
	}
	return result;
}

/* !! Front ends need to implement */
#ifndef XP_MAC
PR_IMPLEMENT(XP_Bool)
PREF_IsAutoAdminEnabled()
{
	if (m_AutoAdminLib == NULL)
		m_AutoAdminLib = pref_LoadAutoAdminLib();
	
	return (m_AutoAdminLib != NULL);
}
#endif

/* Called from JavaScript */
typedef char* (*ldap_func)(char*, char*, char*, char*, char**); 

JSBool PR_CALLBACK pref_NativeGetLDAPAttr
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
#ifndef MOZ_LITE
	ldap_func get_ldap_attributes = NULL;
#if (defined (XP_MAC) && defined(powerc)) || defined (XP_WIN) || defined(XP_UNIX)
	if (m_AutoAdminLib == NULL) {
		m_AutoAdminLib = pref_LoadAutoAdminLib();
	}
		
	if (m_AutoAdminLib) {
		get_ldap_attributes = (ldap_func)
#ifndef NSPR20
            PR_FindSymbol(
#ifndef XP_WIN16
			"pref_get_ldap_attributes"
#else
			MAKEINTRESOURCE(1)
#endif
    		, m_AutoAdminLib);
#else
            PR_FindSymbol(m_AutoAdminLib, "pref_get_ldap_attributes");
#endif
	}
	if (get_ldap_attributes == NULL) {
		/* This indicates the AutoAdmin dll was not found. */
		*rval = JSVAL_NULL;
		return JS_TRUE;
	}
#else
	get_ldap_attributes = pref_get_ldap_attributes;
#endif

	if (argc >= 4 && JSVAL_IS_STRING(argv[0])
		&& JSVAL_IS_STRING(argv[1])
		&& JSVAL_IS_STRING(argv[2])
		&& JSVAL_IS_STRING(argv[3])) {
		char *return_error = NULL;
		char *value = get_ldap_attributes(
			JS_GetStringBytes(JSVAL_TO_STRING(argv[0])),
			JS_GetStringBytes(JSVAL_TO_STRING(argv[1])),
			JS_GetStringBytes(JSVAL_TO_STRING(argv[2])),
			JS_GetStringBytes(JSVAL_TO_STRING(argv[3])),
			&return_error );
		
		if (value) {
			JSString* str = JS_NewStringCopyZ(cx, value);
			XP_FREE(value);
			if (str) {
				*rval = STRING_TO_JSVAL(str);
				return JS_TRUE;
			}
		}
		if (return_error) {
			pref_Alert(return_error);
		}
	}
#endif
	
	*rval = JSVAL_NULL;
	return JS_TRUE;
}


/*
 * pref_NativeGetenv
 */
JSBool PR_CALLBACK pref_NativeGetenv
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	char* name;
	char* value;

	if ( argc != 1 || !JSVAL_IS_STRING(argv[0]) || 
		 (name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]))) == NULL ) {
		return JS_FALSE;
	}

	if ( (value = PR_GetEnv(name)) == NULL ) {
		*rval = JSVAL_NULL;
	} else {
		JSString* str = JS_NewStringCopyZ(cx, value);
		*rval = STRING_TO_JSVAL(str);
	}

	return JS_TRUE;
}


/*
 * pref_NativePutenv
 */
JSBool PR_CALLBACK pref_NativePutenv
	(JSContext *cx, JSObject *obj, unsigned int argc, jsval *argv, jsval *rval)
{
	char* arg;

	if ( argc != 1 || !JSVAL_IS_STRING(argv[0]) || 
		 (arg = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]))) == NULL ) {
		return JS_FALSE;
	}

#ifndef NSPR20
    *rval = INT_TO_JSVAL(PR_PutEnv(arg));

	return JS_TRUE;
#else
	return JS_FALSE;
#endif
}


PR_IMPLEMENT(char *)
PREF_AboutConfig()
{
    return m_profile->DumpPrefs();
}

#define MAYBE_GC_BRANCH_COUNT_MASK	4095

JSBool PR_CALLBACK
pref_BranchCallback(JSContext *cx, JSScript *script)
{ 
	static uint32	count = 0;
	
	/*
	 * If we've been running for a long time, then try a GC to 
	 * free up some memory.
	 */	
	if ( (++count & MAYBE_GC_BRANCH_COUNT_MASK) == 0 )
		JS_MaybeGC(cx); 

#ifdef LATER
    JSDecoder *decoder;
    char *message;
    JSBool ok = JS_TRUE;

    decoder = JS_GetPrivate(cx, JS_GetGlobalObject(cx));
    if (decoder->window_context && ++decoder->branch_count == 1000000) {
	decoder->branch_count = 0;
	message = PR_smprintf("Lengthy %s still running.  Continue?",
			      lm_language_name);
	if (message) {
	    ok = FE_Confirm(decoder->window_context, message);
	    XP_FREE(message);
	}
    }
#endif
    return JS_TRUE;
}

/* copied from libmocha */
void
pref_ErrorReporter(JSContext *cx, const char *message,
				 JSErrorReport *report)
{
	char *last;

	const char *s, *t;

	last = PR_sprintf_append(0, "An error occurred reading the startup configuration file.  "
		"Please contact your administrator.");

	last = PR_sprintf_append(last, LINEBREAK LINEBREAK);
	if (!report) {
		last = PR_sprintf_append(last, "%s\n", message);
	} else {
		if (report->filename)
			last = PR_sprintf_append(last, "%s, ",
									 report->filename, report->filename);
		if (report->lineno)
			last = PR_sprintf_append(last, "line %u: ", report->lineno);
		last = PR_sprintf_append(last, "%s. ", message);
		if (report->linebuf) {
			for (s = t = report->linebuf; *s != '\0'; s = t) {
				for (; t != report->tokenptr && *t != '<' && *t != '\0'; t++)
					;
				last = PR_sprintf_append(last, "%.*s", t - s, s);
				if (*t == '\0')
					break;
				last = PR_sprintf_append(last, (*t == '<') ? "" : "%c", *t);
				t++;
			}
		}
	}

	if (last) {
		pref_Alert(last);
		XP_FREE(last);
	}
}

/* Platform specific alert messages */
void pref_Alert(char* msg)
{
#if defined(XP_MAC) || defined(XP_UNIX)
#if defined(XP_UNIX)
    if ( getenv("NO_PREF_SPAM") == NULL )
#endif
	FE_Alert(NULL, msg);
#endif
#if defined (XP_WIN)
		MessageBox (NULL, msg, "Netscape -- JS Preference Warning", MB_OK);
#endif
}


#ifdef XP_WIN16
#define ADMNLIBNAME "adm1640.dll"
#elif defined XP_WIN32
#define ADMNLIBNAME "adm3240.dll"
#elif defined XP_UNIX
#define ADMNLIBNAME "libAutoAdmin.so"
extern void fe_GetProgramDirectory(char *path, int len);
#else
#define ADMNLIBNAME "AutoAdmin"	/* internal fragment name */
#endif

/* Try to load AutoAdminLib */
PRLibrary *
pref_LoadAutoAdminLib()
{
	PRLibrary *lib = NULL;

#ifdef XP_MAC
	const char *oldpath = PR_GetLibraryPath();
	PR_SetLibraryPath( "/usr/local/netscape/" );
#endif

#ifdef XP_UNIX
	{
		char aalib[MAXPATHLEN];

		if (getenv("NS_ADMIN_LIB"))
		{
			lib = PR_LoadLibrary(getenv("NS_ADMIN_LIB"));
		}
		else
		{
			if (getenv("MOZILLA_HOME"))
			{
				strcpy(aalib, getenv("MOZILLA_HOME"));
				strcat(aalib, "/");
				lib = PR_LoadLibrary(strcat(aalib, ADMNLIBNAME));
			}
			if (lib == NULL)
			{
				fe_GetProgramDirectory(aalib, sizeof(aalib)-1);
				lib = PR_LoadLibrary(strcat(aalib, ADMNLIBNAME));
			}
			if (lib == NULL)
			{
				(void) strcpy(aalib, "/usr/local/netscape/");
				lib = PR_LoadLibrary(strcat(aalib, ADMNLIBNAME));
			}
		}
	}
	/* Make sure it's really libAutoAdmin.so */
	if ( lib && PR_FindSymbol("_POLARIS_SplashPro", lib) == NULL ) return NULL;
#else
	lib = PR_LoadLibrary( ADMNLIBNAME );
#endif

#ifdef XP_MAC
	PR_SetLibraryPath(oldpath);
#endif

	return lib;
}

/*
 * Native implementations of JavaScript functions
	pref		-> pref_NativeDefaultPref
	defaultPref -> "
	userPref	-> pref_NativeUserPref
	lockPref	-> pref_NativeLockPref
	unlockPref	-> pref_NativeUnlockPref
	getPref		-> pref_NativeGetPref
	config		-> pref_NativeSetConfig
 */
static JSBool pref_HashJSPref(unsigned int argc, jsval *argv, PrefAction action)
{	
    if (argc >= 2 && JSVAL_IS_STRING(argv[0])) {
    	const char *key = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
    	
    	if (JSVAL_IS_STRING(argv[1])) {
            m_profile->SetPref(key, (char *) JS_GetStringBytes(JSVAL_TO_STRING(argv[1])), action);
    	}
    	else if (JSVAL_IS_INT(argv[1])) {
            m_profile->SetPref(key, (int32) JSVAL_TO_INT(argv[1]), action);
    	}
    	else if (JSVAL_IS_BOOLEAN(argv[1])) {
            m_profile->SetPref(key, (PRBool) JSVAL_TO_BOOLEAN(argv[1]), action);
    	}
    }
	return JS_TRUE;
}

/* This is more recent than the some routines which should be obsoleted */
PR_IMPLEMENT(JSBool)
PREF_EvaluateConfigScript(const char * js_buffer, size_t length,
	const char* filename, XP_Bool bGlobalContext, XP_Bool bCallbacks,
    XP_Bool skipFirstLine)
{
	JSBool ok;
	jsval result;
	JSObject* scope;
	JSErrorReporter errReporter;
	
	if (bGlobalContext)
		scope = m_GlobalConfigObject;
	else
		scope = m_mochaPrefObject;
		
	if (!m_mochaContext || !scope)
		return JS_FALSE;

	errReporter = JS_SetErrorReporter(m_mochaContext, pref_ErrorReporter);
	PREF_SetCallbacksStatus(bCallbacks);

    if (skipFirstLine) {
        /* In order to protect the privacy of the JavaScript preferences file 
         * from loading by the browser, we make the first line unparseable
         * by JavaScript. We must skip that line here before executing 
         * the JavaScript code.
         */
        unsigned int i=0;
        while (i < length) {
            char c = js_buffer[i++];
            if (c == '\r') {
                if (js_buffer[i] == '\n')
                    i++;
                break;
            }
            if (c == '\n')
                break;
        }
        m_SavedLine = (char *) malloc(i+1);
        if (!m_SavedLine)
            return JS_FALSE;
        memcpy(m_SavedLine, js_buffer, i);
        m_SavedLine[i] = '\0';
        length -= i;
        js_buffer += i;
    }

	ok = JS_EvaluateScript(m_mochaContext, scope, js_buffer, 
	                       length, filename, 0, &result);
	
	PREF_SetCallbacksStatus(TRUE);		/* ?? want to enable after reading user/lock file */
	JS_SetErrorReporter(m_mochaContext, errReporter);
	
	return ok;
}


/*
 * pref_CountListMembers
 */
static int
pref_CountListMembers(char* list)
{
	int members = 0;
	char* p = list = XP_STRDUP(list);

	for ( p = XP_STRTOK(p, LISTPREF_SEPARATOR); p != NULL; p = XP_STRTOK(NULL, LISTPREF_SEPARATOR) ) {
		members++;
	}

	XP_FREEIF(list);

	return members;
}


/*
 * PREF_GetListPref
 * Splits a comma separated strings into an array of strings.
 * The array of strings is actually just an array of pointers into a copy
 * of the value returned by PREF_CopyCharPref().  So, we don't have to
 * allocate each string separately.
 */
PR_IMPLEMENT(int)
PREF_GetListPref(const char* pref, char*** list)
{
	char* value;
	char** p;
	int num_members;

	*list = NULL;

	if ( PREF_CopyCharPref(pref, &value) != PREF_OK || value == NULL ) {
		return PREF_ERROR;
	}

	num_members = pref_CountListMembers(value);

	p = *list = (char**) XP_ALLOC((num_members+1) * sizeof(char**));
	if ( *list == NULL ) return PREF_ERROR;

	for ( *p = XP_STRTOK(XP_STRDUP(value), LISTPREF_SEPARATOR); 
          *p != NULL; 
          *(++p) = XP_STRTOK(NULL, LISTPREF_SEPARATOR) ) /* Empty body */ ;

	return PREF_OK;
}


/*
 * PREF_SetListPref
 * TODO: Call Javascript callback to make sure user is allowed to make this
 * change.
 */
PR_IMPLEMENT(int)
PREF_SetListPref(const char* pref, char** list)
{
	int status;
	int len;
	char** p;
	char* value = NULL;

	if ( pref == NULL || list == NULL ) return PREF_ERROR;

	for ( len = 0, p = list; p != NULL && *p != NULL; p++ ) {
		len+= (XP_STRLEN(*p)+1); /* The '+1' is for a comma or '\0' */
	}

	if ( len <= 0 || (value = (char *) XP_ALLOC(len)) == NULL ) {
		return PREF_ERROR;
	}

	(void) XP_STRCPY(value, *list);
	for ( p = list+1; p != NULL && *p != NULL; p++ ) {
		(void) XP_STRCAT(value, LISTPREF_SEPARATOR);
		(void) XP_STRCAT(value, *p);
	}

	status = PREF_SetCharPref(pref, value);

	XP_FREEIF(value);

	return status;	
}

PR_IMPLEMENT(int)
PREF_AppendListPref(const char* pref, const char* value)
{
	char *pListPref = NULL, *pNewList = NULL;
    int nPrefLen = 0;

	PREF_CopyCharPref(pref, &pListPref);

	if (pListPref)
	{
        nPrefLen = XP_STRLEN(pListPref);
    }

    if (nPrefLen == 0) {
		PREF_SetCharPref(pref, value);
    } else {
        pNewList = (char *) XP_ALLOC((nPrefLen + XP_STRLEN(value) + 1));
        if (pNewList) {
            XP_STRCPY(pNewList, pListPref);
			XP_STRCAT(pNewList, ",");
			XP_STRCAT(pNewList, value);
    		PREF_SetCharPref(pref, pNewList);
            XP_FREE(pNewList);
        }

    }

    XP_FREEIF(pListPref);

    return 0;
}

/*
 * PREF_FreeListPref
 * Free each element in the list, then free the list, then NULL the
 * list out.
 * We don't have to free each element of list, because we allocated one
 * string that was tokenized by strtok().
 */
PR_IMPLEMENT(int)
PREF_FreeListPref(char*** list)
{
	if ( list == NULL ) return PREF_ERROR;

	XP_FREEIF(**list);
	XP_FREEIF(*list);

	*list = NULL;

	return PREF_OK;
}


