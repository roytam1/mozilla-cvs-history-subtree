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

////////////////////////////////////////////////////////////////////////////////
//
// BROWSER-SIDE INTERFACE TO NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).
//
////////////////////////////////////////////////////////////////////////////////

#ifndef pluginimpl_h__
#define pluginimpl_h__

#include "nsIPlug.h"
#include "nsILCPlg.h"
#include "np.h"         // for NPEmbeddedApp, etc.
#include "nsAgg.h"      // nsPluginManager aggregates nsJVMManager
#include "net.h"
#include "nsIPluginRegistry.h"
#include "nsIMimeTypeRegistry.h"

#ifdef OJI
#include "nsjvm.h"
#endif

////////////////////////////////////////////////////////////////////////////////

typedef NPPluginError
(PR_CALLBACK* NP_CREATEPLUGIN)(NPIPluginManager* mgr, NPIPlugin* *result);

class nsPluginManager : public NPIPluginManager {
public:

    // QueryInterface may be used to obtain a JRIEnv or JNIEnv
    // from an NPIPluginManager.
    // (Corresponds to NPN_GetJavaEnv.)

    ////////////////////////////////////////////////////////////////////////////
    // from NPIPluginManager:

    // (Corresponds to NPN_ReloadPlugins.)
    NS_IMETHOD_(void)
    ReloadPlugins(PRBool reloadPages);

    // (Corresponds to NPN_MemAlloc.)
    NS_IMETHOD_(void*)
    MemAlloc(PRUint32 size);

    // (Corresponds to NPN_MemFree.)
    NS_IMETHOD_(void)
    MemFree(void* ptr);

    // (Corresponds to NPN_MemFlush.)
    NS_IMETHOD_(PRUint32)
    MemFlush(PRUint32 size);

    ////////////////////////////////////////////////////////////////////////////
    // nsPluginManager specific methods:

    NS_DECL_AGGREGATED

    static NS_METHOD
    Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    virtual NS_METHOD
    Initialize(void);

    virtual nsIPluginRegistry*
    GetPluginRegistry(void);

    virtual nsIMimeTypeRegistry*
    GetMimeTypeRegistry(void);

protected:
    nsPluginManager(nsISupports* outer);
    virtual ~nsPluginManager(void);

#ifdef OJI
    // aggregated sub-intefaces:
    NPIJVMPluginManager* GetJVMMgr(const nsIID& aIID);
    nsISupports* fJVMMgr;
#endif

    nsIPluginRegistry* fPlugins;

    nsIMimeTypeRegistry* fMimeTypes;

    virtual NS_METHOD
    RegisterPlugins(const char* path);
};

extern nsPluginManager* thePluginManager;

////////////////////////////////////////////////////////////////////////////////

class nsPluginInstancePeer : public NPILiveConnectPluginInstancePeer {
public:

    nsPluginInstancePeer(nsISupports* outer,
                         LO_EmbedStruct* embed_struct);

    virtual ~nsPluginInstancePeer(void);

    NS_DECL_AGGREGATED

    ////////////////////////////////////////////////////////////////////////////
    // Methods specific to NPIPluginInstancePeer:

    NS_IMETHOD_(NPIPlugin*)
    GetClass(void);

    // (Corresponds to NPP_New's MIMEType argument.)
    NS_IMETHOD_(const char*)
    GetMIMEType(void);

    // (Corresponds to NPP_New's mode argument.)
    NS_IMETHOD_(NPPluginType)
    GetMode(void);

    // Get a ptr to the paired list of attribute names and values,
    // returns the length of the array.
    //
    // Each name or value is a null-terminated string.
    //
    NS_IMETHOD_(NPPluginError)
    GetAttributes(PRUint16& n, const char*const*& names, const char*const*& values);

    // Get the value for the named attribute.  Returns null
    // if the attribute was not set.
    NS_IMETHOD_(const char*)
    GetAttribute(const char* name);

    // Get a ptr to the paired list of parameter names and values,
    // returns the length of the array.
    //
    // Each name or value is a null-terminated string.
    NS_IMETHOD_(NPPluginError)
    GetParameters(PRUint16& n, const char*const*& names, const char*const*& values);

    // Get the value for the named parameter.  Returns null
    // if the parameter was not set.
    NS_IMETHOD_(const char*)
    GetParameter(const char* name);

    // Get the complete text of the HTML tag that was
    // used to instantiate this plugin
    NS_IMETHOD_(const char *)
    GetTagText(void);

