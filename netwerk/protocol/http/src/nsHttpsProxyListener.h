#ifndef nsHttpsProxyListener_h__
#define nsHttpsProxyListener_h__

#include "nsIStreamListener.h"

class nsHttpConnection;
class nsHttpTransaction;

//-----------------------------------------------------------------------------
// nsHttpsProxyListener: listener of a CONNECT request for SSL proxy
//-----------------------------------------------------------------------------

class nsHttpsProxyListener : public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    nsHttpsProxyListener(nsHttpConnection *, nsHttpTransaction *);
    virtual ~nsHttpsProxyListener();

private:
    nsHttpConnection  *mConnection;
    nsHttpTransaction *mPendingTransaction;
};

#endif // nsHttpsProxyListener_h__
