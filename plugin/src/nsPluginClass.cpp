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

#include "nsIPlug.h" // for NPI* stuff
#include "pluginimpl.h"
#include "nsPluginClass.h"
#include "ns4xPlugin.h"
#include "plstr.h"
#include "prlog.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIContentHandlerIID, NS_ICONTENTHANDLER_IID);
static NS_DEFINE_IID(kIPluginClassIID, NS_IPLUGINCLASS_IID);
static NS_DEFINE_IID(kIMimeTypeIID, NS_IMIMETYPE_IID);


////////////////////////////////////////////////////////////////////////


nsPluginClass::nsPluginClass(char* name,
                             char* dllFilename,
                             char* description)
    : fFactory(NULL), fDll(NULL), fEnabled(TRUE), fInstances(NULL)
{
    NS_INIT_REFCNT();
    fName        = name        ? PL_strdup(name)        : NULL;
    fDllFilename = dllFilename ? PL_strdup(dllFilename) : NULL;
    fDescription = description ? PL_strdup(description) : NULL;
}


nsPluginClass::~nsPluginClass(void)
{
    if (fFactory != NULL) {
        fFactory->Release();
        fFactory = NULL;
    }

    if (fDll != NULL) {
        PR_UnloadLibrary(fDll);
        fDll = NULL;
    }

    if (fName != NULL)
        PL_strfree(fName);

    if (fDllFilename != NULL)
        PL_strfree(fDllFilename);

    if (fDescription != NULL)
        PL_strfree(fDescription);
}

NS_IMPL_ADDREF(nsPluginClass);
NS_IMPL_RELEASE(nsPluginClass);


NS_METHOD
nsPluginClass::QueryInterface(const nsIID& iid, void** instance)
{
    if (iid.Equals(kISupportsIID) ||
        iid.Equals(kIContentHandlerIID) ||
        iid.Equals(kIPluginClassIID)) {
        *instance = this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}



NS_IMETHODIMP_(void)
nsPluginClass::SetEnabled(PRBool enabled)
{
    fEnabled = enabled;
    // XXX what else?
}



NS_IMETHODIMP_(NET_StreamClass*)
nsPluginClass::CreateStream(FO_Present_Types formatOut,
                            URL_Struct* url,
                            MWContext* cx)
{
    NPEmbeddedApp* app = (NPEmbeddedApp*) url->fe_data;
    nsPluginInstancePeer* peer = (nsPluginInstancePeer*) app->np_data;
    PR_ASSERT(peer != NULL);
    if (peer == NULL)
        return NULL;

    return peer->CreateStream(formatOut, url, cx);
}



NS_IMETHODIMP_(nsresult)
nsPluginClass::NewInstance(NPIPluginInstancePeer* peer, NPIPluginInstance* *result)
{
    NPPluginError error;
    if (fFactory == NULL) {
        // If we don't have a factory yet, then we need to install
        // one. Load the plugin's DLL and create the plugin.

#if 0 // XXX Need the MWContext
		FE_Progress(cx, XP_GetString(XP_PLUGIN_LOADING_PLUGIN));
#endif

        // XXX change dirs/library path to the right place?
        if ((fDll = PR_LoadLibrary(fDllFilename)) == NULL)
            return NS_ERROR_FACTORY_NOT_LOADED;

        // Look for the our entrypoint...
        // XXX What about Warren/Sudu's idea to do "real" COM here...
        NP_CREATEPLUGIN npCreatePlugin =
            (NP_CREATEPLUGIN) PR_FindSymbol(fDll, "NP_CreatePlugin");

        if (npCreatePlugin != NULL) {
            // It's a 5.0 style plugin. Instantiate it.
            error = npCreatePlugin(thePluginManager, &fFactory);
        } else {
            // It's a pre-5.0-style plugin. Create a wrapper.
            error = ns4xPlugin::CreatePlugin(fDll, &fFactory);
        }

        if (error != NPPluginError_NoError || fFactory == NULL) {
            // Unload the library if something wen't wrong...
            PR_UnloadLibrary(fDll);
            fDll = NULL;
            fFactory = NULL;

            return NS_ERROR_FACTORY_NOT_LOADED;
        }
    }

    error = fFactory->NewInstance(peer, result);

    // XXX Ensure that the factory actually gave the plugin instance a
    // refcount of "1". This could be put inside an "#ifdef DEBUG",
    // but since we're calling into 3rd party code, and this is such a
    // dumb, easy mistake to make, we might as well be ultra
    // defensive...
    if (error == NPPluginError_NoError && (*result) != NULL) {
        if ((*result)->AddRef() == 1) {
            // They gave it a refcount of zero. Well, at least _now_
            // it has a refcount of one...
            PR_ASSERT(0);

        } else {
            // They gave it a refcount of at least one. So undo our
            // AddRef()...
            (*result)->Release();
        }
    }


#if 0 // XXX No good way to remove it!
    // Add it to the set of instances associated with this plugin class
    Instance* entry = new Instance();
    entry->fInstance = (*result);
    (*result)->AddRef();
    entry->fNext = fInstances;
    fInstances = entry;
#endif

    return NS_OK;
}



NS_IMETHODIMP_(nsresult)
nsPluginClass::RemoveInstance(const NPIPluginInstance* instance)
{
    if (fInstances == NULL)
        return NS_ERROR_FAILURE;

    // Special case for the head...
    if (fInstances->fInstance == instance) {
        fInstances->fInstance->Release();
        fInstances = fInstances->fNext;

        if (fInstances == NULL) {
            // We just removed the last instance. Unload the DLL.
            PR_UnloadLibrary(fDll);
            fDll = NULL;
            fFactory = NULL;
        }
        return NS_OK;
    }

    // ...else look through the list to find it...
    Instance *entry = fInstances;
    while (entry->fNext != NULL) {
        if (entry->fNext->fInstance == instance) {
            entry->fNext->fInstance->Release();
            entry->fNext = entry->fNext->fNext;
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE; // XXX
}


////////////////////////////////////////////////////////////////////////


