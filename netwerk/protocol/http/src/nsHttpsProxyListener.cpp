#include "nsHttpsProxyListener.h"
#include "nsHttpConnection.h"
#include "nsHttpTransaction.h"
#include "nsHttpResponseHead.h"

//-----------------------------------------------------------------------------
// nsHttpsProxyListener <public>
//-----------------------------------------------------------------------------

nsHttpsProxyListener::nsHttpsProxyListener(nsHttpConnection *conn,
                                           nsHttpTransaction *trans)
    : mConnection(conn)
    , mPendingTransaction(trans)
{
    NS_ADDREF(mConnection);
    NS_ADDREF(mPendingTransaction);
}

nsHttpsProxyListener::~nsHttpsProxyListener()
{
    NS_IF_RELEASE(mConnection);
    NS_IF_RELEASE(mPendingTransaction);
}

//-----------------------------------------------------------------------------
// nsHttpsProxyListener::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(nsHttpsProxyListener,
                              nsIStreamListener,
                              nsIRequestObserver)

//-----------------------------------------------------------------------------
// nsHttpsProxyListener::nsIRequestObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpsProxyListener::OnStartRequest(nsIRequest *req, nsISupports *ctx)
{
    LOG(("nsHttpsProxyListener::OnStartRequest [this=%x req=%x]\n",
        this, req));

    // eat this event
    return NS_OK;
}

NS_IMETHODIMP
nsHttpsProxyListener::OnStopRequest(nsIRequest *req, nsISupports *ctx,
                                    nsresult status)
{
    LOG(("nsHttpsProxyListener::OnStopRequest [this=%x req=%x]\n",
        this, req));

    // advance the connection to the next transaction, ie. the pending
    // transaction.
    mConnection->SetTransaction(mPendingTransaction);

    // now we're done with the pending transaction.
    NS_RELEASE(mPendingTransaction);
    mPendingTransaction = 0;

    PRBool failed = NS_FAILED(status);
    if (!failed) {
        // if the transaction appeared to succeed but the http response itself
        // was not "200 Connection Established", then we must likewise abort.
        nsHttpTransaction *trans = (nsHttpTransaction *) req;
        if (trans->ResponseHead() && (trans->ResponseHead()->Status() != 200)) {
            failed = PR_TRUE;
            status = NS_BINDING_FAILED;
        }
    }

    if (failed)
        mConnection->OnTransactionComplete(status);

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpsProxyListener::nsIStreamListener
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpsProxyListener::OnDataAvailable(nsIRequest *req, nsISupports *ctx,
                                      nsIInputStream *stream,
                                      PRUint32 offset, PRUint32 count)
{
    LOG(("nsHttpsProxyListener::OnDataAvailable [this=%x req=%x] "
         "proxy connect response contains an entity body\n", this, req));
    
    // this should cause the connection to be cancelled.
    return NS_BASE_STREAM_CLOSED;
}
