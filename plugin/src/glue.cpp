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

#include "np.h"
#include "pluginimpl.h"
#include "net.h"

#include "xp.h"

#include "plstr.h"
#include "prmem.h"

#include "nsIPlug.h"
#include "pluginimpl.h"
#include "nsIMimeTypeRegistry.h"
#include "nsIMimeType.h"
#include "nsIPluginClass.h"



////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIPluginClassIID, NS_IPLUGINCLASS_IID);


////////////////////////////////////////////////////////////////////////


/*
 * Standard conversion from NetLib status
 * code to plug-in reason code.
 */
static NPPluginReason
np_statusToReason(int status)
{
    switch (status) {
    case MK_DATA_LOADED:
		return NPPluginReason_Done;

    case MK_INTERRUPTED:
        return NPPluginReason_UserBreak;

    default:
        return NPPluginReason_NetworkErr;
    }
}


////////////////////////////////////////////////////////////////////////


extern "C" void
NPL_Init(void)
{
    static NS_DEFINE_IID(kIPluginManagerIID, NP_IPLUGINMANAGER_IID);
    if (nsPluginManager::Create(NULL, kIPluginManagerIID, (void**)&thePluginManager) == NS_OK) {
        thePluginManager->Initialize();
    }
}


extern "C" void
NPL_Shutdown(void)
{
    if (thePluginManager != NULL) {
        thePluginManager->Release();
        thePluginManager = NULL;
    }
}


extern "C" NPError
NPL_RefreshPluginList(XP_Bool reloadPages)
{
    return NPERR_NO_ERROR;
}


extern "C" NPBool
NPL_IteratePluginFiles(NPReference* ref, char** name, char** filename, char** description)
{
    return FALSE;
}


extern "C" NPBool
NPL_IteratePluginTypes(NPReference* ref, NPReference plugin, NPMIMEType* type,
                       char*** extents, char** description, void** fileType)
{
    return FALSE;
}

// XXX A better API would return an nsIEnumerator object.
extern "C" char**
NPL_FindPluginsForType(const char* typeToFind)
{
    return NULL;
}



/**
 * Returns the new C-string that is the name of the plugin
 * that is registered to handle the specified mime type, or
 * NULL if no plugin is registered to handle that type.
 */
extern "C" char*
NPL_FindPluginEnabledForType(const char* pszMimeType)
{
    char* result = NULL;
    nsIMimeTypeRegistry* registry = NULL;
    nsIMimeType* mimeType = NULL;
    nsIContentHandler* contentHandler = NULL;
    nsIPluginClass* pluginClass = NULL;

    if ((registry = thePluginManager->GetMimeTypeRegistry()) == NULL)
        goto done;

    if (! registry->Find(pszMimeType, &mimeType))
        goto done;

    if ((contentHandler = mimeType->GetHandler()) == NULL)
        goto done;

    if (contentHandler->QueryInterface(kIPluginClassIID, (void**) &pluginClass) != NS_OK)
        goto done;

    // whew...
    // XXX Note the PL_strdup() leaks all over the floor, but this is
    // the way the old API does it, and since I haven't had a chance
    // to go through all the callers...well, you get the idea.
    result = PL_strdup(pluginClass->GetName());

done:
    if (pluginClass != NULL)
        pluginClass->Release();

    if (contentHandler != NULL)
        contentHandler->Release();

    if (mimeType != NULL)
        mimeType->Release();

    if (registry != NULL)
        registry->Release();

    return result;
}


extern "C" NPError
NPL_EnablePlugin(NPMIMEType type,
                 const char* pluginName,
                 XP_Bool enabled)
{
    return NPERR_NO_ERROR;
}

extern "C" NPError
NPL_EnablePluginType(NPMIMEType type, void* pdesc, XP_Bool enabled)
{
    return NPERR_NO_ERROR;
}

extern "C" NPError
NPL_DisablePlugin(NPMIMEType type)
{
    return NPERR_NO_ERROR;
}



////////////////////////////////////////////////////////////////////////
// Stream related stuff
//

