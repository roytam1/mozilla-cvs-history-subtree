#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpChannel.h"
#include "nsHttpConnection.h"
#include "nsHttpResponseHead.h"
#include "nsIURL.h"
#include "nsICacheService.h"
#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsPrintfCString.h"
#include "nsCOMPtr.h"
#include "nsNetCID.h"
#include "prprf.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <sys/utsname.h>
#endif

#if defined(XP_PC) && !defined(XP_OS2)
#include <windows.h>
#endif

static const char NETWORK_PREFS[] = "network.";
static const char INTL_ACCEPT_LANGUAGES[] = "intl.accept_languages";
static const char INTL_ACCEPT_CHARSET[] = "intl.charset.default";

static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kProtocolProxyServiceCID, NS_PROTOCOLPROXYSERVICE_CID);
static NS_DEFINE_CID(kPrefServiceCID, NS_PREF_CID);
static NS_DEFINE_CID(kCategoryManagerCID, NS_CATEGORYMANAGER_CID);

#define UA_PREF_PREFIX "general.useragent."
#define UA_APPNAME "Mozilla"
#define UA_APPVERSION "5.0"
#define UA_APPSECURITY_FALLBACK "N"

//-----------------------------------------------------------------------------
// nsHttpHandler
//-----------------------------------------------------------------------------

nsHttpHandler *nsHttpHandler::mGlobalInstance = 0;

nsHttpHandler::nsHttpHandler()
{
    NS_INIT_ISUPPORTS();

#if defined(PR_LOGGING)
    gHttpLog = PR_NewLogModule("nsHttp");
#endif

    LOG(("Creating nsHttpHandler [this=%x].\n", this));

    NS_ASSERTION(!mGlobalInstance, "HTTP handler already created!");
    mGlobalInstance = this;

    PR_INIT_CLIST(&mIdleConnections);
}

