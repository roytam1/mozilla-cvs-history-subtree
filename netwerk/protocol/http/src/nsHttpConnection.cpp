/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Darin Fisher <darin@netscape.com> (original author)
 */

#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsHttpResponseHead.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "netCore.h"
#include "nsNetCID.h"
#include "prmem.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

//-----------------------------------------------------------------------------
// nsHttpConnection <public>
//-----------------------------------------------------------------------------

nsHttpConnection::nsHttpConnection()
    : mTransaction(0)
    , mConnectionInfo(0)
    , mState(IDLE)
    , mReuseCount(0)
    , mMaxReuseCount(0)
    , mIdleTimeout(0)
    , mLastActiveTime(0)
    , mKeepAlive(0)
{
    LOG(("Creating nsHttpConnection @%x\n", this));

    NS_INIT_ISUPPORTS();
    PR_INIT_CLIST(this);
}

nsHttpConnection::~nsHttpConnection()
{
    LOG(("Destroying nsHttpConnection @%x\n", this));

    NS_IF_RELEASE(mConnectionInfo);
    mConnectionInfo = 0;
 
    NS_IF_RELEASE(mTransaction);
    mTransaction = 0;

    if (mSocketTransport)
        mSocketTransport->SetReuseConnection(PR_FALSE);

    // ensure that we're no longer part of any list
    PR_REMOVE_AND_INIT_LINK(this);
}

nsresult
nsHttpConnection::Init(nsHttpConnectionInfo *info)
{
    LOG(("nsHttpConnection::Init [this=%x]\n"));

    NS_ENSURE_ARG_POINTER(info);

    mConnectionInfo = info;
    NS_ADDREF(mConnectionInfo);

    return NS_OK;
}

nsresult
nsHttpConnection::SetTransaction(nsHttpTransaction *transaction)
{
    LOG(("nsHttpConnection::SetTransaction [this=%x]\n"));

    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_ARG_POINTER(transaction);

    // take ownership of the transaction
    mTransaction = transaction;
    NS_ADDREF(mTransaction); // XXX may want to make this weak

    // use this transactions notification callbacks
    mCallbacks = transaction->Callbacks();

    mProgressSink = 0;
    if (mCallbacks)
        mProgressSink = do_GetInterface(mCallbacks);

    // assign ourselves to the transaction
    mTransaction->SetConnection(this);

    return ActivateConnection();
}

// called from the socket thread
nsresult
nsHttpConnection::OnHeadersAvailable(nsHttpTransaction *trans)
{
    LOG(("nsHttpConnection::OnHeadersAvailable [this=%x trans=%x]\n",
        this, trans));

    NS_ENSURE_ARG_POINTER(trans);

    // be pesimistic
    mKeepAlive = PR_FALSE;

    if (!trans || !trans->ResponseHead()) {
        LOG(("trans->ResponseHead() = %x\n", trans));
        return NS_OK;
    }

    // inspect the connection headers for keep-alive info provided the
    // transaction completed successfully.
    const char *val = trans->ResponseHead()->PeekHeader(nsHttp::Connection);
    if (val) {
        if (PL_strcasecmp(val, "keep-alive") == 0) {
            mKeepAlive = PR_TRUE;

            val = trans->ResponseHead()->PeekHeader(nsHttp::Keep_Alive);

            LOG(("val = [%s]\n", val));

            const char *cp = PL_strstr(val, "max=");
            if (cp)
                mMaxReuseCount = (PRUint32) atoi(cp + 4);

            cp = PL_strstr(val, "timeout=");
            if (cp)
                mIdleTimeout = (PRUint32) atoi(cp + 8);
            
            LOG(("Connection can be reused [this=%x max-reuse=%u "
                 "keep-alive-timeout=%u\n", this, mMaxReuseCount, mIdleTimeout));
        }
    }

    return NS_OK;
}