#if defined(XP_WIN) || defined(XP_OS2)
extern "C" void
FE_EmbedURLExit(URL_Struct* url, int status, MWContext* context); // XXX go away!
#endif


/** 
 * This is a special URLExit routine that is called only from the
 * "main" embed stream that is started in NPL_EmbedStart(). The reason
 * this exit procedure is used is because on Win32, the FE does some
 * other "magic", like spin the animation, etc., that we want to make
 * sure gets stopped.
 */
extern "C" void
NPL_EmbedURLExit(URL_Struct *url, int status, MWContext *cx)
{
	if (url && status != MK_CHANGING_CONTEXT) {
#if defined(XP_WIN) || defined(XP_OS2)
		/* WinFE is responsible for deleting the URL_Struct */
        NPEmbeddedApp* app = (NPEmbeddedApp*) url->fe_data;
        nsPluginInstancePeer* peer = (nsPluginInstancePeer*) app->np_data;
        PR_ASSERT(peer != NULL);
        if (peer == NULL) {
            NET_FreeURLStruct(url);
            return;
        }

        NPIPluginInstance* userInst = peer->GetUserInstance();
        PR_ASSERT(userInst != NULL);
        if (userInst != NULL) {
            userInst->URLNotify(url->address,
                                url->window_target,
                                np_statusToReason(status),
                                /* notifyData */ NULL);

            userInst->Release();
        }

	    FE_EmbedURLExit(url, status, cx);
#else
        NET_FreeURLStruct(url);
#endif
    }
}


////////////////////////////////////////////////////////////////////////
// Life-cycle and screen real-estate stuff
//

/**
 * Allocates a newly initialized NPEmbeddedApp struct. The "np_data"
 * field points to a new nsPluginInstancePeer.
 */
extern "C" NPEmbeddedApp*
NPL_EmbedCreate(MWContext *context, LO_EmbedStruct *embed_struct)
{
    // See if a peer was stashed away in the session data.
    nsPluginInstancePeer* peer
        = (nsPluginInstancePeer*) embed_struct->objTag.session_data;

    if (peer == NULL) {
        // Nope. This is the first time we've seen this
        // plugin. Allocate a new plugin instance peer for it.
        if ((peer = new nsPluginInstancePeer(NULL, /* outer, ok -- not aggregated */
                                             embed_struct)) == NULL)
            return NULL;

        // refcount of one, because we'll be returning it via the app...
        peer->AddRef();

        // Rebind the peer to the new MWContext.
        peer->SetContext(context);

        if (! (embed_struct->objTag.ele_attrmask & LO_ELE_HIDDEN))
            FE_CreateEmbedWindow(context, peer->GetApp()); // XXX Errors?

        // If they've specified a TYPE attribute on the tag, we can
        // create the user instance immediately. Otherwise, we'll need
        // to wait for the stream to be created to actually
        // instantiate the user instance.
        if (peer->GetMIMEType() != NULL) {
#ifdef LAYERS
            // XXX Why do we do this here? Couldn't we do it in NPL_EmbedCreate()?
            LO_SetEmbedType(peer->GetEmbedStruct(), peer->IsWindowed());
#endif
        }

        // If a SRC attribute is defined, start a stream. We only want
        // to do this the first time that a plugin is created, and we
        // create a "special" stream that we'll release with
        // NPL_EmbedURLExit()
        if (embed_struct->embed_src != NULL) {
            char* pszURL;
            PA_LOCK(pszURL, char*, embed_struct->embed_src);
            PR_ASSERT(pszURL != NULL);

            URL_Struct* url = NET_CreateURLStruct(pszURL, NET_DONT_RELOAD);
            PR_ASSERT(url != NULL);
            if (url != NULL) {
                url->fe_data = (void*) peer->GetApp(); // XXX wish it could be the peer
		
                // XXX We could #ifdef XP_WIN the NPL_EmbedURLExit() call...
                (void) NET_GetURL(url, FO_CACHE_AND_EMBED, context, NPL_EmbedURLExit);
            }

            PA_UNLOCK(embed_struct->embed_src); 
        }
    } else {
        // Okay, we're restoring a plugin that had been hidden off in
        // the session data. Grab the new embed struct...
        peer->SetEmbedStruct(embed_struct);

        // Bind to the new context.
        peer->SetContext(context);

        // Restore it's window...
        if (! (embed_struct->objTag.ele_attrmask & LO_ELE_HIDDEN))
            FE_RestoreEmbedWindow(context, peer->GetApp()); // XXX Errors?
    }

    NPIPluginInstance* userInst;
    if ((userInst = peer->GetUserInstance()) != NULL) {
        // Start the plugin instance.
        userInst->SetWindow((NPPluginWindow*) peer->GetApp()->wdata);
        userInst->Start();
        userInst->Release();
    }

    // Attach the app to the layout info
    embed_struct->objTag.FE_Data = peer->GetApp();

    return peer->GetApp();
}