nsHttpHandler::~nsHttpHandler()
{
    LOG(("Deleting nsHttpHandler [this=%x]\n", this));

    nsHttp::DestroyAtomTable();

    if (mPrefs) {
        mPrefs->UnregisterCallback(NETWORK_PREFS, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(INTL_ACCEPT_LANGUAGES, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "override", 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(INTL_ACCEPT_CHARSET, 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "locale", 
                nsHttpHandler::PrefsCallback, (void*)this);
        mPrefs->UnregisterCallback(UA_PREF_PREFIX "misc",
                nsHttpHandler::PrefsCallback, (void *)this);
    }

    mGlobalInstance = nsnull;
}

NS_METHOD
nsHttpHandler::Create(nsISupports *outer, REFNSIID iid, void **result)
{
    nsresult rv;
    if (outer)
        return NS_ERROR_NO_AGGREGATION;
    nsHttpHandler *handler = get();
    if (!handler) {
        // create the one any only instance of nsHttpHandler
        NS_NEWXPCOM(handler, nsHttpHandler);
        if (!handler)
            return NS_ERROR_OUT_OF_MEMORY;
        rv = handler->Init();
        if (NS_FAILED(rv)) {
            NS_DELETEXPCOM(handler);
            return rv;
        }
    }
    NS_ADDREF(handler);
    rv = handler->QueryInterface(iid, result);
    NS_RELEASE(handler);
    return rv;
}

nsresult
nsHttpHandler::Init()
{
    nsresult rv = NS_OK;

    /*
    mProxySvc = do_GetService(kProtocolProxyServiceCID, &rv);
    if (NS_FAILED(rv))
        LOG(("protocol proxy service not available"));
    */

    mPrefs = do_GetService(kPrefServiceCID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without prefs service");
        return rv;
    }

    InitUserAgentComponents();

    mPrefs->RegisterCallback(NETWORK_PREFS, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(INTL_ACCEPT_LANGUAGES, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "override", 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(INTL_ACCEPT_CHARSET, 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "locale", 
            nsHttpHandler::PrefsCallback, (void*)this);
    mPrefs->RegisterCallback(UA_PREF_PREFIX "misc",
            nsHttpHandler::PrefsCallback, (void *)this);

    PrefsChanged();

    //mSessionStartTime = NowInSeconds();


    /*
    rv = NS_NewISupportsArray(getter_AddRefs(mConnections));
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewISupportsArray(getter_AddRefs(mPendingChannelList));
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewISupportsArray(getter_AddRefs(mTransportList));
    if (NS_FAILED(rv)) return rv;
    
    // At some later stage we could merge this with the transport
    // list and add a field to each transport to determine its 
    // state. 
    rv = NS_NewISupportsArray(getter_AddRefs(mIdleTransports));
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewISupportsArray(getter_AddRefs(mPipelinedRequests));
    if (NS_FAILED(rv)) return rv;
    */

    // Startup the http category
    // Bring alive the objects in the http-protocol-startup category
    //CreateServicesFromCategory(NS_HTTP_STARTUP_CATEGORY);
    
    nsCOMPtr<nsIObserverService> observerSvc =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (observerSvc)
        observerSvc->AddObserver(this, NS_LITERAL_STRING("profile-before-change").get());

    return NS_OK;
}

nsresult
nsHttpHandler::AddStandardRequestHeaders(nsHttpHeaderArray *request,
                                         PRUint32 caps)
{
    nsresult rv;

    LOG(("nsHttpHandler::AddStandardRequestHeaders\n"));

    // Add the User-Agent header:
    rv = request->SetHeader(nsHttp::User_Agent, nsLocalCString(UserAgent()));
    if (NS_FAILED(rv)) return rv;

    // Add the Accept header:
    //
    // Send */*. We're no longer chopping MIME-types for acceptance.
    // MIME based content negotiation has died.
    rv = request->SetHeader(nsHttp::Accept, NS_LITERAL_CSTRING("*/*"));
    if (NS_FAILED(rv)) return rv;

    // Add the Accept-Language header:
    rv = request->SetHeader(nsHttp::Accept_Language, mAcceptLanguages);
    if (NS_FAILED(rv)) return rv;

    // Add the Accept-Encoding header:
    rv = request->SetHeader(nsHttp::Accept_Encoding, mAcceptEncodings);
    if (NS_FAILED(rv)) return rv;

    // Add the Accept-Charset header:
    rv = request->SetHeader(nsHttp::Accept_Charset, mAcceptCharsets);
    if (NS_FAILED(rv)) return rv;

    // Add the Connection header:
    const char *connectionType = "close";
    if (caps && ALLOW_KEEPALIVE) {
        rv = request->SetHeader(nsHttp::Keep_Alive,
                nsPrintfCString("%d", mKeepAliveTimeout));
        if (NS_FAILED(rv)) return rv;
        
        connectionType = "keep-alive";
    }
    return request->SetHeader(nsHttp::Connection, nsLocalCString(connectionType));
}

nsresult
nsHttpHandler::InitiateTransaction(nsHttpTransaction *transaction,
                                   nsHttpConnectionInfo *connectionInfo)
{
    LOG(("nsHttpHandler::InitiateTransaction\n"));

    NS_ENSURE_ARG_POINTER(transaction);
    NS_ENSURE_ARG_POINTER(connectionInfo);

    /*
    if (mNumActiveConnections == PRUint32(mMaxConnections))
        // unable to perform this transaction at this time
        return EnqueueTransaction(transaction, connectionInfo);
    */

    nsHttpConnection *conn = nsnull;

    // search the idle connection list
    PRCList *node = PR_LIST_HEAD(&mIdleConnections);
    while (node != &mIdleConnections) {
        conn = (nsHttpConnection *) node; 
        node = PR_NEXT_LINK(node);

        if (conn->ConnectionInfo()->Equals(connectionInfo)) {
            // found a matching connection; remove from list
            PR_REMOVE_LINK(conn);

            if (conn->CanReuse()) {
                LOG(("reusing connection [conn=%x]\n", conn));
                break;
            }
            else {
                LOG(("dropping stale connection: [conn=%x]\n", conn));
                NS_RELEASE(conn);
                conn = nsnull;
            }
        }
    }

    if (!conn) {
        // XXX need to make sure we aren't exceeding max-connections

        LOG(("creating new connection...\n"));
        NS_NEWXPCOM(conn, nsHttpConnection);
        if (!conn)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(conn);
    }

    nsresult rv = conn->Init(connectionInfo);
    if (NS_FAILED(rv)) goto failed;

    rv = conn->SetTransaction(transaction);
    if (NS_FAILED(rv)) goto failed;

    return NS_OK;

failed:
    NS_RELEASE(conn);
    return rv;
}

nsresult
nsHttpHandler::RecycleConnection(nsHttpConnection *connection)
{
    NS_ENSURE_ARG_POINTER(connection);

    if (connection->CanReuse()) {
        PR_INSERT_AFTER(connection, &mIdleConnections);
        mNumIdleConnections++;
    }

    /*
    mNumActiveConnections--;
    if (!PR_CLIST_IS_EMPTY(&mTransactionQ))
        ProcessTransactionQ();
    */

    return NS_OK;
}

const char *
nsHttpHandler::UserAgent()
{
    if (mUserAgentIsDirty) {
        BuildUserAgent();
        mUserAgentIsDirty = PR_FALSE;
    }
    return mUserAgent.get();
}

void
nsHttpHandler::ProcessTransactionQ()
{
    /*
    LOG(("nsHttpHandler::ProcessTransactionQ\n"));
    nsPendingTransaction *pt = nsnull;
    PRCList *node = PR_LIST_HEAD(&mTransactionQ);
    while ((node != &mTransactionQ) && (mNumActiveConnections < mMaxConnections)) {
        pt = (nsPendingTransaction *) node;
        node = PR_NEXT_LINK(node);
        nsresult rv = InitiateTransaction(pt->transaction, pt->connectionInfo);
        if (NS_FAILED(rv)) break;
        PR_REMOVE_LINK(pt);
        NS_RELEASE(pt->transaction);
        NS_RELEASE(pt->connectionInfo);
        delete pt;
    }
    */
}

nsresult
nsHttpHandler::EnqueueTransaction(nsHttpTransaction *transaction,
                                  nsHttpConnectionInfo *connectionInfo)
{
    /*
    nsPendingTransaction pt = new nsPendingTransaction;
    if (!pt)
        return NS_ERROR_OUT_OF_MEMORY;

    PR_INIT_CLIST(pt);

    pt->transaction = transaction;
    pt->connectionInfo = connectionInfo;

    NS_ADDREF(pt->transaction);
    NS_ADDREF(pt->connectionInfo);

    PR_APPEND_LINK(pt, &mPendingTransactionQ);
    return NS_OK;
    */
    return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsHttpHandler::BuildUserAgent()
{
    LOG(("nsHttpHandler::BuildUserAgent\n"));

    NS_ASSERTION(mAppName &&
                 mAppVersion &&
                 mPlatform &&
                 mSecurity &&
                 mOscpu,
                 "HTTP cannot send practical requests without this much");

    // Application portion
    mUserAgent.Assign(mAppName);
    mUserAgent += '/';
    mUserAgent += mAppVersion;
    mUserAgent += ' ';

    // Application comment
    mUserAgent += '(';
    mUserAgent += mPlatform;
    mUserAgent += "; ";
    mUserAgent += mSecurity;
    mUserAgent += "; ";
    mUserAgent += mOscpu;
    if (mLanguage) {
        mUserAgent += "; ";
        mUserAgent += mLanguage;
    }
    if (mMisc) {
        mUserAgent += "; ";
        mUserAgent += mMisc;
    }
    mUserAgent += ')';

    // Product portion
    if (mProduct) {
        mUserAgent += ' ';
        mUserAgent += mProduct;
        if (mProductSub) {
            mUserAgent += '/';
            mUserAgent += mProductSub;
        }
        if (mProductComment) {
            mUserAgent += " (";
            mUserAgent += mProductComment;
            mUserAgent += ')';
        }
    }

    // Vendor portion
    if (mVendor) {
        mUserAgent += ' ';
        mUserAgent += mVendor;
        if (mVendorSub) {
            mUserAgent += '/';
            mUserAgent += mVendorSub;
        }
        if (mVendorComment) {
            mUserAgent += " (";
            mUserAgent += mVendorComment;
            mUserAgent += ')';
        }
    }
}

void
nsHttpHandler::InitUserAgentComponents()
{
    // User-specified override
    mPrefs->CopyCharPref(UA_PREF_PREFIX "override",
        getter_Copies(mUserAgentOverride));

    // Gather vendor values.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendor",
        getter_Copies(mVendor));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendorSub",
        getter_Copies(mVendorSub));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "vendorComment",
        getter_Copies(mVendorComment));

    // Gather product values.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "product",
        getter_Copies(mProduct));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "productSub",
        getter_Copies(mProductSub));

    mPrefs->CopyCharPref(UA_PREF_PREFIX "productComment",
        getter_Copies(mProductComment));

    // Gather misc value.
    mPrefs->CopyCharPref(UA_PREF_PREFIX "misc",
        getter_Copies(mMisc));

    // Gather Application name and Version.
    mAppName = UA_APPNAME;
    mAppVersion = UA_APPVERSION;

    // Get Security level supported
    mPrefs->CopyCharPref(UA_PREF_PREFIX "security",
        getter_Copies(mSecurity));
    if (!mSecurity)
        mSecurity = UA_APPSECURITY_FALLBACK;

    // Gather locale.
    nsXPIDLString uval;
    mPrefs->GetLocalizedUnicharPref(UA_PREF_PREFIX "locale", 
        getter_Copies(uval));
    if (uval)
        mLanguage = NS_ConvertUCS2toUTF8(uval).get();

    // Gather platform.
#if defined(XP_OS2)
    mPlatform = "OS/2";
#elif defined(XP_PC)
    mPlatform = "Windows";
#elif defined(RHAPSODY)
    mPlatform = "Macintosh";
#elif defined (XP_UNIX)
    mPlatform = "X11";
#elif defined(XP_BEOS)
    mPlatform = "BeOS";
#elif defined(XP_MAC)
    mPlatform = "Macintosh";
#endif

    // Gather OS/CPU.
#if defined(XP_OS2)
    ULONG os2ver = 0;
    DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR,
                    &os2ver, sizeof(os2ver));
    if (os2ver == 11)
        mOscpu = "2.11";
    else if (os2ver == 30)
        mOscpu = "Warp 3";
    else if (os2ver == 40)
        mOscpu = "Warp 4";
    else if (os2ver == 45)
        mOscpu = "Warp 4.5";

