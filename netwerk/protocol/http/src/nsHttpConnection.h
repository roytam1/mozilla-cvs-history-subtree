#ifndef nsHttpConnection_h__
#define nsHttpConnection_h__

#include "nsIStreamListener.h"
#include "nsIStreamProvider.h"
#include "nsISocketTransport.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "prclist.h"

class nsHttpHandler;
class nsHttpConnectionInfo;
class nsHttpTransaction;

//-----------------------------------------------------------------------------
// nsHttpConnection - represents a connection to a HTTP server (or proxy)
//-----------------------------------------------------------------------------

class nsHttpConnection : public nsIStreamListener
                       , public nsIStreamProvider
                       , public PRCList
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMPROVIDER
    NS_DECL_NSIREQUESTOBSERVER

    nsHttpConnection();
    virtual ~nsHttpConnection();

    nsresult Init(nsHttpConnectionInfo *);

    // SetTransaction causes the given transaction to be processed on this
    // connection.  It fails if there is already an existing transaction.
    nsresult SetTransaction(nsHttpTransaction *);

    // called by the transaction to inform the connection that all of the
    // headers are available.
    nsresult OnHeadersAvailable(nsHttpTransaction *);

    // called by the transaction to inform the connection that it is done.
    nsresult OnTransactionComplete(nsHttpTransaction *, nsresult status);

    PRBool   CanReuse(); // can this connection be reused?
    PRBool   IsAlive();
    PRUint32 ReuseCount() { return mReuseCount; }
    PRUint32 MaxReuseCount() { return mMaxReuseCount; }
    PRUint32 IdleTimeout() { return mIdleTimeout; }

    //nsISocketTransport   *SocketTransport() { return mTransport.get(); }
    nsHttpTransaction    *Transaction()     { return mTransaction; }
    nsHttpConnectionInfo *ConnectionInfo()  { return mConnectionInfo; }

private:
    enum {
        IDLE,
        WAITING_FOR_WRITE,
        WRITING,
        WAITING_FOR_READ,
        READING
    };

    nsresult ActivateConnection();
    nsresult CreateTransport();

private:
    nsCOMPtr<nsISocketTransport> mSocketTransport;
    nsCOMPtr<nsIRequest>         mWriteRequest;
    nsCOMPtr<nsIRequest>         mReadRequest;
    nsHttpTransaction           *mTransaction;    // hard ref
    nsHttpConnectionInfo        *mConnectionInfo; // hard ref
    PRUint32                     mState;
    PRUint32                     mReuseCount;
    PRUint32                     mMaxReuseCount; // value of keep-alive: max=
    PRUint32                     mIdleTimeout;   // value of keep-alive: timeout=
    PRPackedBool                 mKeepAlive;
};

//-----------------------------------------------------------------------------
// nsHttpConnectionInfo - holds the properties of a connection
//-----------------------------------------------------------------------------

class nsHttpConnectionInfo : public nsISupports
{
public:
    NS_DECL_ISUPPORTS

    nsHttpConnectionInfo(const char *host, PRInt32 port,
                         const char *proxyHost=0, PRInt32 proxyPort=-1,
                         const char *proxyType=0, PRBool usingSSL=0)
        : mPort(port)
        , mProxyPort(proxyPort)
        , mUsingSSL(usingSSL)
    {
        NS_INIT_ISUPPORTS();

        mHost = host;
        mProxyHost = proxyHost;
        mProxyType = proxyType;
    }
    
    virtual ~nsHttpConnectionInfo() {}

    // Compare this connection info to another...
    PRBool Equals(const nsHttpConnectionInfo *info)
    {
        return !PL_strcasecmp(info->mHost, mHost) &&
               !PL_strcasecmp(info->mProxyHost, mProxyHost) &&
               !PL_strcasecmp(info->mProxyType, mProxyType) &&
                info->mPort == mPort &&
                info->mProxyPort == mProxyPort &&
                info->mUsingSSL == mUsingSSL;
    }

    const char *Host()      { return mHost; }
    PRInt32     Port()      { return mPort; }
    const char *ProxyHost() { return mProxyHost; }
    PRInt32     ProxyPort() { return mProxyPort; }
    const char *ProxyType() { return mProxyType; }
    PRBool      UsingSSL()  { return mUsingSSL; }

private:
    nsXPIDLCString     mHost;
    PRInt32            mPort;
    nsXPIDLCString     mProxyHost;
    PRInt32            mProxyPort;
    nsXPIDLCString     mProxyType;
    PRPackedBool       mUsingSSL;
};

#endif // nsHttpConnection_h__
