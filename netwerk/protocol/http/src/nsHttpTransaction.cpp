#include "nsHttpHandler.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpChunkedDecoder.h"
#include "nsIStringStream.h"
#include "pratom.h"

//-----------------------------------------------------------------------------
// nsHttpTransaction
//-----------------------------------------------------------------------------

nsHttpTransaction::nsHttpTransaction(nsIStreamListener *listener)
    : mListener(listener)
    , mConnection(nsnull)
    , mResponseHead(nsnull)
    , mContentLength(-1)
    , mContentRead(0)
    , mChunkedDecoder(nsnull)
    , mTransactionDone(0)
    , mHaveStatusLine(0)
    , mHaveAllHeaders(0)
    , mFiredOnStart(0)
{
    NS_INIT_ISUPPORTS();

    NS_PRECONDITION(listener, "null listener");
}

nsHttpTransaction::~nsHttpTransaction()
{
    if (mConnection) {
        nsHttpHandler::get()->ReleaseConnection(mConnection);
        NS_RELEASE(mConnection);
    }

    if (mChunkedDecoder)
        delete mChunkedDecoder;
}

nsresult
nsHttpTransaction::SetupRequest(nsHttpRequestHead *requestHead,
                                nsIInputStream *requestStream)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(requestHead);

    mReqHeaderBuf.SetLength(0);

    rv = requestHead->Flatten(mReqHeaderBuf);
    if (NS_FAILED(rv)) return rv;

    LOG(("nsHttpTransaction::SetupRequest [this=%x\n%s]\n",
        this, mReqHeaderBuf.get()));

    mReqUploadStream = requestStream;
    if (!mReqUploadStream)
        // Write out end-of-headers sequence if NOT uploading data:
        mReqHeaderBuf.Append("\r\n");

    // Create a string stream for the request header buf
    nsCOMPtr<nsISupports> sup;
    rv = NS_NewCStringInputStream(getter_AddRefs(sup), mReqHeaderBuf);
    if (NS_FAILED(rv)) return rv;
    mReqHeaderStream = do_QueryInterface(sup, &rv);

    return rv;
}

void
nsHttpTransaction::SetConnection(nsHttpConnection *conn)
{
    NS_IF_RELEASE(mConnection);
    mConnection = conn;
    NS_IF_ADDREF(mConnection);
}