#elif defined(XP_PC)
    OSVERSIONINFO info = { sizeof OSVERSIONINFO };
    if (GetVersionEx(&info)) {
        if (info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            if (info.dwMajorVersion      == 3)
                mAppOSCPU = "WinNT3.51";
            else if (info.dwMajorVersion == 4)
                mAppOSCPU = "WinNT4.0";
            else if (info.dwMajorVersion >= 5) {
                char *buf = PR_smprintf("Windows NT %ld.%ld",
                                        info.dwMajorVersion,
                                        info.dwMinorVersion);
                if (buf) {
                    mOscpu = buf;
                    PR_smprintf_free(buf);
                }
            }
            else
                mOscpu = "WinNT";
        } else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            if (info.dwMinorVersion == 90)
                mOscpu = "Win 9x 4.90";
            else if (info.dwMinorVersion > 0)
                mOscpu = "Win98";
            else
                mOscpu = "Win95";
        }
    }
#elif defined (XP_UNIX) || defined (XP_BEOS)
    struct utsname name;
    
    int ret = uname(&name);
    if (ret >= 0) {
        nsCString buf;  
        buf =  (char*)name.sysname;
        buf += ' ';
        buf += (char*)name.release;
        buf += ' ';
        buf += (char*)name.machine;
        mOscpu = buf;
    }
