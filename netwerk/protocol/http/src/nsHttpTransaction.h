#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"

class nsHttpConnection;
class nsHttpConnectionInfo;
class nsHTTPChunkConvContext;

//-----------------------------------------------------------------------------
// nsHttpTransaction represents a single HTTP transaction.  It is thread-safe,
// intended to run on the socket thread.
//-----------------------------------------------------------------------------

class nsHttpTransaction : public nsIRequest
                        , public nsIHttpHeaderVisitor
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIHTTPHEADERVISITOR

    // A transaction is constructed from request headers.
    nsHttpTransaction(nsIStreamListener *);
    virtual ~nsHttpTransaction();

    // Called when added to a connection
    nsresult SetConnection(nsHttpConnection *);

    // Called to initialize the transaction
    nsresult SetRequestInfo(nsHttpAtom method,
                            nsHttpVersion version,
                            const char *requestURI,
                            nsHttpHeaderArray *headers);

    nsIStreamListener    *Listener()           { return mListener; }
    nsHttpConnection     *Connection()         { return mConnection; }

    nsHttpHeaderArray    &ResponseHeaders()    { return mResponseHeaders; }
    nsHttpVersion         ResponseVersion()    { return mResponseVersion; }
    PRUint32              ResponseStatus()     { return mResponseStatus; }
    const char           *ResponseStatusText() { return mResponseStatusText; }

    // Called to write data to the socket until return NS_BASE_STREAM_CLOSED
    nsresult OnDataWritable(nsIOutputStream *, PRUint32 count);

    // Called to read data from the socket
    nsresult OnDataAvailable(nsIInputStream *, PRUint32 count);

    // Called when the transaction completes, possibly prematurely with an error.
    nsresult OnStopRequest(nsresult);

private:
    nsresult ParseVersion(const char *version);
    nsresult ParseStatusLine(const char *line);
    nsresult ParseHeaderLine(const char *line);
    nsresult ParseLine(const char *line);
    nsresult HandleSegment(const char *segment, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContent(nsIInputStream *, PRUint32 count);
    nsresult InstallChunkedDecoder();

    // ReadSegments callback
    static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                     PRUint32, PRUint32, PRUint32 *);

private:
    nsCOMPtr<nsIStreamListener> mListener;

    nsHttpConnection           *mConnection; // hard reference

    nsCString                   mRequestBuf;   // flattened request headers
    nsCOMPtr<nsIInputStream>    mRequestHeaderStream;

    nsHttpHeaderArray           mResponseHeaders;
    nsHttpVersion               mResponseVersion;
    PRUint32                    mResponseStatus;
    nsXPIDLCString              mResponseStatusText;

    char                       *mReadBuf; // read ahead buffer
    nsCString                   mLineBuf; // may contain a partial line

    // we hold onto this context to know when eof has been reached
    nsHTTPChunkConvContext     *mChunkConvCtx;

    PRPackedBool                mHaveStatusLine;
    PRPackedBool                mHaveAllHeaders;
    PRPackedBool                mFiredOnStart;
};

#endif
