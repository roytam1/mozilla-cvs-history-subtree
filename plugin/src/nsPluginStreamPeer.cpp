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
#include "prmem.h"
#include "merrors.h"     // for MK_*

////////////////////////////////////////////////////////////////////////////////
// Plugin Stream Peer Interface

nsPluginStreamPeer::nsPluginStreamPeer(nsPluginInstancePeer* instancePeer,
                                       FO_Present_Types formatOut,
                                       URL_Struct *url)
    : fUserStream(NULL), fInstancePeer(instancePeer), fFormatOut(formatOut),
      fURL(url), fReason(NPPluginReason_NoReason)
{
    NS_INIT_REFCNT();
    fInstancePeer->AddRef(); // XXX right thing to do?

    fNetlibStream = PR_NEWZAP(NET_StreamClass);
    fNetlibStream->name           = "plug-in";
    fNetlibStream->complete       = Complete;
    fNetlibStream->abort          = Abort;
    fNetlibStream->is_write_ready = WriteReady;
    fNetlibStream->put_block      = Write;
    fNetlibStream->data_object    = (void*) this;
    fNetlibStream->window_id      = fInstancePeer->GetContext();
}

nsPluginStreamPeer::~nsPluginStreamPeer(void)
{
    // XXX Netlib will free the fNetlibStream object.

    SetUserStream(NULL);

    if (fInstancePeer != NULL) {
        fInstancePeer->Release();
        fInstancePeer = NULL;
    }
}


////////////////////////////////////////////////////////////////////////
// nsISupports methods
//

static NS_DEFINE_IID(kSeekablePluginStreamPeerIID, NP_ISEEKABLEPLUGINSTREAMPEER_IID);
static NS_DEFINE_IID(kPluginStreamPeerIID, NP_IPLUGINSTREAMPEER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

NS_IMPL_ADDREF(nsPluginStreamPeer);
NS_IMPL_RELEASE(nsPluginStreamPeer);

NS_METHOD
nsPluginStreamPeer::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    if (NULL == aInstancePtr) {
        return NS_ERROR_NULL_POINTER; 
    }
#if 0
    if (aIID.Equals(kSeekablePluginStreamPeerIID) &&
        IsSeekable()) {
    }
#endif
    if (aIID.Equals(kPluginStreamPeerIID) ||
        aIID.Equals(kISupportsIID)) {
        *aInstancePtr = (void*) ((nsISupports*)this); 
        AddRef(); 
        return NS_OK; 
    }
    return NS_NOINTERFACE; 
} 



////////////////////////////////////////////////////////////////////////
// NPIPluginStreamPeer methods
//

NS_IMETHODIMP_(const char*)
nsPluginStreamPeer::GetURL(void)
{
    return fURL->address; // XXX Is this right?
}

NS_IMETHODIMP_(PRUint32)
nsPluginStreamPeer::GetEnd(void)
{
    return fURL->content_length;
}

NS_IMETHODIMP_(PRUint32)
nsPluginStreamPeer::GetLastModified(void)
{
    return (PRUint32) fURL->last_modified;
}

NS_IMETHODIMP_(void*)
nsPluginStreamPeer::GetNotifyData(void)
{
    return NULL; // XXX what the heck is this? see np_makestreamobjects() to start...
}

NS_METHOD_(NPPluginReason)
nsPluginStreamPeer::GetReason(void)
{
    return fReason;
}

NS_METHOD_(nsMIMEType)
nsPluginStreamPeer::GetMIMEType(void)
{
    return (nsMIMEType)fURL->content_type;
}

