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

#include "pluginimpl.h"

#include "nsIPluginRegistry.h"
#include "nsPluginRegistry.h"
#include "nsMimeTypeRegistry.h"
#include "nsIPluginClass.h"
#include "plugin_md.h"

#include "net.h"      // for NET_*
#include "cvactive.h" // for CV_MakeMultipleDocumentStream()
#include "prlog.h"
#include "prmem.h"

// XXX
extern "C" void EDT_RegisterPlugin(const char* csFileSpec);



////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////


nsPluginManager* thePluginManager = NULL;
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginManagerIID, NP_IPLUGINMANAGER_IID);
static NS_DEFINE_IID(kJRIEnvIID, NP_IJRIENV_IID); 
static NS_DEFINE_IID(kJNIEnvIID, NP_IJNIENV_IID); 




////////////////////////////////////////////////////////////////////////

nsPluginManager::nsPluginManager(nsISupports* outer)
#ifdef OJI
    : fJVMMgr(NULL)
#endif
    : fPlugins(NULL), fMimeTypes(NULL)
{
    NS_INIT_AGGREGATED(outer);
}


nsPluginManager::~nsPluginManager(void)
{
#ifdef OJI
    fJVMMgr->Release();
    fJVMMgr = NULL;
#endif

    // XXX Unregister content type converters

    if (fPlugins)
        fPlugins->Release();
}


NS_IMPL_AGGREGATED(nsPluginManager);

NS_METHOD
nsPluginManager::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      
    if (aIID.Equals(kJRIEnvIID)) {
#if 0
        // XXX Need to implement ISupports for JRIEnv
        *aInstancePtr = (void*) ((nsISupports*)npn_getJavaEnv()); 
//        AddRef();     // XXX should the plugin instance peer and the env be linked?
        return NS_OK; 
#endif
        return NS_NOINTERFACE; // XXX
    } 
#if 0   // later
    if (aIID.Equals(kJNIEnvIID)) {
        // XXX Need to implement ISupports for JNIEnv
        *aInstancePtr = (void*) ((nsISupports*)npn_getJavaEnv());       // XXX need JNI version
//        AddRef();     // XXX should the plugin instance peer and the env be linked?
        return NS_OK; 
    }
#endif 
    if (aIID.Equals(kIPluginManagerIID)) {
        *aInstancePtr = (void*) this; 
        AddRef(); 
        return NS_OK; 
    } 
    if (aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this); 
        AddRef(); 
        return NS_OK; 
    } 
#ifdef OJI
    // Aggregates...
    NPIJVMPluginManager* jvmMgr = GetJVMMgr(aIID);
    if (jvmMgr) {
        *aInstancePtr = (void*) ((nsISupports*)jvmMgr);
        return NS_OK; 
    }
#endif
    return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////

NS_METHOD
nsPluginManager::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    if (outer && !aIID.Equals(kISupportsIID))
        return NS_NOINTERFACE;   // XXX right error?
    nsPluginManager* mgr = new nsPluginManager(outer);
    nsresult result = mgr->QueryInterface(aIID, aInstancePtr);
    if (result != NS_OK) {
        delete mgr;
    }
    return result;
}


////////////////////////////////////////////////////////////////////////


NS_METHOD_(void)
nsPluginManager::ReloadPlugins(PRBool reloadPages)
{
    //npn_reloadplugins(reloadPages);
}

NS_METHOD_(void*)
nsPluginManager::MemAlloc(PRUint32 size)
{
    return PR_MALLOC(size);
}

NS_METHOD_(void)
nsPluginManager::MemFree(void* ptr)
{
    PR_FREEIF(ptr);
}

NS_METHOD_(PRUint32)
nsPluginManager::MemFlush(PRUint32 size)
{
#ifdef XP_MAC
    /* Try to free some memory and return the amount we freed. */
    if (CallCacheFlushers(size))
        return size;
#endif
    return 0;
}