nsHttpResponseHead *
nsHttpTransaction::TakeResponseHead()
{
    nsHttpResponseHead *head = mResponseHead;
    mResponseHead = nsnull;
    return head;
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnDataWritable(nsIOutputStream *os)
{
    PRUint32 n = 0;

    LOG(("nsHttpTransaction::OnDataWritable [this=%x]\n", this));

    // check if we're done writing the headers
    nsresult rv = mReqHeaderStream->Available(&n);
    if (NS_FAILED(rv)) return rv;

    // let at most NS_HTTP_BUFFER_SIZE bytes be written at a time.
    
    if (n != 0)
        return os->WriteFrom(mReqHeaderStream, NS_HTTP_BUFFER_SIZE, &n);

    if (mReqUploadStream)
        return os->WriteFrom(mReqUploadStream, NS_HTTP_BUFFER_SIZE, &n);

    return NS_BASE_STREAM_CLOSED;
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnDataReadable(nsIInputStream *is)
{
    LOG(("nsHttpTransaction::OnDataReadable [this=%x]\n", this));

    mSource = is;

    // let our listener try to read up to NS_HTTP_BUFFER_SIZE from us.
    return mListener->OnDataAvailable(this, nsnull, this,
                                      mContentRead, NS_HTTP_BUFFER_SIZE);
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnStopTransaction(nsresult status)
{
    LOG(("nsHttpTransaction::OnStopTransaction [this=%x status=%x]\n",
        this, status));

    if (!mFiredOnStart) {
        mFiredOnStart = PR_TRUE;
        mListener->OnStartRequest(this, nsnull); 
    }

    return mListener->OnStopRequest(this, nsnull, status);
}

nsresult
nsHttpTransaction::ParseLine(char *line)
{
    nsresult rv;

    NS_PRECONDITION(!mHaveAllHeaders, "already have all headers");

    LOG(("nsHttpTransaction::ParseLine [%s]\n", line));
    
    // allocate the response head object if necessary
    if (!mResponseHead) {
        mResponseHead = new nsHttpResponseHead();
        if (!mResponseHead)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!mHaveStatusLine) {
        rv = mResponseHead->ParseStatusLine(line);
        if (NS_SUCCEEDED(rv))
            mHaveStatusLine = PR_TRUE;
        return rv;
    }

    if (*line == '\0') {
        mHaveAllHeaders = PR_TRUE;
        return NS_OK;
    }

    return mResponseHead->ParseHeaderLine(line);
}

nsresult
nsHttpTransaction::ParseHeaders(char *buf,
                                PRUint32 count,
                                PRUint32 *countRead)
{
    char *eol;

    *countRead = 0;

    NS_PRECONDITION(!mHaveAllHeaders, "oops");

    while ((eol = PL_strstr(buf, "\r\n")) != nsnull) {
        // found line in range [buf:eol]
        *eol = 0;

        // we may have a partial line to complete...
        if (!mLineBuf.IsEmpty()) {
            mLineBuf.Append(buf);
            ParseLine((char *) mLineBuf.get());
            mLineBuf.SetLength(0);
        }
        else
            ParseLine(buf);

        *countRead += (eol + 2 - buf);
        NS_ASSERTION(*countRead <= count, "oops");

        // skip over line
        buf = eol + 2;

        if (mHaveAllHeaders)
            break;
    }

    if (!mHaveAllHeaders && (count > *countRead)) {
        // remember this partial line
        mLineBuf.Assign(buf, count - *countRead);
    }

    // read something
    return NS_OK;
}

nsresult
nsHttpTransaction::HandleContent(char *buf,
                                 PRUint32 count,
                                 PRUint32 *countRead)
{
    nsresult rv;

    LOG(("nsHttpTransaction::HandleContent [this=%x count=%u]\n",
        this, count));

    *countRead = 0;

    if (mTransactionDone)
        return NS_OK;

    NS_PRECONDITION(mConnection, "no connection");

    if (!mFiredOnStart) {
        // notify the transaction sink first
        if (mConnection)
            mConnection->OnHeadersAvailable(this);

        LOG(("nsHttpTransaction [this=%x] sending OnStartRequest\n", this));

        mFiredOnStart = PR_TRUE;

        rv = mListener->OnStartRequest(this, nsnull);
        if (NS_FAILED(rv)) {
            LOG(("OnStartRequest failed with [rv=%x]\n", rv));
            return rv;
        }

        // grab the content-length from the response headers
        mContentLength = mResponseHead->ContentLength();

        // handle chunked encoding here, so we'll know immediately when
        // we're done with the socket.  please note that _all_ other
        // decoding is done when the channel receives the content data
        // so as not to block the socket transport thread too much.
        const char *val =
            mResponseHead->PeekHeader(nsHttp::Transfer_Encoding);
        if (val) {
            // we only support the "chunked" transfer encoding right now.
            mChunkedDecoder = new nsHttpChunkedDecoder();
            if (!mChunkedDecoder)
                return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    if (mChunkedDecoder) {
        // give the buf over to the chunked decoder so it can reformat the
        // data and tell us how much is really there.
        rv = mChunkedDecoder->HandleChunkedContent(buf, count, countRead);
        if (NS_FAILED(rv)) return rv;
    }
    else
        *countRead = PR_MIN(count, mContentLength - mContentRead);

    // update count of content bytes read..
    mContentRead += *countRead;

    LOG(("nsHttpTransaction [this=%x count=%u read=%u mContentRead=%u mContentLength=%d]\n",
        this, count, *countRead, mContentRead, mContentLength));

    // check for end-of-file
    if (mContentRead == PRUint32(mContentLength) ||
        mChunkedDecoder->ReachedEOF()) {
        // atomically mark the transaction as complete to ensure that
        // OnTransactionComplete is fired only once!
        PRInt32 priorVal = PR_AtomicSet(&mTransactionDone, 1);
        if (priorVal == 0 && mConnection) {
            // let the connection know that we are done with it; this should
            // result in OnStopTransaction being fired.
            return mConnection->OnTransactionComplete(this, NS_OK);
        }
    }

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpTransaction::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(nsHttpTransaction,
                              nsIRequest,
                              nsIInputStream)

//-----------------------------------------------------------------------------
// nsHttpTransaction::nsIRequest
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpTransaction::GetName(PRUnichar **aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::IsPending(PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::GetStatus(nsresult *aStatus)
{
    *aStatus = NS_OK;
    return NS_OK;
}

// called from any thread
NS_IMETHODIMP
nsHttpTransaction::Cancel(nsresult status)
{
    if (!mConnection) {
        // the connection is not assigned to a connection yet, so we must
        // notify the HTTP handler, so it can process the cancelation. 
        return nsHttpHandler::get()->CancelPendingTransaction(this, status);
    }

    // atomically cancel the connection.  it's important to consider that
    // the socket thread could already be in the middle of processing
    // completion, in which case we should ignore this cancelation request.

    PRInt32 priorVal = PR_AtomicSet(&mTransactionDone, 1);
    if (priorVal == 0)
        mConnection->OnTransactionComplete(this, status);

    return NS_OK;
}

NS_IMETHODIMP
nsHttpTransaction::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::Resume()
{
    LOG(("nsHttpTransaction::Resume [this=%x]\n", this));
    if (mConnection)
        mConnection->Resume();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpTransaction::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpTransaction::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpTransaction::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// nsHttpTransaction::nsIInputStream
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpTransaction::Close()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::Available(PRUint32 *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::Read(char *buf, PRUint32 bufSize, PRUint32 *bytesWritten)
{
    nsresult rv;

    LOG(("nsHttpTransaction::Read [this=%x bufSize=%u]\n", this, bufSize));

    NS_ENSURE_TRUE(mSource, NS_ERROR_NOT_INITIALIZED);

    // read some data from our source and put it in the given buf
    rv = mSource->Read(buf, bufSize, bytesWritten);
    if (NS_FAILED(rv) || (*bytesWritten == 0)) return rv;

    // pretend that no bytes were written (since we're just borrowing the
    // given buf anyways).
    bufSize = *bytesWritten;
    *bytesWritten = 0;

    //if (mHaveAllHeaders)
        return HandleContent(buf, bufSize, bytesWritten);

    PRUint32 offset = 0, count = bufSize, bytesConsumed;

    while (count) {
        bytesConsumed = 0;

        rv = ParseHeaders(buf + offset, count, &bytesConsumed);
        if (NS_FAILED(rv)) return rv;

        count -= bytesConsumed;
        offset += bytesConsumed;

        // see if we're done reading headers
        if (mHaveAllHeaders)
            break;
    }

    if (count) {
        // buf has some content in it; shift bytes to top of buf.
        memmove(buf, buf + offset, count);

        return HandleContent(buf, count, bytesWritten);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsHttpTransaction::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                PRUint32 count, PRUint32 *countRead)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::GetNonBlocking(PRBool *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpTransaction::GetObserver(nsIInputStreamObserver **obs)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpTransaction::SetObserver(nsIInputStreamObserver *obs)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
