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

#ifndef ns4xPlugin_h__
#define ns4xPlugin_h__

#include "nsIPlug.h" // for NPI* interfaces
#include "prlink.h"  // for PRLibrary
#include "npupp.h"


////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////

/**
 * A 5.0 wrapper for a 4.x style plugin.
 */
class ns4xPlugin : public NPILiveConnectPlugin
{
public:

    ////////////////////////////////////////////////////////////////////////
    // NPIPlugin-specifi methods

    /**
     * Create a new instance of the plugin.
     */
    NS_IMETHOD_(NPPluginError)
    NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result);

    /**
     * (Corresponds to NPP_GetMIMEDescription.)
     */
    NS_IMETHOD_(const char*)
    GetMIMEDescription(void);

    ////////////////////////////////////////////////////////////////////////
    // NPILiveConnectPlugin-specific methods

    /**
     * Return the Java class for this plugin.
     */
    NS_IMETHOD_(jclass)
    GetJavaClass(void);

    ////////////////////////////////////////////////////////////////////
    // ns4xPlugin-specific methods

    /**
     * A static factory method for constructing 4.x plugins. Constructs
     * and initializes an ns4xPlugin object, and returns it in
     * <b>result</b>.
     */
    static NPPluginError
    CreatePlugin(PRLibrary* library,
                 NPIPlugin* *result);

    NS_DECL_ISUPPORTS

protected:
    /**
     * Construct a new plugin with the specified callback entrypoints.
     */
    ns4xPlugin(NPPluginFuncs* callbacks);

    // use Release()!
    virtual ~ns4xPlugin(void);

    /**
     * The plugin-side callbacks that the browser calls. One set of
     * plugin callbacks for each plugin.
     */
    NPPluginFuncs fCallbacks;

    /**
     * The browser-side callbacks that a 4.x-style plugin calls.
     */
    static NPNetscapeFuncs CALLBACKS;

    /**
     * Ensures that the static CALLBACKS is properly initialized
     */
    static void CheckClassInitialized(void);

    ////////////////////////////////////////////////////////////////////////
    // Static stub functions that are exported to the 4.x plugin as entry
    // points via the CALLBACKS variable.
    //
    static NPError NP_EXPORT
    _requestread(NPStream *pstream, NPByteRange *rangeList);

    static NPError NP_EXPORT
    _geturlnotify(NPP npp, const char* relativeURL, const char* target, void* notifyData);

    static NPError NP_EXPORT
    _getvalue(NPP npp, NPNVariable variable, void *r_value);

    static NPError NP_EXPORT
    _setvalue(NPP npp, NPPVariable variable, void *r_value);

    static NPError NP_EXPORT
    _geturl(NPP npp, const char* relativeURL, const char* target);

    static NPError NP_EXPORT
    _posturlnotify(NPP npp, const char* relativeURL, const char *target,
                      uint32 len, const char *buf, NPBool file, void* notifyData);

    static NPError NP_EXPORT
    _posturl(NPP npp, const char* relativeURL, const char *target, uint32 len,
                const char *buf, NPBool file);

    static NPError NP_EXPORT
    _newstream(NPP npp, NPMIMEType type, const char* window, NPStream** pstream);

    static int32 NP_EXPORT
    _write(NPP npp, NPStream *pstream, int32 len, void *buffer);

    static NPError NP_EXPORT
    _destroystream(NPP npp, NPStream *pstream, NPError reason);

    static void NP_EXPORT
    _status(NPP npp, const char *message);

    static void NP_EXPORT
    _registerwindow(NPP npp, void* window);

    static void NP_EXPORT
    _unregisterwindow(NPP npp, void* window);

    static int16 NP_EXPORT
    _allocateMenuID(NPP npp, XP_Bool isSubmenu);

    static void NP_EXPORT
    _memfree (void *ptr);

    static uint32 NP_EXPORT
    _memflush(uint32 size);

    static void NP_EXPORT
    _reloadplugins(NPBool reloadPages);

    static void NP_EXPORT
    _invalidaterect(NPP npp, NPRect *invalidRect);

    static void NP_EXPORT
    _invalidateregion(NPP npp, NPRegion invalidRegion);

    static void NP_EXPORT
    _forceredraw(NPP npp);

    ////////////////////////////////////////////////////////////////////////
    // Anything that returns a pointer needs to be _HERE_ for 68K Mac to
    // work.
    //

#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_D0
#endif

    static const char* NP_EXPORT
    _useragent(NPP npp);

    static void* NP_EXPORT
    _memalloc (uint32 size);

    static JRIEnv* NP_EXPORT
    _getJavaEnv(void);

    static jref NP_EXPORT
    _getJavaPeer(NPP npp);

#if defined(XP_MAC) && !defined(powerc)
#pragma pointers_in_A0
#endif

    //
    ////////////////////////////////////////////////////////////////////////
};




#endif // ns4xPlugin_h__
