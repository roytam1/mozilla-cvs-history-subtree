#ifndef nsHttpChannel_h__
#define nsHttpChannel_h__

#include "nsHttpRequestHead.h"
#include "nsIHttpChannel.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"

class nsHttpTransaction;
class nsHttpConnectionInfo;
class nsHttpResponseHead;

//-----------------------------------------------------------------------------
// nsHttpChannel
//-----------------------------------------------------------------------------

class nsHttpChannel : public nsIHttpChannel
                    , public nsIStreamListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIHTTPCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    nsHttpChannel();
    virtual ~nsHttpChannel();

    nsresult Init(nsIURI *uri,
                  PRUint32 capabilities,
                  const char *proxyHost=0,
                  PRInt32 proxyPort=-1,
                  const char *proxyType=0);

private:
    nsresult Connect();
    nsresult SetupTransaction();
    nsresult BuildConnectionInfo(nsHttpConnectionInfo **);
    nsresult BuildStreamListenerProxy(nsIStreamListener **);
    nsresult ProcessServerResponse();
    nsresult ProcessNormal();
    nsresult ProcessNotModified();
    nsresult ProcessRedirection(PRUint32 httpStatus);
    nsresult ProcessAuthentication(PRUint32 httpStatus);

private:
    nsCOMPtr<nsIURI>                mURI;
    nsCOMPtr<nsIStreamListener>     mListener;
    nsCOMPtr<nsISupports>           mListenerContext;
    nsCOMPtr<nsILoadGroup>          mLoadGroup;
    nsCOMPtr<nsISupports>           mOwner;
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsIURI>                mReferrer;

    nsHttpRequestHead               mRequestHead;
    nsHttpResponseHead             *mResponseHead;

    nsHttpTransaction              *mTransaction;    // hard ref
    nsHttpConnectionInfo           *mConnectionInfo; // hard ref

    nsXPIDLCString                  mSpec;

    PRUint32                        mLoadFlags;
    PRUint32                        mCapabilities;
    PRUint32                        mStatus;

    PRPackedBool                    mIsPending;
};

#endif
