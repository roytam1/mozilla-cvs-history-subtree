#include "nsStreamListenerProxy.h"
#include "nsIGenericFactory.h"
#include "nsIInputStream.h"
#include "nsIPipe.h"
#include "nsAutoLock.h"

#define PRINTF NS_LOG_PRINTF(nsStreamProxyLog)
#define FLUSH NS_LOG_FLUSH(nsStreamProxyLog)

#define DEFAULT_BUFFER_SEGMENT_SIZE 2048
#define DEFAULT_BUFFER_MAX_SIZE  (4*2048)

#define GET_LISTENER_PROXY(p) \
    ((nsStreamListenerProxy *) (nsIStreamObserverProxy *) p)

#ifdef DEBUG
//
//----------------------------------------------------------------------------
// nsInputStreamGuard
//   - limits the listener's access to the underlying pipe
//   - records the number of bytes read from the pipe for debugging purposes.
//----------------------------------------------------------------------------
//
class nsInputStreamGuard : public nsIInputStream
{
    NS_DECL_ISUPPORTS

    nsInputStreamGuard() : mBytesRead(0) {NS_INIT_ISUPPORTS();}
    virtual ~nsInputStreamGuard() {}

    NS_IMETHOD Close() {
        mSource = 0;
        return NS_OK;
    }
    NS_IMETHOD Available(PRUint32 *aResult) {
        NS_PRECONDITION(mSource, "source is null");
        return mSource->Available(aResult);
    }
    NS_IMETHOD Read(char *aBuf, PRUint32 aCount, PRUint32 *aBytesRead) {
        NS_PRECONDITION(mSource, "source is null");
        nsresult rv = mSource->Read(aBuf, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                            PRUint32 aCount, PRUint32 *aBytesRead) {
        NS_PRECONDITION(mSource, "source is null");
        nsresult rv = mSource->ReadSegments(aWriter, aClosure, aCount, aBytesRead);
        if (NS_SUCCEEDED(rv))
            mBytesRead += *aBytesRead;
        return rv;
    }
    NS_IMETHOD GetNonBlocking(PRBool *aResult) {
        NS_PRECONDITION(mSource, "source is null");
        return mSource->GetNonBlocking(aResult);
    }
    NS_IMETHOD GetObserver(nsIInputStreamObserver **aObserver) {
        NS_NOTREACHED("GetObserver");
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    NS_IMETHOD SetObserver(nsIInputStreamObserver *aObserver) {
        NS_NOTREACHED("SetObserver");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    // 
    // Helper methods
    //
    void SetSource(nsIInputStream *aSource) {
        mSource = aSource;
    }
    PRUint32 GetBytesRead() {
        return mBytesRead;
    }

protected:
    nsCOMPtr<nsIInputStream> mSource;
    PRUint32                 mBytesRead;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsInputStreamGuard,
                              nsIInputStream)

#define TO_INPUT_STREAM_GUARD(p) \
    ((nsInputStreamGuard *) (nsIInputStream *) p)

#endif /* !DEBUG */

//
//----------------------------------------------------------------------------
// nsStreamListenerProxy implementation...
//----------------------------------------------------------------------------
//

nsStreamListenerProxy::nsStreamListenerProxy()
    : mListenerStatus(NS_OK)
    , mLMonitor(nsnull)
    , mCMonitor(nsnull)
{ }

nsStreamListenerProxy::~nsStreamListenerProxy()
{
    if (mLMonitor) {
        nsAutoMonitor::DestroyMonitor(mLMonitor);
        mLMonitor = nsnull;
    }
    if (mCMonitor) {
        nsAutoMonitor::DestroyMonitor(mCMonitor);
        mCMonitor = nsnull;
    }
}

//
//----------------------------------------------------------------------------
// nsOnDataAvailableEvent internal class...
//----------------------------------------------------------------------------
//
class nsOnDataAvailableEvent : public nsStreamObserverEvent
{
public:
    nsOnDataAvailableEvent(nsIStreamObserverProxy *aProxy,
                           nsIChannel *aChannel,
                           nsISupports *aContext,
                           nsIInputStream *aSource,
                           PRUint32 aOffset,
                           PRUint32 aCount)
        : nsStreamObserverEvent(aProxy, aChannel, aContext)
        , mSource(aSource)
        , mOffset(aOffset)
        , mCount(aCount)
    {
        MOZ_COUNT_CTOR(nsOnDataAvailableEvent);
    }

   ~nsOnDataAvailableEvent()
    {
        MOZ_COUNT_DTOR(nsOnDataAvailableEvent);
    }

    NS_IMETHOD HandleEvent();

protected:
   nsCOMPtr<nsIInputStream> mSource;
   PRUint32                 mOffset;
   PRUint32                 mCount;
};

NS_IMETHODIMP
nsOnDataAvailableEvent::HandleEvent()
{
    PRINTF("HandleEvent -- OnDataAvailable [event=%x]", this);

    nsStreamListenerProxy *listenerProxy = GET_LISTENER_PROXY(mProxy);

    nsIStreamListener *listener = listenerProxy->GetListener();
    if (!listener) {
        PRINTF("Already called OnStopRequest (listener is NULL)\n");
        return NS_ERROR_FAILURE;
    }

    nsresult status = NS_OK;
    nsresult rv = mChannel->GetStatus(&status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetStatus failed");

    //
    // We should only forward this event to the listener if the channel is
    // still in a "good" state.  Because these events are being processed
    // asynchronously, there is a very real chance that the listener might
    // have cancelled the channel after _this_ event was triggered.
    //
    if (NS_SUCCEEDED(status)) {
#ifdef NS_ENABLE_LOGGING
        {
            PRUint32 avail;
            mSource->Available(&avail);
            PRINTF("HandleEvent -- calling the consumer's OnDataAvailable [offset=%u count=%u avail=%u]\n",
                    mOffset, mCount, avail);FLUSH();
        }
#endif

        //
        // If we are the only event at the moment and the pipe has extra data...
        // feed that to the client...
        //
        //if (listenerProxy->mChannelToResume && (avail > mCount)) {
        //    NS_WARNING("pipe contains unread data");
        //    mCount = avail; // this is potentially very bad
        //}

        // Give the listener a chance to read some data.
        rv = listener->OnDataAvailable(mChannel, mContext, mSource, mOffset, mCount);

        PRINTF("HandleEvent -- done with the consumer's OnDataAvailable [rv=%x]\n", rv);FLUSH();

#ifdef DEBUG
        PRUint32 bytesRead = TO_INPUT_STREAM_GUARD(mSource)->GetBytesRead();
        PRINTF("HandleEvent -- %u bytes read out of %u total\n", bytesRead, mCount);FLUSH();
#endif

        //
        // We must honor the listener's desire to suspend the channel.
        //
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
            mChannel->Suspend();
            rv = NS_OK;
        }

        //
        // Update the listener status
        //
        nsAutoMonitor mon(listenerProxy->mLMonitor);
        listenerProxy->mListenerStatus = rv;
    }
#ifdef NS_ENABLE_LOGGING
    else {
        PRINTF("not calling OnDataAvailable");FLUSH();
    }
#endif
    return NS_OK;
}

//
//----------------------------------------------------------------------------
// nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_ISUPPORTS_INHERITED3(nsStreamListenerProxy,
                             nsStreamObserverProxy,
                             nsIStreamListenerProxy,
                             nsIStreamListener,
                             nsIInputStreamObserver)

//
//----------------------------------------------------------------------------
// nsIStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamListenerProxy::OnStartRequest(nsIChannel *aChannel,
                                      nsISupports *aContext)
{

    return nsStreamObserverProxy::OnStartRequest(aChannel, aContext);
}

NS_IMETHODIMP
nsStreamListenerProxy::OnStopRequest(nsIChannel *aChannel,
                                     nsISupports *aContext,
                                     nsresult aStatus,
                                     const PRUnichar *aStatusText)
{
    //
    // We are done with the pipe.
    //
    mPipeIn = 0;
    mPipeOut = 0;

    return nsStreamObserverProxy::OnStopRequest(aChannel, aContext,
                                                aStatus, aStatusText);
}

//
//----------------------------------------------------------------------------
// nsIStreamListener implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamListenerProxy::OnDataAvailable(nsIChannel *aChannel,
                                       nsISupports *aContext,
                                       nsIInputStream *aSource,
                                       PRUint32 aOffset,
                                       PRUint32 aCount)
{
    nsresult rv;

    PRINTF("nsStreamListenerProxy::OnDataAvailable [offset=%u, count=%u]\n",
            aOffset, aCount);

    NS_PRECONDITION(mChannelToResume == 0, "Unexpected call to OnDataAvailable");
    NS_PRECONDITION(mPipeIn, "Pipe not initialized");
    NS_PRECONDITION(mPipeOut, "Pipe not initialized");
    NS_PRECONDITION(mListenerStatus != NS_BASE_STREAM_WOULD_BLOCK,
                    "Invalid listener status");

    //
    // Any non-successful listener status gets passed back to the caller
    //
    {
        nsAutoMonitor mon(mLMonitor);
        PRINTF("mListenerStatus=%x\n", mListenerStatus);
        if (NS_FAILED(mListenerStatus)) 
            return mListenerStatus;
    }
    
    //
    // Enter the ChannelToResume monitor
    //
    nsAutoMonitor mon(mCMonitor);

    // 
    // If there is data already in the pipe, then suspend the calling
    // channel.  It will be resumed when the pipe is emptied.  Being
    // inside the "C" monitor ensures that the resume will follow the
    // suspend.
    //
    PRUint32 count, bytesWritten=0;
    rv = mPipeIn->Available(&count);
    if (NS_FAILED(rv)) return rv;

    if (count == 0) {
        PRINTF("Writing to the pipe...\n");FLUSH();
        rv = mPipeOut->WriteFrom(aSource, aCount, &bytesWritten);

        PRINTF("mPipeOut->WriteFrom(aSource) [rv=%x aCount=%u bytesWritten=%u]\n",
                rv, aCount, bytesWritten);FLUSH();
    }
    else {
        PRINTF("Already %u bytes in the pipe\n", count);
        rv = NS_BASE_STREAM_WOULD_BLOCK;
    }

    if (NS_FAILED(rv)) {
        if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
            PRINTF("Setting channel to resume\n");FLUSH();
            mChannelToResume = aChannel;
        }
        return rv;
    }
    else if (bytesWritten == 0) {
        PRINTF("Copied zero bytes; not posting an event\n");
        return NS_BASE_STREAM_CLOSED; // there was no more data to read!
    }

    //
    // Post an event for the number of bytes actually written.
    //
    nsCOMPtr<nsIInputStream> source;