#elif defined (XP_MAC)
    mOscpu = "PPC";
#endif

    mUserAgentIsDirty = PR_TRUE;
}

void
nsHttpHandler::PrefsChanged(const char *pref)
{
    PRBool bChangedAll = pref ? PR_FALSE : PR_TRUE;

    if (!mPrefs) {
        NS_NOTREACHED("No preference service available!");
        return;
    }

    nsresult rv = NS_OK;

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.timeout") == 0)
        mPrefs->GetIntPref("network.http.keep-alive.timeout", &mKeepAliveTimeout);

    if (bChangedAll || PL_strcmp(pref, "network.http.max-connections") == 0)
        mPrefs->GetIntPref("network.http.max-connections", &mMaxConnections);

    if (bChangedAll || PL_strcmp(pref, "network.http.browse-anonymously") == 0)
        mPrefs->GetIntPref("network.http.browse-anonymously", &mBrowseAnonymously);

    if (bChangedAll || PL_strcmp(pref, "network.http.version") == 0) {
        nsXPIDLCString httpVersion;
        mPrefs->CopyCharPref("network.http.version", getter_Copies(httpVersion));
	
        if (httpVersion) {
            if (PL_strcmp(httpVersion, "1.1") == 0)
                mHttpVersion = HTTP_VERSION_1_1;
            else if (PL_strcmp(httpVersion, "0.9") == 0)
                mHttpVersion = HTTP_VERSION_0_9;
            else
                mHttpVersion = HTTP_VERSION_1_0;
        }

        if (mHttpVersion == HTTP_VERSION_1_1) {
            mCapabilities = ALLOW_KEEPALIVE;
            mProxyCapabilities = ALLOW_KEEPALIVE;
        }
        else {
            mCapabilities = 0;
            mProxyCapabilities = 0;
        }
    }

    PRBool cVar = PR_FALSE;

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive") == 0) {
        rv = mPrefs->GetBoolPref("network.http.keep-alive", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |= ALLOW_KEEPALIVE;
            else
                mCapabilities &= ~ALLOW_KEEPALIVE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.keep-alive") == 0) {
        rv = mPrefs->GetBoolPref("network.http.proxy.keep-alive", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |= ALLOW_KEEPALIVE;
            else
                mProxyCapabilities &= ~ALLOW_KEEPALIVE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.pipelining") == 0) {
        rv = mPrefs->GetBoolPref("network.http.pipelining", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |=  ALLOW_PIPELINING;
            else
                mCapabilities &= ~ALLOW_PIPELINING;
        }
    }

    /*
    mPipelineFirstRequest = PR_FALSE;
    rv = mPrefs->GetBoolPref("network.http.pipelining.firstrequest", &mPipelineFirstRequest);

    mPipelineMaxRequests  = DEFAULT_PIPELINE_MAX_REQUESTS;
    rv = mPrefs->GetIntPref("network.http.pipelining.maxrequests", &mPipelineMaxRequests );
    */

    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.pipelining") == 0) {
        rv = mPrefs->GetBoolPref("network.http.proxy.pipelining", &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |=  ALLOW_PIPELINING;
            else
                mProxyCapabilities &= ~ALLOW_PIPELINING;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.proxy.ssl.connect") == 0)
        mPrefs->GetBoolPref("network.http.proxy.ssl.connect", &mProxySSLConnectAllowed);

    if (bChangedAll || PL_strcmp(pref, "network.http.connect.timeout") == 0)
        mPrefs->GetIntPref("network.http.connect.timeout", &mConnectTimeout);

    if (bChangedAll || PL_strcmp(pref, "network.http.request.timeout") == 0)
        mPrefs->GetIntPref("network.http.request.timeout", &mRequestTimeout);

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.max-connections") == 0)
        mPrefs->GetIntPref("network.http.keep-alive.max-connections", &mMaxAllowedKeepAlives);

    if (bChangedAll || PL_strcmp(pref, "network.http.keep-alive.max-connections-per-server") == 0)
        mPrefs->GetIntPref("network.http.keep-alive.max-connections-per-server", &mMaxAllowedKeepAlivesPerServer);

    if (bChangedAll || PL_strcmp(pref, INTL_ACCEPT_LANGUAGES) == 0) {
        nsXPIDLString acceptLanguages;
        rv = mPrefs->GetLocalizedUnicharPref(INTL_ACCEPT_LANGUAGES, 
                getter_Copies(acceptLanguages));
        if (NS_SUCCEEDED(rv))
            SetAcceptLanguages(NS_ConvertUCS2toUTF8(acceptLanguages));
    }

    if (bChangedAll || PL_strcmp(pref, INTL_ACCEPT_CHARSET) == 0) {
        nsXPIDLString acceptCharset;
        rv = mPrefs->GetLocalizedUnicharPref(INTL_ACCEPT_CHARSET, 
                getter_Copies(acceptCharset));
        if (NS_SUCCEEDED(rv))
            SetAcceptCharsets(NS_ConvertUCS2toUTF8(acceptCharset));
    }

    // general.useragent.override
    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "override") == 0) {
        nsXPIDLCString uval;
        rv = mPrefs->CopyCharPref(UA_PREF_PREFIX "override",
                                  getter_Copies(uval));
        if (NS_SUCCEEDED(rv)) {
            mUserAgentOverride = uval.get();
            mUserAgentIsDirty = PR_TRUE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "locale") == 0) {
        // 55156: re-Gather locale.
        nsXPIDLString uval;
        rv = mPrefs->GetLocalizedUnicharPref(UA_PREF_PREFIX "locale", 
                                             getter_Copies(uval));
        if (NS_SUCCEEDED(rv)) {
            mLanguage = NS_ConvertUCS2toUTF8(uval).get();
            mUserAgentIsDirty = PR_TRUE;
        }
    }

    // general.useragent.misc
    if (bChangedAll || PL_strcmp(pref, UA_PREF_PREFIX "misc") == 0) {
        nsXPIDLCString uval;
        rv = mPrefs->CopyCharPref(UA_PREF_PREFIX "misc",
                                  getter_Copies(uval));
        if (NS_SUCCEEDED(rv)) {
            mMisc = uval.get();
            mUserAgentIsDirty = PR_TRUE;
        }
    }

    if (bChangedAll || PL_strcmp(pref, "network.http.accept-encoding") == 0) {
        nsXPIDLCString acceptEncodings;
        rv = mPrefs->CopyCharPref("network.http.accept-encoding",
                                  getter_Copies(acceptEncodings));
        if (NS_SUCCEEDED(rv))
            SetAcceptEncodings(nsLocalCString(acceptEncodings));
    }
}