#ifdef OJI
NPIJVMPluginManager*
nsPluginManager::GetJVMMgr(const nsIID& aIID)
{
    NPIJVMPluginManager* result = NULL;
    if (fJVMMgr == NULL) {
        // The plugin manager is the outer of the JVM manager
        if (JVMMgr::Create(this, kISupportsIID, (void**)&fJVMMgr) != NS_OK)
            return NULL;
    }
    if (fJVMMgr->QueryInterface(aIID, (void**)&result) != NS_OK)
        return NULL;

    return result;
}
#endif


nsIPluginRegistry*
nsPluginManager::GetPluginRegistry(void)
{
    if (fPlugins != NULL)
        fPlugins->AddRef();

    return fPlugins;
}


nsIMimeTypeRegistry*
nsPluginManager::GetMimeTypeRegistry(void)
{
    if (fMimeTypes != NULL)
        fMimeTypes->AddRef();

    return fMimeTypes;
}


////////////////////////////////////////////////////////////////////////

NS_METHOD
nsPluginManager::Initialize(void)
{
    nsresult error;
    nsString pluginDir;

    // Register all default plugin converters. Do this before
    // FE_RegisterPlugins() because this registers a not found
    // converter for "*" and FE can override that with the nullplugin
    // if one is available.
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_EMBED, NULL, NET_CacheConverter);
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_PLUGIN, NULL, NET_CacheConverter);
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_BYTERANGE, NULL, NET_CacheConverter);

    NET_RegisterContentTypeConverter("multipart/x-byteranges", FO_CACHE_AND_BYTERANGE, NULL, CV_MakeMultipleDocumentStream);

#if 0
    NET_RegisterContentTypeConverter("*", FO_PLUGIN, NULL, nsPluginManager::NewPluginStream);
    NET_RegisterContentTypeConverter("*", FO_BYTERANGE, NULL, nsPluginManager::NewByteRangeStream);
#ifdef XP_UNIX
    NET_RegisterContentTypeConverter("*", FO_EMBED, NULL, nsPluginManager::NoEmbedFound);
#endif
#endif
        
    fPlugins = new nsPluginRegistry(this);
    if (fPlugins == NULL) {
        error = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    // We don't AddRef() because nsPluginRegistry is aggregated 
    // back to this

    fMimeTypes = new nsMimeTypeRegistry(this);
    if (fMimeTypes == NULL) {
        error = NS_ERROR_OUT_OF_MEMORY;
        goto done;
    }
    // We don't AddRef() because nsMimeTypeRegistry is aggregated 
    // back to this

    pluginDir = ::nplmd_GetPluginDir();
    if ((error = RegisterPlugins(pluginDir)) != NS_OK)
        goto done;

    error = NS_OK;

done:
    return error;
}



NS_METHOD
nsPluginManager::RegisterPlugins(const char* path)
{
    nsresult error = NS_OK;
    PRDir* dir;

    if ((dir = PR_OpenDir(path)) == NULL)
        return NS_ERROR_FAILURE; // XXX

    // Store the current directory, and change directories to the new
    // directory.
    nsString previousDir = nplmd_GetCurrentDir();
    nplmd_ChangeDir(nsString(path));

    // Iterate through all of the plugins in the directory
    PRDirEntry* dirEntry;
    while ((dirEntry = PR_ReadDir(dir, PR_SKIP_BOTH)) != NULL) {
        const char* name = PR_DirName(dirEntry);

        PRFileInfo info;
        if ((PR_GetFileInfo(name, &info)) != PR_SUCCESS)
            continue;

        nsString pluginPath = path;
        pluginPath += SEPARATOR;
        pluginPath += name;

        if (info.type == PR_FILE_DIRECTORY) {
            error = RegisterPlugins(pluginPath);

        } else if (::nplmd_IsPlugin(name)) {
            nsIPluginClass* pluginClass;

            if ((error = ::nplmd_CreatePluginClass(pluginPath, &pluginClass)) == NS_OK)
                fPlugins->Register(pluginClass);

        } else if (::nplmd_IsComposerPlugin(name)) {
            ::EDT_RegisterPlugin(name);
        }

        if (error != NS_OK)
            break;
    }

    nplmd_ChangeDir(previousDir);
    PR_CloseDir(dir);
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
