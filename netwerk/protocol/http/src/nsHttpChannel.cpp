/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Darin Fisher <darin@netscape.com> (original author)
 */

#include "nsHttpChannel.h"
#include "nsHttpTransaction.h"
#include "nsHttpConnection.h"
#include "nsHttpHandler.h"
#include "nsHttpAuthCache.h"
#include "nsHttpResponseHead.h"
#include "nsHttp.h"
#include "nsIHttpAuthenticator.h"
#include "nsIAuthPrompt.h"
#include "nsIStringBundle.h"
#include "nsIStreamConverterService.h"
#include "nsNetUtil.h"
#include "nsString2.h"
#include "nsReadableUtils.h"
#include "plstr.h"

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
    , mApplyConversion(PR_TRUE)
    , mTriedCredentialsFromPrehost(PR_FALSE)
    , mTriedCredentials(PR_FALSE)
{
    LOG(("Creating nsHttpChannel @%x\n", this));

    NS_INIT_ISUPPORTS();
}

nsHttpChannel::~nsHttpChannel()
{
    LOG(("Destroying nsHttpChannel @%x\n", this));

    if (mResponseHead) {
        delete mResponseHead;
        mResponseHead = 0;
    }

    NS_IF_RELEASE(mConnectionInfo);
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
    mOriginalURI = uri;
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
    NS_ADDREF(mConnectionInfo);

    // Set default request method
    mRequestHead.SetMethod(nsHttp::Get);

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

    rv = nsHttpHandler::get()->AddStandardRequestHeaders(&mRequestHead.Headers(), caps);
    if (NS_FAILED(rv)) return rv;

    // check to see if authorization headers should be included
    rv = AddAuthorizationHeaders();
    if (NS_FAILED(rv)) return rv;

    // Notify nsIHttpNotify implementations
    return nsHttpHandler::get()->OnModifyRequest(this);
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
    nsresult rv = NS_NewStreamListenerProxy(getter_AddRefs(listenerProxy),
                                            this, nsnull,
                                            NS_HTTP_SEGMENT_SIZE,
                                            NS_HTTP_BUFFER_SIZE);
    if (NS_FAILED(rv)) return rv;

    // Create the transaction object
    mTransaction = new nsHttpTransaction(listenerProxy, this);
    if (!mTransaction)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(mTransaction);

    // Use the URI path if not proxying
    nsXPIDLCString path;
    if (mConnectionInfo->ProxyHost() == nsnull) {
        rv = mURI->GetPath(getter_Copies(path));
        if (NS_FAILED(rv)) return rv;
    }

    mRequestHead.SetVersion(HTTP_VERSION_1_1);
    mRequestHead.SetRequestURI(path ? path : mSpec);

    return mTransaction->SetupRequest(&mRequestHead, mUploadStream);
}

nsresult
nsHttpChannel::ProcessResponse()
{
    nsresult rv = NS_OK;
    PRUint32 httpStatus = mResponseHead->Status();

    LOG(("nsHttpChannel::ProcessResponse [this=%x httpStatus=%u]\n",
        this, httpStatus));

    // handle different server response categories
    switch (httpStatus) {
    case 200:
    case 203:
        // XXX store response headers in cache
        // XXX install cache tee
        rv = ProcessNormal();
        break;
    case 300:
    case 301:
        // XXX store response headers in cache
        // XXX close cache entry
        rv = ProcessRedirection(httpStatus);
        if (NS_FAILED(rv))
            rv = ProcessNormal();
        break;
    case 302:
    case 303:
    case 305:
    case 307:
        // XXX doom cache entry
        rv = ProcessRedirection(httpStatus);
        if (NS_FAILED(rv))
            rv = ProcessNormal();
        break;
    case 304:
        rv = ProcessNotModified();
        break;
    case 401:
    case 407:
        rv = ProcessAuthentication(httpStatus);
        if (NS_FAILED(rv))
            rv = ProcessNormal();
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
    // install stream converter(s) if required
    if (mApplyConversion) {
        const char *val = mResponseHead->PeekHeader(nsHttp::Content_Encoding);
        if (val) {
            nsCOMPtr<nsIStreamConverterService> serv;
            nsresult rv = nsHttpHandler::get()->
                    GetStreamConverterService(getter_AddRefs(serv));
            // we won't fail to load the page just because we couldn't load the
            // stream converter service.. carry on..
            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsIStreamListener> converter;
                nsAutoString from = NS_ConvertASCIItoUCS2(val);
                rv = serv->AsyncConvertData(from.get(),
                                            NS_LITERAL_STRING("uncompressed").get(),
                                            mListener,
                                            mListenerContext,
                                            getter_AddRefs(converter));
                if (NS_SUCCEEDED(rv))
                    mListener = converter;
            }
        }
    }

    return mListener->OnStartRequest(this, mListenerContext);
}