PRInt32 PR_CALLBACK
nsHttpHandler::PrefsCallback(const char *pref, void *self)
{
    nsHttpHandler *handler = (nsHttpHandler *) self;
    NS_ASSERTION(handler, "bad instance data");
    if (handler)
        handler->PrefsChanged(pref);
    return 0;
}

/*
 * CreateServicesFromCategory()
 *
 * Given a category, this convenience functions enumerates the category and 
 * creates a service of every CID or ContractID registered under the category
 *
 * @category: Input category
 * @return: returns error if any CID or ContractID registered failed to create.
 */
nsresult
nsHttpHandler::CreateServicesFromCategory(const char *category)
{
    nsresult rv = NS_OK;

    int nFailed = 0; 
    nsCOMPtr<nsICategoryManager> categoryManager = 
        do_GetService("@mozilla.org/categorymanager;1", &rv);
    if (!categoryManager) return rv;

    nsCOMPtr<nsISimpleEnumerator> enumerator;
    rv = categoryManager->EnumerateCategory(category, 
            getter_AddRefs(enumerator));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISupports> entry;
    while (NS_SUCCEEDED(enumerator->GetNext(getter_AddRefs(entry)))) {
        // From here on just skip any error we get.
        nsCOMPtr<nsISupportsString> catEntry = do_QueryInterface(entry, &rv);
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
        nsXPIDLCString entryString;
        rv = catEntry->GetData(getter_Copies(entryString));
        if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }
		nsXPIDLCString contractID;
		rv = categoryManager->GetCategoryEntry(category,(const char *)entryString, getter_Copies(contractID));
		if (NS_FAILED(rv)) {
            nFailed++;
            continue;
        }

#ifdef DEBUG_HTTP_STARTUP_CATEGORY
        printf("HttpHandler: Instantiating contractid %s \
                in http startup category.\n", (const char *)contractID);
#endif
        nsCOMPtr<nsISupports> instance = do_GetService(contractID, &rv);
        if (NS_FAILED(rv))
            nFailed++;

        // try an observer, if it implements it.
        nsCOMPtr<nsIObserver> observer = do_QueryInterface(instance, &rv);
        if (NS_SUCCEEDED(rv) && observer)
            observer->Observe(NS_STATIC_CAST(nsISupports*,NS_STATIC_CAST(void*,this)),
                              NS_HTTP_STARTUP_TOPIC,
                              NS_LITERAL_STRING("").get());
    }
    return (nFailed ? NS_ERROR_FAILURE : NS_OK);
}

