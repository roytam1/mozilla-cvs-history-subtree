#ifndef nsStreamListenerProxy_h__
#define nsStreamListenerProxy_h__

#include "nsStreamObserverProxy.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"

class nsStreamListenerProxy : public nsStreamObserverProxy
                            , public nsIStreamListenerProxy
                            , public nsIInputStreamObserver
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSISTREAMOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSISTREAMLISTENERPROXY
    NS_DECL_NSIINPUTSTREAMOBSERVER

    nsStreamListenerProxy();
    virtual ~nsStreamListenerProxy();

    nsIStreamListener *GetListener()
    {
        return NS_STATIC_CAST(nsIStreamListener *, GetReceiver());
    }

    //
    // The listener status is controlled by the event on the
    // listener's thread. The monitor protects access to the
    // listener's status.
    //
    nsresult   mListenerStatus;
    PRMonitor *mLMonitor;

    nsCOMPtr<nsIChannel>      mChannelToResume;

protected:
    nsCOMPtr<nsIInputStream>  mPipeIn;
    nsCOMPtr<nsIOutputStream> mPipeOut;
    PRMonitor                *mCMonitor;
};

#endif /* !nsStreamListenerProxy_h__ */