NS_METHOD_(PRUint32)
nsPluginStreamPeer::GetContentLength(void)
{
    return fURL->content_length;
}
#if 0
NS_METHOD_(const char*)
nsPluginStreamPeer::GetContentEncoding(void)
{
    return fURL->content_encoding;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetCharSet(void)
{
    return fURL->charset;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetBoundary(void)
{
    return fURL->boundary;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetContentName(void)
{
    return fURL->content_name;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetExpires(void)
{
    return fURL->expires;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetLastModified(void)
{
    return fURL->last_modified;
}

NS_METHOD_(time_t)
nsPluginStreamPeer::GetServerDate(void)
{
    return fURL->server_date;
}

NS_METHOD_(NPServerStatus)
nsPluginStreamPeer::GetServerStatus(void)
{
    return fURL->server_status;
}
#endif
NS_METHOD_(PRUint32)
nsPluginStreamPeer::GetHeaderFieldCount(void)
{
    return fURL->all_headers.empty_index;
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetHeaderFieldKey(PRUint32 index)
{
    return fURL->all_headers.key[index];
}

NS_METHOD_(const char*)
nsPluginStreamPeer::GetHeaderField(PRUint32 index)
{
    return fURL->all_headers.value[index];
}

NS_METHOD_(NPPluginError)
nsPluginStreamPeer::RequestRead(nsByteRange* rangeList)
{
#if 0
    return (NPPluginError)npn_requestread(stream->pstream,
                                          (NPByteRange*)rangeList);
#endif
    return NPPluginError_NoError;
}


////////////////////////////////////////////////////////////////////////
// nsPluginStreamPeer specific methods
//

NPIPluginStream*
nsPluginStreamPeer::GetUserStream(void)
{
    if (fUserStream != NULL)
        fUserStream->AddRef();

    return fUserStream;
}

void
nsPluginStreamPeer::SetUserStream(NPIPluginStream* str)
{
    if (fUserStream != NULL)
        fUserStream->Release();

    fUserStream = str;

    if (fUserStream != NULL)
        fUserStream->AddRef();
};



////////////////////////////////////////////////////////////////////////
// Static callbacks through which netlib calls get routed
//

unsigned int
nsPluginStreamPeer::WriteReady(NET_StreamClass* s)
{
    nsPluginStreamPeer* streamPeer = (nsPluginStreamPeer*) s->data_object;
    PR_ASSERT(streamPeer != NULL);
    if (streamPeer == NULL)
        return 0;

    NPIPluginStream* stream = streamPeer->GetUserStream();
    PR_ASSERT(stream != NULL);
    if (stream == NULL)
        return 0;

    int result = stream->WriteReady();
    stream->Release();

    return result;
}


int
nsPluginStreamPeer::Write(NET_StreamClass* s, const char* bytes, int32 len)
{
    nsPluginStreamPeer* streamPeer = (nsPluginStreamPeer*) s->data_object;
    PR_ASSERT(streamPeer != NULL);
    if (streamPeer == NULL)
        return 0;

    NPIPluginStream* stream = streamPeer->GetUserStream();
    PR_ASSERT(stream != NULL);
    if (stream == NULL)
        return 0;

    int result = stream->Write(len, bytes);
    stream->Release();

    return result;
}

void
nsPluginStreamPeer::Complete(NET_StreamClass* s)
{
    nsPluginStreamPeer* streamPeer = (nsPluginStreamPeer*) s->data_object;
    PR_ASSERT(streamPeer != NULL);
    if (streamPeer == NULL)
        return;

    streamPeer->SetReason(NPPluginReason_Done);
    streamPeer->SetUserStream(NULL);

    // XXX Remove from the set of streams that the instance has open
    // rather than simply releasing
    streamPeer->Release();
}


void
nsPluginStreamPeer::Abort(NET_StreamClass* s, int status)
{
    nsPluginStreamPeer* streamPeer = (nsPluginStreamPeer*) s->data_object;
    PR_ASSERT(streamPeer != NULL);
    if (streamPeer == NULL)
        return;

    switch (status) {
    case MK_DATA_LOADED:
        PR_ASSERT(0); // XXX should've gotten a Complete()!
        streamPeer->SetReason(NPPluginReason_Done);
        break;

    case MK_INTERRUPTED:
        streamPeer->SetReason(NPPluginReason_UserBreak);
        break;

    default:
        streamPeer->SetReason(NPPluginReason_NetworkErr);
        break;
    }

    streamPeer->SetUserStream(NULL);

    // XXX Remove from the set of streams that the instance has open,
    // rather than simply releasing
    streamPeer->Release();
}


