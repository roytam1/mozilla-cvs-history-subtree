#include "nsSimpleStreamProvider.h"
#include "nsIOutputStream.h"

//
//----------------------------------------------------------------------------
// nsISupports implementation...
//----------------------------------------------------------------------------
//
NS_IMPL_ISUPPORTS3(nsSimpleStreamProvider,
                   nsISimpleStreamProvider,
                   nsIStreamProvider,
                   nsIStreamObserver)

//
//----------------------------------------------------------------------------
// nsIStreamObserver implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsSimpleStreamProvider::OnStartRequest(nsIChannel *aChannel,
                                       nsISupports *aContext)
{
    return mObserver ?
        mObserver->OnStartRequest(aChannel, aContext) : NS_OK;
}

NS_IMETHODIMP
nsSimpleStreamProvider::OnStopRequest(nsIChannel *aChannel,
                                      nsISupports *aContext,
                                      nsresult aStatus,
                                      const PRUnichar *aStatusText)
{
    return mObserver ?
        mObserver->OnStopRequest(aChannel, aContext, aStatus, aStatusText) : NS_OK;
}

//
//----------------------------------------------------------------------------
// nsIStreamProvider implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsSimpleStreamProvider::OnProvideData(nsIChannel *aChannel,
                                      nsISupports *aContext,
                                      nsIOutputStream *aOutput,
                                      PRUint32 aOffset,
                                      PRUint32 aCount)
{
    PRUint32 writeCount;
    nsresult rv = aOutput->WriteFrom(mSource, aCount, &writeCount);
    //
    // Equate zero bytes written and NS_SUCCEEDED to EOF
    //
    if (NS_SUCCEEDED(rv) && (writeCount == 0))
        return NS_BASE_STREAM_CLOSED;
    return rv;
}

//
//----------------------------------------------------------------------------
// nsISimpleStreamProvider implementation...
//----------------------------------------------------------------------------
//
NS_IMETHODIMP
nsSimpleStreamProvider::Init(nsIInputStream *aSource,
                             nsIStreamObserver *aObserver)
{
    NS_PRECONDITION(aSource, "null input stream");

    mSource = aSource;
    mObserver = aObserver;

    return NS_OK;
}
