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
    , mBuf(nsnull)
    , mBufCursor(nsnull)
    , mBufUnread(0)
    , mState(IDLE)
    , mReuseCount(0)
    , mMaxReuseCount(0)
    , mIdleTimeout(0)
    , mKeepAlive(0)
{
    NS_INIT_ISUPPORTS();
    PR_INIT_CLIST(this);
}

nsHttpConnection::~nsHttpConnection()
{
    NS_IF_RELEASE(mConnectionInfo);
    NS_IF_RELEASE(mTransaction);

    if (mSocketTransport)
        mSocketTransport->SetReuseConnection(PR_FALSE);

    // ensure that we're no longer part of any list
    PR_REMOVE_LINK(this);

    if (mBuf)
        PR_Free(mBuf);
}

nsresult
nsHttpConnection::Init(nsHttpConnectionInfo *info)
{
    LOG(("nsHttpConnection::Init [this=%x]\n"));

    NS_ENSURE_ARG_POINTER(info);

    mConnectionInfo = info;
    NS_ADDREF(mConnectionInfo);

    // allocate the read buffer
    if (!mBuf) {
        mBuf = (char *) PR_Malloc(NS_HTTP_SEGMENT_SIZE);
        if (!mBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }

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

    // inspect the connection headers for keep-alive info provided the
    // transaction completed successfully.
    const char *val = trans->ResponseHead()->PeekHeader(nsHttp::Connection);
    if (val) {
        if (PL_strcmp(val, "keep-alive") == 0) {
            mKeepAlive = PR_TRUE;

            val = trans->ResponseHead()->PeekHeader(nsHttp::Keep_Alive);

            const char *cp = PL_strstr(val, "max=");
            if (cp)
                mMaxReuseCount = (PRUint32) atoi(cp + 4);

            cp = PL_strstr(val, "timeout=");
            if (cp)
                mIdleTimeout = (PRUint32) atoi(cp + 8);
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
    // XXX need to keep track of the last time this connection was used
    return (mReuseCount < mMaxReuseCount) && IsAlive();
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

    // QI for the nsISocketTransport iface
    mSocketTransport = do_QueryInterface(transport, &rv);
    return rv;
}

nsresult
nsHttpConnection::FlushBuf()
{
    LOG(("nsHttpConnection::FlushBuf [this=%x]\n", this));

    PRUint32 n;
    nsresult rv;

    while (mBufUnread) {
        // notify the transaction that there is data to read
        rv = mTransaction->OnDataReadable(mBufCursor, mBufUnread, &n);
        if (NS_FAILED(rv) || !n) return rv;

        mBufCursor += n;
        mBufUnread -= n;
    }

    mBufCursor = mBuf;
    return NS_OK;
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
        nsresult rv = FlushBuf();
        NS_POSTCONDITION(NS_SUCCEEDED(rv), "FlushBuf failed");

        // Done reading, so signal transaction complete...
        mState = IDLE;
        mReadRequest = 0;

        mTransaction->OnStopTransaction(status);

        NS_RELEASE(mTransaction);
        mTransaction = 0;
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

    return mTransaction->OnDataWritable(outputStream, count);
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

    LOG(("nsHttpConnection::OnDataAvailable [this=%x state=%d]\n",
        this, mState));

    nsresult rv = FlushBuf();
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(mBufCursor == mBuf, "wtf");
    NS_ASSERTION(mBufUnread == 0, "wtf");

    rv = inputStream->Read(mBuf, NS_HTTP_SEGMENT_SIZE, &mBufUnread);
    if (NS_FAILED(rv) || !mBufUnread) return rv;

    return FlushBuf();
}

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(nsHttpConnectionInfo)