//-----------------------------------------------------------------------------
// nsHttpChannel <cache>
//-----------------------------------------------------------------------------

nsresult
nsHttpChannel::ProcessNotModified()
{
    NS_NOTREACHED("not implemented");
    return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// nsHttpChannel <redirect>
//-----------------------------------------------------------------------------

nsresult
nsHttpChannel::ProcessRedirection(PRUint32 redirectType)
{
    LOG(("nsHttpChannel::ProcessRedirection [this=%x type=%u]\n",
        this, redirectType));

    const char *location = mResponseHead->PeekHeader(nsHttp::Location);

    // if a location header was not given, then we can't perform the redirect,
    // so just carry on as though this were a normal response.
    if (!location)
        return NS_ERROR_FAILURE;

    LOG(("redirecting to: %s\n", location));

    nsresult rv;
    nsCOMPtr<nsIChannel> newChannel;

    if (redirectType == 305) {
        // we must repeat the request via the proxy specified by location
 
        PRInt32 proxyPort;
        
        // location is of the form "host:port"
        char *p = PL_strchr(location, ':');
        if (p) {
            *p = 0;
            proxyPort = atoi(p+1);
        }
        else
            proxyPort = 80;

        // talk to the http handler directly for this case
        rv = nsHttpHandler::get()->
                NewProxyChannel(mURI, location, proxyPort, "http",
                                getter_AddRefs(newChannel));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        //
        // this redirect could be to ANY uri, so we need to talk to the
        // IO service to create the new channel.
        //
        nsCOMPtr<nsIIOService> serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;

        // create a new URI using the location header and the current URL
        // as a base...
        nsCOMPtr<nsIURI> newURI;
        rv = serv->NewURI(location, mURI, getter_AddRefs(newURI));
        if (NS_FAILED(rv)) return rv;

        // move the reference of the old location to the new one if the new
        // one has none.
        nsCOMPtr<nsIURL> newURL = do_QueryInterface(newURI, &rv);
        if (NS_SUCCEEDED(rv)) {
            nsXPIDLCString ref;
            rv = newURL->GetRef(getter_Copies(ref));
            if (NS_SUCCEEDED(rv) && !ref) {
                nsCOMPtr<nsIURL> baseURL = do_QueryInterface(mURI, &rv);
                if (NS_SUCCEEDED(rv)) {
                    baseURL->GetRef(getter_Copies(ref));
                    if (ref)
                        newURL->SetRef(ref);
                }
            }
        }

        // build the new channel
        rv = NS_OpenURI(getter_AddRefs(newChannel), newURI, serv, mLoadGroup,
                        mCallbacks, mLoadFlags | LOAD_REPLACE);
        if (NS_FAILED(rv)) return rv;
    }

    // convey the original uri
    rv = newChannel->SetOriginalURI(mOriginalURI);
    if (NS_FAILED(rv)) return rv;

    // convey the referrer if one was used for this channel to the next one
    if (mReferrer) {
        nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(newChannel);
        if (httpChannel)
            httpChannel->SetReferrer(mReferrer);
    }

    // call out to the event sink to notify it of this redirection.
    if (mHttpEventSink) {
        rv = mHttpEventSink->OnRedirect(this, newChannel);
        if (NS_FAILED(rv)) return rv;
    }
    // XXX we used to talk directly with the script security manager, but that
    // should really be handled by the event sink implementation.

    // begin loading the new channel
    rv = newChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv)) return rv;

    // close down this channel
    mTransaction->Cancel(NS_BINDING_REDIRECTED);

    // disconnect from our listener
    mListener = 0;
    mListenerContext = 0;

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChannel <auth>
//-----------------------------------------------------------------------------

