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
#include "npapi.h"

#include "xp.h"       // for MWContext
#include "xpgetstr.h"

#include "lo_ele.h"   // for LO_EmbedStruct
#include "layers.h"   // for CL_* stuff
#include "prefapi.h"  // for PREF_* stuff

#include "plstr.h"
#include "prprf.h"    // for PR_smprintf()

////////////////////////////////////////////////////////////////////////

extern int XP_PLUGIN_CANT_LOAD_PLUGIN;
static NS_DEFINE_IID(kIPluginClassIID, NS_IPLUGINCLASS_IID);

// XXX Ugh. This should be exported via a header file! Besides the
// fact that it's different between Unix and Mac/Win!
extern "C" NPPluginError
FE_PluginGetValue(MWContext* context,
                  NPEmbeddedApp* app,
                  NPPluginManagerVariable variable,
                  void* result);



////////////////////////////////////////////////////////////////////////////////
// Plugin Instance Peer Interface

nsPluginInstancePeer::nsPluginInstancePeer(nsISupports* outer,
                                           LO_EmbedStruct* embed_struct)
    : fUserInst(NULL), fClass(NULL), fMimeType(NULL), fEmbedStruct(embed_struct),
      fContext(NULL), fWindowed(TRUE), fTransparent(FALSE)
#ifdef PLUGIN_TIMER_EVENT
      ,fTimeOut(NULL), fTimerInterval(0)
#endif
#ifdef OJI
      ,fJVMInstancePeer(NULL)
#endif
{
    NS_INIT_AGGREGATED(outer);

    fApp.next           = NULL;
    fApp.type           = NP_Plugin;   // XXX If it was NP_OLE, we wouldn't be here, right?
    fApp.fe_data        = NULL;
    fApp.np_data        = this;
    fApp.wdata          = (NPWindow*) NULL;
    fApp.pagePluginType = NP_Embedded; // default. may change later...

    if (GetMIMEType() != NULL)
        CreatePluginInstance();
}




nsPluginInstancePeer::~nsPluginInstancePeer(void)
{
#ifdef PLUGIN_TIMER_EVENT			
    if (fTimeout != NULL)
        FE_ClearTimeout(fTimeout);
#endif			

    // XXX Kill of open streams

    if (fUserInst)
        fUserInst->Release();

#ifdef JAVA
    // XXX Break association between us & Java
#endif // JAVA

    // Uncouple from layout, if necessary (XXX should've been done by now...)
    if (fEmbedStruct != NULL && fEmbedStruct->objTag.FE_Data == this)
        fEmbedStruct->objTag.FE_Data = NULL;

    if (fMimeType != NULL)
        fMimeType->Release();

    if (fClass != NULL)
        fClass->Release();
}

////////////////////////////////////////////////////////////////////////
// Implementation of the aggregated interface
//

NS_IMPL_AGGREGATED(nsPluginInstancePeer);

NS_METHOD
nsPluginInstancePeer::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr) 
{
    if (NULL == aInstancePtr) {                                            
        return NS_ERROR_NULL_POINTER;                                        
    }                                                                      

    static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
    static NS_DEFINE_IID(kLiveConnectPluginInstancePeerIID, NP_ILIVECONNECTPLUGININSTANCEPEER_IID); 
    static NS_DEFINE_IID(kPluginInstancePeerIID, NP_IPLUGININSTANCEPEER_IID); 

    if (aIID.Equals(kLiveConnectPluginInstancePeerIID) ||
        aIID.Equals(kPluginInstancePeerIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this); 
        AddRef(); 
        return NS_OK; 
    } 
#ifdef OJI
    // Aggregates...
    if (fJVMInstancePeer == NULL) {
        np_instance* instance = (np_instance*) npp->ndata;
        NPEmbeddedApp* app = instance->app;
        MWContext* cx = instance->cx;
        np_data* ndata = (np_data*) app->np_data;
        nsresult result =
            JVMInstancePeer::Create((nsISupports*)this, kISupportsIID, (void**)&fJVMInstancePeer,
                                    cx, (struct LO_JavaAppStruct_struct*)ndata->lo_struct); // XXX wrong kind of LO_Struct!
        if (result != NS_OK) return result;
    }
#endif
#ifdef OJI
    return fJVMInstancePeer->QueryInterface(aIID, aInstancePtr);
#endif
    return NS_NOINTERFACE;
}