    // Get the type of the HTML tag that was used ot instantiate this
    // plugin.  Currently supported tags are EMBED, OBJECT and APPLET.
    // 
    // returns a NPTagType.
    NS_IMETHOD_(NPTagType) 
    GetTagType(void);

    NS_IMETHOD_(NPIPluginManager*)
    GetPluginManager(void);

    // (Corresponds to NPN_GetURL and NPN_GetURLNotify.)
    NS_IMETHOD_(NPPluginError)
    GetURL(const char* url, const char* target, void* notifyData, 
           const char* altHost, const char* referer, PRBool forceJSEnabled);

    // (Corresponds to NPN_PostURL and NPN_PostURLNotify.)
    NS_IMETHOD_(NPPluginError)
    PostURL(const char* url, const char* target,
            PRUint32 len, const char* buf, PRBool file, void* notifyData, 
            const char* altHost, const char* referer, PRBool forceJSEnabled,
            PRUint32 postHeadersLength, const char* postHeaders);

    // (Corresponds to NPN_NewStream.)
    NS_IMETHOD_(NPPluginError)
    NewStream(nsMIMEType type, const char* target,
              NPIPluginManagerStream* *result);

    // (Corresponds to NPN_Status.)
    NS_IMETHOD_(void)
    ShowStatus(const char* message);

    // (Corresponds to NPN_UserAgent.)
    NS_IMETHOD_(const char*)
    UserAgent(void);

    // (Corresponds to NPN_GetValue.)
    NS_IMETHOD_(NPPluginError)
    GetValue(NPPluginManagerVariable variable, void *value);

    // (Corresponds to NPN_SetValue.)
    NS_IMETHOD_(NPPluginError)
    SetValue(NPPluginVariable variable, void *value);

    // (Corresponds to NPN_InvalidateRect.)
    NS_IMETHOD_(void)
    InvalidateRect(nsRect *invalidRect);

    // (Corresponds to NPN_InvalidateRegion.)
    NS_IMETHOD_(void)
    InvalidateRegion(nsRegion invalidRegion);

    // (Corresponds to NPN_ForceRedraw.)
    NS_IMETHOD_(void)
    ForceRedraw(void);

    NS_IMETHOD_(void)
    RegisterWindow(void* window);
	
    NS_IMETHOD_(void)
    UnregisterWindow(void* window);	

    NS_IMETHOD_(PRInt16)
	AllocateMenuID(PRBool isSubmenu);

    ////////////////////////////////////////////////////////////////////////////
    // Methods specific to NPILiveConnectPluginInstancePeer:

    // (Corresponds to NPN_GetJavaPeer.)
    NS_IMETHOD_(jobject)
    GetJavaPeer(void);

    ////////////////////////////////////////////////////////////////////////////
    // Methods specific to nsPluginInstancePeer

    NS_METHOD_(NPIPluginInstance*)
    GetUserInstance(void);

    NS_METHOD_(void)
    SetUserInstance(NPIPluginInstance* inst);

    NS_METHOD_(LO_EmbedStruct*) // no need to refcount
    GetEmbedStruct(void) {
        return fEmbedStruct;
    };

    NS_METHOD_(void)
    SetEmbedStruct(LO_EmbedStruct* embedStruct) {
        fEmbedStruct = embedStruct;
    };

    NS_METHOD_(NPEmbeddedApp*) // no need to refcount
    GetApp(void) {
        return &fApp;
    };

    NS_METHOD_(PRBool)
    IsWindowed(void) {
        return fWindowed;
    };

    NS_METHOD_(PRBool)
    IsTransparent(void) {
        return fTransparent;
    };

    NS_METHOD_(void)
    SetContext(MWContext* context);

    NS_METHOD_(MWContext*)
    GetContext(void) {
        return fContext;
    };

    NS_METHOD_(NET_StreamClass*)
    CreateStream(FO_Present_Types formatOut,
                 URL_Struct* url,
                 MWContext* cx); // XXX do we need this?

protected:
    /**
     * Create the user plugin instance
     */
    nsresult
    CreatePluginInstance(void);

    NPIPluginInstance* fUserInst;
    nsIPluginClass*    fClass;
    nsIMimeType*       fMimeType;
    NPEmbeddedApp      fApp;           // XXX Get rid of this!
    LO_EmbedStruct*    fEmbedStruct;
    MWContext*         fContext;
    PRBool             fWindowed;      // set via NPPVpluginWindowBool
    PRBool             fTransparent;   // set via NPPVpluginTransparentBool
#ifdef PLUGIN_TIMER_EVENT
	void*              fTimeout;
	PRUint32           fTimerInterval; // set via NPPVpluginTimerInterval

