#include <stdlib.h> // atoi
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsIStringStream.h"
#include "nsIStreamConverterService.h"
#include "nsISupportsPrimitives.h"
#include "nsIServiceManager.h"
#include "nsHTTPChunkConv.h"
#include "nsNetCID.h"
#include "prmem.h"

static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kSupportsVoidCID, NS_SUPPORTS_VOID_CID);

//-----------------------------------------------------------------------------
// nsHttpTransaction
//-----------------------------------------------------------------------------

nsHttpTransaction::nsHttpTransaction(nsIStreamListener *listener)
    : mListener(listener)
    , mConnection(nsnull)
    , mResponseVersion(HTTP_VERSION_UNKNOWN)
    , mResponseStatus(0)
    , mReadBuf(0)
    , mContentLength(-1)
    , mContentRead(0)
    , mChunkConvCtx(0)
    , mHaveStatusLine(0)
    , mHaveAllHeaders(0)
    , mFiredOnStart(0)
{
    NS_INIT_ISUPPORTS();

    NS_PRECONDITION(listener, "null listener");
}

nsHttpTransaction::~nsHttpTransaction()
{
    NS_IF_RELEASE(mConnection);
}

nsresult
nsHttpTransaction::SetRequestInfo(nsHttpAtom method,
                                  nsHttpVersion version,
                                  const char *requestURI,
                                  nsHttpHeaderArray *requestHeaders)
{
    // Write out request line:
    mRequestBuf.Assign(method.get());
    mRequestBuf.Append(' ');
    mRequestBuf.Append(requestURI);
    mRequestBuf.Append(" HTTP/");
    switch (version) {
    case HTTP_VERSION_1_1:
        mRequestBuf.Append("1.1");
        break;
    case HTTP_VERSION_0_9:
        mRequestBuf.Append("0.9");
        break;
    default:
        mRequestBuf.Append("1.0");
    }
    mRequestBuf.Append("\r\n");

    // Write out request headers:
    nsresult rv = requestHeaders->VisitHeaders(this);
    if (NS_FAILED(rv)) return rv;

    // Write out end-of-headers sequence:
    mRequestBuf.Append("\r\n");

    // Create a string stream for the request header buf
    nsCOMPtr<nsISupports> sup;
    rv = NS_NewCStringInputStream(getter_AddRefs(sup), mRequestBuf);
    if (NS_FAILED(rv)) return rv;
    mRequestHeaderStream = do_QueryInterface(sup, &rv);
    return rv;
}

void
nsHttpTransaction::SetConnection(nsHttpConnection *connection)
{
    NS_IF_RELEASE(mConnection);
    mConnection = connection;
    NS_IF_ADDREF(mConnection);
}