////////////////////////////////////////////////////////////////////////
// Methods specific to nsIPluginInstancePeer
//

NS_METHOD_(NPIPlugin*)
nsPluginInstancePeer::GetClass(void)
{
    return NULL;        // XXX
}



NS_METHOD_(const char*)
nsPluginInstancePeer::GetMIMEType(void)
{
    // If it's been computed already, just return it.
    if (fMimeType != NULL)
        return (const char*) (*fMimeType);

    // Next look through the tags to see if a TYPE attribute has been
    // specified: we can set our mime type from that.
    for (int i = 0; i < fEmbedStruct->attribute_cnt; i++) {
        if (PL_strcasecmp(fEmbedStruct->attribute_list[i], "type") != 0)
            continue;

        const char* pszMimeType = fEmbedStruct->value_list[i];

        nsIMimeTypeRegistry* registry = NULL;
        if ((registry = thePluginManager->GetMimeTypeRegistry()) != NULL) {
            registry->FindClosestMatch(pszMimeType, &fMimeType);
            registry->Release();
        }
    
        if (fMimeType == NULL)
            return NULL;

        return (const char*) (*fMimeType);
    }

    // Otherwise, we're just out of luck...
    return NULL;
}




NS_METHOD_(NPPluginType)
nsPluginInstancePeer::GetMode(void)
{
    return (fApp.pagePluginType == NP_FullPage) ? NPPluginType_Full : NPPluginType_Embedded;
}



static char* empty_list[] = { "", NULL };




NS_IMETHODIMP_(NPPluginError)
nsPluginInstancePeer::GetAttributes(PRUint16& n, const char*const*& names, const char*const*& values)
{
    if (GetMode() == NPPluginType_Embedded) {
#ifdef OJI
        names = (const char*const*)fEmbedStruct->attributes.names;
        values = (const char*const*)fEmbedStruct->attributes.values;
        n = (PRUint16)fEmbedStruct->attributes.n;
#else
        // XXX I think all the parameters and attributes just get
        // munged together under MOZ_JAVA...
        names = (const char*const*) fEmbedStruct->attribute_list;
        values = (const char*const*) fEmbedStruct->value_list;
        n = (PRUint16) fEmbedStruct->attribute_cnt;
#endif

        return NPPluginError_NoError;
    } else {
        static char _name[] = "PALETTE";
        static char* _names[1];

        static char _value[] = "foreground";
        static char* _values[1];

        _names[0] = _name;
        _values[0] = _value;

        names = (const char*const*) _names;
        values = (const char*const*) _values;
        n = 1;

        return NPPluginError_NoError;
    }

    // random, sun-spot induced error
    PR_ASSERT( 0 );

    n = 0;
    // const char* const* empty_list = { { '\0' } };
    names = values = (const char*const*)empty_list;

    return NPPluginError_GenericError;
}




NS_IMETHODIMP_(const char*)
nsPluginInstancePeer::GetAttribute(const char* name)
{
    PRUint16 nAttrs, i;
    const char*const* names;
    const char*const* values;

    if( NPCallFailed( GetAttributes( nAttrs, names, values )) )
      return 0;

    for( i = 0; i < nAttrs; i++ ) {
        if( PL_strcasecmp( name, names[i] ) == 0 )
            return values[i];
    }

    return 0;
}




