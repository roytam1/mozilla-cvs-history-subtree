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


#include "ns4xPluginStream.h"
#include "prlog.h"             // for PR_ASSERT

////////////////////////////////////////////////////////////////////////


ns4xPluginStream::ns4xPluginStream(ns4xPluginInstance* instance,
                                   NPIPluginStreamPeer* peer)
    : fInstance(instance), fPeer(peer), fStreamType(NPStreamType_Normal),
      fSeekable(FALSE), fPosition(0)
{
    NS_INIT_REFCNT();
    PR_ASSERT(fInstance != NULL);
    fInstance->AddRef();

    PR_ASSERT(fPeer != NULL);
    fPeer->AddRef();

    // Initialize the 4.x interface structure
    memset(&fNPStream, 0, sizeof(fNPStream));
    fNPStream.ndata        = (void*) fPeer;
    fNPStream.url          = fPeer->GetURL();
    fNPStream.end          = fPeer->GetContentLength();
    fNPStream.lastmodified = fPeer->GetLastModified();

    // Are we seekable?
    static NS_DEFINE_IID(kISeekablePluginStreamPeerIID, NP_ISEEKABLEPLUGINSTREAMPEER_IID);
    nsISupports* seekablePeer;
    if (fPeer->QueryInterface(kISeekablePluginStreamPeerIID,
                              (void**) &seekablePeer) == NS_OK) {
        fSeekable = TRUE;
        seekablePeer->Release();
    }
}


ns4xPluginStream::~ns4xPluginStream(void)
{
    if (fInstance->GetCallbacks()->destroystream != NULL) {
        CallNPP_DestroyStreamProc(fInstance->GetCallbacks()->destroystream,
                                  fInstance->GetNPP(),
                                  &fNPStream,
                                  fPeer->GetReason());
    }

    if (fPeer != NULL)
        fPeer->Release();

    if (fInstance != NULL)
        fInstance->Release();
}


////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(ns4xPluginStream);
NS_IMPL_RELEASE(ns4xPluginStream);

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIPluginStreamIID, NP_IPLUGINSTREAM_IID);

NS_METHOD
ns4xPluginStream::QueryInterface(const nsIID& iid, void** instance)
{
    if (instance == NULL)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIPluginStreamIID) ||
        iid.Equals(kISupportsIID)) {
        *instance = (void*) this;
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////


NS_METHOD_(NPPluginError)
ns4xPluginStream::Initialize(void)
{
    if (fInstance->GetCallbacks()->newstream == NULL)
        return NPPluginError_GenericError;

    PRUint16 streamType = (PRUint16) fStreamType;

    NPPluginError error
        = (NPPluginError) CallNPP_NewStreamProc(fInstance->GetCallbacks()->newstream,
                                                fInstance->GetNPP(),
                                                (char*) fPeer->GetMIMEType(),
                                                &fNPStream,
                                                fSeekable,
                                                &streamType);

    fStreamType = (NPStreamType) streamType;

    return error;
}



NS_METHOD_(PRInt32)
ns4xPluginStream::WriteReady(void)
{
    if (fInstance->GetCallbacks()->writeready == NULL)
        return 0;

    PRInt32 result =
        CallNPP_WriteReadyProc(fInstance->GetCallbacks()->writeready,
                               fInstance->GetNPP(),
                               &fNPStream);

    return result;
}



NS_METHOD_(PRInt32)
ns4xPluginStream::Write(PRInt32 len, const char* buffer)
{
    if (fInstance->GetCallbacks()->write == NULL)
        return 0;

    PRInt32 result =
        CallNPP_WriteProc(fInstance->GetCallbacks()->write,
                          fInstance->GetNPP(),
                          &fNPStream, 
                          fPosition,
                          len,
                          (void *)buffer);

    fPosition += len;
    return result;
}



NS_METHOD_(NPStreamType)
ns4xPluginStream::GetStreamType(void)
{
    return fStreamType;
}




NS_METHOD_(void)
ns4xPluginStream::AsFile(const char* filename)
{
    if (fInstance->GetCallbacks()->asfile == NULL)
        return;

    CallNPP_StreamAsFileProc(fInstance->GetCallbacks()->asfile,
                             fInstance->GetNPP(),
                             &fNPStream,
                             filename);
}



////////////////////////////////////////////////////////////////////////

