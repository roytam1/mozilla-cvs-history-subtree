#include "nsHttpChannel.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpHandler.h"
#include "nsHttp.h"
#include "netCore.h"
#include "nsNetCID.h"
#include "nsString2.h"
#include "nsReadableUtils.h"

static NS_DEFINE_CID(kStreamListenerProxyCID, NS_STREAMLISTENERPROXY_CID);

//-----------------------------------------------------------------------------
// nsHttpChannel
//-----------------------------------------------------------------------------

nsHttpChannel::nsHttpChannel()
    : mTransaction(0)
    , mConnectionInfo(0)
    , mLoadFlags(LOAD_NORMAL)
    , mCapabilities(0)
    , mIsPending(PR_FALSE)
{
    NS_INIT_ISUPPORTS();
}

nsHttpChannel::~nsHttpChannel()
{
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
    rv = mRequestHeaders.SetHeader(nsHttp::Host, hostLine.get());
    if (NS_FAILED(rv)) return rv;

    rv = nsHttpHandler::get()->
            AddStandardRequestHeaders(&mRequestHeaders, caps);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

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

    return mTransaction->SetRequestInfo(nsHttp::GET,
                                        HTTP_VERSION_1_1,
                                        path ? path.get() : mSpec.get(),
                                        &mRequestHeaders); 
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
    *aStatus = NS_OK;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::Cancel(nsresult status)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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
nsHttpChannel::GetOriginalURI(nsIURI **aOriginalURI)
{
    return GetURI(aOriginalURI);
}
NS_IMETHODIMP
nsHttpChannel::SetOriginalURI(nsIURI *aOriginalURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetURI(nsIURI **aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);
    *aURI = mURI;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetOwner(nsISupports **aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetOwner(nsISupports *aOwner)
{
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetSecurityInfo(nsISupports **aSecurityInfo)
{
    NS_ENSURE_ARG_POINTER(aSecurityInfo);
    *aSecurityInfo = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetContentType(char **aContentType)
{
    NS_ENSURE_ARG_POINTER(aContentType);

    if (mTransaction && mTransaction->ContentType())
        *aContentType = PL_strdup(mTransaction->ContentType());
    else
        *aContentType = nsnull;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetContentType(const char *aContentType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetContentLength(PRInt32 *aContentLength)
{
    NS_ENSURE_ARG_POINTER(aContentLength);
    if (mTransaction)
        *aContentLength = mTransaction->ContentLength();
    else
        *aContentLength = -1;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetContentLength(PRInt32 aContentLength)
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
nsHttpChannel::GetRequestMethod(char **aRequestMethod)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetRequestMethod(const char *aRequestMethod)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestURI(char **aRequestURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetRequestURI(const char *aRequestURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetReferrer(nsIURI **aReferrer)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsHttpChannel::SetReferrer(nsIURI *aReferrer)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetRequestHeader(const char *header, char **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::SetRequestHeader(const char *header, const char *value)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *visitor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatus(PRUint32 *aResponseStatus)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseStatusText(char **aResponseStatusText)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetResponseHeader(const char *header, char **_retval)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::SetResponseHeader(const char *header, const char *value)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::VisitResponseHeaders(nsIHttpHeaderVisitor *visitor)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetCharset(char **aCharset)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_NOT_AVAILABLE);
    return NS_ERROR_NOT_IMPLEMENTED;
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
    NS_ENSURE_TRUE(mListener, NS_ERROR_NULL_POINTER);
    LOG(("nsHttpChannel::OnStartRequest [this=%x]\n", this));
    return mListener->OnStartRequest(this, mListenerContext);
}

NS_IMETHODIMP
nsHttpChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
    NS_ENSURE_TRUE(mListener, NS_ERROR_NULL_POINTER);

    LOG(("nsHttpChannel::OnStopRequest [this=%x status=%x]\n",
        this, status));

    mIsPending = PR_FALSE;

    mListener->OnStopRequest(this, mListenerContext, status);

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    NS_ASSERTION(mTransaction, "what? no transaction!");
    // 
    // we need to decide what to do with the transaction's connection.  if
    // authentication is required, then we would want to explicitly reuse
    // the connection.  otherwise, the connection can just be recycled.
    //
    // XXX need to support authentication
    if (mTransaction->Connection()) {
        nsHttpHandler::get()->RecycleConnection(mTransaction->Connection());
        mTransaction->SetConnection(nsnull);
    }

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