/**
 * Instantiate a new plugin instance
 *
 * XXX This is broken in general. Right now (and in the previous API),
 * this function would create a plugin instance if the TYPE attribute
 * was available on the tag. It would start a stream if the DATA or
 * SRC attribute was set on the tag, which might have the side effect
 * of creating a new instance if we hadn't already done so here.
 *
 * I want to be able to "start" the embed here, but I guess I really
 * can't because if the TYPE tag isn't available, we won't have
 * created an instance.
 */
extern "C" NPError
NPL_EmbedStart(MWContext* cx, LO_EmbedStruct* embed_struct, NPEmbeddedApp* app)
{
    nsPluginInstancePeer* peer
        = (nsPluginInstancePeer*) app->np_data;

    /*
     * Now check for the SRC attribute.
     * - If it's full-page, create a instance now since we already 
     *	 know the MIME type (NPL_NewStream has already happened).
     * - If it's embedded, create a stream for the URL (we'll create 
     *	 the instance when we get the stream in NPL_NewStream).
     */
    if (embed_struct->embed_src == NULL)
        return (NPError) NPPluginError_NoError;

    char* pszURL;
    PA_LOCK(pszURL, char*, embed_struct->embed_src);
    PR_ASSERT(pszURL != NULL);

    if (PL_strcmp(pszURL, "internal-external-plugin") == 0) {
#if 0
        // XXX What does this do??? See special case code for
        // audio/LiveAudio in the cold codebase for clues...

        /*
         * Full-page case: Stream already exists, so now
         * we can create the instance.
         */
        np_reconnect* reconnect;
        np_mimetype* mimetype;
        np_handle* handle;
        np_instance* instance;
        char* requestedtype;
	    	
        app->pagePluginType = NP_FullPage;

        reconnect = (np_reconnect*) cx->pluginReconnect;
        XP_ASSERT(reconnect); 
        if (!reconnect) {
            PA_UNLOCK(embed_struct->embed_src); 
            goto error;
        }
			
        mimetype = reconnect->mimetype;
        requestedtype = reconnect->requestedtype;
        handle = mimetype->handle;
			
        /* Now we can create the instance */
        XP_ASSERT(ndata->instance == NULL);
        instance = np_newinstance(handle, cx, app, mimetype, requestedtype);
        if (!instance) {
            PA_UNLOCK(embed_struct->embed_src); 
            goto error;
        }
	    
        reconnect->app = app;
        ndata->instance =  instance;
        ndata->handle = handle;
        ndata->instance->app = app;
        FE_ShowScrollBars(cx, FALSE);
#ifdef LAYERS
        LO_SetEmbedType(ndata->lo_struct, (PRBool) ndata->instance->windowed);
#endif
#endif // 0
    }
	    
    PA_UNLOCK(embed_struct->embed_src); 
    return (NPError) NPPluginError_NoError;
}


/**
 * This is called whenever the FE decides that the shape of the embed
 * needs to change, the embed needs a new window, whatever. In the old
 * API, it was also overloaded to "start" the embed (for the new C++
 * embeds).
 */