#ifdef DEBUG
    NS_NEWXPCOM(source, nsInputStreamGuard);
    TO_INPUT_STREAM_GUARD(source)->SetSource(mPipeIn);
#else
    source = mPipeIn;
#endif
    nsOnDataAvailableEvent *ev =
        new nsOnDataAvailableEvent(this, aChannel, aContext, source,
                                   aOffset, bytesWritten);
    if (!ev) return NS_ERROR_OUT_OF_MEMORY;

    rv = ev->FireEvent(mEventQueue);
    if (NS_FAILED(rv)) {
        delete ev;
        return rv;
    }
    return NS_OK;
}

//
//----------------------------------------------------------------------------
// nsIStreamListenerProxy implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamListenerProxy::Init(nsIStreamListener *aListener,
                            nsIEventQueue *aEventQ,
                            PRUint32 aBufferSegmentSize,
                            PRUint32 aBufferMaxSize)
{
    NS_PRECONDITION(mReceiver == 0, "Listener already set");
    NS_PRECONDITION(mEventQueue == 0, "Event queue already set");

    mLMonitor = nsAutoMonitor::NewMonitor("ListenerStatus");
    if (!mLMonitor) return NS_ERROR_OUT_OF_MEMORY;
    mCMonitor = nsAutoMonitor::NewMonitor("ChannelToResume");
    if (!mCMonitor) return NS_ERROR_OUT_OF_MEMORY;

    //
    // Create the pipe
    //
    if (aBufferSegmentSize == 0)
        aBufferSegmentSize = DEFAULT_BUFFER_SEGMENT_SIZE;
    if (aBufferMaxSize == 0)
        aBufferMaxSize = DEFAULT_BUFFER_MAX_SIZE;
    // The segment size must not exceed the maximum
    aBufferSegmentSize = PR_MIN(aBufferMaxSize, aBufferSegmentSize);

    nsresult rv = NS_NewPipe(getter_AddRefs(mPipeIn),
                             getter_AddRefs(mPipeOut),
                             aBufferSegmentSize,
                             aBufferMaxSize,
                             PR_TRUE, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = mPipeIn->SetObserver(this);
    if (NS_FAILED(rv)) {
        PRINTF("SetObserver failed: rv=%x\n", rv);
        return rv;
    }

    nsCOMPtr<nsIInputStreamObserver> ob;
    rv = mPipeIn->GetObserver(getter_AddRefs(ob));
    if (NS_FAILED(rv)) {
        PRINTF("GetObserver failed: rv=%x\n", rv);
        return rv;
    }
    PRINTF("this=%x, observer=%x\n", this, ob.get());

    return nsStreamObserverProxy::Init(aListener, aEventQ);
}

//
//----------------------------------------------------------------------------
// nsIInputStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamListenerProxy::OnEmpty(nsIInputStream *aInputStream)
{
    PRINTF("OnEmpty\n");FLUSH();
    //
    // The pipe has been emptied by the listener.  If the channel
    // has been suspended (waiting for the pipe to be emptied), then
    // go ahead and resume it.  But take care not to resume while 
    // holding the "C" monitor.
    //
    nsCOMPtr<nsIChannel> chan;
    {
        nsAutoMonitor mon(mCMonitor);

        chan = mChannelToResume;
        mChannelToResume = 0;
    }
    if (chan) {
        PRINTF("OnEmpty -- resuming channel\n");FLUSH();
        chan->Resume();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsStreamListenerProxy::OnClose(nsIInputStream *aInputStream)
{
    PRINTF("OnClose\n");FLUSH();
    return NS_OK;
}