// called from any thread
nsresult
nsHttpConnection::OnTransactionComplete(nsHttpTransaction *trans, nsresult status)
{
    LOG(("nsHttpConnection::OnTransactionComplete [this=%x status=%x]\n",
        this, status));

    NS_ENSURE_TRUE(mSocketTransport, NS_ERROR_UNEXPECTED);
    NS_ENSURE_TRUE(trans == mTransaction, NS_ERROR_UNEXPECTED);

    // cancel the requests... this will cause OnStopRequest to be fired
    if (mWriteRequest) {
        mWriteRequest->Cancel(status);
        mWriteRequest = 0;
    }
    if (mReadRequest) {
        mReadRequest->Cancel(status);
        mReadRequest = 0;
    }

    if (!mKeepAlive) {
        // if we're not going to be keeping this connection alive...
        mSocketTransport->SetReuseConnection(PR_FALSE);
        mSocketTransport = 0;
    }

    return NS_OK;
}

// not called from the socket thread
nsresult
nsHttpConnection::Resume()
{
    // XXX may require a lock to ensure thread safety

    if (mReadRequest)
        mReadRequest->Resume();

    return NS_OK;
}

PRBool
nsHttpConnection::CanReuse()
{
    return (mReuseCount < mMaxReuseCount) && 
           (NowInSeconds() - mLastActiveTime < mIdleTimeout) && IsAlive();
}

PRBool
nsHttpConnection::IsAlive()
{
    if (!mSocketTransport)
        return PR_FALSE;

    PRBool isAlive = PR_FALSE;
    nsresult rv = mSocketTransport->IsAlive(0, &isAlive);
    NS_ASSERTION(NS_SUCCEEDED(rv), "IsAlive test failed");
    return isAlive;
}

void
nsHttpConnection::ReportProgress(PRUint32 progress, PRInt32 progressMax)
{
    if (mProgressSink)
        mProgressSink->OnProgress(nsnull, nsnull, progress,
                                  progressMax < 0 ? 0 : PRUint32(progressMax));
}

//-----------------------------------------------------------------------------
// nsHttpConnection <private>
//-----------------------------------------------------------------------------

nsresult
nsHttpConnection::ActivateConnection()
{
    nsresult rv;

    // If we don't have a socket transport then create a new one
    if (!mSocketTransport) {
        rv = CreateTransport();
        if (NS_FAILED(rv)) return rv;
    }

    mState = WAITING_FOR_WRITE;

    rv = mSocketTransport->SetReuseConnection(PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = mSocketTransport->AsyncWrite(this, nsnull, 0, PRUint32(-1),
                                      nsITransport::DONT_PROXY_OBSERVER |
                                      nsITransport::DONT_PROXY_PROVIDER,
                                      getter_AddRefs(mWriteRequest));
    if (NS_FAILED(rv)) return rv;

    rv = mSocketTransport->AsyncRead(this, nsnull, 0, PRUint32(-1),
                                     nsITransport::DONT_PROXY_OBSERVER |
                                     nsITransport::DONT_PROXY_LISTENER,
                                     getter_AddRefs(mReadRequest));
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsHttpConnection::CreateTransport()
{
    nsresult rv;

    NS_PRECONDITION(!mSocketTransport, "unexpected");

    nsCOMPtr<nsISocketTransportService> sts =
            do_GetService(kSocketTransportServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsITransport> transport;
    rv = sts->CreateTransport(mConnectionInfo->Host(),
                              mConnectionInfo->Port(),
                              mConnectionInfo->ProxyHost(),
                              mConnectionInfo->ProxyPort(),
                              NS_HTTP_SEGMENT_SIZE,
                              NS_HTTP_BUFFER_SIZE,
                              getter_AddRefs(transport));
    if (NS_FAILED(rv)) return rv;

    // allow the socket transport to call us directly on progress
    rv = transport->SetNotificationCallbacks(this,
                                             nsITransport::DONT_PROXY_PROGRESS);
    if (NS_FAILED(rv)) return rv;

    // QI for the nsISocketTransport iface
    mSocketTransport = do_QueryInterface(transport, &rv);
    return rv;
}

//-----------------------------------------------------------------------------
// nsHttpConnection::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ADDREF(nsHttpConnection)
NS_IMPL_THREADSAFE_RELEASE(nsHttpConnection)

NS_INTERFACE_MAP_BEGIN(nsHttpConnection)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIStreamProvider)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIRequestObserver, nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
NS_INTERFACE_MAP_END_THREADSAFE

//-----------------------------------------------------------------------------
// nsHttpConnection::nsIRequestObserver
//-----------------------------------------------------------------------------

// called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    LOG(("nsHttpConnection::OnStartRequest [this=%x state=%d]\n",
        this, mState));

    return NS_OK;
}