nsresult
nsHttpChannel::ProcessAuthentication(PRUint32 httpStatus)
{
    LOG(("nsHttpChannel::ProcessAuthentication [this=%x code=%u]\n",
        this, httpStatus));

    const char *challenge;
    PRBool proxyAuth = (httpStatus == 407);

    if (proxyAuth)
        challenge = mResponseHead->PeekHeader(nsHttp::Proxy_Authenticate);
    else
        challenge = mResponseHead->PeekHeader(nsHttp::WWW_Authenticate);

    if (!challenge) {
        LOG(("null challenge!\n"));
        return NS_ERROR_UNEXPECTED;
    }

    LOG(("challenge=%s\n", challenge));

    nsCAutoString creds;
    nsresult rv = GetCredentials(challenge, proxyAuth, creds);
    if (NS_FAILED(rv)) return rv;

    // set the authentication credentials
    if (proxyAuth)
        mRequestHead.SetHeader(nsHttp::Proxy_Authorization, creds);
    else
        mRequestHead.SetHeader(nsHttp::Authorization, creds);

    // kill off the current transaction
    mTransaction->Cancel(NS_BINDING_REDIRECTED);
    NS_RELEASE(mTransaction);
    mTransaction = 0;

    // and create a new one...
    rv = SetupTransaction();
    if (NS_FAILED(rv)) return rv;

    rv = nsHttpHandler::get()->
            InitiateTransaction(mTransaction, mConnectionInfo);
    if (NS_FAILED(rv)) return rv;

    mTriedCredentials = PR_TRUE;
    return NS_OK;
}

nsresult
nsHttpChannel::GetCredentials(const char *challenges,
                              PRBool proxyAuth,
                              nsAFlatCString &creds)
{
    nsAutoString user, pass;
    nsresult rv;
    
    LOG(("nsHttpChannel::GetCredentials [this=%x proxyAuth=%d challenges=%s]\n",
        this, proxyAuth, challenges));

    nsHttpAuthCache *authCache = nsHttpHandler::get()->AuthCache();
    if (!authCache)
        return NS_ERROR_NOT_INITIALIZED;

    // proxy auth's never in prehost
    if (!mTriedCredentialsFromPrehost && !proxyAuth) {
        rv = GetUserPassFromURI(user, pass);
        if (NS_FAILED(rv)) return rv;
    }

    // figure out which challenge we can handle and which authenticator to use.
    nsCAutoString challenge;
    nsCOMPtr<nsIHttpAuthenticator> auth;

    rv = SelectChallenge(challenges, challenge, getter_AddRefs(auth));

    if (!auth) {
        LOG(("authentication type not supported\n"));
        return NS_ERROR_FAILURE;
    }

    nsCAutoString realm;
    rv = ParseRealm(challenge.get(), realm);
    if (NS_FAILED(rv)) return rv;

    const char *triedCreds = nsnull;
    const char *host;
    nsXPIDLCString path;
    PRInt32 port;

    if (proxyAuth) {
        host = mConnectionInfo->ProxyHost();
        port = mConnectionInfo->ProxyPort();
        triedCreds = mRequestHead.PeekHeader(nsHttp::Proxy_Authorization);
    }
    else {
        host = mConnectionInfo->Host();
        port = mConnectionInfo->Port();
        triedCreds = mRequestHead.PeekHeader(nsHttp::Authorization);

        rv = GetCurrentPath(getter_Copies(path));
        if (NS_FAILED(rv)) return rv;
    }

    //
    // if we already tried some credentials for this transaction, then
    // we need to possibly clear them from the cache, unless the credentials
    // in the cache have changed, in which case we'd want to give them a
    // try instead.
    //
    authCache->GetCredentialsForDomain(host, port, realm.get(), creds);

    if (triedCreds && !PL_strcmp(triedCreds, creds.get())) {
        // ok.. clear the credentials from the cache
        authCache->SetCredentials(host, port, nsnull, realm.get(), nsnull);
        creds.Truncate(0);
    }
    // otherwise, let's try the credentials we got from the cache

    if (!creds.IsEmpty()) {
        LOG(("using cached credentials!\n"));
        return NS_OK;
    }

    if (user.IsEmpty()) {
        // at this point we are forced to interact with the user to get their
        // username and password for this domain.
        rv = PromptForUserPass(host, port, proxyAuth, realm.get(), user, pass);
        if (NS_FAILED(rv)) return rv;
    }

    // talk to the authenticator to get credentials for this user/pass combo.
    nsXPIDLCString result;
    rv = auth->GenerateCredentials(this,
                                   challenge.get(),
                                   user.get(),
                                   pass.get(),
                                   getter_Copies(result));
    if (NS_FAILED(rv)) return rv;

    creds.Assign(result);

    // store these credentials in the cache.  we do this even though we don't
    // yet know that these credentials are valid b/c we need to avoid prompting
    // the user more than once in case the credentials are valid.
    return authCache->SetCredentials(host, port, path, realm.get(), creds.get());
}

