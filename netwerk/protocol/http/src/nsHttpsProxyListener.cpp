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

    // if the connect was successful then instruct the connection to start
    // speaking SSL.
    if (!failed)
        mConnection->ProxyStepUp();

    // advance the connection to the next transaction, ie. the pending
    // transaction.  we have to set the transaction even if the connect
    // failed so that it can receive the proper notifications.
    mConnection->SetTransaction(mPendingTransaction);

    // now we're done with the pending transaction.
    NS_RELEASE(mPendingTransaction);
    mPendingTransaction = 0;

    // if the connect actually failed, then we have to kill the transaction.
    if (failed) {
        // XXX we need to transfer the response head from the SSL proxy connect
        // transaction to the new transaction.  This is especially important
        // since SSL proxy servers could request authentication!
        mConnection->OnTransactionComplete(status);
    }  

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
    LOG(("nsHttpsProxyListener::OnDataAvailable [this=%x req=%x]\n", this, req));

    // we need to just eat whatever data there may be in the stream.
    // XXX would be nice to not eat an error message!

    char buf[256];
    while (count) {
        PRUint32 n = PR_MIN(256, count);

        nsresult rv = stream->Read(buf, n, &n);
        if (NS_FAILED(rv)) return rv;

        count -= n;
    }
    
    return NS_OK;
}
