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
class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHTTPChunkConvContext;

//-----------------------------------------------------------------------------
// nsHttpTransaction represents a single HTTP transaction.  It is thread-safe,
// intended to run on the socket thread.
//-----------------------------------------------------------------------------

class nsHttpTransaction : public nsIRequest
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST

    // A transaction is constructed from request headers.
    nsHttpTransaction(nsIStreamListener *);
    virtual ~nsHttpTransaction();

    // Called when added to a connection
    void SetConnection(nsHttpConnection *);

    // Called to initialize the transaction
    nsresult SetRequestInfo(nsHttpRequestHead *, nsIInputStream *);

    nsIStreamListener    *Listener()           { return mListener; }
    nsHttpConnection     *Connection()         { return mConnection; }

    nsHttpResponseHead   *ResponseHead()       { return mResponseHead; }

    // Called to take ownership of the response headers; the transaction
    // will drop any reference to the response headers after this call.
    nsHttpResponseHead   *TakeResponseHead();

    // Called to write data to the socket until return NS_BASE_STREAM_CLOSED
    nsresult OnDataWritable(nsIOutputStream *, PRUint32 count);

    // Called to read data from the socket
    nsresult OnDataAvailable(nsIInputStream *, PRUint32 count);

    // Called when the transaction completes, possibly prematurely with an error.
    nsresult OnStopTransaction(nsresult);

private:
    nsresult ParseLine(const char *line);
    nsresult HandleSegment(const char *segment, PRUint32 count, PRUint32 *countRead);
    nsresult HandleContent(nsIInputStream *, PRUint32 count, PRBool fromSocket);
    nsresult InstallChunkedDecoder();

private:
    nsCOMPtr<nsIStreamListener> mListener;

    nsHttpConnection           *mConnection; // hard reference

    nsCString                   mRequestBuf;          // flattened request headers
    nsCOMPtr<nsIInputStream>    mRequestHeaderStream; // header data stream
    nsCOMPtr<nsIInputStream>    mRequestUploadStream; // upload data stream

    nsHttpResponseHead         *mResponseHead;

    char                       *mReadBuf; // read ahead buffer
    nsCommonCString             mLineBuf; // may contain a partial line

    PRInt32                     mContentLength; // equals -1 if unknown
    PRUint32                    mContentRead;   // count of consumed content bytes

    // we hold onto this context to know when eof has been reached
    nsHTTPChunkConvContext     *mChunkConvCtx;

    PRPackedBool                mHaveStatusLine;
    PRPackedBool                mHaveAllHeaders;
    PRPackedBool                mFiredOnStart;
};

#endif
