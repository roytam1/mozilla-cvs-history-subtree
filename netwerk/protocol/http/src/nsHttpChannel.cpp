#include "nsHttpChannel.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpHandler.h"
#include "nsHttpResponseHead.h"
#include "nsHttp.h"
#include "netCore.h"
#include "nsNetCID.h"
#include "nsString2.h"
#include "nsReadableUtils.h"

static NS_DEFINE_CID(kStreamListenerProxyCID, NS_STREAMLISTENERPROXY_CID);

//-----------------------------------------------------------------------------
// nsHttpChannel <public>
//-----------------------------------------------------------------------------

nsHttpChannel::nsHttpChannel()
    : mResponseHead(nsnull)
    , mTransaction(nsnull)
    , mConnectionInfo(nsnull)
    , mLoadFlags(LOAD_NORMAL)
    , mCapabilities(0)
    , mStatus(NS_OK)
    , mIsPending(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
}

nsHttpChannel::~nsHttpChannel()
{
    if (mResponseHead)
        delete mResponseHead;
}

nsresult
nsHttpChannel::Init(nsIURI *uri,
                    PRUint32 caps,
                    const char *proxyHost,
                    PRInt32 proxyPort,
                    const char *proxyType)
{
    nsresult rv;

    LOG(("nsHttpChannel::Init [this=%x]\n"));

    NS_PRECONDITION(uri, "null uri");

    mURI = uri;
    mCapabilities = caps;

    rv = mURI->GetSpec(getter_Copies(mSpec));
    if (NS_FAILED(rv)) return rv;

    //
    // Construct connection info object
    //
    nsXPIDLCString host;
    PRInt32 port = -1;
    PRBool usingSSL = PR_FALSE;
    
    rv = mURI->SchemeIs("https", &usingSSL);
    if (NS_FAILED(rv)) return rv;

    rv = mURI->GetHost(getter_Copies(host));
    if (NS_FAILED(rv)) return rv;

    rv = mURI->GetPort(&port);
    if (NS_FAILED(rv)) return rv;

    if (port == -1)
        port = 80;

    LOG(("host=%s port=%d\n", host.get(), port));

    mConnectionInfo = new nsHttpConnectionInfo(host, port,
                                               proxyHost, proxyPort,
                                               proxyType, usingSSL);
    if (!mConnectionInfo)
        return NS_ERROR_OUT_OF_MEMORY;

    //
    // Set request headers
    //
    nsCString hostLine;
    hostLine.Assign(host.get());
    if (port != -1) {
        hostLine.Append(':');
        hostLine.AppendInt(port);
    }
    rv = mRequestHead.SetHeader(nsHttp::Host, hostLine);
    if (NS_FAILED(rv)) return rv;

    rv = nsHttpHandler::get()->
            AddStandardRequestHeaders(&mRequestHead.Headers(), caps);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChannel <private>
//-----------------------------------------------------------------------------

nsresult
nsHttpChannel::Connect()
{
    LOG(("nsHttpChannel::Connect [this=%x]\n", this));

    nsresult rv = NS_OK;

    rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nsnull);

    return nsHttpHandler::get()->
            InitiateTransaction(mTransaction, mConnectionInfo);
}

nsresult
nsHttpChannel::SetupTransaction()
{
    NS_ENSURE_TRUE(!mTransaction, NS_ERROR_ALREADY_INITIALIZED);

    nsCOMPtr<nsIStreamListener> listenerProxy;
    nsresult rv = BuildStreamListenerProxy(getter_AddRefs(listenerProxy));
    if (NS_FAILED(rv)) return rv;

    // Create the transaction object
    mTransaction = new nsHttpTransaction(listenerProxy);
    if (!mTransaction)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mTransaction);

    // Use the URI path if not proxying
    nsXPIDLCString path;
    if (mConnectionInfo->ProxyHost() == nsnull) {
        rv = mURI->GetPath(getter_Copies(path));
        if (NS_FAILED(rv)) return rv;
    }

    mRequestHead.SetMethod(nsHttp::GET);
    mRequestHead.SetVersion(HTTP_VERSION_1_1);
    mRequestHead.SetRequestURI(path ? path : mSpec);

    return mTransaction->SetupRequest(&mRequestHead, nsnull);
}

