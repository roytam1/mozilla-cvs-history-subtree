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

#include "nsMemCache.h"
#include "nsMemCacheChannel.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIStorageStream.h"
#include "nsIOutputStream.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kEventQueueService, NS_EVENTQUEUESERVICE_CID);

NS_IMPL_ISUPPORTS(nsMemCacheChannel, NS_GET_IID(nsIChannel))

void
nsMemCacheChannel::NotifyStorageInUse(PRInt32 aBytesUsed)
{
    mRecord->mCache->mOccupancy += aBytesUsed;
}

/**
 * This class acts as an adaptor around a synchronous input stream to add async
 * read capabilities.  It adds methods for initiating, suspending, resuming and
 * cancelling async reads. 
 */
class AsyncReadStreamAdaptor : public nsIInputStream {
public:
    AsyncReadStreamAdaptor(nsIChannel* aChannel, nsIInputStream *aSyncStream):
        mChannel(aChannel), mSyncStream(aSyncStream), mLogicalCursor(0),
        mAborted(false), mSuspended(false), mRemaining(0)
        { NS_INIT_REFCNT(); }
    
    NS_DECL_ISUPPORTS

    nsresult
    IsPending(PRBool* aIsPending) {
        *aIsPending = (mRemaining != 0) && !mAborted;
        return NS_OK;
    }

    nsresult
    Cancel(void) {
        mAborted = true;
        return mStreamListener->OnStopRequest(mChannel, mContext, NS_BINDING_ABORTED, nsnull);
    }

    nsresult
    Suspend(void) { mSuspended = true; return NS_OK; }

    nsresult
    Resume(void) {
        if (!mSuspended)
            return NS_ERROR_FAILURE;
        mSuspended = false;
        return NextListenerEvent();
    }

    NS_IMETHOD
    Available(PRUint32 *aNumBytes) { return mSyncStream->Available(aNumBytes); }

    NS_IMETHOD
    Read(char* aBuf, PRUint32 aCount, PRUint32 *aBytesRead) {
        if (mAborted)
            return NS_ERROR_ABORT;

        nsresult rv = mSyncStream->Read(aBuf, aCount, aBytesRead);
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) return rv;
        if (NS_FAILED(rv)) {
            Fail();
            return rv;
        }

        if (!mSuspended) {
            rv = NextListenerEvent();
            if (NS_FAILED(rv)) {
                Fail();
                return rv;
            }
        }

        return NS_OK;
    }

    NS_IMETHOD
    Close() {
        nsresult rv = mSyncStream->Close();
        mSyncStream = 0;
        mContext = 0;
        mStreamListener = 0;
        return rv;
    }

    nsresult
    AsyncRead(PRUint32 aStartPosition, PRInt32 aReadCount,
              nsISupports* aContext, nsIStreamListener* aListener) {

        nsresult rv;
        nsIEventQueue *eventQ;

        mContext = aContext;
        mStreamListener = aListener;
        mRemaining = aReadCount;

        NS_WITH_SERVICE(nsIIOService, serv, kIOServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueService, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), &eventQ);
        if (NS_FAILED(rv)) return rv;

        rv = serv->NewAsyncStreamListener(aListener, eventQ, getter_AddRefs(mStreamListener));
        NS_RELEASE(eventQ);
        if (NS_FAILED(rv)) return rv;

        rv = mStreamListener->OnStartRequest(mChannel, aContext);
        if (NS_FAILED(rv)) return rv;

        return NextListenerEvent();
    }

protected:
    nsresult
    Fail(void) {
        mAborted = true;
        return mStreamListener->OnStopRequest(mChannel, mContext, NS_BINDING_FAILED, nsnull);
    }

    nsresult
    NextListenerEvent() {
        PRUint32 available;
        nsresult rv = Available(&available);
        if (NS_FAILED(rv)) return rv;
        available = PR_MIN(available, mRemaining);

        if (available) {
            PRUint32 size = PR_MIN(available, MEM_CACHE_SEGMENT_SIZE);
            rv = mStreamListener->OnDataAvailable(mChannel, mContext, this, mLogicalCursor, size);
            mLogicalCursor += size;
            mRemaining -= size;
            return rv;
        } else {
            return mStreamListener->OnStopRequest(mChannel, mContext, NS_OK, nsnull);
        }
    }
    
private:
    nsCOMPtr<nsISupports>       mContext;
    nsCOMPtr<nsIStreamListener> mStreamListener;
    nsCOMPtr<nsIInputStream>    mSyncStream;
    PRUint32                    mLogicalCursor;
    PRUint32                    mRemaining;
    nsIChannel*                 mChannel;
    bool                        mAborted;
    bool                        mSuspended;
};

NS_IMPL_ISUPPORTS(AsyncReadStreamAdaptor,  NS_GET_IID(nsIInputStream))

// The only purpose of this output stream wrapper is to adjust the cache's
// overall occupancy as new data flows into the cache entry.
class MemCacheWriteStreamWrapper : public nsIOutputStream {
public:
    MemCacheWriteStreamWrapper(nsMemCacheChannel* aChannel, nsIOutputStream *aBaseStream):
        mChannel(aChannel), mBaseStream(aBaseStream)
        { NS_INIT_REFCNT(); }
    