// called on the socket transport thread
nsresult
nsHttpTransaction::OnDataWritable(nsIOutputStream *os, PRUint32 count)
{
    // Write out the request
    PRUint32 writeCount = 0;
    LOG(("nsHttpTransaction::OnDataWritable [this=%x]\n", this));
    return os->WriteFrom(mRequestHeaderStream, count, &writeCount);
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
nsHttpTransaction::ParseVersion(const char *ver)
{
    // Parse HTTP-Version:: "HTTP" "/" 1*DIGIT "." 1*DIGIT
    char *p = PL_strstr(ver, "HTTP");
    if (p != ver)
        return NS_ERROR_UNEXPECTED;
    p += 4;
    if (*p != '/') {
        LOG(("server did not send a version number; assuming HTTP/1.0\n"));
        // NCSA/1.5.2 has a bug in which it fails to send a version number
        // if the request version is HTTP/1.1, so we fall back on HTTP/1.0
        mResponseVersion = HTTP_VERSION_1_0;
        return NS_OK;
    }

    ver = p + 1; // let ver point to the major version
    if ((p = PL_strchr(ver, '.')) == nsnull) {
        LOG(("mal-formed server version; assuming HTTP/1.0\n"));
        mResponseVersion = HTTP_VERSION_1_0;
        return NS_OK;
    }
    *p = 0;

    p++; // let p point to the minor version

    int major = atoi(ver);
    int minor = atoi(p);

    if ((major > 1) || ((major == 1) && (minor >= 1)))
        // at least HTTP/1.1
        mResponseVersion = HTTP_VERSION_1_1;
    else
        // treat anything else as version 1.0
        mResponseVersion = HTTP_VERSION_1_0;

    return NS_OK;
}

nsresult
nsHttpTransaction::ParseStatusLine(const char *line)
{
    LOG(("nsHttpTransaction::ParseStatusLine [%s]\n", line));

    NS_PRECONDITION(!mHaveStatusLine, "should not be called");

    //
    // Parse Status-Line:: HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    //
    nsresult rv;
    char *p;
 
    // HTTP-Version
    if ((p = PL_strchr(line, ' ')) == nsnull) {
        // 0.9 servers do not send a status line
        LOG(("looks like a HTTP/0.9 response\n"));
        mResponseVersion = HTTP_VERSION_0_9;
        mResponseStatus = 200;
        mResponseStatusText = "OK";
        goto end;
    }
    *p = 0;
    rv = ParseVersion(line);
    if (NS_FAILED(rv)) return rv;
    line = p + 1;
    
    // Status-Code
    if ((p = PL_strchr(line, ' ')) == nsnull) {
        LOG(("mal-formed response line; assuming status = 200\n"));
        mResponseStatus = 200;
        mResponseStatusText = "OK";
        goto end;
    }
    *p = 0;
    mResponseStatus = atoi(line);
    if (mResponseStatus == 0) {
        LOG(("mal-formed response status; assuming status = 200\n"));
        mResponseStatus = 200;
    }
    line = p + 1;

    // Reason-Phrase is whatever is remaining of the line
    mResponseStatusText = line;

end:
    mHaveStatusLine = PR_TRUE;
    LOG(("Have status line [version=%d status=%d statusText=%s]\n",
        mResponseVersion, mResponseStatus, mResponseStatusText.get()));
    return NS_OK;
}

nsresult
nsHttpTransaction::ParseHeaderLine(const char *line)
{
    LOG(("nsHttpTransaction::ParseHeaderLine [%s]\n", line));

    NS_PRECONDITION(!mHaveAllHeaders, "should not be called");

    char *p;
    if (*line == '\0')
        mHaveAllHeaders = PR_TRUE;
    else if ((p = PL_strchr(line, ':')) != nsnull) {
        *p = 0;
        nsHttpAtom atom = nsHttp::ResolveAtom(line);
        if (atom) {
            // skip over whitespace
            do {
                p++;
            } while (*p == ' ');
            // assign response header
            mResponseHeaders.SetHeader(atom, p);
        }
        else
            LOG(("unknown header; skipping\n"));
    }
    else
        LOG(("mal-formed header\n"));

    // We ignore mal-formed headers in the hope that we'll still be able
    // to do something useful with the response.
    return NS_OK;
}

nsresult
nsHttpTransaction::ParseLine(const char *line)
{
    if (!mHaveStatusLine)
        return ParseStatusLine(line);

    return ParseHeaderLine(line);
}

nsresult
nsHttpTransaction::HandleSegment(const char *segment,
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
            ParseLine(mLineBuf.get());
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

    NS_PRECONDITION(mConnection, "no connection");

    if (!mFiredOnStart) {
        mFiredOnStart = PR_TRUE;

        LOG(("nsHttpTransaction [this=%x] sending OnStartRequest\n", this));
        rv = mListener->OnStartRequest(this, nsnull);
        if (NS_FAILED(rv)) return rv;

        const char *val;

        // decode the content-length header if given
        val = mResponseHeaders.PeekHeader(nsHttp::Content_Length);
        if (val)
            mContentLength = atoi(val);

        // decode the content-type header if given
        val = mResponseHeaders.PeekHeader(nsHttp::Content_Type);
        if (val)
            DecodeContentType(val);

        // handle chunked encoding here, so we'll know immediately when
        // we're done with the socket.  please note that _all_ other
        // decoding is done when the channel receives the content data
        // so as not to block the socket transport thread too much.
        val = mResponseHeaders.PeekHeader(nsHttp::Transfer_Encoding);
        if (val) {
            // we only support the "chunked" transfer encoding right now.
            rv = InstallChunkedDecoder();
            if (NS_FAILED(rv)) return rv;
        }
    }

    // we cannot trust that all of "count" data will be read, so we have to
    // interogate the connection for the current number of bytes read.
    PRUint32 before = 0, after = 0;
    if (fromSocket) {
        rv = mConnection->GetBytesRead(&before);
        if (NS_FAILED(rv)) return rv;
    }

    rv = mListener->OnDataAvailable(this, nsnull, stream, mContentRead, count);
    if (NS_FAILED(rv)) return rv;

    if (fromSocket) {
        rv = mConnection->GetBytesRead(&after);
        if (NS_FAILED(rv)) return rv;
    }
    else
        after = count;

    NS_ASSERTION(after >= before, "bytes read NOT monotonically increasing!");

    // update count of content bytes read..
    mContentRead += (after - before);

    LOG(("nsHttpTransaction [this=%x mContentRead=%u mContentLength=%d]\n",
        this, mContentRead, mContentLength));

    // check for end-of-file
    if ((PRInt32(mContentRead) == mContentLength) ||
            (mChunkConvCtx && mChunkConvCtx->GetEOF())) {
        // let the connection know that we are done with it; this should
        // result in OnStopTransaction being fired.
        rv = mConnection->OnTransactionComplete(NS_OK);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsHttpTransaction::DecodeContentType(const char *val)
{
    nsCAutoString type(val);

    // we don't care about comments
    PRInt32 i = type.FindChar('(');
    if (i != kNotFound) {
        type.Truncate(i);
        type.Trim(" ", PR_FALSE);
    }

    if (!type.IsEmpty()) {
        i = type.FindChar(';');
        if (i == kNotFound)
            mContentType = type.get();
        else {
            // the content-type value has additional fields
            nsCAutoString buf;
            type.Left(buf, i);
            mContentType = buf.get();

            // does the content-type value contain a charset?
            type.Mid(buf, i+1, PRUint32(-1));
            buf.Trim(" ");
            if (buf.Find("charset=", PR_TRUE) == 0) {
                // set the charset
                buf.Cut(0, sizeof("charset=") - 1);
                mContentCharset = buf.get();
            }
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
    const char *val = mResponseHeaders.PeekHeader(nsHttp::Trailer);
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

NS_IMPL_THREADSAFE_ISUPPORTS2(nsHttpTransaction,
                              nsIRequest,
                              nsIHttpHeaderVisitor)

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

NS_IMETHODIMP
nsHttpTransaction::Cancel(nsresult status)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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

//-----------------------------------------------------------------------------
// nsHttpTransaction::nsIHttpHeaderVisitor
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpTransaction::VisitHeader(const char *header, const char *value)
{
    mRequestBuf.Append(header);
    mRequestBuf.Append(": ");
    mRequestBuf.Append(value);
    mRequestBuf.Append("\r\n");
    return NS_OK;
}
