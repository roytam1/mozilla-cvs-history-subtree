/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef npglue_h__
#define npglue_h__

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef XP_UNIX
#undef Bool
#endif
#include "xp_core.h"
#include "np.h"
#include "nppg.h"
#include "client.h"
#include "net.h"
#include "xpassert.h" 
#include "ntypes.h"
#include "fe_proto.h"
#include "cvactive.h"
#include "gui.h"			/* For XP_AppCodeName */
#include "merrors.h"
#include "xpgetstr.h"
#include "nsICapsManager.h"
#ifdef JAVA
#include "java.h"
#endif
#include "nppriv.h"
#include "shist.h"

#include "prefapi.h"
#include "proto.h"

#include "libmocha.h"
#include "libevent.h"
#include "layout.h"         /* XXX From ../layout */

#ifdef LAYERS
#include "layers.h"
#endif /* LAYERS */

#include "nsplugin.h"

#include "plstr.h"

extern int XP_PLUGIN_LOADING_PLUGIN;
extern int MK_BAD_CONNECT;
extern int XP_PLUGIN_NOT_FOUND;
extern int XP_PLUGIN_CANT_LOAD_PLUGIN;
extern int XP_PROGRESS_STARTING_JAVA;

#define NP_LOCK    1
#define NP_UNLOCK  0

#define NPTRACE(n, msg)	TRACEMSG(msg)

#define RANGE_EQUALS  "bytes="

/* @@@@ steal the private call from netlib */
extern void NET_SetCallNetlibAllTheTime(MWContext *context, char *caller);
extern void NET_ClearCallNetlibAllTheTime(MWContext *context, char *caller);

#if defined(XP_WIN) || defined(XP_OS2)
/* Can't include FEEMBED.H because it's full of C++ */
extern NET_StreamClass *EmbedStream(int iFormatOut, void *pDataObj, URL_Struct *pUrlData, MWContext *pContext);
extern void EmbedUrlExit(URL_Struct *pUrl, int iStatus, MWContext *pContext);
#endif

extern void NET_RegisterAllEncodingConverters(char* format_in, FO_Present_Types format_out);


/* Internal prototypes */

void
NPL_EmbedURLExit(URL_Struct *urls, int status, MWContext *cx);

void
NPL_URLExit(URL_Struct *urls, int status, MWContext *cx);

void
np_streamAsFile(np_stream* stream);

NPError
np_switchHandlers(np_instance* instance,
				  np_handle* newHandle,
				  np_mimetype* newMimeType,
				  char* requestedType);

NET_StreamClass*
np_newstream(URL_Struct *urls, np_handle *handle, np_instance *instance);

void
np_findPluginType(NPMIMEType type, void* pdesc, np_handle** outHandle, np_mimetype** outMimetype);

void 
np_enablePluginType(np_handle* handle, np_mimetype* mimetype, XP_Bool enabled);

void
np_bindContext(NPEmbeddedApp* app, MWContext* cx);

void
np_unbindContext(NPEmbeddedApp* app, MWContext* cx);

void
np_deleteapp(MWContext* cx, NPEmbeddedApp* app);

np_instance*
np_newinstance(np_handle *handle, MWContext *cx, NPEmbeddedApp *app,
			   np_mimetype *mimetype, char *requestedType);
			   				
void
np_delete_instance(np_instance *instance);

void
np_recover_mochaWindow(JRIEnv * env, np_instance * instance);

XP_Bool
np_FakeHTMLStream(URL_Struct* urls, MWContext* cx, char * fakehtml);

/* Navigator plug-in API function prototypes */

/*
 * Use this macro before each exported function
 * (between the return address and the function
 * itself), to ensure that the function has the
 * right calling conventions on Win16.
 */
#ifdef XP_WIN16
#define NP_EXPORT __export
#elif defined(XP_OS2)
#define NP_EXPORT _System
#else
#define NP_EXPORT
#endif

NPError NP_EXPORT
npn_requestread(NPStream *pstream, NPByteRange *rangeList);

NPError NP_EXPORT
npn_geturlnotify(NPP npp, const char* relativeURL, const char* target, void* notifyData);

NPError NP_EXPORT
npn_getvalue(NPP npp, NPNVariable variable, void *r_value);

NPError NP_EXPORT
npn_setvalue(NPP npp, NPPVariable variable, void *r_value);

NPError NP_EXPORT
npn_geturl(NPP npp, const char* relativeURL, const char* target);

NPError NP_EXPORT
npn_posturlnotify(NPP npp, const char* relativeURL, const char *target,
                  uint32 len, const char *buf, NPBool file, void* notifyData);

NPError NP_EXPORT
npn_posturl(NPP npp, const char* relativeURL, const char *target, uint32 len,
            const char *buf, NPBool file);

NPError
np_geturlinternal(NPP npp, const char* relativeURL, const char* target, 
                  const char* altHost, const char* referer, PRBool forceJSEnabled,
                  NPBool notify, void* notifyData);

NPError
np_posturlinternal(NPP npp, const char* relativeURL, const char *target, 
                   const char* altHost, const char* referer, PRBool forceJSEnabled,
                   uint32 len, const char *buf, NPBool file, NPBool notify, void* notifyData);

NPError NP_EXPORT
npn_newstream(NPP npp, NPMIMEType type, const char* window, NPStream** pstream);

int32 NP_EXPORT
npn_write(NPP npp, NPStream *pstream, int32 len, void *buffer);

NPError NP_EXPORT
npn_destroystream(NPP npp, NPStream *pstream, NPError reason);

void NP_EXPORT
npn_status(NPP npp, const char *message);

void NP_EXPORT
npn_registerwindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);

void NP_EXPORT
npn_unregisterwindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);

#if 0
int16 NP_EXPORT
npn_allocateMenuID(NPP npp, XP_Bool isSubmenu);
#endif

#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_D0
#endif
const char* NP_EXPORT
npn_useragent(NPP npp);
#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_A0
#endif

#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_D0
#endif
void* NP_EXPORT
npn_memalloc (uint32 size);
#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_A0
#endif


void NP_EXPORT
npn_memfree (void *ptr);

uint32 NP_EXPORT
npn_memflush(uint32 size);

void NP_EXPORT
npn_reloadplugins(NPBool reloadPages);

void NP_EXPORT
npn_invalidaterect(NPP npp, NPRect *invalidRect);

void NP_EXPORT
npn_invalidateregion(NPP npp, NPRegion invalidRegion);

void NP_EXPORT
npn_forceredraw(NPP npp);

#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_D0
#endif
#if defined(OJI)
JNIEnv* NP_EXPORT
npn_getJavaEnv(void);
#else
JRIEnv* NP_EXPORT
npn_getJavaEnv(void);
#endif
#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_A0
#endif


#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_D0
#endif
jref NP_EXPORT
npn_getJavaPeer(NPP npp);
#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_A0
#endif

extern NPError
npn_SetWindowSize(np_instance* instance, NPSize* pnpsz);

/* End of function prototypes */

/* this is a hack for now */
#define NP_MAXBUF (0xE000)

#if defined(__cplusplus)
} /* extern "C" */
#endif

extern nsresult fromNPError[];

#endif /* npglue_h__ */
