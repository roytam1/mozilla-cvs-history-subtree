#include "nsStreamProviderProxy.h"
#include "nsIPipe.h"

#define PRINTF NS_LOG_PRINTF(nsStreamProxyLog)
#define FLUSH NS_LOG_FLUSH(nsStreamProxyLog)

#define DEFAULT_BUFFER_SEGMENT_SIZE 2048
#define DEFAULT_BUFFER_MAX_SIZE  (4*2048)

nsStreamProviderProxy::nsStreamProviderProxy()
    : mProviderStatus(NS_OK)
{ }

nsStreamProviderProxy::~nsStreamProviderProxy()
{ }

//
//----------------------------------------------------------------------------
// nsOnProvideDataEvent internal class...
//----------------------------------------------------------------------------
//
class nsOnProvideDataEvent : public nsStreamObserverEvent
{
public:
    nsOnProvideDataEvent(nsStreamObserverProxyBase *aProxy,
                         nsIChannel *aChannel,
                         nsISupports *aContext,
                         nsIOutputStream *aSink,
                         PRUint32 aOffset,
                         PRUint32 aCount)
        : nsStreamObserverEvent(aProxy, aChannel, aContext)
        , mSink(aSink)
        , mOffset(aOffset)
        , mCount(aCount)
    {
        MOZ_COUNT_CTOR(nsOnProvideDataEvent);
    }

   ~nsOnProvideDataEvent()
    {
        MOZ_COUNT_DTOR(nsOnProvideDataEvent);
    }

    NS_IMETHOD HandleEvent();

protected:
   nsCOMPtr<nsIOutputStream> mSink;
   PRUint32                  mOffset;
   PRUint32                  mCount;
};

NS_IMETHODIMP
nsOnProvideDataEvent::HandleEvent()
{
    PRINTF("HandleEvent -- OnProvideData [event=%x]", this);

    nsStreamProviderProxy *providerProxy = 
        NS_STATIC_CAST(nsStreamProviderProxy *, mProxy);

    nsCOMPtr<nsIStreamProvider> provider = providerProxy->GetProvider();
    if (!provider) {
        PRINTF("Already called OnStopRequest (provider is NULL)\n");
        return NS_ERROR_FAILURE;
    }

    nsresult status = NS_OK;
    nsresult rv = mChannel->GetStatus(&status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "GetStatus failed");

    //
    // We should only forward this event to the provider if the channel is
    // still in a "good" state.  Because these events are being processed
    // asynchronously, there is a very real chance that the provider might
    // have cancelled the channel after _this_ event was triggered.
    //
    if (NS_SUCCEEDED(status)) {
        PRINTF("HandleEvent -- calling the consumer's OnProvideData\n");
        rv = provider->OnProvideData(mChannel, mContext, mSink, mOffset, mCount);
        PRINTF("HandleEvent -- done with the consumer's OnProvideData [rv=%x]\n", rv);

        //
        // Mask NS_BASE_STREAM_WOULD_BLOCK return values.
        //
        providerProxy->mProviderStatus = 
            rv != NS_BASE_STREAM_WOULD_BLOCK ? rv : NS_OK;

        //
        // The channel is already suspended, so unless the provider returned
        // NS_BASE_STREAM_WOULD_BLOCK, we should wake up the channel.
        //
        if (rv != NS_BASE_STREAM_WOULD_BLOCK)
            mChannel->Resume();
    }
#ifdef NS_ENABLE_LOGGING
    else
        PRINTF("not calling OnProvideData");
#endif
    return NS_OK;
}

//
//----------------------------------------------------------------------------
// nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_ISUPPORTS_INHERITED2(nsStreamProviderProxy,
                             nsStreamObserverProxyBase,
                             nsIStreamProviderProxy,
                             nsIStreamProvider)

//
//----------------------------------------------------------------------------
// nsIStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamProviderProxy::OnStartRequest(nsIChannel *aChannel,
                                      nsISupports *aContext)
{
    return nsStreamObserverProxyBase::OnStartRequest(aChannel, aContext);
}

NS_IMETHODIMP
nsStreamProviderProxy::OnStopRequest(nsIChannel *aChannel,
                                     nsISupports *aContext,
                                     nsresult aStatus,
                                     const PRUnichar *aStatusText)
{
    //
    // Close the pipe
    //
    mPipeIn = 0;
    mPipeOut = 0;

    return nsStreamObserverProxyBase::OnStopRequest(aChannel, aContext,
                                                    aStatus, aStatusText);
}

//
//----------------------------------------------------------------------------
// nsIStreamProvider implementation...
//----------------------------------------------------------------------------
//
static NS_METHOD
nsWriteToSink(nsIInputStream *source,
              void *closure,
              const char *fromRawSegment,
              PRUint32 offset,
              PRUint32 count,
              PRUint32 *writeCount)
{
    nsIOutputStream *sink = (nsIOutputStream *) closure;
    return sink->Write(fromRawSegment, count, writeCount);
}

NS_IMETHODIMP
nsStreamProviderProxy::OnProvideData(nsIChannel *aChannel,
                                     nsISupports *aContext,
                                     nsIOutputStream *aSink,
                                     PRUint32 aOffset,
                                     PRUint32 aCount)
{
    nsresult rv;

    PRINTF("nsStreamProviderProxy::OnProvideData [offset=%u, count=%u]\n",
            aOffset, aCount);

    NS_PRECONDITION(aCount > 0, "Invalid parameter");
    NS_PRECONDITION(mPipeIn, "Pipe not initialized");
    NS_PRECONDITION(mPipeOut, "Pipe not initialized");
    NS_PRECONDITION(mProviderStatus != NS_BASE_STREAM_WOULD_BLOCK,
                    "Invalid provider status");

    //
    // Any non-successful provider status gets passed back to the caller
    //
    PRINTF("mProviderStatus=%x\n", mProviderStatus);
    if (NS_FAILED(mProviderStatus))
        return mProviderStatus;

    //
    // Provide the channel with whatever data is already in the pipe (not
    // exceeding aCount).
    //
    PRUint32 count;
    rv = mPipeIn->Available(&count);
    if (NS_FAILED(rv)) return rv;

    if (count > 0) {
        count = PR_MIN(count, aCount);

        PRUint32 bytesWritten;
        rv = mPipeIn->ReadSegments(nsWriteToSink, aSink, count, &bytesWritten);
        if (NS_FAILED(rv)) return rv;

        return NS_OK;
    }

    //
    // Post an event requesting the provider for more data.
    //
    nsOnProvideDataEvent *ev =
        new nsOnProvideDataEvent(this, aChannel, aContext,
                                 mPipeOut, aOffset, aCount);
    if (!ev)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = ev->FireEvent(GetEventQueue());
    if (NS_FAILED(rv)) {
        delete ev;
        return rv;
    }
    return NS_BASE_STREAM_WOULD_BLOCK;
}

//
//----------------------------------------------------------------------------
// nsIStreamProviderProxy implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsStreamProviderProxy::Init(nsIStreamProvider *aProvider,
                            nsIEventQueue *aEventQ,
                            PRUint32 aBufferSegmentSize,
                            PRUint32 aBufferMaxSize)
{
    NS_PRECONDITION(GetReceiver() == nsnull, "Listener already set");
    NS_PRECONDITION(GetEventQueue() == nsnull, "Event queue already set");

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

    SetReceiver(aProvider);
    return SetEventQueue(aEventQ);
}
