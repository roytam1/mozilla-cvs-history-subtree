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
/**
 * <B>INTERFACE TO NETSCAPE COMMUNICATOR PLUGINS (NEW C++ API).</B>
 *
 * <P>This superscedes the old plugin API (npapi.h, npupp.h), and 
 * eliminates the need for glue files: npunix.c, npwin.cpp and npmac.cpp that
 * get linked with the plugin. You will however need to link with the "backward
 * adapter" (badapter.cpp) in order to allow your plugin to run in pre-5.0
 * browsers. 
 *
 * <P>See nsplugin.h for an overview of how this interface fits with the 
 * overall plugin architecture.
 */
////////////////////////////////////////////////////////////////////////////////

#ifndef nsIPluginManager_h___
#define nsIPluginManager_h___

#include "nsplugindefs.h"
#include "nsISupports.h"

/**
 * These arguments are passed to nsIPluginManager::GetURL and can be used to
 * perform both GET and POST http requests. If no post data or post headers are
 * specified, then a GET request is sent.
 *
 * @param version - the version of this struct
 * @param url - the URL to fetch
 * @param target - the target window into which to load the URL
 * @param notifyData - when present, URLNotify is called passing the notifyData back
 *   to the client. When NULL, this call behaves like NPN_GetURL.
 * @param altHost - an IP-address string that will be used instead of the host
 *   specified in the URL. This is used to prevent DNS-spoofing attacks.
 *   Can be defaulted to NULL meaning use the host in the URL.
 * @param referrer - the referring URL (may be NULL)
 * @param forceJSEnabled - forces JavaScript to be enabled for 'javascript:' URLs,
 *   even if the user currently has JavaScript disabled (usually specify PR_FALSE) 
 * @param postData - the data to POST. NULL specifies that there is not post
 *   data
 * @param postDataLength - the length of postData (if non-NULL)
 * @param postHeaders - the headers to POST. NULL specifies that there are no
 *   post headers
 * @param postHeadersLength - the length of postHeaders (if non-NULL)
 * @param postFile - whether the postData specifies the name of a file to 
 *   post instead of data. The file will be deleted afterwards.
 */
struct nsURLInfo {
    PRUint32    version;
    const char* url;
    const char* target;
    void*       notifyData;
    const char* altHost;
    const char* referrer;
    PRBool      forceJSEnabled;
    const char* postData;
    PRUint32    postDataLength;
    const char* postHeaders;
    PRUint32    postHeadersLength;
    PRBool      postFile;
    // other fields may be added here for version numbers beyond 0x00010000
};

/**
 * nsURLInfo_Version is the current version number for the nsURLInfo 
 * struct specified above. The nsVersionOk macro will be used when comparing
 * a supplied nsURLInfo struct against this version.
 */
#define nsURLInfo_Version       0x00010000

/**
 * The nsIPluginManager interface defines the minimum set of functionality that
 * the browser will support if it allows plugins. Plugins can call QueryInterface
 * to determine if a plugin manager implements more specific APIs or other 
 * browser interfaces for the plugin to use.
 */
class nsIPluginManager : public nsISupports {
public:

    /**
     * Causes the plugins directory to be searched again for new plugin 
     * libraries.
     *
     * (Corresponds to NPN_ReloadPlugins.)
     *
     * @param reloadPages - indicates whether currently visible pages should 
     * also be reloaded
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    ReloadPlugins(PRBool reloadPages) = 0;

    /**
     * Returns the user agent string for the browser. 
     *
     * (Corresponds to NPN_UserAgent.)
     *
     * @param resultingAgentString - the resulting user agent string
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    UserAgent(const char* *resultingAgentString) = 0;

    /**
     * Returns the value of a variable associated with the plugin manager.
     *
     * (Corresponds to NPN_GetValue.)
     *
     * @param variable - the plugin manager variable to get
     * @param value - the address of where to store the resulting value
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    GetValue(nsPluginManagerVariable variable, void *value) = 0;

    /**
     * Sets the value of a variable associated with the plugin manager.
     *
     * (Corresponds to NPN_SetValue.)
     *
     * @param variable - the plugin manager variable to get
     * @param value - the address of the value to store
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    SetValue(nsPluginManagerVariable variable, void *value) = 0;

    /**
     * Fetches a URL.
     *
     * (Corresponds to NPN_GetURL NPN_GetURLNotify, NPN_PostURL and
     * NPN_PostURLNotify.)
     *
     * @param peer - a plugin instance peer. The peer's window will be used to 
     * display progress information. If NULL, the load happens in the background.
     * @param urlInfo - the URL information to GET or POST
     * @result - NS_OK if this operation was successful
     */
    NS_IMETHOD
    FetchURL(nsISupports* peer, nsURLInfo* urlInfo) = 0;

};

#define NS_IPLUGINMANAGER_IID                        \
{ /* f10b9600-a1bc-11d1-85b1-00805f0e4dfe */         \
    0xf10b9600,                                      \
    0xa1bc,                                          \
    0x11d1,                                          \
    {0x85, 0xb1, 0x00, 0x80, 0x5f, 0x0e, 0x4d, 0xfe} \
}

////////////////////////////////////////////////////////////////////////////////

#endif /* nsIPluginManager_h___ */