NS_IMETHODIMP_(NPPluginError)
nsPluginInstancePeer::GetParameters(PRUint16& n, const char*const*& names,
                                    const char*const*& values)
{
    if (GetMode() == NPPluginType_Embedded) {

#ifdef OJI
        names = (const char*const*)fEmbedStruct->parameters.names;
        values = (const char*const*)fEmbedStruct->parameters.values;
        n = (PRUint16)fEmbedStruct->parameters.n;
#else
        // XXX I think all the parameters and attributes just get
        // munged together under MOZ_JAVA...
        names = (const char*const*) fEmbedStruct->attribute_list;
        values = (const char*const*) fEmbedStruct->value_list;
        n = (PRUint16)fEmbedStruct->attribute_cnt;
#endif

        return NPPluginError_NoError;
    } else {
        static char _name[] = "PALETTE";
        static char* _names[1];

        static char _value[] = "foreground";
        static char* _values[1];

        _names[0] = _name;
        _values[0] = _value;

        names = (const char*const*) _names;
        values = (const char*const*) _values;
        n = 1;

        return NPPluginError_NoError;
    }

    // random, sun-spot induced error
    PR_ASSERT( 0 );

    n = 0;
    // static const char* const* empty_list = { { '\0' } };
    names = values = (const char*const*)empty_list;

    return NPPluginError_GenericError;
}




NS_IMETHODIMP_(const char*)
nsPluginInstancePeer::GetParameter(const char* name)
{
    PRUint16 nParams, i;
    const char*const* names;
    const char*const* values;

    if( NPCallFailed( GetParameters( nParams, names, values )) )
      return 0;

    for( i = 0; i < nParams; i++ ) {
        if( PL_strcasecmp( name, names[i] ) == 0 )
            return values[i];
    }

    return 0;
}




NS_IMETHODIMP_(const char *)
nsPluginInstancePeer::GetTagText(void)
{
    // XXX
    return NULL;
}




NS_IMETHODIMP_(NPTagType) 
nsPluginInstancePeer::GetTagType(void)
{
#ifdef XXX
    switch (fLayoutElement->lo_element.type) {
      case LO_JAVA:
        return NPTagType_Applet;
      case LO_EMBED:
        return NPTagType_Embed;
      case LO_OBJECT:
        return NPTagType_Object;

      default:
        return NPTagType_Unknown;
    }
#endif
    return NPTagType_Unknown;
}




NS_METHOD_(NPIPluginManager*)
nsPluginInstancePeer::GetPluginManager(void)
{
    return thePluginManager;
}





NS_METHOD_(NPPluginError)
nsPluginInstancePeer::GetURL(const char* url, const char* target, void* notifyData,
                             const char* altHost, const char* referrer,
                             PRBool forceJSEnabled)
{
#if 0
    return (NPPluginError)np_geturlinternal(npp, url, target, altHost, referrer,
                                            forceJSEnabled, notifyData != NULL, notifyData);
#endif
    return NPPluginError_NoError;
}




NS_METHOD_(NPPluginError)
nsPluginInstancePeer::PostURL(const char* url, const char* target, PRUint32 bufLen, 
                              const char* buf, PRBool file, void* notifyData,
                              const char* altHost, const char* referrer,
                              PRBool forceJSEnabled,
                              PRUint32 postHeadersLength, const char* postHeaders)
{
#if 0
    return (NPPluginError)np_posturlinternal(npp, url, target, altHost, referrer, forceJSEnabled,
                                             bufLen, buf, file, notifyData != NULL, notifyData);
#endif
    return NPPluginError_NoError;
}




NS_METHOD_(NPPluginError)
nsPluginInstancePeer::NewStream(nsMIMEType type, const char* target,
                                NPIPluginManagerStream* *result)
{
#if 0
    NPStream* pstream;
    NPPluginError err = (NPPluginError)
        npn_newstream(npp, (char*)type, (char*)target, &pstream);
    if (err != NPPluginError_NoError)
        return err;
    *result = new nsPluginManagerStream(npp, pstream);
    return NPPluginError_NoError;
#endif
    return NPPluginError_NoError;
}