extern "C" void
NPL_EmbedSize(NPEmbeddedApp *app)
{
    // Some sanity checks...
    PR_ASSERT(app != NULL);
    if (app == NULL)
        return;

    nsPluginInstancePeer* peer = (nsPluginInstancePeer*) app->np_data;
    PR_ASSERT(peer != NULL);
    if (peer == NULL)
        return;

    LO_EmbedStruct* embed_struct = peer->GetEmbedStruct();
    PR_ASSERT(embed_struct != NULL);
    if (embed_struct == NULL)
        return;

#ifndef XP_MAC
	/*
	 * On Windows and UNIX, we don't want to give a window
	 * to hidden plug-ins.  To determine if we're hidden,
	 * we can look at the flag bit of the LO_EmbedStruct.
	 */
    if (embed_struct->objTag.ele_attrmask & LO_ELE_HIDDEN)
        return;
#endif

    NPIPluginInstance* userInst = peer->GetUserInstance();
    PR_ASSERT(userInst != NULL);
    if (userInst == NULL)
        return;

    // XXX Note that NPWindow and NPPluginWindow are compatible
    userInst->SetWindow((NPPluginWindow*) app->wdata); // XXX Ignore error?
    userInst->Release();
}



/**
 * Delete the embedded object. This has some wacky semantics. If
 * NPL_SamePage() has been called, then the FE has instructed us that
 * we should "cache" the plugin, rather than really delete it. We also
 * have the case where the embed is being deleted from a printing
 * context. And the case where it's really being blown away for
 * real.
 *
 * What this _really_ means is that the MWContext that the embed is
 * living in is about to get real transient, that it should unattach
 * from the context, invalidate it's embed struct, etc.
 */
extern "C" void
NPL_EmbedDelete(MWContext *context, LO_EmbedStruct *embed_struct)
{
    // Some sanity checks
    PR_ASSERT(context != NULL);
    if (context == NULL)
        return;

    PR_ASSERT(embed_struct != NULL);
    if (embed_struct == NULL)
        return;

    NPEmbeddedApp* app = (NPEmbeddedApp*) embed_struct->objTag.FE_Data;
    PR_ASSERT(app != NULL);
    if (app == NULL)
        return;

    // Disconnect the plugin from layout...
    embed_struct->objTag.FE_Data = NULL;

    nsPluginInstancePeer* peer = (nsPluginInstancePeer*) app->np_data;
    PR_ASSERT(peer != NULL);
    if (peer == NULL)
        return;

    // Stuff the peer into the session_data pointer. This will
    // eventually get passed back to us in NPL_EmbedCreate(), or will
    // be released in NPL_DeleteSessionData()
    embed_struct->objTag.session_data = (void*) peer;

    // XXX When done printing, don't delete and don't save session
    // data. This might be take care of automagically now that we have
    // refcounting...
    if (context->type == MWContextPrint ||
        context->type == MWContextMetaFile ||
        context->type == MWContextPostScript) {
        FE_SaveEmbedWindow(context, app);
        return;
    }

#if 0 // XXX So we've been told via NPL_SamePage() to cache rather than delete...
    if (peer->ShouldCacheOnDelete()) {
        peer->SetCacheOnDelete(FALSE);
        peer->SetCached(TRUE);
        peer->SetContext(NULL);
        FE_SaveEmbedWindow(context, app);
        return;
    }
#endif

    NPIPluginInstance* userInst = peer->GetUserInstance();
    PR_ASSERT(userInst != NULL);
    if (userInst != NULL) {
        NPPluginError error = userInst->Stop();
        userInst->Release();

        if (error != NPPluginError_NoError) {
            // XXX Couldn't stop...hmm...
        }
        peer->SetEmbedStruct(NULL);
        peer->SetContext(NULL);
        FE_SaveEmbedWindow(context, app);
    }

    // Unbind from the MWContext
    peer->SetContext(NULL);
}


/**
 * This is called when the embed leaves the browser's history. It is,
 * in some sense, the thing that tells us to _really_ destroy the
 * embed instance.
 */
