#ifndef nsHttpHandler_h__
#define nsHttpHandler_h__

#include "nsIHttpProtocolHandler.h"
#include "nsIProtocolProxyService.h"
#include "nsIPref.h"
#include "nsIObserver.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsHttpConnection;
class nsHttpConnectionInfo;
class nsHttpTransaction;
class nsHttpHeaderArray;

//-----------------------------------------------------------------------------
// nsHttpHandler - protocol handler for HTTP and HTTPS
//-----------------------------------------------------------------------------

class nsHttpHandler : public nsIHttpProtocolHandler
                    , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROTOCOLHANDLER
    NS_DECL_NSIHTTPPROTOCOLHANDLER
    NS_DECL_NSIOBSERVER

    enum {
        ALLOW_KEEPALIVE  = 1 << 0,
        ALLOW_PIPELINING = 1 << 1
    };

    nsHttpHandler();
    virtual ~nsHttpHandler();

    // Returns a pointer to the one and only HTTP handler
    static nsHttpHandler *get() { return mGlobalInstance; }

    nsresult Init();
    nsresult AddStandardRequestHeaders(nsHttpHeaderArray *,
                                       PRUint32 capabilities);

    //
    // Connection management methods:
    //
    // - the handler only owns idle connections; it does not own active
    //   connections.
    //
    // - the handler keeps a count of active connections to enforce the
    //   steady-state max-connections pref.
    // 

    // Called to kick-off a new transaction
    nsresult InitiateTransaction(nsHttpTransaction *,
                                 nsHttpConnectionInfo *);

    // The connection count indicates the number of http sockets currently
    // in existence.
    void     IncrementConnectionCount();
    void     DecrementConnectionCount();
    PRUint32 ConnectionCount();

    // The active connection count indicates the number of http sockets
    // currently processing a transaction.
    void     IncrementActiveConnectionCount();
    void     DecrementActiveConnectionCount();
    PRUint32 ActiveConnectionCount();

    // Connection recycling is explicit
    nsresult RecycleConnection(nsHttpConnection *);

private:
    void BuildUserAgent();
    void InitUserAgentComponents();
    void PrefsChanged(const char *pref = nsnull);

    nsresult SetAcceptLanguages(const char *);
    nsresult SetAcceptEncodings(const char *);
    nsresult SetAcceptCharsets(const char *);

    const char *AcceptLanguages() { return mAcceptLanguages.get(); }
    const char *AcceptEncodings() { return mAcceptEncodings.get(); }
    const char *AcceptCharsets() { return mAcceptCharsets.get(); }
    const char *UserAgent();

    static PRInt32 PR_CALLBACK PrefsCallback(const char *, void *);

    nsresult CreateServicesFromCategory(const char *category);

private:
    static nsHttpHandler *mGlobalInstance;

    // cached services
    nsCOMPtr<nsIPref>                 mPrefs;
    nsCOMPtr<nsIProtocolProxyService> mProxySvc;

    // prefs
    PRInt32  mKeepAliveTimeout;
    PRInt32  mMaxConnections;
    PRInt32  mBrowseAnonymously;
    PRUint32 mHttpVersion;
    PRUint32 mCapabilities;
    PRUint32 mProxyCapabilities;
    PRBool   mProxySSLConnectAllowed;
    PRInt32  mConnectTimeout;
    PRInt32  mRequestTimeout;
    PRInt32  mMaxAllowedKeepAlives;
    PRInt32  mMaxAllowedKeepAlivesPerServer;

    nsXPIDLCString mAcceptLanguages;
    nsXPIDLCString mAcceptEncodings;
    nsXPIDLCString mAcceptCharsets;

    // useragent components
    nsXPIDLCString mAppName;
    nsXPIDLCString mAppVersion;
    nsXPIDLCString mPlatform;
    nsXPIDLCString mOscpu;
    nsXPIDLCString mSecurity;
    nsXPIDLCString mLanguage;
    nsXPIDLCString mMisc;
    nsXPIDLCString mVendor;
    nsXPIDLCString mVendorSub;
    nsXPIDLCString mVendorComment;
    nsXPIDLCString mProduct;
    nsXPIDLCString mProductSub;
    nsXPIDLCString mProductComment;

    nsCString      mUserAgent;
    nsXPIDLCString mUserAgentOverride;
    PRPackedBool   mUserAgentIsDirty; // true if mUserAgent should be rebuilt
};

#endif // nsHttpHandler_h__