NS_METHOD_(void)
nsPluginInstancePeer::ShowStatus(const char* message)
{
    MWContext* context = GetContext();
    if (context == NULL)
        return;

#ifdef XP_MAC
    /* Special entry point so MacFE can save/restore port state */
    FE_PluginProgress(context, message);
#else
    FE_Progress(context, message);
#endif
}



NS_METHOD_(const char*)
nsPluginInstancePeer::UserAgent(void)
{
    static char *userAgent = 0;
    if (userAgent == NULL) {
        userAgent = PR_smprintf("%.100s/%.90s", XP_AppCodeName, XP_AppVersion);
    }
    return userAgent;
}



NS_METHOD_(NPPluginError)
nsPluginInstancePeer::GetValue(NPPluginManagerVariable variable, void *value)
{
	if (value == NULL)
		return NPPluginError_InvalidParam;

	/*
     * Some of these variabled may be handled by backend. The rest is FE.
	 * So Handle all the backend variables and pass the rest over to FE.
	 */

	switch(variable) {
    case NPNVjavascriptEnabledBool : 
        if (PREF_GetBoolPref("javascript.enabled", (PRBool*)value) == 0) {
            return NPPluginError_NoError;
        } else {
            return NPPluginError_GenericError;
        }

    case NPNVasdEnabledBool :
        if (PREF_GetBoolPref("autoupdate.enabled", (PRBool*)value) == 0) {
            return NPPluginError_NoError;
        } else {
            return NPPluginError_GenericError;
        }

#if 0 // XXX MOZ_OFFLINE
    case NPNVisOfflineBool:
        {
            PRBool *bptr = (PRBool*)value; 
            *bptr = NET_IsOffline();
        }
        return NPPluginError_NoError;
#endif /* MOZ_OFFLINE */

    default:
        // Hand over to the FE...
        break;
    }

    // Ensure that we have an MWContext to call thru to the FE with...
    if (fContext == NULL)
        return NPPluginError_GenericError; // XXX

#ifdef XP_UNIX
    return FE_PluginGetValue(instance?instance->handle->pdesc:NULL, // XXX!
                             variable, r_value);
#else
    return FE_PluginGetValue(fContext, &fApp, variable, value);
#endif /* XP_UNIX */

    return NPPluginError_NoError;
}






NS_METHOD_(NPPluginError)
nsPluginInstancePeer::SetValue(NPPluginVariable variable, void *value)
{
    switch (variable) {
    case NPPVpluginWindowBool:
        /* 
         * XXX On the Mac, a window has already been allocated by the time NPP_New
         * has been called - which is fine, since we'll still use the window. 
         * Unfortunately, we can't use the window's presence to determine whether
         * it's too late to set the windowed property.
         */
#ifdef XP_MAC
        /* 
         * If the window has already been allocated, it's too late
         * to tell us.
         */
        if (fApp->wdata->window == NULL)
            return NPPluginError_InvalidParam;
#endif

        fWindowed = (PRBool) value;
        break;

    case NPPVpluginTransparentBool:
        {
            fTransparent = (PRBool) value;
#ifdef LAYERS
            CL_Layer* layer = fEmbedStruct->objTag.layer;
            if (layer && 
                (fTransparent != !(CL_GetLayerFlags(layer) & CL_OPAQUE))) {// XXX Huh?
            	XP_Rect bbox;
            	
            	/* Get the bbox and convert it into its own coordinate space */
            	CL_GetLayerBbox(layer, &bbox);
            	
                CL_ChangeLayerFlag(layer, CL_OPAQUE, (PRBool)!fTransparent);
                CL_ChangeLayerFlag(layer, 
                                   CL_PREFER_DRAW_OFFSCREEN,
                                   (PRBool)fTransparent);

				/* Force drawing of the entire transparent plug-in. */
                CL_UpdateLayerRect(CL_GetLayerCompositor(layer),
                				   layer, &bbox, PR_FALSE);
            }
#endif /* LAYERS */
        }
        break;

#ifdef PLUGIN_TIMER_EVENT
    case NPPVpluginTimerInterval:
        fTimerInterval = *((PRUint32*) value);
        fTimeout = FE_SetTimeout(TimerCallback, (void*) this, fInterval);
        break;
#endif

    default:
        break;
    }
    return NPPluginError_NoError;
}