nsresult
nsHttpChannel::BuildStreamListenerProxy(nsIStreamListener **result)
{
    nsresult rv;
    nsCOMPtr<nsIStreamListenerProxy> proxy
        = do_CreateInstance(kStreamListenerProxyCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // Setup proxy back to this thread with default buffer size.
    rv = proxy->Init(this, nsnull, NS_HTTP_SEGMENT_SIZE, NS_HTTP_BUFFER_SIZE);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(proxy, result);
}

nsresult
nsHttpChannel::ProcessServerResponse()
{
    NS_PRECONDITION(mResponseHead, "null response head");

    nsresult rv = NS_OK;
    PRUint32 httpStatus = mResponseHead->Status();

    // handle different server response categories
    switch (httpStatus) {
    case 200:
    case 203:
        rv = ProcessNormal();
        break;
    case 300:
    case 301:
        rv = ProcessRedirection(httpStatus);
        break;
    case 302:
    case 303:
    case 305:
    case 307:
        // XXX doom cache entry
        rv = ProcessRedirection(httpStatus);
        break;
    case 304:
        rv = ProcessNotModified();
        break;
    case 401:
    case 407:
        // XXX doom cache entry
        rv = ProcessAuthentication(httpStatus);
        break;
    default:
        // XXX doom cache entry
        rv = ProcessNormal();
        break;
    }

    return rv;
}

nsresult
nsHttpChannel::ProcessNormal()
{
    // XXX install cache listener tee
    // XXX install stream converter(s)
    return mListener->OnStartRequest(this, mListenerContext);
}

nsresult
nsHttpChannel::ProcessNotModified()
{
    NS_NOTREACHED("not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsHttpChannel::ProcessRedirection(PRUint32 httpStatus)
{
    NS_NOTREACHED("not implemented");
    return ProcessNormal();
}

nsresult
nsHttpChannel::ProcessAuthentication(PRUint32 httpStatus)
{
    NS_NOTREACHED("not implemented");
    return ProcessNormal();
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_ISUPPORTS5(nsHttpChannel,
                   nsIRequest,
                   nsIChannel,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIHttpChannel)

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIRequest
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::GetName(PRUnichar **aName)
{
    NS_ENSURE_ARG_POINTER(aName);
    *aName = ToNewUnicode(NS_ConvertASCIItoUCS2(mSpec));
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::IsPending(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = mIsPending;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetStatus(nsresult *aStatus)
{
    NS_ENSURE_ARG_POINTER(aStatus);
    *aStatus = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Cancel(nsresult status)
{
    if (mTransaction)
        mTransaction->Cancel(status);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Suspend()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::Resume()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    NS_ENSURE_ARG_POINTER(aLoadGroup);
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    NS_ENSURE_ARG_POINTER(aLoadFlags);
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIChannel
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::GetOriginalURI(nsIURI **originalURI)
{
    return GetURI(originalURI);
}
NS_IMETHODIMP
nsHttpChannel::SetOriginalURI(nsIURI *originalURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetURI(nsIURI **URI)
{
    NS_ENSURE_ARG_POINTER(URI);
    *URI = mURI;
    NS_IF_ADDREF(*URI);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetOwner(nsISupports **owner)
{
    NS_ENSURE_ARG_POINTER(owner);
    *owner = mOwner;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetOwner(nsISupports *owner)
{
    mOwner = owner;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetNotificationCallbacks(nsIInterfaceRequestor **callbacks)
{
    NS_ENSURE_ARG_POINTER(callbacks);
    *callbacks = mCallbacks;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetNotificationCallbacks(nsIInterfaceRequestor *callbacks)
{
    mCallbacks = callbacks;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetSecurityInfo(nsISupports **securityInfo)
{
    NS_ENSURE_ARG_POINTER(securityInfo);
    *securityInfo = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetContentType(char **value)
{
    NS_ENSURE_ARG_POINTER(value);

    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;

    return DupString(mResponseHead->ContentType(), value);
}
NS_IMETHODIMP
nsHttpChannel::SetContentType(const char *value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetContentLength(PRInt32 *value)
{
    NS_ENSURE_ARG_POINTER(value);

    if (!mResponseHead)
        return NS_ERROR_NOT_AVAILABLE;

    *value = mResponseHead->ContentLength();
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetContentLength(PRInt32 value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::Open(nsIInputStream **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *context)
{
    LOG(("nsHttpChannel::AsyncOpen [this=%x]\n", this));

    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    mIsPending = PR_TRUE;

    mListener = listener;
    mListenerContext = context;

    nsresult rv = Connect();
    if (NS_FAILED(rv)) {
        mListener = 0;
        mListenerContext = 0;
    }
    return rv;
}
//-----------------------------------------------------------------------------
// nsHttpChannel::nsIHttpChannel
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::GetRequestMethod(char **method)
{
    return DupString(mRequestHead.Method().get(), method);
}
NS_IMETHODIMP
nsHttpChannel::SetRequestMethod(const char *method)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    nsHttpAtom atom = nsHttp::ResolveAtom(method);
    if (!atom)
        return NS_ERROR_FAILURE;

    mRequestHead.SetMethod(atom);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetReferrer(nsIURI **referrer)
{
    NS_ENSURE_ARG_POINTER(referrer);
    *referrer = mReferrer;
    NS_ADDREF(*referrer);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetReferrer(nsIURI *referrer)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    if (nsHttpHandler::get()->BrowseAnonymously())
        return NS_OK;

    // save a copy of the referrer so we can return it if requested
    mReferrer = referrer;

    nsXPIDLCString spec;
    referrer->GetSpec(getter_Copies(spec));
    if (spec) {
        nsCAutoString ref(spec.get());
        // strip away any prehost; we don't want to be giving out passwords ;-)
        nsXPIDLCString prehost;
        referrer->GetPreHost(getter_Copies(prehost));
        if (prehost && *prehost) {
            PRUint32 prehostLoc = PRUint32(ref.Find(prehost, PR_TRUE));
            ref.Cut(prehostLoc, nsCharTraits<char>::length(prehost) + 1); // + 1 for @
        }
        mRequestHead.SetHeader(nsHttp::Referer, ref);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestHeader(const char *header, char **value)
{
    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;

    return mRequestHead.GetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::SetRequestHeader(const char *header, const char *value)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;

    return mRequestHead.SetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *visitor)
{
    return mRequestHead.Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatus(PRUint32 *value)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    NS_ENSURE_ARG_POINTER(value);
    *value = mResponseHead->Status();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatusText(char **value)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    return DupString(mResponseHead->StatusText(), value);
}

NS_IMETHODIMP
nsHttpChannel::GetResponseHeader(const char *header, char **value)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;
    return mResponseHead->GetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::SetResponseHeader(const char *header, const char *value)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom)
        return NS_ERROR_NOT_AVAILABLE;
    return mResponseHead->SetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::VisitResponseHeaders(nsIHttpHeaderVisitor *visitor)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    return mResponseHead->Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
nsHttpChannel::GetCharset(char **value)
{
    NS_ENSURE_TRUE(mResponseHead, NS_ERROR_NOT_AVAILABLE);
    return DupString(mResponseHead->ContentCharset(), value);
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIProxy
//-----------------------------------------------------------------------------

/*
NS_IMETHODIMP
nsHttpChannel::GetProxyHost(char **aProxyHost)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetProxyHost(const char *aProxyHost)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetProxyPort(PRInt32 *aProxyPort)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetProxyPort(PRInt32 aProxyPort)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetProxyType(char **aProxyType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetProxyType(const char *aProxyType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
*/

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIRequestObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    LOG(("nsHttpChannel::OnStartRequest [this=%x]\n", this));

    NS_PRECONDITION(mListener, "null listener");
    NS_PRECONDITION(mTransaction, "null transaction");

    // All of the response headers have been acquired, so we can take ownership
    // of them from the transaction.
    mResponseHead = mTransaction->TakeResponseHead();

    return ProcessServerResponse();
}

NS_IMETHODIMP
nsHttpChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
    LOG(("nsHttpChannel::OnStopRequest [this=%x status=%x]\n",
        this, status));

    NS_PRECONDITION(mListener, "null listener");
    NS_PRECONDITION(mTransaction, "null transaction");

    mIsPending = PR_FALSE;
    mStatus = status;

    // at this point, we're done with the transaction
    NS_RELEASE(mTransaction);
    mTransaction = nsnull;

    mListener->OnStopRequest(this, mListenerContext, status);

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIStreamListener
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *input,
                               PRUint32 offset, PRUint32 count)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NULL_POINTER);
    LOG(("nsHttpChannel::OnDataAvailable [this=%x offset=%u count=%u]\n",
        this, offset, count));
    return mListener->OnDataAvailable(this, mListenerContext, input, offset, count);
}
