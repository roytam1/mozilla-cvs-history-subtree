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

#include "nsMemCacheChannel.h"
#include "nsIChannel.h"
#include "nsIStorageStream.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);

NS_IMPL_ISUPPORTS(nsMemCacheChannel, NS_GET_IID(nsIChannel))

nsMemCacheChannel::nsMemCacheChannel(nsMemCacheRecord *aRecord, nsILoadGroup *aLoadGroup)
    : mRecord(aRecord), mLoadGroup(aLoadGroup)
{
    NS_INIT_REFCNT();
}

nsMemCacheChannel::~nsMemCacheChannel()
{
}

NS_IMETHODIMP
nsMemCacheChannel::IsPending(PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::Cancel(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::Suspend(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::Resume(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetURI(nsIURI * *aURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenInputStream(PRUint32 startPosition, PRInt32 readCount,
                                   nsIInputStream* *aResult)
{
    NS_ENSURE_ARG(aResult);
    if (mInputStream)
        return NS_ERROR_NOT_AVAILABLE;

    // FIXME:  handle readCount args
    NS_ASSERTION(readCount == -1, "Can't handle readCount arg yet");
    if (readCount != -1)
        return NS_ERROR_INVALID_ARG;

    nsresult rv;
    rv = mRecord->mStorageStream->NewInputStream(startPosition, getter_AddRefs(mInputStream));
    if (NS_FAILED(rv)) return rv;
    
    *aResult = mInputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream* *aResult)
{
    NS_ENSURE_ARG(aResult);

    // FIXME handle startPosition arg
    return mRecord->mStorageStream->GetOutputStream(startPosition, aResult);
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports *ctxt)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncRead(PRUint32 startPosition, PRInt32 readCount,
                             nsISupports *ctxt, nsIStreamListener *aListener)
{
    nsresult rv;
    nsIEventQueue *eventQ;
    nsIInputStream *inputStream;

    NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueService, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), &eventQ);
    if (NS_FAILED(rv)) return rv;

    rv = serv->NewAsyncStreamListener(aListener, eventQ, getter_AddRefs(mStreamListener));
    NS_RELEASE(eventQ);
    if (NS_FAILED(rv)) return rv;

    rv = OpenInputStream(startPosition, readCount, &inputStream);
    if (NS_FAILED(rv)) return rv;

    rv = mStreamListener->OnStartRequest(this, ctxt);
    if (NS_FAILED(rv)) return rv;
    
    PRUint32 streamLen;
    rv = mInputStream->Available(&streamLen);
    if (NS_FAILED(rv)) return rv;

    rv = mStreamListener->OnDataAvailable(this, ctxt, mInputStream, 0, streamLen);
    if (NS_FAILED(rv)) return rv;

    rv = mStreamListener->OnStopRequest(this, ctxt, NS_OK, nsnull);

    return rv;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition,
                              PRInt32 writeCount, nsISupports *ctxt,
                              nsIStreamObserver *observer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentType(char * *aContentType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentLength(PRInt32 *aContentLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetOwner(nsISupports* *aOwner)
{
    *aOwner = mOwner.get();
    NS_IF_ADDREF(*aOwner);
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::SetOwner(nsISupports* aOwner)
{
    mOwner = aOwner;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}