#ifdef PLUGIN_TIMER_EVENT
void
nsPluginInstancePeer::TimerCallback(void* closure)
{
    nsPluginInstancePeer* inst = (nsPluginInstance*) closure;
    NPPluginEvent event;

#ifdef XP_MAC
	((EventRecord)event).what = nullEvent;
#elif defined(XP_WIN)
	event.event = 0; // ?
#elif defined(XP_OS2)
	event.event = 0; // ?
#elif defined(XP_UNIX)
	// not sure what to do here
#endif

	inst->fTimeout = FE_SetTimeout(TimerCallback, (void*)inst, inst->fTimerInterval);

    NPIPluginInstance* userInst = inst->GetUserInstance();

    PR_ASSERT(userInst != NULL);
    if (userInst == NULL)
        return;

    userInst->HandleEvent(&event);
    userInst->Release();
}
#endif



NS_METHOD_(void)
nsPluginInstancePeer::InvalidateRect(nsRect *invalidRect)
{
#if 0
    npn_invalidaterect(npp, (NPRect*)invalidRect);
#endif
}

NS_METHOD_(void)
nsPluginInstancePeer::InvalidateRegion(nsRegion invalidRegion)
{
#if 0
    npn_invalidateregion(npp, invalidRegion);
#endif
}

NS_METHOD_(void)
nsPluginInstancePeer::ForceRedraw(void)
{
#if 0
    npn_forceredraw(npp);
#endif
}

NS_METHOD_(void)
nsPluginInstancePeer::RegisterWindow(void* window)
{
#if 0
	npn_registerwindow(npp, window);
#endif
}

NS_METHOD_(void)
nsPluginInstancePeer::UnregisterWindow(void* window)
{
#if 0
	npn_unregisterwindow(npp, window);
#endif
}

NS_METHOD_(PRInt16)
nsPluginInstancePeer::AllocateMenuID(PRBool isSubmenu)
{
#if 0
	return npn_allocateMenuID(npp, isSubmenu);
#endif
    return 0;
}

NS_METHOD_(jref)
nsPluginInstancePeer::GetJavaPeer(void)
{
#if 0
    return npn_getJavaPeer(npp);
#endif
    return NULL;
}


////////////////////////////////////////////////////////////////////////
// Methods specific to nsPluginInstancePeer
//

NS_METHOD_(NPIPluginInstance*)
nsPluginInstancePeer::GetUserInstance(void)
{
    if (fUserInst)
        fUserInst->AddRef();

    return fUserInst;
};




NS_METHOD_(void)
nsPluginInstancePeer::SetUserInstance(NPIPluginInstance* inst)
{
    if (fUserInst)
        fUserInst->Release();

    fUserInst = inst;
    if (fUserInst)
        fUserInst->AddRef();
};



NS_METHOD_(void)
nsPluginInstancePeer::SetContext(MWContext* context)
{
    NPEmbeddedApp* app = GetApp();

    // unbind ourselves from the current context
    if (fContext != NULL) {
        if (app == fContext->pluginList) {
            fContext->pluginList = app->next;
        } else {
            NPEmbeddedApp *p = fContext->pluginList;
            while (p->next) {
                if (p->next == app) {
                    p->next = p->next->next;
                    break;
                }
                p = p->next;
            }
        }
        app->next = NULL;
    }

    fContext = context;

    // rebind ourselves to the new context
    if (fContext != NULL) {
        PR_ASSERT(app->next == NULL);

        if (fContext->pluginList == NULL) {
            fContext->pluginList = app;
        } else {
            NPEmbeddedApp* p = fContext->pluginList;
            while (p->next)
                p = p->next;

            p->next = app;
        }
    }
}



