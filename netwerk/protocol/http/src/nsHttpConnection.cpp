#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIServiceManager.h"
#include "netCore.h"
#include "nsNetCID.h"

static NS_DEFINE_CID(kSocketTransportServiceCID, NS_SOCKETTRANSPORTSERVICE_CID);

//-----------------------------------------------------------------------------
// nsHttpConnection
//-----------------------------------------------------------------------------

nsHttpConnection::nsHttpConnection()
    : mTransaction(0)
    , mConnectionInfo(0)
    , mState(IDLE)
{
    NS_INIT_ISUPPORTS();
}

nsHttpConnection::~nsHttpConnection()
{
    NS_IF_RELEASE(mConnectionInfo);
    NS_IF_RELEASE(mTransaction);
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

    mTransaction = transaction;
    NS_ADDREF(mTransaction);

    return ActivateConnection();
}

nsresult
nsHttpConnection::ActivateConnection()
{
    nsresult rv;

    // If we don't have a socket transport then create a new one
    if (!mTransport) {
        rv = CreateTransport();
        if (NS_FAILED(rv)) return rv;
    }

    mState = WAITING_FOR_WRITE;

    rv = mTransport->SetReuseConnection(PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = mTransport->AsyncWrite(this, nsnull, 0, PRUint32(-1),
                                nsITransport::DONT_PROXY_STREAM_OBSERVER |
                                nsITransport::DONT_PROXY_STREAM_PROVIDER,
                                getter_AddRefs(mWriteReq));
    if (NS_FAILED(rv)) return rv;

    rv = mTransport->AsyncRead(this, nsnull, 0, PRUint32(-1),
                               nsITransport::DONT_PROXY_STREAM_OBSERVER |
                               nsITransport::DONT_PROXY_STREAM_LISTENER,
                               getter_AddRefs(mReadReq));
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsHttpConnection::CreateTransport()
{
    nsresult rv;

    NS_PRECONDITION(!mTransport, "unexpected");

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
    mTransport = do_QueryInterface(transport, &rv);
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

    if (mState == WRITING) {
        mState = WAITING_FOR_READ;
    } 
    else {
        // Done reading, so signal transaction complete...
        mState = IDLE;

        mTransaction->OnStopRequest(status);

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
    mState = READING;

    LOG(("nsHttpConnection::OnDataAvailable [this=%x state=%d]\n",
        this, mState));

    return mTransaction->OnDataAvailable(inputStream, count);
}

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS0(nsHttpConnectionInfo)
