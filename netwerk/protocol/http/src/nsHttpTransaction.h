#ifndef nsHttpTransaction_h__
#define nsHttpTransaction_h__

#include "nsHttp.h"
#include "nsHttpHeaderArray.h"
#include "nsIStreamListener.h"
#include "nsIInputStream.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"

class nsHttpRequestHead;
class nsHttpResponseHead;
class nsHttpConnection;
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

    // Called when assigned to a connection
    void SetConnection(nsHttpConnection *);

    // Called to initialize the transaction
    nsresult SetupRequest(nsHttpRequestHead *, nsIInputStream *);

    nsIStreamListener  *Listener()     { return mListener; }
    nsHttpConnection   *Connection()   { return mConnection; }
    nsHttpResponseHead *ResponseHead() { return mResponseHead; }

    // Called to take ownership of the response headers; the transaction
    // will drop any reference to the response headers after this call.
    nsHttpResponseHead *TakeResponseHead();

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

    nsHttpConnection           *mConnection; // hard ref

    nsCString                   mReqHeaderBuf;    // flattened request headers
    nsCOMPtr<nsIInputStream>    mReqHeaderStream; // header data stream
    nsCOMPtr<nsIInputStream>    mReqUploadStream; // upload data stream

    nsHttpResponseHead         *mResponseHead;

    char                       *mReadBuf;         // read ahead buffer
    nsCString                   mLineBuf;         // may contain a partial line

    PRInt32                     mContentLength;   // equals -1 if unknown
    PRUint32                    mContentRead;     // count of consumed content bytes

    // we hold onto this context to know when eof has been reached
    nsHTTPChunkConvContext     *mChunkConvCtx;

    PRInt32                     mTransactionDone; // atomically {in,de}cremented

    PRPackedBool                mHaveStatusLine;
    PRPackedBool                mHaveAllHeaders;
    PRPackedBool                mFiredOnStart;
};

#if 0
//-----------------------------------------------------------------------------
// nsAHttpTransactionSink recieves notifications from the transaction.  This
// is, for example, implemented by nsHttpConnection.
//-----------------------------------------------------------------------------

class nsAHttpTransactionSink : public nsISupports
{
public:
    virtual nsresult OnHeadersAvailable(nsHttpTransaction *) = 0;
    virtual nsresult OnTransactionComplete(nsHttpTransaction *, nsresult) = 0;
};
#endif

#endif
