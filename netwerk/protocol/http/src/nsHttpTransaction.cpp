#include <stdlib.h> // atoi
#include "nsHttpHandler.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsIStringStream.h"
#include "nsIStreamConverterService.h"
#include "nsISupportsPrimitives.h"
#include "nsIServiceManager.h"
#include "nsIFileStreams.h"
#include "nsHTTPChunkConv.h"
#include "nsNetCID.h"
#include "prmem.h"
#include "pratom.h"

static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kSupportsVoidCID, NS_SUPPORTS_VOID_CID);

//-----------------------------------------------------------------------------
// nsHttpTransaction
//-----------------------------------------------------------------------------

nsHttpTransaction::nsHttpTransaction(nsIStreamListener *listener)
    : mListener(listener)
    , mConnection(nsnull)
    , mResponseHead(nsnull)
    , mReadBuf(0)
    , mContentLength(-1)
    , mContentRead(0)
    , mChunkConvCtx(0)
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
    /*
    nsHttpResponseHead *head = mResponseHead;
    mResponseHead = nsnull;
    return head;
    */
    return mResponseHead;
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnDataWritable(nsIOutputStream *os, PRUint32 count)
{
    PRUint32 n = 0;

    LOG(("nsHttpTransaction::OnDataWritable [this=%x]\n", this));

    // check if we're done writing the headers
    nsresult rv = mReqHeaderStream->Available(&n);
    if (NS_FAILED(rv)) return rv;
    
    if (n != 0)
        return os->WriteFrom(mReqHeaderStream, count, &n);

    if (mReqUploadStream)
        return os->WriteFrom(mReqUploadStream, count, &n);

    return NS_BASE_STREAM_CLOSED;
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnDataAvailable(nsIInputStream *is, PRUint32 count)
{
    LOG(("nsHttpTransaction::OnDataAvailable [this=%x count=%u]\n",
        this, count));

    if (mHaveAllHeaders)
        // simply pipe the data over to the channel's thread
        return HandleContent(is, count, PR_TRUE);

    //
    // need to parse status line and headers
    //
    
    // allocate the response head object if necessary
    if (!mResponseHead) {
        mResponseHead = new nsHttpResponseHead();
        if (!mResponseHead)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    
    // allocate the read ahead buffer if necessary
    if (!mReadBuf) {
        mReadBuf = (char *) PR_Malloc(NS_HTTP_SEGMENT_SIZE);
        if (!mReadBuf)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv;
    PRUint32 bufCount, offset;

    while (count) {
        PRUint32 amount;

        // don't exceed the buffer size
        amount = PR_MIN(count, NS_HTTP_SEGMENT_SIZE);

        bufCount = 0;
        offset = 0;

        rv = is->Read(mReadBuf, amount, &bufCount);
        if (NS_FAILED(rv)) return rv;

        while (bufCount) {
            PRUint32 bytesConsumed = 0;
            rv = HandleSegment(mReadBuf + offset, bufCount, &bytesConsumed);
            if (NS_FAILED(rv)) return rv;

            bufCount -= bytesConsumed;
            count -= bytesConsumed;
            offset += bytesConsumed;

            // see if we're done reading headers
            if (mHaveAllHeaders)
                goto done;
        }
    }

done:
    if (mHaveAllHeaders) {
        if (bufCount) {
            nsCOMPtr<nsISupports> sup;
            // the read ahead buffer still has some data in it which we need
            // to send up stream before reading any more from the socket.
            rv = NS_NewByteInputStream(getter_AddRefs(sup),
                                       mReadBuf + offset,
                                       bufCount);
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIInputStream> stream = do_QueryInterface(sup, &rv);
            if (NS_FAILED(rv)) return rv;

            rv = HandleContent(stream, bufCount, PR_FALSE);
            // the stream listener proxy should have room for all of the data
            // in this stream.
            NS_POSTCONDITION(rv != NS_BASE_STREAM_WOULD_BLOCK, "bad listener");
            bufCount = 0;
            if (NS_FAILED(rv)) return rv;
        }
        if (count) // try to read some more from the socket
            return HandleContent(is, count, PR_TRUE);
    }
    return NS_OK;
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnStopTransaction(nsresult status)
{
    LOG(("nsHttpTransaction::OnStopTransaction [this=%x status=%x]\n",
        this, status));

    if (mReadBuf) {
        PR_Free(mReadBuf);
        mReadBuf = 0;
    }

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

    NS_PRECONDITION(mResponseHead, "null response head");
    NS_PRECONDITION(!mHaveAllHeaders, "already have all headers");

    LOG(("nsHttpTransaction::ParseLine [%s]\n", line));

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
nsHttpTransaction::HandleSegment(char *segment,
                                 PRUint32 count,
                                 PRUint32 *countRead)
{
    char *eol;

    *countRead = 0;

    NS_PRECONDITION(!mHaveAllHeaders, "oops");

    while ((eol = PL_strstr(segment, "\r\n")) != nsnull) {
        // found line in range [segment:eol]
        *eol = 0;

        // we may have a partial line to complete...
        if (!mLineBuf.IsEmpty()) {
            mLineBuf.Append(segment);
            ParseLine((char *) mLineBuf.get());
            mLineBuf.SetLength(0);
        }
        else
            ParseLine(segment);

        *countRead += (eol + 2 - segment);
        NS_ASSERTION(*countRead <= count, "oops");

        // skip over line
        segment = eol + 2;

        if (mHaveAllHeaders)
            break;
    }

    if (!mHaveAllHeaders && (count > *countRead)) {
        // remember this partial line
        mLineBuf.Assign(segment, count - *countRead);
    }

    // read something
    return NS_OK;
}

nsresult
nsHttpTransaction::HandleContent(nsIInputStream *stream,
                                 PRUint32 count,
                                 PRBool fromSocket)
{
    nsresult rv;

    LOG(("nsHttpTransaction::HandleContent [this=%x count=%u]\n",
        this, count));

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
            rv = InstallChunkedDecoder();
            if (NS_FAILED(rv)) {
                LOG(("InstallChunkedDecoder failed with [rv=%x]\n", rv));
                return rv;
            }
        }
    }

    // we cannot trust that all of "count" data will be read, so we have to
    // interogate the stream for the number of bytes read.
    PRUint32 before = 0, after = 0;
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(stream);
    if (seekable)
        seekable->Tell(&before);

    rv = mListener->OnDataAvailable(this, nsnull, stream, mContentRead, count);
    if (NS_FAILED(rv)) return rv;

    // get the stream position now to see how much was read.
    if (seekable)
        seekable->Tell(&after);
    else
        after = count;

    // update count of content bytes read..
    NS_ASSERTION(after >= before, "invalid stream offset!");
    mContentRead += (after - before);

    LOG(("nsHttpTransaction [this=%x count=%u read=%u mContentRead=%u mContentLength=%d]\n",
        this, count, after - before, mContentRead, mContentLength));

    // check for end-of-file
    if ((mContentRead == PRUint32(mContentLength)) ||
        (mChunkConvCtx && mChunkConvCtx->GetEOF())) {

        PRInt32 priorVal = PR_AtomicSet(&mTransactionDone, 1);
        if (priorVal == 0 && mConnection) {
            // let the connection know that we are done with it; this should
            // result in OnStopTransaction being fired.
            return mConnection->OnTransactionComplete(this, NS_OK);
        }
    }

    return NS_OK;
}

nsresult
nsHttpTransaction::InstallChunkedDecoder()
{
    nsresult rv;

    LOG(("nsHttpTransaction::InstallChunkedDecoder [this=%x]\n", this));

    nsCOMPtr<nsIStreamConverterService> serv =
            do_GetService(kStreamConverterServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    mChunkConvCtx = new nsHTTPChunkConvContext();
    if (!mChunkConvCtx)
        return NS_ERROR_OUT_OF_MEMORY;

    // we need to pass in our converter context as an nsISupports
    nsCOMPtr<nsISupportsVoid> ctx =
            do_CreateInstance(kSupportsVoidCID, &rv);
    if (NS_FAILED(rv)) return rv;
    ctx->SetData(mChunkConvCtx);

    // create the "chunked" decoder
    nsCOMPtr<nsIStreamListener> listener;
    rv = serv->AsyncConvertData(NS_LITERAL_STRING("chunked").get(),
                                NS_LITERAL_STRING("unchunked").get(),
                                mListener, ctx,
                                getter_AddRefs(listener));
    if (NS_FAILED(rv)) return rv;

    // data will now be pushed through the "chunked" decoder
    mListener = listener;

    // before we start pushing data through the decoder, we have to
    // tell it about any expected trailer headers, so it can consume
    // them for us.  if we don't do this, then we won't properly detect
    // the end of the data stream.
    const char *val = mResponseHead->PeekHeader(nsHttp::Trailer);
    if (val) {
        nsCString ts(val);
        ts.StripWhitespace();

        //XXXjag convert to new string code sometime
        char *cp = NS_CONST_CAST(char *, ts.get());

        while (*cp) {
            char *pp = PL_strchr(cp, ',');
            if (!pp) {
                mChunkConvCtx->AddTrailerHeader(cp);
                break;
            }
            else {
                *pp = 0;
                mChunkConvCtx->AddTrailerHeader(cp);
                *pp = ',';
                cp = pp + 1;
            }
        }
    }
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpTransaction::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(nsHttpTransaction, nsIRequest)

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
    return NS_ERROR_NOT_IMPLEMENTED;
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