/**
 *  Allocates a C string into that contains a ISO 639 language list
 *  notated with HTTP "q" values for output with a HTTP Accept-Language
 *  header. Previous q values will be stripped because the order of
 *  the langs imply the q value. The q values are calculated by dividing
 *  1.0 amongst the number of languages present.
 *
 *  Ex: passing: "en, ja"
 *      returns: "en, ja; q=0.500"
 *
 *      passing: "en, ja, fr_CA"
 *      returns: "en, ja; q=0.667, fr_CA; q=0.333"
 */

static nsresult
PrepareAcceptLanguages(const nsACString &i_AcceptLanguages,
                             nsACString &o_AcceptLanguages)
{
    if (i_AcceptLanguages.IsEmpty())
        return NS_OK;

    PRUint32 n, size, wrote;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *comma;
    PRInt32 available;


    o_Accept = nsCRT::strdup(PromiseFlatCString(i_AcceptLanguages).get());
    if (nsnull == o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if ((char *) 0 == q_Accept)
        return nsnull;
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p))
    {
        while (*token == ' ' || *token == '\x9') token++;
        char* trim;
        trim = PL_strpbrk(token, "; \x9");
        if (trim != (char*)0)  // remove "; q=..." if present
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? ", " : ""; // delimiter if not first item
            if (q < 0.9995)
                wrote = PR_snprintf(p2, available, "%s%s; q=%1.3f", comma, token, q);
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    nsCRT::free(o_Accept);

    o_AcceptLanguages.Assign((const char *) q_Accept);
    delete [] q_Accept;

    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptLanguages(const nsACString &aAcceptLanguages) 
{
    nsCString buf;
    nsresult rv = PrepareAcceptLanguages(aAcceptLanguages, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptLanguages.Assign(buf.get());
    return rv;
}

/**
 *  Allocates a C string into that contains a character set/encoding list
 *  notated with HTTP "q" values for output with a HTTP Accept-Charset
 *  header. If the UTF-8 character set is not present, it will be added.
 *  If a wildcard catch-all is not present, it will be added. If more than
 *  one charset is set (as of 2001-02-07, only one is used), they will be
 *  comma delimited and with q values set for each charset in decending order.
 *
 *  Ex: passing: "euc-jp"
 *      returns: "euc-jp, utf-8; q=0.667, *; q=0.667"
 *
 *      passing: "UTF-8"
 *      returns: "UTF-8, *"
 */

static nsresult
PrepareAcceptCharsets(const nsACString &i_AcceptCharset,
                            nsACString &o_AcceptCharset)
{
    PRUint32 n, size, wrote;
    PRInt32 available;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *acceptable, *comma;
    PRBool add_utf = PR_FALSE;
    PRBool add_asterick = PR_FALSE;

    if (i_AcceptCharset.IsEmpty())
        acceptable = "";
    else
        acceptable = PromiseFlatCString(i_AcceptCharset).get();
    o_Accept = nsCRT::strdup(acceptable);
    if (nsnull == o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    // only add "utf-8" and "*" to the list if they aren't
    // already specified.

    if (PL_strcasestr(acceptable, "utf-8") == NULL) {
        n++;
        add_utf = PR_TRUE;
    }
    if (PL_strstr(acceptable, "*") == NULL) {
        n++;
        add_asterick = PR_TRUE;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if ((char *) 0 == q_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p)) {
        while (*token == ' ' || *token == '\x9') token++;
        char* trim;
        trim = PL_strpbrk(token, "; \x9");
        if (trim != (char*)0)  // remove "; q=..." if present
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? ", " : ""; // delimiter if not first item
            if (q < 0.9995)
                wrote = PR_snprintf(p2, available, "%s%s; q=%1.3f", comma, token, q);
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    if (add_utf) {
        comma = n++ != 0 ? ", " : ""; // delimiter if not first item
        if (q < 0.9995)
            wrote = PR_snprintf(p2, available, "%sutf-8; q=%1.3f", comma, q);
        else
            wrote = PR_snprintf(p2, available, "%sutf-8", comma);
        q -= dec;
        p2 += wrote;
        available -= wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    if (add_asterick) {
        comma = n++ != 0 ? ", " : ""; // delimiter if not first item

        // keep q of "*" equal to the lowest q value
        // in the event of a tie between the q of "*" and a non-wildcard
        // the non-wildcard always receives preference.

        q += dec;
        if (q < 0.9995) {
            wrote = PR_snprintf(p2, available, "%s*; q=%1.3f", comma, q);
        }
        else
            wrote = PR_snprintf(p2, available, "%s*", comma);
        available -= wrote;
        p2 += wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    nsCRT::free(o_Accept);

    // change alloc from C++ new/delete to nsCRT::strdup's way
    o_AcceptCharset.Assign((const char *) q_Accept);
#if defined DEBUG_havill
    printf("Accept-Charset: %s\n", q_Accept);
#endif
    delete [] q_Accept;
    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptCharsets(const nsACString &aAcceptCharsets) 
{
    nsCString buf;
    nsresult rv = PrepareAcceptCharsets(aAcceptCharsets, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptCharsets.Assign(buf.get());
    return rv;
}

nsresult
nsHttpHandler::SetAcceptEncodings(const nsACString &aAcceptEncodings) 
{
    mAcceptEncodings = aAcceptEncodings;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS3(nsHttpHandler,
                              nsIHttpProtocolHandler,
                              nsIProtocolHandler,
                              nsIObserver)

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIProtocolHandler
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpHandler::GetScheme(char **aScheme)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpHandler::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **aURI)
{
    nsresult rv = NS_OK;

    LOG(("nsHttpHandler::NewURI\n"));

    nsCOMPtr<nsIStandardURL> url = do_CreateInstance(kStandardURLCID, &rv);
    if (NS_FAILED(rv)) return rv;

    // XXX need to choose the default port based on the scheme
    rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY, 80, aSpec, aBaseURI);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, aURI);
}

NS_IMETHODIMP
nsHttpHandler::NewChannel(nsIURI *aURI, nsIChannel **aChannel)
{
    LOG(("nsHttpHandler::NewChannel\n"));

    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aChannel);

    nsHttpChannel *httpChannel = nsnull;
    PRBool isHttp = PR_FALSE, isHttps = PR_FALSE;

    // Verify that we have been given a valid scheme
    nsresult rv = aURI->SchemeIs("http", &isHttp);
    if (NS_FAILED(rv)) return rv;
    if (!isHttp) {
        rv = aURI->SchemeIs("https", &isHttps);
        if (NS_FAILED(rv)) return rv;
        if (!isHttps) {
            NS_WARNING("Invalid URI scheme");
            return NS_ERROR_UNEXPECTED;
        }
    }

    NS_NEWXPCOM(httpChannel, nsHttpChannel);
    if (!httpChannel) return NS_ERROR_OUT_OF_MEMORY;

    rv = httpChannel->Init(aURI, mCapabilities);
    if (NS_FAILED(rv)) goto failed;

    /* XXX implement NewProxyChannel
    if (mProxySvc) {
        PRBool checkForProxy = PR_FALSE;
        rv = mProxySvc->GetProxyEnabled(&checkForProxy);
        if (NS_FAILED(rv)) goto failed;
        if (checkForProxy) {
            rv = mProxySvc->ExamineForProxy(aURI, httpChannel);
            if (NS_FAILED(rv)) goto failed;
        }
    }
    */
    return CallQueryInterface((nsIHttpChannel *) httpChannel, aChannel);

failed:
    delete httpChannel;
    return rv;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIHttpProtocolHandler
//-----------------------------------------------------------------------------
 
NS_IMETHODIMP
nsHttpHandler::NewProxyChannel(nsIURI *uri,
                               const char *proxyHost,
                               PRInt32 proxyPort,
                               const char *proxyType,
                               nsIChannel **)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpHandler::GetUserAgent(char **aUserAgent)
{
    return DupString(UserAgent(), aUserAgent);
}

NS_IMETHODIMP
nsHttpHandler::GetAppName(char **aAppName)
{
    return DupString(mAppName, aAppName);
}

NS_IMETHODIMP
nsHttpHandler::GetAppVersion(char **aAppVersion)
{
    return DupString(mAppVersion, aAppVersion);
}

NS_IMETHODIMP
nsHttpHandler::GetVendor(char **aVendor)
{
    return DupString(mVendor, aVendor);
}
NS_IMETHODIMP
nsHttpHandler::SetVendor(const char *aVendor)
{
    mVendor = aVendor;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorSub(char **aVendorSub)
{
    return DupString(mVendorSub, aVendorSub);
}
NS_IMETHODIMP
nsHttpHandler::SetVendorSub(const char *aVendorSub)
{
    mVendorSub = aVendorSub;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorComment(char **aVendorComment)
{
    return DupString(mVendorComment, aVendorComment);
}
NS_IMETHODIMP
nsHttpHandler::SetVendorComment(const char *aVendorComment)
{
    mVendorComment = aVendorComment;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProduct(char **aProduct)
{
    return DupString(mProduct, aProduct);
}
NS_IMETHODIMP
nsHttpHandler::SetProduct(const char *aProduct)
{
    mProduct = aProduct;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductSub(char **aProductSub)
{
    return DupString(mProductSub, aProductSub);
}
NS_IMETHODIMP
nsHttpHandler::SetProductSub(const char *aProductSub)
{
    mProductSub = aProductSub;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductComment(char **aProductComment)
{
    return DupString(mProductComment, aProductComment);
}
NS_IMETHODIMP
nsHttpHandler::SetProductComment(const char *aProductComment)
{
    mProductComment = aProductComment;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetPlatform(char **aPlatform)
{
    return DupString(mPlatform, aPlatform);
}

NS_IMETHODIMP
nsHttpHandler::GetOscpu(char **aOscpu)
{
    return DupString(mOscpu, aOscpu);
}

NS_IMETHODIMP
nsHttpHandler::GetLanguage(char **aLanguage)
{
    return DupString(mLanguage, aLanguage);
}
NS_IMETHODIMP
nsHttpHandler::SetLanguage(const char *aLanguage)
{
    mLanguage = aLanguage;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetMisc(char **aMisc)
{
    return DupString(mMisc, aMisc);
}
NS_IMETHODIMP
nsHttpHandler::SetMisc(const char *aMisc)
{
    mMisc = aMisc;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpHandler::nsIObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpHandler::Observe(nsISupports *subject,
                       const PRUnichar *topic,
                       const PRUnichar *data)
{
    return NS_OK;
}
