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

////////////////////////////////////////////////////////////////////////////////
// Plugin Manager Stream Interface

nsPluginManagerStream::nsPluginManagerStream(void)
{
    NS_INIT_REFCNT();
}

nsPluginManagerStream::~nsPluginManagerStream(void)
{
}

NS_METHOD_(PRInt32)
nsPluginManagerStream::WriteReady(void)
{
    // XXX This call didn't exist before, but I think that it should have.
    // Is this implementation right?
    return NP_STREAM_MAXREADY;
}

NS_METHOD_(PRInt32)
nsPluginManagerStream::Write(PRInt32 len, const char* buffer)
{
#if 0
    return npn_write(npp, pstream, len, buffer);
#endif
    return 0;
}

NS_METHOD_(const char*)
nsPluginManagerStream::GetURL(void)
{
#if 0
    return pstream->url;
#endif
    return NULL;
}

NS_METHOD_(PRUint32)
nsPluginManagerStream::GetEnd(void)
{
#if 0
    return pstream->end;
#endif
    return 0;
}

NS_METHOD_(PRUint32)
nsPluginManagerStream::GetLastModified(void)
{
#if 0
    return pstream->lastmodified;
#endif
    return 0;
}

NS_METHOD_(void*)
nsPluginManagerStream::GetNotifyData(void)
{
#if 0
    return pstream->notifyData;
#endif
    return NULL;
}

NS_DEFINE_IID(kPluginManagerStreamIID, NP_IPLUGINMANAGERSTREAM_IID);
NS_IMPL_QUERY_INTERFACE(nsPluginManagerStream, kPluginManagerStreamIID);
NS_IMPL_ADDREF(nsPluginManagerStream);
NS_IMPL_RELEASE(nsPluginManagerStream);