extern "C" void
NPL_DeleteSessionData(MWContext* context, void* sessionData)
{
    nsPluginInstancePeer* peer = (nsPluginInstancePeer*) sessionData;

    NPIPluginInstance* userInst = peer->GetUserInstance();
    if (userInst) {
        userInst->SetWindow(NULL);  // XXX crashes 4.x plugins
        userInst->Release();
    }

    if (context != NULL) {
#ifdef XP_MAC
        /* turn scrollbars back on */
        if (peer->GetApp()->pagePluginType == NP_FullPage)
            FE_ShowScrollBars(context, TRUE);
#endif

        FE_DestroyEmbedWindow(context, peer->GetApp());
    }

    // Since the MWContext is going away, unbind it _now_. There may
    // still be plugin-side references to the peer that could be
    // valid.
    peer->SetContext(NULL);

    // We don't need it anymore...
    peer->SetUserInstance(NULL);
    peer->Release();
}

////////////////////////////////////////////////////////////////////////

/* ~~av the following is used in CGenericDoc::FreeEmbedElement */
extern "C" int32
NPL_GetEmbedReferenceCount(NPEmbeddedApp *app)
{
    return 0;
}

extern "C" int
NPL_HandleEvent(NPEmbeddedApp *app, void *event, void* window)
{
    return 0;
}
 /* window may be NULL */
extern "C" void
NPL_Print(NPEmbeddedApp *app, void *printData)
{
}

extern "C" void
NPL_SamePage(MWContext *context)
{
}

extern "C" void
NPL_SameElement(LO_EmbedStruct *embed)
{
}

extern "C" XP_Bool
NPL_IsLiveConnected(LO_EmbedStruct *embed)
{
    return FALSE;
}


extern "C" XP_Bool
NPL_HandleURL(MWContext *pContext, FO_Present_Types iFormatOut, URL_Struct *pURL,
              Net_GetUrlExitFunc *pExitFunc)
{
    return FALSE;
}

#ifndef XP_MAC								
extern "C" void
NPL_DisplayPluginsAsHTML(FO_Present_Types format_out, URL_Struct *urls, MWContext *cx)
{
}

#endif


extern "C" void
NPL_PreparePrint(MWContext* context, SHIST_SavedData* savedData)
{
}


extern "C" NET_StreamClass*
NPL_NewEmbedStream(FO_Present_Types format_out, void *type, URL_Struct *urls, MWContext *cx)
{
    return NULL;
}

extern "C" NET_StreamClass*
NPL_NewPresentStream(FO_Present_Types format_out, void *type, URL_Struct *urls, MWContext *cx)
{
    return NULL;
}

extern "C" unsigned int
NPL_WriteReady(NET_StreamClass *stream)
{
    return 0;
}

extern "C" int
NPL_Write(NET_StreamClass *stream, const unsigned char *str, int32 len)
{
    return 0;
}

extern "C" void
NPL_Complete(NET_StreamClass *stream)
{
}

extern "C" void
NPL_Abort(NET_StreamClass *stream, int status)
{
}

extern "C" XP_Bool
NPL_IsEmbedWindowed(NPEmbeddedApp *app)
{
    if (app != NULL) {
        nsPluginInstancePeer* peer = (nsPluginInstancePeer*) app->np_data;
        return peer->IsWindowed();
    }
    return FALSE;;
}

extern "C" void
NPL_URLExit(URL_Struct *urls, int status, MWContext *cx)
{
}


#ifdef ANTHRAX
extern "C" char**
NPL_FindAppletsForType(const char* typeToFind)
{
    return NULL;
}

extern "C" char*
NPL_FindAppletEnabledForMimetype(const char* mimetype)
{
    return NULL;
}

extern "C" NPError
NPL_RegisterAppletType(NPMIMEType type)
{
    return NPERR_NO_ERROR;
}

#endif /* ANTHRAX */

PR_EXTERN(void)
NPL_SetPluginWindow(void *data)
{
}



