#ifndef nsStreamListenerProxy_h__
#define nsStreamListenerProxy_h__

#include "nsStreamObserverProxy.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsCOMPtr.h"

//
// Our nsIPipe implementation does not always call OnEmpty when 
// the pipe is emptied.  This is most definitely a bug, and it
// needs to be fixed.
//
//#define HAVE_BROKEN_PIPE_IMPL

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

    nsCOMPtr<nsIChannel> mChannelToResume;
    PRMonitor           *mCMonitor;

#ifdef HAVE_BROKEN_PIPE_IMPL
    PRInt32 mPendingEvents;
#endif

protected:
    nsCOMPtr<nsIInputStream>  mPipeIn;
    nsCOMPtr<nsIOutputStream> mPipeOut;
};

#endif /* !nsStreamListenerProxy_h__ */