    static nsresult
    Create(nsMemCacheChannel* aChannel, nsIOutputStream *aBaseStream, nsIOutputStream* *aWrapper) {
        MemCacheWriteStreamWrapper *wrapper = new MemCacheWriteStreamWrapper(aChannel, aBaseStream);
        if (!wrapper) return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(wrapper);
        *aWrapper = wrapper;
        return NS_OK;
    }
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD
    Write(const char *aBuffer, PRUint32 aCount, PRUint32 *aNumWritten) {
        *aNumWritten = 0;
        nsresult rv = mBaseStream->Write(aBuffer, aCount, aNumWritten);
        mChannel->NotifyStorageInUse(*aNumWritten);
        return rv;
    }

    NS_IMETHOD
    Flush() { return mBaseStream->Flush(); }

    NS_IMETHOD
    Close() { return mBaseStream->Close(); }

private:
    nsCOMPtr<nsIOutputStream>   mBaseStream;
    nsMemCacheChannel*          mChannel;
};

NS_IMPL_ISUPPORTS(MemCacheWriteStreamWrapper,  NS_GET_IID(nsIOutputStream))

nsMemCacheChannel::nsMemCacheChannel(nsMemCacheRecord *aRecord, nsILoadGroup *aLoadGroup)
    : mRecord(aRecord), mLoadGroup(aLoadGroup)
{
    NS_INIT_REFCNT();
    mRecord->mNumChannels++;
}

nsMemCacheChannel::~nsMemCacheChannel()
{
    mRecord->mNumChannels--;
}

NS_IMETHODIMP
nsMemCacheChannel::IsPending(PRBool* aIsPending)
{
    *aIsPending = PR_FALSE;
    if (!mAsyncReadStream)
        return NS_OK;
    return mAsyncReadStream->IsPending(aIsPending);
}

NS_IMETHODIMP
nsMemCacheChannel::Cancel(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Cancel();
}

NS_IMETHODIMP
nsMemCacheChannel::Suspend(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Suspend();
}

NS_IMETHODIMP
nsMemCacheChannel::Resume(void)
{
    if (!mAsyncReadStream)
        return NS_ERROR_FAILURE;
    return mAsyncReadStream->Resume();
}

NS_IMETHODIMP
nsMemCacheChannel::GetOriginalURI(nsIURI * *aURI)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetURI(nsIURI * *aURI)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenInputStream(PRUint32 aStartPosition, PRInt32 aReadCount,
                                   nsIInputStream* *aResult)
{
    nsresult rv;
    NS_ENSURE_ARG(aResult);
    if (mInputStream)
        return NS_ERROR_NOT_AVAILABLE;

    rv = mRecord->mStorageStream->NewInputStream(aStartPosition, getter_AddRefs(mInputStream));
    *aResult = mInputStream;
    NS_ADDREF(*aResult);
    return rv;
}

NS_IMETHODIMP
nsMemCacheChannel::OpenOutputStream(PRUint32 startPosition, nsIOutputStream* *aResult)
{
    nsresult rv;
    NS_ENSURE_ARG(aResult);

    nsCOMPtr<nsIOutputStream> outputStream;

    PRUint32 oldLength;
    mRecord->mStorageStream->GetLength(&oldLength);
    rv = mRecord->mStorageStream->GetOutputStream(startPosition, getter_AddRefs(outputStream));
    if (NS_FAILED(rv)) return rv;
    if (startPosition < oldLength)
        NotifyStorageInUse(startPosition - oldLength);

    return MemCacheWriteStreamWrapper::Create(this, outputStream, aResult);
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncOpen(nsIStreamObserver *observer, nsISupports *ctxt)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncRead(PRUint32 aStartPosition, PRInt32 aReadCount,
                             nsISupports *aContext, nsIStreamListener *aListener)
{
    nsCOMPtr<nsIInputStream> inputStream;
    nsresult rv = OpenInputStream(aStartPosition, aReadCount, getter_AddRefs(inputStream));
    if (NS_FAILED(rv)) return rv;
    
    AsyncReadStreamAdaptor *asyncReadStreamAdaptor;
    asyncReadStreamAdaptor = new AsyncReadStreamAdaptor(this, inputStream);
    if (!asyncReadStreamAdaptor)
        return NS_ERROR_OUT_OF_MEMORY;
    mAsyncReadStream = asyncReadStreamAdaptor;

    rv = asyncReadStreamAdaptor->AsyncRead(aStartPosition, aReadCount, aContext, aListener);
    if (NS_FAILED(rv))
        delete asyncReadStreamAdaptor;
    return rv;
}

NS_IMETHODIMP
nsMemCacheChannel::AsyncWrite(nsIInputStream *fromStream, PRUint32 startPosition,
                              PRInt32 writeCount, nsISupports *ctxt,
                              nsIStreamObserver *observer)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetLoadAttributes(nsLoadFlags *aLoadAttributes)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::SetLoadAttributes(nsLoadFlags aLoadAttributes)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentType(char * *aContentType)
{
    // Not required
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheChannel::GetContentLength(PRInt32 *aContentLength)
{
    // Not required
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