nsresult
nsHttpChannel::SelectChallenge(const char *challenges,
                               nsAFlatCString &challenge,
                               nsIHttpAuthenticator **auth)
{
    nsCAutoString scheme;

    LOG(("nsHttpChannel::SelectChallenge [this=%x]\n", this));

    // loop over the various challenges (LF separated)...
    for (const char *eol = challenges - 1; eol; ) {
        const char *p = eol + 1;

        // get the challenge string
        if ((eol = PL_strchr(p, '\n')) != nsnull)
            challenge.Assign(p, eol - p);
        else
            challenge.Assign(p);

        // get the challenge type
        if ((p = PL_strchr(challenge.get(), ' ')) != nsnull)
            scheme.Assign(challenge.get(), p - challenge.get());
        else
            scheme.Assign(challenge);

        // normalize to lowercase
        ToLowerCase(scheme);

        if (NS_SUCCEEDED(GetAuthenticator(scheme.get(), auth)))
            return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

nsresult
nsHttpChannel::GetAuthenticator(const char *scheme, nsIHttpAuthenticator **auth)
{
    LOG(("nsHttpChannel::GetAuthenticator [this=%x scheme=%s]\n", this, scheme));

    nsCAutoString contractid;
    contractid.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractid.Append(scheme);

    nsresult rv;
    nsCOMPtr<nsIHttpAuthenticator> serv = do_GetService(contractid, &rv);
    if (NS_FAILED(rv)) return rv;

    *auth = serv;
    NS_ADDREF(*auth);
    return NS_OK;
}

nsresult
nsHttpChannel::GetUserPassFromURI(nsAString &user,
                                  nsAString &pass)
{
    // XXX should be a necko utility function 
    nsXPIDLCString prehost;
    mURI->GetPreHost(getter_Copies(prehost));
    if (prehost) {
        nsresult rv;

        nsCOMPtr<nsIIOService> serv = do_GetIOService(&rv);
        if (NS_FAILED(rv)) return rv;

        nsXPIDLCString buf;
        rv = serv->Unescape(prehost, getter_Copies(buf));
        if (NS_FAILED(rv)) return rv;

        char *p = PL_strchr(buf, ':');
        if (p) {
            // user:pass
            *p = 0;
            user = NS_ConvertASCIItoUCS2(buf);
            pass = NS_ConvertASCIItoUCS2(p+1);
        }
        else {
            // user
            user = NS_ConvertASCIItoUCS2(buf);
        }
    }
    return NS_OK;
}

nsresult
nsHttpChannel::ParseRealm(const char *challenge, nsACString &realm)
{
    //
    // From RFC2617 section 1.2, the realm value is defined as such:
    //
    //    realm       = "realm" "=" realm-value
    //    realm-value = quoted-string
    //
    // but, we'll accept anything after the the "=" up to the first space, or
    // end-of-line, if the string is not quoted.
    //
    const char *p = PL_strstr(challenge, "realm=");
    if (p) {
        p += 6;
        if (*p == '"')
            p++;
        const char *end = PL_strchr(p, '"');
        if (!end)
            end = PL_strchr(p, ' ');
        if (end)
            realm.Assign(p, end - p);
        else
            realm.Assign(p);
    }
    return NS_OK;
}

nsresult
nsHttpChannel::PromptForUserPass(const char *host,
                                 PRInt32 port,
                                 PRBool proxyAuth,
                                 const char *realm,
                                 nsAString &user,
                                 nsAString &pass)
{
    LOG(("nsHttpChannel::PromptForUserPass [this=%x realm=%s]\n", this, realm));

    nsresult rv;
    nsCOMPtr<nsIAuthPrompt> authPrompt = do_GetInterface(mCallbacks, &rv); 
    if (NS_FAILED(rv)) {
        NS_WARNING("notification callbacks should provide nsIAuthPrompt");
        return rv;
    }

    nsAutoString realmU;
    realmU.Assign(NS_LITERAL_STRING("\""));
    realmU.AppendWithConversion(realm);
    realmU.Append(NS_LITERAL_STRING("\""));

    // construct the domain string
    nsCAutoString domain;
    domain.Assign(host);
    domain.Append(':');
    domain.AppendInt(port);

    nsAutoString hostU = NS_ConvertASCIItoUCS2(domain);

    domain.Append(" (");
    domain.Append(realm);
    domain.Append(')');

    // construct the message string
    nsCOMPtr<nsIStringBundleService> bundleSvc =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleSvc->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(bundle));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString message;
    const PRUnichar *strings[] = { realmU.GetUnicode(), hostU.GetUnicode() };

    rv = bundle->FormatStringFromName(
                    NS_LITERAL_STRING("EnterUserPasswordForRealm").get(),
                    strings, 2,
                    getter_Copies(message));
    if (NS_FAILED(rv)) return rv;

    // prompt the user...
    nsXPIDLString userBuf, passBuf;
    PRBool retval = PR_FALSE;
    rv = authPrompt->PromptUsernameAndPassword(nsnull,
                                               message.get(),
                                               NS_ConvertASCIItoUCS2(domain).get(),
                                               nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
                                               getter_Copies(userBuf),
                                               getter_Copies(passBuf),
                                               &retval);
    if (NS_FAILED(rv))
        return rv;
    if (!retval)
        return NS_ERROR_ABORT;

    user.Assign(userBuf);
    pass.Assign(passBuf);
    return NS_OK;
}

nsresult
nsHttpChannel::AddAuthorizationHeaders()
{
    LOG(("nsHttpChannel::AddAuthorizationHeaders [this=%x]\n", this));
    nsHttpAuthCache *authCache = nsHttpHandler::get()->AuthCache();
    if (authCache) {
        nsCAutoString creds;
        nsCAutoString realm;
        nsresult rv;

        // check if proxy credentials should be sent
        const char *proxyHost = mConnectionInfo->ProxyHost();
        const char *proxyType = mConnectionInfo->ProxyType();
        if (proxyHost && !PL_strcmp(proxyType, "http")) {
            rv = authCache->GetCredentialsForPath(proxyHost,
                                                  mConnectionInfo->ProxyPort(),
                                                  nsnull, realm, creds);
            if (NS_SUCCEEDED(rv)) {
                LOG(("adding Proxy_Authorization [creds=%s]\n", creds.get()));
                mRequestHead.SetHeader(nsHttp::Proxy_Authorization, creds.get());
            }
        }

        // check if server credentials should be sent
        nsXPIDLCString path;
        rv = GetCurrentPath(getter_Copies(path));
        if (NS_FAILED(rv)) return rv;

        rv = authCache->GetCredentialsForPath(mConnectionInfo->Host(),
                                              mConnectionInfo->Port(),
                                              path.get(),
                                              realm,
                                              creds);
        if (NS_SUCCEEDED(rv)) {
            LOG(("adding Authorization [creds=%s]\n", creds.get()));
            mRequestHead.SetHeader(nsHttp::Authorization, creds.get());
        }
    }
    return NS_OK;
}

nsresult
nsHttpChannel::GetCurrentPath(char **path)
{
    nsresult rv;
    nsCOMPtr<nsIURL> url = do_QueryInterface(mURI);
    if (url)
        rv = url->GetDirectory(path);
    else
        rv = mURI->GetPath(path);
    return rv;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsISupports
//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS8(nsHttpChannel,
                              nsIRequest,
                              nsIChannel,
                              nsIRequestObserver,
                              nsIStreamListener,
                              nsIHttpChannel,
                              nsIInterfaceRequestor,
                              nsIProgressEventSink,
                              nsICachingChannel)

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
    NS_ENSURE_ARG_POINTER(originalURI);
    *originalURI = mOriginalURI;
    NS_IF_ADDREF(*originalURI);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetOriginalURI(nsIURI *originalURI)
{
    mOriginalURI = originalURI;
    return NS_OK;
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
    NS_IF_ADDREF(*owner);
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
    NS_IF_ADDREF(*callbacks);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetNotificationCallbacks(nsIInterfaceRequestor *callbacks)
{
    mCallbacks = callbacks;

    mHttpEventSink = do_GetInterface(mCallbacks);
    mProgressSink = do_GetInterface(mCallbacks);

    nsresult rv = NS_OK;
    if (mProgressSink) {
        nsCOMPtr<nsIProgressEventSink> temp = mProgressSink;
        nsCOMPtr<nsIProxyObjectManager> mgr;

        rv = nsHttpHandler::get()->GetProxyObjectManager(getter_AddRefs(mgr));

        if (mgr)
            rv = mgr->GetProxyForObject(NS_CURRENT_EVENTQ,
                                        NS_GET_IID(nsIProgressEventSink),
                                        temp,
                                        PROXY_ASYNC | PROXY_ALWAYS,
                                        getter_AddRefs(mProgressSink));
    }
    return rv;
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
    NS_IF_ADDREF(*referrer);
    return NS_OK;
}
NS_IMETHODIMP
nsHttpChannel::SetReferrer(nsIURI *referrer)
{
    NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

    if (nsHttpHandler::get()->SendReferrer())
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

    LOG(("nsHttpChannel::SetRequestHeader [this=%x header=%s value=%s]\n",
        this, header, value));

    nsHttpAtom atom = nsHttp::ResolveAtom(header);
    if (!atom) {
        NS_WARNING("failed to resolve atom");
        return NS_ERROR_NOT_AVAILABLE;
    }

    return mRequestHead.SetHeader(atom, value);
}

NS_IMETHODIMP
nsHttpChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *visitor)
{
    return mRequestHead.Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
nsHttpChannel::GetUploadStream(nsIInputStream **stream)
{
    NS_ENSURE_ARG_POINTER(stream);
    *stream = mUploadStream;
    NS_IF_ADDREF(*stream);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetUploadStream(nsIInputStream *stream)
{
    mUploadStream = stream;
    if (mUploadStream)
        mRequestHead.SetMethod(nsHttp::Post);
    else
        mRequestHead.SetMethod(nsHttp::Get);
    return NS_OK;
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

NS_IMETHODIMP
nsHttpChannel::GetApplyConversion(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    *value = mApplyConversion;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetApplyConversion(PRBool value)
{
    mApplyConversion = value;
    return NS_OK;
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
    // if we no longer reference the transaction, then simply drop this event.
    if (request != mTransaction)
        return NS_OK;

    LOG(("nsHttpChannel::OnStartRequest [this=%x]\n", this));

    // All of the response headers have been acquired, so we can take ownership
    // of them from the transaction.
    mResponseHead = mTransaction->TakeResponseHead();

    // Notify nsIHttpNotify implementations
    if (mResponseHead)
        nsHttpHandler::get()->OnExamineResponse(this);
    else {
        // there won't be a response head if we've been cancelled
        return mListener->OnStartRequest(this, mListenerContext);
    }

    return ProcessResponse();
}

NS_IMETHODIMP
nsHttpChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult status)
{
    // if we no longer reference the transaction, then simply drop this event.
    if (request != mTransaction)
        return NS_OK;

    LOG(("nsHttpChannel::OnStopRequest [this=%x status=%x]\n",
        this, status));

    mIsPending = PR_FALSE;
    mStatus = status;

    // at this point, we're done with the transaction
    NS_RELEASE(mTransaction);
    mTransaction = nsnull;

    if (mListener)
        mListener->OnStopRequest(this, mListenerContext, status);

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, status);

    //mCallbacks = 0;
    //mProgressSink = 0;
    //mHttpEventSink = 0;

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
    // if we no longer reference the transaction, then simply drop this event.
    if (request != mTransaction)
        return NS_OK;

    LOG(("nsHttpChannel::OnDataAvailable [this=%x offset=%u count=%u]\n",
        this, offset, count));

    if (mListener)
        return mListener->OnDataAvailable(this, mListenerContext, input, offset, count);

    return NS_BASE_STREAM_CLOSED;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIInterfaceRequestor
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::GetInterface(const nsIID &iid, void **result)
{
    if (iid.Equals(NS_GET_IID(nsIProgressEventSink))) {
        //
        // we return ourselves as the progress event sink so we can intercept
        // notifications and set the correct request and context parameters.
        // but, if we don't have a progress sink to forward those messages
        // to, then there's no point in handing out a reference to ourselves.
        //
        if (!mProgressSink)
            return NS_ERROR_NO_INTERFACE;

        return QueryInterface(iid, result);
    }

    if (mCallbacks)
        return mCallbacks->GetInterface(iid, result);

    return NS_ERROR_NO_INTERFACE;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsIProgressEventSink
//-----------------------------------------------------------------------------

// called on the socket thread
NS_IMETHODIMP
nsHttpChannel::OnStatus(nsIRequest *req, nsISupports *ctx, nsresult status,
                        const PRUnichar *statusText)
{
    if (mProgressSink)
        mProgressSink->OnStatus(this, mListenerContext, status, statusText);

    return NS_OK;
}

// called on the socket thread
NS_IMETHODIMP
nsHttpChannel::OnProgress(nsIRequest *req, nsISupports *ctx,
                          PRUint32 progress, PRUint32 progressMax)
{
    if (mProgressSink)
        mProgressSink->OnProgress(this, mListenerContext, progress, progressMax);

    return NS_OK;
}

//-----------------------------------------------------------------------------
// nsHttpChannel::nsICachingChannel
//-----------------------------------------------------------------------------

NS_IMETHODIMP
nsHttpChannel::GetCacheToken(nsISupports **token)
{
    NS_ENSURE_ARG_POINTER(token);
    /*
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    return CallQueryInterface(mCacheEntry, token);
    */
    *token = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetCacheToken(nsISupports *token)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheKey(nsISupports **key)
{
    //nsresult rv;
    NS_ENSURE_ARG_POINTER(key);

    *key = nsnull;
    return NS_OK;

    /*
    nsCOMPtr<nsISupportsPRUint32> container =
        do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = container->SetData(mPostID);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(container, key);
    */
}

NS_IMETHODIMP
nsHttpChannel::SetCacheKey(nsISupports *key, PRBool fromCacheOnly)
{
    /*
    nsresult rv;
    NS_ENSURE_ARG_POINTER(key);

    nsCOMPtr<nsISupportsPRUint32> container = do_QueryInterface(key, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = container->GetData(&mPostID);
    if (NS_FAILED(rv)) return rv;

    mFromCacheOnly = fromCacheOnly;
    */
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheAsFile(PRBool *value)
{
    NS_ENSURE_ARG_POINTER(value);
    /*
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    nsCacheStoragePolicy storagePolicy;
    mCacheEntry->GetStoragePolicy(&storagePolicy);
    *value = (storagePolicy == nsICache::STORE_ON_DISK_AS_FILE);
    */
    *value = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::SetCacheAsFile(PRBool value)
{
    /*
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    nsCacheStoragePolicy policy;
    if (value)
        policy = nsICache::STORE_ON_DISK_AS_FILE;
    else
        policy = nsICache::STORE_ANYWHERE;
    return mCacheEntry->SetStoragePolicy(policy);
    */
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannel::GetCacheFile(nsIFile **cacheFile)
{
    /*
    if (!mCacheEntry)
        return NS_ERROR_NOT_AVAILABLE;
    return mCacheEntry->GetFile(cacheFile);
    */
    return NS_OK;
}