// called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                nsresult status)
{
    LOG(("nsHttpConnection::OnStopRequest [this=%x ctxt=%x state=%d status=%x]\n",
        this, ctxt, mState, status));

    if (!mTransaction)
        return NS_OK;

    if (mState == WRITING) {
        mState = WAITING_FOR_READ;
        mWriteRequest = 0;
    } 
    else {
        // Done reading, so signal transaction complete...
        mState = IDLE;
        mReadRequest = 0;

        mTransaction->OnStopTransaction(status);

        NS_RELEASE(mTransaction);
        mTransaction = 0;

        // don't need these anymore
        mCallbacks = 0;
        mProgressSink = 0;
    }
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpConnection::nsIStreamProvider
//-----------------------------------------------------------------------------

// called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::OnDataWritable(nsIRequest *request, nsISupports *context,
                                 nsIOutputStream *outputStream,
                                 PRUint32 offset, PRUint32 count)
{
    if (!mTransaction)
        return NS_BASE_STREAM_CLOSED;

    mState = WRITING;

    LOG(("nsHttpConnection::OnDataWritable [this=%x state=%d]\n",
        this, mState));

    return mTransaction->OnDataWritable(outputStream);
}

//-----------------------------------------------------------------------------
// nsHttpConnection::nsIStreamListener
//-----------------------------------------------------------------------------

// called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::OnDataAvailable(nsIRequest *request, nsISupports *context,
                                  nsIInputStream *inputStream,
                                  PRUint32 offset, PRUint32 count)
{
    if (!mTransaction)
        return NS_BASE_STREAM_CLOSED;

    mState = READING;
    mLastActiveTime = NowInSeconds();

    LOG(("nsHttpConnection::OnDataAvailable [this=%x state=%d]\n",
        this, mState));

    return mTransaction->OnDataReadable(inputStream);
}

//-----------------------------------------------------------------------------
// nsHttpConnection::nsIInterfaceRequestor
//-----------------------------------------------------------------------------

// not called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::GetInterface(const nsIID &iid, void **result)
{
    if (iid.Equals(NS_GET_IID(nsIProgressEventSink)))
        return QueryInterface(iid, result);

    if (mCallbacks)
        return mCallbacks->GetInterface(iid, result);

    return NS_ERROR_NO_INTERFACE;
}

//-----------------------------------------------------------------------------
// nsHttpConnection::nsIProgressEventSink
//-----------------------------------------------------------------------------

// called on the socket transport thread
NS_IMETHODIMP
nsHttpConnection::OnStatus(nsIRequest *req, nsISupports *ctx, nsresult status,
                           const PRUnichar *statusText)
{
    if (mProgressSink)
        mProgressSink->OnStatus(nsnull, nsnull, status, statusText);

    return NS_OK;
}

NS_IMETHODIMP
nsHttpConnection::OnProgress(nsIRequest *req, nsISupports *ctx,
                             PRUint32 progress, PRUint32 progressMax)
{
    // we ignore progress notifications from the socket transport.
    // we'll generate these ourselves from OnDataAvailable
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(nsHttpConnectionInfo)