NS_METHOD_(NET_StreamClass*)
nsPluginInstancePeer::CreateStream(FO_Present_Types formatOut,
                                   URL_Struct* url,
                                   MWContext* context)
{
    if (fUserInst == NULL) {
        // We don't have an instance yet: create one using the MIME
        // type to find the appropriate factory.
        PR_ASSERT(url->content_type != NULL);
        if (url->content_type == NULL)
            return NULL;

        // Find the mime type that corresponds to the URL's content type
        nsIMimeTypeRegistry* registry;
        if ((registry = thePluginManager->GetMimeTypeRegistry()) != NULL) {
            registry->FindClosestMatch(url->content_type, &fMimeType);
            registry->Release();
        }
    
        PR_ASSERT(fMimeType != NULL);
        if (fMimeType == NULL)
            return NULL;

        // ...and create a new instance.
        if (CreatePluginInstance() != NS_OK)
            return NULL;

        PR_ASSERT(fUserInst != NULL);
        fUserInst->SetWindow((NPPluginWindow*) fApp.wdata);
        fUserInst->Start();
    }

    // Now that we've ensured that there's an instance, create a
    // stream for it.
    nsPluginStreamPeer* streamPeer
        = new nsPluginStreamPeer(this, formatOut, url);

    PR_ASSERT(streamPeer != NULL);
    if (streamPeer == NULL)
        return NULL;

    // XXX Add to the set of open streams for this instance, rather
    // than just addreffing...

    // A "netlib" refcount, released from nsPluginStreamPeer::Complete()
    // or nsPluginStreamPeer::Abort()
    streamPeer->AddRef();

    NPIPluginStream* stream = NULL;
    if ((fUserInst->NewStream(streamPeer, &stream) != NPPluginError_NoError) ||
        (stream == NULL)) {
        // Couldn't create a plugin-side stream.
        streamPeer->Release();
        return NULL;
    }
    
    streamPeer->SetUserStream(stream);

    // Now that the stream peer has a handle to it, we don't need
    // it any more. (NewStream initializes the ref count to one.)
    stream->Release();

    return streamPeer->GetNetlibStream();
}



nsresult
nsPluginInstancePeer::CreatePluginInstance(void)
{
    PR_ASSERT(fUserInst == NULL);
    if (fUserInst != NULL)
        return NS_ERROR_UNEXPECTED;

    PR_ASSERT(fMimeType != NULL);
    if (fMimeType == NULL)
        return NS_ERROR_NULL_POINTER;

    nsresult error;
    nsIContentHandler* contentHandler = NULL;

    if ((contentHandler = fMimeType->GetHandler()) == NULL) {
        error = NS_ERROR_NOT_AVAILABLE;
        goto done;
    }

    if (contentHandler->QueryInterface(kIPluginClassIID, (void**) &fClass) != NS_OK) {
        error = NS_ERROR_UNEXPECTED;
        goto done;
    }

    // Whew. Now we've got the factory for instances: go ahead and
    // create one.
    if ((error = fClass->NewInstance(this, &fUserInst)) != NS_OK) {
        if (GetContext() != NULL) {
            char* msg = PR_smprintf(XP_GetString(XP_PLUGIN_CANT_LOAD_PLUGIN),
                                    fClass->GetName(),
                                    (const char*) fMimeType);

            FE_Alert(GetContext(), msg);
            PR_smprintf_free(msg);
        }
        goto done;
    }

    error = NS_OK;

done:
    if (contentHandler != NULL)
        contentHandler->Release();

    return error;
}









