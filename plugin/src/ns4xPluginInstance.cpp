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

#include "ns4xPluginInstance.h"
#include "ns4xPluginStream.h"
#include "prlog.h"

////////////////////////////////////////////////////////////////////////


ns4xPluginInstance::ns4xPluginInstance(NPIPluginInstancePeer* peer,
                                       NPPluginFuncs* callbacks)
    : fPeer(peer), fCallbacks(callbacks)
{
    NS_INIT_REFCNT();
    PR_ASSERT(fPeer != NULL);
    PR_ASSERT(fCallbacks != NULL);

    fPeer->AddRef();

    // Initialize the NPP structure.
    fNPP.pdata = NULL;
    fNPP.ndata = this;
}


ns4xPluginInstance::~ns4xPluginInstance(void)
{
    fPeer->Release();
}

////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(ns4xPluginInstance);
NS_IMPL_RELEASE(ns4xPluginInstance);

static NS_DEFINE_IID(kIPluginInstanceIID, NP_IPLUGININSTANCE_IID); 
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

NS_METHOD
ns4xPluginInstance::QueryInterface(const nsIID& iid, void** instance)
{
    if (instance == NULL)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIPluginInstanceIID) ||
        iid.Equals(kISupportsIID)) {
        *instance = (void*) this;
        AddRef();
        return NS_OK;
    }

    return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////

NS_METHOD_(NPPluginError)
ns4xPluginInstance::Initialize(void)
{
    PRUint16 count;
    const char* const* names;
    const char* const* values;

    NPPluginError error;
    if ((error = fPeer->GetParameters(count, names, values)) != NPPluginError_NoError)
        return error;

    if (fCallbacks->newp == NULL)
        return NPPluginError_InvalidFunctableError; // XXX right error?
        
    // XXX Note that the NPPluginType_* enums were crafted to be
    // backward compatible...
    error = (NPPluginError)
        CallNPP_NewProc(fCallbacks->newp,
                        (char*) fPeer->GetMIMEType(),
                        &fNPP,
                        (PRUint16) fPeer->GetMode(),
                        count,
                        (char**) names,
                        (char**) values,
                        NULL); // saved data

    return error;
}

NS_METHOD_(NPPluginError)
ns4xPluginInstance::Start(void)
{
    // XXX At some point, we maybe should implement start and stop to
    // load/unload the 4.x plugin, just in case there are some plugins
    // that rely on that behavior...
    return NPPluginError_NoError;
}


NS_METHOD_(NPPluginError)
ns4xPluginInstance::Stop(void)
{
    return NPPluginError_NoError;
}



NS_METHOD_(NPPluginError)
ns4xPluginInstance::SetWindow(NPPluginWindow* window)
{
    // XXX 4.x plugins don't want a SetWindow(NULL).
    if (window == NULL)
        return NPPluginError_NoError;

    NPPluginError error = NPPluginError_NoError;
    if (fCallbacks->setwindow) {
        // XXX Turns out that NPPluginWindow and NPWindow are structurally
        // identical (on purpose!), so there's no need to make a copy.
        error = (NPPluginError) CallNPP_SetWindowProc(fCallbacks->setwindow,
                                                      &fNPP,
                                                      (NPWindow*) window);

        // XXX In the old code, we'd just ignore any errors coming
        // back from the plugin's SetWindow(). Is this the correct
        // behavior?!?
        PR_ASSERT(error == NPPluginError_NoError);
    }

    return error;
}


NS_METHOD_(NPPluginError)
ns4xPluginInstance::NewStream(NPIPluginStreamPeer* peer, NPIPluginStream* *result)
{
    (*result) = NULL;

    ns4xPluginStream* stream = new ns4xPluginStream(this, peer); // does it need the peer?
    if (stream == NULL)
        return NPPluginError_OutOfMemoryError;

    stream->AddRef();

    NPPluginError error;
    if ((error = stream->Initialize()) != NPPluginError_NoError) {
        stream->Release();
        return error;
    }

    (*result) = stream;
    return NPPluginError_NoError;
}


NS_METHOD_(void)
ns4xPluginInstance::Print(NPPluginPrint* platformPrint)
{
}


NS_METHOD_(PRInt16)
ns4xPluginInstance::HandleEvent(NPPluginEvent* event)
{
    return 0;
}


NS_METHOD_(void)
ns4xPluginInstance::URLNotify(const char* url, const char* target,
                              NPPluginReason reason, void* notifyData)
{
    if (fCallbacks->urlnotify != NULL) {
        CallNPP_URLNotifyProc(fCallbacks->urlnotify,
                              &fNPP,
                              url,
                              reason,
                              notifyData);
    }
}


NS_METHOD_(NPPluginError)
ns4xPluginInstance::GetValue(NPPluginVariable variable, void *value)
{
    return NPPluginError_NoError;
}


NS_METHOD_(NPPluginError)
ns4xPluginInstance::SetValue(NPPluginManagerVariable variable, void *value)
{
    return NPPluginError_NoError;
}


NS_METHOD_(jobject)
ns4xPluginInstance::GetJavaPeer(void)
{
    return (jobject) NULL;
}