    static void
    TimerCallback(void* closure);
#endif

#ifdef OJI
    // aggregated sub-intefaces:
    nsISupports* fJVMInstancePeer;
#endif

};

////////////////////////////////////////////////////////////////////////////////

class nsPluginManagerStream : public NPIPluginManagerStream {
public:

    nsPluginManagerStream(void);
    virtual ~nsPluginManagerStream(void);

    NS_DECL_ISUPPORTS

    ////////////////////////////////////////////////////////////////////////////
    // from NPIStream:

    // (Corresponds to NPP_WriteReady.)
    NS_IMETHOD_(PRInt32)
    WriteReady(void);

    // (Corresponds to NPP_Write and NPN_Write.)
    NS_IMETHOD_(PRInt32)
    Write(PRInt32 len, const char* buffer);

    ////////////////////////////////////////////////////////////////////////////
    // from NPIPluginManagerStream:

    // (Corresponds to NPStream's url field.)
    NS_IMETHOD_(const char*)
    GetURL(void);

    // (Corresponds to NPStream's end field.)
    NS_IMETHOD_(PRUint32)
    GetEnd(void);

    // (Corresponds to NPStream's lastmodified field.)
    NS_IMETHOD_(PRUint32)
    GetLastModified(void);

    // (Corresponds to NPStream's notifyData field.)
    NS_IMETHOD_(void*)
    GetNotifyData(void);

protected:

};

////////////////////////////////////////////////////////////////////////////////

class nsPluginStreamPeer : public NPISeekablePluginStreamPeer {
public:

    ////////////////////////////////////////////////////////////////////////////
    // from NPIPluginStreamPeer:

    // (Corresponds to NPStream's url field.)
    NS_IMETHOD_(const char*)
    GetURL(void);

    // (Corresponds to NPStream's end field.)
    NS_IMETHOD_(PRUint32)
    GetEnd(void);

    // (Corresponds to NPStream's lastmodified field.)
    NS_IMETHOD_(PRUint32)
    GetLastModified(void);

    // (Corresponds to NPStream's notifyData field.)
    NS_IMETHOD_(void*)
    GetNotifyData(void);

    // (Corresponds to NPP_DestroyStream's reason argument.)
    NS_IMETHOD_(NPPluginReason)
    GetReason(void);

    // (Corresponds to NPP_NewStream's MIMEType argument.)
    NS_IMETHOD_(nsMIMEType)
    GetMIMEType(void);

    NS_IMETHOD_(PRUint32)
    GetContentLength(void);

    NS_IMETHOD_(PRUint32)
    GetHeaderFieldCount(void);

    NS_IMETHOD_(const char*)
    GetHeaderFieldKey(PRUint32 index);

    NS_IMETHOD_(const char*)
    GetHeaderField(PRUint32 index);

    ////////////////////////////////////////////////////////////////////////////
    // from NPISeekablePluginStreamPeer:

    // (Corresponds to NPN_RequestRead.)
    NS_IMETHOD_(NPPluginError)
    RequestRead(nsByteRange* rangeList);


    ////////////////////////////////////////////////////////////////////////
    // nsPluginStream methods
    //

    nsPluginStreamPeer(nsPluginInstancePeer* instancePeer,
                       FO_Present_Types formatOut,
                       URL_Struct *urls);

    virtual ~nsPluginStreamPeer(void);

    NS_DECL_ISUPPORTS
    
    NS_METHOD_(NPIPluginStream*)
    GetUserStream(void);

    NS_METHOD_(void)
    SetUserStream(NPIPluginStream* str);

    NS_METHOD_(void)
    SetReason(NPPluginReason reason) { // XXX still necessary? public?
        fReason = reason;
    };

    NS_METHOD_(NET_StreamClass*)
    GetNetlibStream(void) {
        return fNetlibStream;
    };

protected:
    NPIPluginStream*      fUserStream;
    nsPluginInstancePeer* fInstancePeer;
    FO_Present_Types      fFormatOut;
    URL_Struct*           fURL;
    NET_StreamClass*      fNetlibStream;
    NPPluginReason        fReason;

    static unsigned int
    WriteReady(NET_StreamClass* s);

    static int
    Write(NET_StreamClass* s, const char* bytes, int32 len);

    static void
    Complete(NET_StreamClass* s);

    static void
    Abort(NET_StreamClass* s, int status);
};

////////////////////////////////////////////////////////////////////////////////

#endif /* pluginimpl_h__ */
