#ifndef nsStreamProviderProxy_h__
#define nsStreamProviderProxy_h__

#include "nsStreamObserverProxy.h"
#include "nsIStreamProvider.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"

class nsStreamProviderProxy : public nsStreamObserverProxyBase
                            , public nsIStreamProviderProxy
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMPROVIDER
    NS_DECL_NSISTREAMPROVIDERPROXY

    nsStreamProviderProxy();
    virtual ~nsStreamProviderProxy();

    nsIStreamProvider *GetProvider()
    {
        return NS_STATIC_CAST(nsIStreamProvider *, GetReceiver());
    }

    //
    // The provider status is controlled by the event on the
    // provider's thread. The monitor protects access to the
    // provider's status.
    //
    nsresult mProviderStatus;

protected:
    nsCOMPtr<nsIInputStream>  mPipeIn;
    nsCOMPtr<nsIOutputStream> mPipeOut;
};

#endif /* !nsStreamProviderProxy_h__ */
