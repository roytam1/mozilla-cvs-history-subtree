/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsDnsService.h"
#include "nsIDNSListener.h"
#include "nsIRequest.h"
#include "nsISupportsArray.h"
#include "nsError.h"
#include "prnetdb.h"
#include "nsString2.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "netCore.h"
#include "nsAutoLock.h"
#include "nsIStreamObserver.h"

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);

////////////////////////////////////////////////////////////////////////////////
// Platform specific defines and includes
////////////////////////////////////////////////////////////////////////////////

// PC
#if defined(XP_PC)
#define WM_DNS_SHUTDOWN         (WM_USER + 1)
#endif /* XP_PC */

// MAC
#if defined(XP_MAC)
#include "pprthred.h"

#if TARGET_CARBON

#define nsDNS_NOTIFIER_ROUTINE	nsDnsServiceNotifierRoutineUPP
#define INIT_OPEN_TRANSPORT()	InitOpenTransport(mClientContext, kInitOTForExtensionMask)
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServices(config, flags, err, mClientContext)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpoint(config, flags, info, err, mClientContext)

#else

#define nsDNS_NOTIFIER_ROUTINE	nsDnsServiceNotifierRoutine
#define INIT_OPEN_TRANSPORT()	InitOpenTransport()
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServices(config, flags, err)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpoint(config, flags, info, err)
#endif /* TARGET_CARBON */

typedef struct nsInetHostInfo {
    InetHostInfo    hostInfo;
    nsDNSLookup *   lookup;
} nsInetHostInfo;

typedef struct nsLookupElement {
	QElem	         qElem;
	nsDNSLookup *    lookup;
	
} nsLookupElement;
#endif /* XP_MAC */

// UNIX
// XXX needs implementation

////////////////////////////////////////////////////////////////////////////////

class nsDNSLookup;

class nsDNSRequest : public nsIRequest
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST

    nsDNSRequest() : mLookup(nsnull), mSuspendCount(0) { NS_INIT_REFCNT(); }
    virtual ~nsDNSRequest() {}

    nsresult Init(nsDNSLookup* lookup,
                  nsIDNSListener* userListener,
                  nsISupports* userContext);
    nsresult FireStart();
    nsresult FireStop(nsresult status);

protected:
    nsCOMPtr<nsIDNSListener>    mUserListener;
    nsCOMPtr<nsISupports>       mUserContext;
    nsDNSLookup*                mLookup;        // weak ref
    PRUint32                    mSuspendCount;
};

////////////////////////////////////////////////////////////////////////////////

class nsDNSLookup
{
public:
    nsDNSLookup();
    ~nsDNSLookup(void);

    nsresult            Init(const char * hostName);
    const char *        HostName()  { return mHostName; }
    nsHostEnt*          HostEntry() { return &mHostEntry; }
    PRBool              IsComplete() { return mComplete; }
    nsresult            HandleRequest(nsDNSRequest* req);
    nsresult            InitiateLookup(nsDNSService* dnsService);
    nsresult            CompletedLookup(nsresult status);
    nsresult            Suspend(nsDNSRequest* req);
    nsresult            Resume(nsDNSRequest* req);
    static PRBool       FindEntry(nsHashKey *aKey, void *aData, void* closure);
    static PRBool       CompletedEntry(nsHashKey *aKey, void *aData, void* closure);
    static PRBool       DeleteEntry(nsHashKey *aKey, void *aData, void* closure);
    PRBool              IsExpired() { 
        char buf[256];
        PRExplodedTime et;

        PR_ExplodeTime(mTimestamp, PR_LocalTimeParameters, &et);
        PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &et);
        printf("\nDNS %s lookup %s\n", mHostName, buf);

        PR_ExplodeTime(mTimestamp + nsDNSService::gService->mExpirationInterval,
                       PR_LocalTimeParameters, &et);
        PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &et);
        printf("expires %s\n", buf);

        PR_ExplodeTime(nsTime(), PR_LocalTimeParameters, &et);
        PR_FormatTimeUSEnglish(buf, sizeof(buf), "%c", &et);
        printf("now %s ==> %s\n", buf,
               mTimestamp + nsDNSService::gService->mExpirationInterval < nsTime()
               ? "expired" : "valid");

        return mTimestamp + nsDNSService::gService->mExpirationInterval < nsTime();
    }

    friend class nsDNSService;

protected:
    nsCOMPtr<nsISupportsArray>  mRequests;
    char *                      mHostName;
    // Result of the DNS Lookup
    nsHostEnt                   mHostEntry;
    nsresult                    mStatus;
    PRBool                      mComplete;
    nsTime                      mTimestamp;

    // Platform specific portions
#if defined(XP_MAC)
    friend pascal void nsDnsServiceNotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);
    nsresult FinishHostEntry(void);

    nsLookupElement             mLookupElement;
    nsInetHostInfo              mInetHostInfo;
#endif

#if defined(XP_PC)
    friend static LRESULT CALLBACK nsDNSEventProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HANDLE                      mLookupHandle;
#endif
};

////////////////////////////////////////////////////////////////////////////////
// utility routines:
////////////////////////////////////////////////////////////////////////////////
//
// Allocate space from the buffer, aligning it to "align" before doing
// the allocation. "align" must be a power of 2.
// NOTE: this code was taken from NSPR.

static char *
BufAlloc(PRIntn amount, char **bufp, PRIntn *buflenp, PRIntn align) {
    char *buf = *bufp;
    PRIntn buflen = *buflenp;

    if (align && ((long)buf & (align - 1))) {
        PRIntn skip = align - ((ptrdiff_t)buf & (align - 1));
        if (buflen < skip) {
            return 0;
        }
        buf += skip;
        buflen -= skip;
    }
    if (buflen < amount) {
        return 0;
    }
    *bufp = buf + amount;
    *buflenp = buflen - amount;
    return buf;
}

////////////////////////////////////////////////////////////////////////////////
// nsDNSRequest methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMPL_ISUPPORTS1(nsDNSRequest, nsIRequest);

nsresult
nsDNSRequest::Init(nsDNSLookup* lookup,
                   nsIDNSListener* userListener,
                   nsISupports* userContext)
{
    mLookup = lookup;
    mUserListener = userListener;
    mUserContext = userContext;
    return NS_OK;
}

nsresult
nsDNSRequest::FireStart() 
{
    nsresult rv;
    NS_ASSERTION(mUserListener, "calling FireStart more than once");
    if (mUserListener == nsnull)
        return NS_ERROR_FAILURE;
    rv = mUserListener->OnStartLookup(mUserContext, mLookup->HostName());
    return rv;
}

nsresult
nsDNSRequest::FireStop(nsresult status)
{
    nsresult rv;
    NS_ASSERTION(mUserListener, "calling FireStop more than once");
    if (mUserListener == nsnull)
        return NS_ERROR_FAILURE;

    if (NS_SUCCEEDED(status)) {
        rv = mUserListener->OnFound(mUserContext,
                                    mLookup->HostName(),
                                    mLookup->HostEntry());
        NS_ASSERTION(NS_SUCCEEDED(rv), "OnFound failed");
    }
    rv = mUserListener->OnStopLookup(mUserContext,
                                     mLookup->HostName(),
                                     status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "OnStopLookup failed");

    mUserListener = nsnull;
    mUserContext = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::IsPending(PRBool *result)
{
    *result = !mLookup->IsComplete();
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::Cancel(void)
{
    if (mUserListener) {
        (void)mLookup->Suspend(this);
        return FireStop(NS_BINDING_ABORTED);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::Suspend(void)
{
    if (mSuspendCount++ == 0)
        return mLookup->Suspend(this);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRequest::Resume(void)
{
    if (--mSuspendCount == 0)
        return mLookup->Resume(this);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// nsDNSLookup methods:
////////////////////////////////////////////////////////////////////////////////

nsDNSLookup::nsDNSLookup()
    : mHostName(nsnull),
      mStatus(NS_OK),
      mComplete(PR_FALSE),
      mTimestamp(0)
{
}

nsresult
nsDNSLookup::Init(const char * hostName)
{
    nsresult rv;

    // Store input into member variables
    mHostName = nsCRT::strdup(hostName);
    if (mHostName == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = NS_NewISupportsArray(getter_AddRefs(mRequests));
    if (NS_FAILED(rv)) return rv;

    // Initialize result holders
    mHostEntry.bufLen = PR_NETDB_BUF_SIZE;
    mHostEntry.bufPtr = mHostEntry.buffer;
    mComplete = PR_FALSE;
    mStatus = NS_OK;

    // Platform specific initializations
#if defined(XP_MAC)
    mInetHostInfo.lookup = this;
    mLookupElement.lookup = this;
#endif

    return NS_OK;
}

nsDNSLookup::~nsDNSLookup(void)
{
    if (mHostName)
        nsCRT::free(mHostName);
}

#if defined(XP_MAC)
nsresult
nsDNSLookup::FinishHostEntry(void)
{
    PRIntn  len, count, i;

    // convert InetHostInfo to PRHostEnt
    
    // copy name
    len = nsCRT::strlen(mInetHostInfo.hostInfo.name);
    mHostEntry.hostEnt.h_name = BufAlloc(len, &mHostEntry.bufPtr, &mHostEntry.bufLen, 0);
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_name,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_name == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    nsCRT::memcpy(mHostEntry.hostEnt.h_name, mInetHostInfo.hostInfo.name, len);
    
    // set aliases to nsnull
    mHostEntry.hostEnt.h_aliases = (char **)BufAlloc(sizeof(char *), &mHostEntry.bufPtr, &mHostEntry.bufLen, sizeof(char *));
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_aliases,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_aliases == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    *mHostEntry.hostEnt.h_aliases = nsnull;
    
    // fill in address type
    mHostEntry.hostEnt.h_addrtype = AF_INET;
    
    // set address length
    mHostEntry.hostEnt.h_length = sizeof(long);
    
    // count addresses and allocate storage for the pointers
    for (count = 1; count < kMaxHostAddrs && mInetHostInfo.hostInfo.addrs[count] != nsnull; count++){;}
    mHostEntry.hostEnt.h_addr_list = (char **)BufAlloc(count * sizeof(char *), &mHostEntry.bufPtr, &mHostEntry.bufLen, sizeof(char *));
    NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_addr_list,"nsHostEnt buffer full.");
    if (mHostEntry.hostEnt.h_addr_list == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    // copy addresses one at a time
    len = mHostEntry.hostEnt.h_length;
    for (i = 0; i < kMaxHostAddrs && mInetHostInfo.hostInfo.addrs[i] != nsnull; i++)
    {
        mHostEntry.hostEnt.h_addr_list[i] = BufAlloc(len, &mHostEntry.bufPtr, &mHostEntry.bufLen, len);
        NS_ASSERTION(nsnull != mHostEntry.hostEnt.h_addr_list[i],"nsHostEnt buffer full.");
        if (mHostEntry.hostEnt.h_addr_list[i] == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
        *(InetHost *)mHostEntry.hostEnt.h_addr_list[i] = mInetHostInfo.hostInfo.addrs[i];
    }

    mComplete = PR_TRUE;

    return NS_OK;
}
#endif

nsresult
nsDNSLookup::HandleRequest(nsDNSRequest* req)
{
    nsresult rv;
    rv = req->FireStart();
    if (NS_FAILED(rv)) return rv;
    return Resume(req);
}

nsresult
nsDNSLookup::InitiateLookup(nsDNSService* dnsService)
{
    nsresult rv = NS_OK;
    nsAutoCMonitor mon(this);
	
    PRStatus status = PR_SUCCESS;

    PRBool numeric = PR_TRUE;
    for (const char *hostCheck = mHostName; *hostCheck; hostCheck++) {
        if (!nsString2::IsDigit(*hostCheck) && (*hostCheck != '.') ) {
            numeric = PR_FALSE;
            break;
        }
    }

    if (numeric) {
        // If it is numeric then try to convert it into an IP-Address
        PRNetAddr *netAddr = (PRNetAddr*)nsAllocator::Alloc(sizeof(PRNetAddr));
        if (!netAddr) return NS_ERROR_OUT_OF_MEMORY;

        status = PR_StringToNetAddr(mHostName, netAddr);
        if (PR_SUCCESS == status) {
            // slam the IP in and move on.
            PRHostEnt *ent = &mHostEntry.hostEnt;
            mHostEntry.bufLen = PR_NETDB_BUF_SIZE;

            PRUint32 hostNameLen = nsCRT::strlen(mHostName);
            mHostEntry.hostEnt.h_name = (char*)BufAlloc(hostNameLen + 1,
                                                        (char**)&mHostEntry.buffer,
                                                        &mHostEntry.bufLen,
                                                        0);
            memcpy(mHostEntry.hostEnt.h_name, mHostName, hostNameLen + 1);

            mHostEntry.hostEnt.h_aliases = (char**)BufAlloc(1 * sizeof(char*),
                                                            (char**)&mHostEntry.buffer,
                                                            &mHostEntry.bufLen,
                                                            sizeof(char **));
            mHostEntry.hostEnt.h_aliases[0] = '\0';

            mHostEntry.hostEnt.h_addrtype = 2;
            mHostEntry.hostEnt.h_length = 4;
            mHostEntry.hostEnt.h_addr_list = (char**)BufAlloc(2 * sizeof(char*),
                                                              (char**)&mHostEntry.buffer,
                                                              &mHostEntry.bufLen,
                                                              sizeof(char **));
            mHostEntry.hostEnt.h_addr_list[0] = (char*)BufAlloc(mHostEntry.hostEnt.h_length,
                                                                (char**)&mHostEntry.buffer,
                                                                &mHostEntry.bufLen,
                                                                0);
            memcpy(mHostEntry.hostEnt.h_addr_list[0], &netAddr->inet.ip, mHostEntry.hostEnt.h_length);
            mHostEntry.hostEnt.h_addr_list[1] = '\0';

            return CompletedLookup(NS_OK);
        }
    }

    // Incomming hostname is not a numeric ip address. Need to do the actual
    // dns lookup.

#if defined(XP_MAC)
    OSErr   err;
	
    err = OTInetStringToAddress(dnsService->mServiceRef, (char *)mHostName, (InetHostInfo *)&mInetHostInfo);
    if (err != noErr)
        rv = NS_ERROR_UNEXPECTED;
#endif /* XP_MAC */

#if defined(XP_PC)
    mLookupHandle = WSAAsyncGetHostByName(dnsService->mDNSWindow, dnsService->mMsgFoundDNS,
                                          mHostName, (char *)&mHostEntry.hostEnt, PR_NETDB_BUF_SIZE);
    // check for error conditions
    if (mLookupHandle == nsnull) {
        rv = NS_ERROR_UNEXPECTED;  // or call WSAGetLastError() for details;
    }
#endif /* XP_PC */

#ifdef XP_UNIX
    // temporary SYNC version
    PRStatus status = PR_GetHostByName(mHostName, mHostEntry.buffer, 
                                       PR_NETDB_BUF_SIZE, 
                                       &(mHostEntry.hostEnt));
    if (PR_SUCCESS != status) rv = NS_ERROR_UNKNOWN_HOST;

    return CompletedLookup(rv);
#endif /* XP_UNIX */

    return rv;
}

nsresult
nsDNSLookup::CompletedLookup(nsresult status)
{
    nsresult rv;
    nsAutoCMonitor mon(this);

    mStatus = status;
    mTimestamp = nsTime();      // now
    mComplete = PR_TRUE;

    while (PR_TRUE) {
        nsDNSRequest* req = (nsDNSRequest*)mRequests->ElementAt(0);
        if (req == nsnull) break;

        rv = mRequests->RemoveElementAt(0) ? NS_OK : NS_ERROR_FAILURE;  // XXX this method incorrectly returns a bool
        if (NS_FAILED(rv)) return rv;

        // We can't be holding the lock around the OnFound/OnStopLookup
        // callbacks:
        rv = req->FireStop(mStatus);
        NS_RELEASE(req);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

nsresult
nsDNSLookup::Suspend(nsDNSRequest* req)
{
    nsresult rv;
    nsAutoCMonitor mon(this);

    if (mComplete)
        return NS_ERROR_FAILURE;

    rv = mRequests->RemoveElement(req) ? NS_OK : NS_ERROR_FAILURE;  // XXX this method incorrectly returns a bool
    if (NS_FAILED(rv)) return rv;

    PRUint32 cnt;
    rv = mRequests->Count(&cnt);
    if (NS_FAILED(rv)) return rv;
    if (cnt == 0) {
        // XXX need to do the platform-specific cancelation here
    }

    return rv;
}

nsresult
nsDNSLookup::Resume(nsDNSRequest* req)
{
    nsresult rv;
    if (mComplete && !IsExpired()) {
        rv = req->FireStop(mStatus);
        return rv;
    }

    nsAutoCMonitor mon(this);

    rv = mRequests->AppendElement(req) ? NS_OK : NS_ERROR_FAILURE;  // XXX this method incorrectly returns a bool
    if (NS_FAILED(rv)) return rv;
    
    PRUint32 reqCount;
    rv = mRequests->Count(&reqCount);
    if (NS_FAILED(rv)) return rv;

    if (reqCount == 1) {
        // if this was the first request, then we need to kick off
        // the lookup
        rv = InitiateLookup(nsDNSService::gService);
    }
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// Platform specific helper routines
////////////////////////////////////////////////////////////////////////////////

#if defined(XP_MAC)
static pascal void
nsDnsServiceNotifierRoutine(void * contextPtr, OTEventCode code, 
                            OTResult result, void * cookie)
{
#pragma unused(contextPtr)

    if (code == T_DNRSTRINGTOADDRCOMPLETE) {
        nsDNSService *  dnsService = (nsDNSService *)contextPtr;
        nsDNSLookup *   dnsLookup = ((nsInetHostInfo *)cookie)->lookup;
        PRThread *      thread;
        
        if (result != kOTNoError)
            dnsLookup->mStatus = NS_ERROR_UNKNOWN_HOST;
        
        // queue result & wake up dns service thread
        Enqueue((QElem *)&dnsLookup->mLookupElement, &dnsService->mCompletionQueue);

        dnsService->mThread->GetPRThread(&thread);
        if (thread)
            PR_Mac_PostAsyncNotify(thread);
    }
    // or else we don't handle the event
}
#endif /* XP_MAC */

#if defined(XP_PC)

struct FindEntryClosure {
    HANDLE              mHandle;
    nsDNSLookup**       mResult;
};

PRBool
nsDNSLookup::FindEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsDNSLookup* lookup = (nsDNSLookup*)aData;
    FindEntryClosure* c = (FindEntryClosure*)closure;
    if (lookup->mLookupHandle == c->mHandle) {
        *c->mResult = lookup;
        return PR_FALSE;        // quit looking
    }
    return PR_TRUE;     // keep going
}

LRESULT
nsDNSService::LookupComplete(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    nsresult rv;
    nsDNSLookup* lookup = nsnull;

    {
        nsAutoMonitor mon(mMonitor);        // protect mLookups

        // which lookup completed?  fetch lookup for this (HANDLE)wParam
        FindEntryClosure closure = { (HANDLE)wParam, &lookup };
        mLookups.Enumerate(nsDNSLookup::FindEntry, &closure);
    }  // exit monitor

    if (lookup) {
        int error = WSAGETASYNCERROR(lParam);
        rv = lookup->CompletedLookup(error ? NS_ERROR_UNKNOWN_HOST : NS_OK);
        return NS_SUCCEEDED(rv) ? 0 : -1;
    }
    return -1;  // XXX right result?
}

static LRESULT CALLBACK
nsDNSEventProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = nsnull;

    nsDNSService *  dnsService = (nsDNSService *)GetWindowLong(hWnd, GWL_USERDATA);

    if ((dnsService != nsnull) && (uMsg == dnsService->mMsgFoundDNS)) {
        result = dnsService->LookupComplete(hWnd, uMsg, wParam, lParam);
    }
    else if (uMsg == WM_DNS_SHUTDOWN) {
        // dispose DNS EventHandler Window
        (void) DestroyWindow(dnsService->mDNSWindow);
        PostQuitMessage(0);
        result = 0;
    }
    else {
        result = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return result;
}
#endif /* XP_PC */

#ifdef XP_UNIX

#endif /* XP_UNIX */

////////////////////////////////////////////////////////////////////////////////
// nsDNSService methods:
////////////////////////////////////////////////////////////////////////////////

nsDNSService*   nsDNSService::gService = nsnull;
nsrefcnt        nsDNSService::gRefcnt = 0;

PRBool
nsDNSLookup::DeleteEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsDNSLookup* lookup = (nsDNSLookup*)aData;
    delete lookup;
    return PR_TRUE;     // keep iterating
}

static PRExplodedTime gExpirationDuration = { 
    0,  // usec
    0,  // sec
    15, // min
    0,  // hours
    0,  // mdays
    0,  // months
    0,  // years
    0,  // wdays
    0   // ydays
};

nsDNSService::nsDNSService()
    : mState(NS_OK),
      mMonitor(nsnull),
      mExpirationInterval(PR_ImplodeTime(&gExpirationDuration)),
      mLookups(nsnull, nsnull, nsDNSLookup::DeleteEntry, nsnull)
{
    NS_INIT_REFCNT();

#if defined(XP_MAC)
    mThreadRunning = PR_FALSE;
    mServiceRef = nsnull;

    mCompletionQueue.qFlags = 0;
    mCompletionQueue.qHead = nsnull;
    mCompletionQueue.qTail = nsnull;
    
#if TARGET_CARBON
    mClientContext = nsnull;
#endif /* TARGET_CARBON */
#endif /* defined(XP_MAC) */
}

NS_IMETHODIMP
nsDNSService::Init()
{
    nsresult rv = NS_OK;

    NS_ASSERTION(mMonitor == nsnull, "nsDNSService not shut down");
    mMonitor = PR_NewMonitor();
    if (mMonitor == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    // initialize DNS cache (persistent?)
#if defined(XP_MAC)
    // create Open Transport Service Provider for DNS Lookups
    OSStatus    errOT;

#if TARGET_CARBON
    nsDnsServiceNotifierRoutineUPP = NewOTNotifyUPP(nsDnsServiceNotifierRoutine);

    errOT = OTAllocClientContext((UInt32)0, &clientContext);
    NS_ASSERTION(errOT == kOTNoError, "error allocating OTClientContext.");
    if (errOT != kOTNoError) return NS_ERROR_FAILURE;
#endif /* TARGET_CARBON */

    errOT = INIT_OPEN_TRANSPORT();
    NS_ASSERTION(errOT == kOTNoError, "InitOpenTransport failed.");
    if (errOT != kOTNoError) return NS_ERROR_FAILURE;

    mServiceRef = OT_OPEN_INTERNET_SERVICES(kDefaultInternetServicesPath, NULL, &errOT);
    if (errOT != kOTNoError) return NS_ERROR_FAILURE;    /* no network -- oh well */
    NS_ASSERTION((mServiceRef != NULL) && (errOT == kOTNoError), "error opening OT service.");

    /* Install notify function for DNR Address To String completion */
    errOT = OTInstallNotifier(mServiceRef, nsDNS_NOTIFIER_ROUTINE, this);
    NS_ASSERTION(errOT == kOTNoError, "error installing dns notification routine.");
    if (errOT != kOTNoError) return NS_ERROR_FAILURE;

    /* Put us into async mode */
    if (errOT == kOTNoError) {
        errOT = OTSetAsynchronous(mServiceRef);
        NS_ASSERTION(errOT == kOTNoError, "error setting service to async mode.");
        if (errOT != kOTNoError) return NS_ERROR_FAILURE;
    } else {
        // if either of the two previous calls failed then dealloc service ref and return NS_ERROR_UNEXPECTED
        OSStatus status = OTCloseProvider((ProviderRef)mServiceRef);
        return NS_ERROR_UNEXPECTED;
    }
#endif

    if (gRefcnt++ == 0)
        gService = this;

#if defined(XP_PC)
    // sync with DNS thread to allow it to create the DNS window
    nsAutoMonitor mon(mMonitor);
#endif

#if defined(XP_MAC) || defined(XP_PC)
    // create DNS thread
    NS_ASSERTION(mThread == nsnull, "nsDNSService not shut down");
    rv = NS_NewThread(getter_AddRefs(mThread), this, 0, PR_JOINABLE_THREAD);
#endif

#if defined(XP_PC)
    mon.Wait();
#endif

    return rv;
}

nsDNSService::~nsDNSService()
{
    nsresult rv = Shutdown();
    NS_ASSERTION(NS_SUCCEEDED(rv), "DNS shutdown failed");
    NS_ASSERTION(mThread == nsnull, "DNS shutdown failed");
    NS_ASSERTION(mLookups.Count() == 0, "didn't clean up lookups");

    if (mMonitor)
        PR_DestroyMonitor(mMonitor);

    if (--gRefcnt == 0)
        gService = nsnull;
}

NS_IMPL_ISUPPORTS2(nsDNSService, nsIDNSService, nsIRunnable);

NS_METHOD
nsDNSService::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult)
{
    nsresult rv;
    
    if (aOuter != nsnull)
    	return NS_ERROR_NO_AGGREGATION;
    
    nsDNSService* dnsService = new nsDNSService();
    if (dnsService == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(dnsService);
    rv = dnsService->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = dnsService->QueryInterface(aIID, aResult);
    }
    NS_RELEASE(dnsService);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// nsIRunnable implementation...
////////////////////////////////////////////////////////////////////////////////

nsresult
nsDNSService::InitDNSThread(void)
{
#if defined(XP_PC)
    WNDCLASS    wc;
    char *      windowClass = "Mozilla:DNSWindowClass";

    // allocate message id for event handler
    mMsgFoundDNS = RegisterWindowMessage("Mozilla:DNSMessage");

    // register window class for DNS event receiver window
    memset(&wc, 0, sizeof(wc));
    wc.style            = 0;
    wc.lpfnWndProc      = nsDNSEventProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = NULL;
    wc.hIcon            = NULL;
    wc.hbrBackground    = (HBRUSH) NULL;
    wc.lpszMenuName     = (LPCSTR) NULL;
    wc.lpszClassName    = windowClass;
    RegisterClass(&wc);

    // create DNS event receiver window
    mDNSWindow = CreateWindow(windowClass, "Mozilla:DNSWindow",
                              0, 0, 0, 10, 10, NULL, NULL, NULL, NULL);

    (void) SetWindowLong(mDNSWindow, GWL_USERDATA, (long)this);

    // sync with Create thread
    nsAutoMonitor mon(mMonitor);
    mon.Notify();
#endif /* XP_PC */

    return NS_OK;
}

NS_IMETHODIMP
nsDNSService::Run(void)
{
    nsresult rv;

    rv = InitDNSThread();
    if (NS_FAILED(rv)) return rv;

#if defined(XP_PC)
    MSG msg;
    
    while(GetMessage(&msg, mDNSWindow, 0, 0)) {
        // no TranslateMessage() because we're not expecting input
        DispatchMessage(&msg);
    }
#endif /* XP_PC */

#if defined(XP_MAC)
    OSErr			    err;
    nsLookupElement *   lookupElement;
    nsDNSLookup *       lookup;
    
    mThreadRunning = PR_TRUE;
	
    while (mThreadRunning) {
		
        PR_Mac_WaitForAsyncNotify(PR_INTERVAL_NO_TIMEOUT);

        nsAutoMonitor mon(mMonitor);

        // check queue for completed DNS lookups
        while ((lookupElement = (nsLookupElement *)mCompletionQueue.qHead) != nsnull) {

            err = Dequeue((QElemPtr)lookupElement, &mCompletionQueue);
            if (err)
                continue;		// assert
        
            lookup = lookupElement->lookup;
            // convert InetHostInfo to nsHostEnt
            rv = lookup->FinishHostEntry();
            if (NS_SUCCEEDED(rv)) {
                rv = lookup->CompletedLookup(NS_OK);
                NS_ASSERTION(NS_SUCCEEDED(rv), "Completed failed");
            }
            NS_RELEASE(lookup);
        }
	}
#endif /* XP_MAC */

    return rv;
}
    
////////////////////////////////////////////////////////////////////////////////
// nsIDNSService methods:
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsDNSService::Lookup(const char*     hostName,
                     nsIDNSListener* userListener,
                     nsISupports*    userContext,
                     nsIRequest*     *result)
{
    nsresult rv;
    nsDNSRequest* req;

    if (mThread == nsnull)
        return NS_ERROR_OFFLINE;

    nsDNSLookup* lookup;
    rv = GetLookupEntry(hostName, &lookup);
    if (NS_FAILED(rv)) goto done;

    req = new nsDNSRequest();
    if (req == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(req);
    rv = req->Init(lookup, userListener, userContext);
    if (NS_FAILED(rv)) goto done;

    rv = lookup->HandleRequest(req);

  done:
    if (NS_FAILED(rv)) {
        NS_RELEASE(req);
    }
    else {
        *result = req;
    }
    return rv;
}

nsresult
nsDNSService::GetLookupEntry(const char* hostName,
                             nsDNSLookup* *result)
{
    nsresult rv;
    void* prev;

    nsAutoMonitor mon(mMonitor);

    nsStringKey key(hostName);
    nsDNSLookup* lookup = (nsDNSLookup*)mLookups.Get(&key);
    if (lookup) {
        nsAutoCMonitor mon(lookup);

        if (lookup->mComplete && lookup->IsExpired()) {
            rv = lookup->Init(lookup->HostName());
            if (NS_FAILED(rv)) return rv;
        }
        
        *result = lookup;
        return NS_OK;
    }

    // no lookup entry exists for this request, either because this
    // is the first time, or an old one was cleaned out 
    
    lookup = new nsDNSLookup();
    if (lookup == nsnull) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = lookup->Init(hostName);
    if (NS_FAILED(rv)) goto done;

    prev = mLookups.Put(&key, lookup);
    NS_ASSERTION(prev == nsnull, "already a nsDNSLookup entry");

    *result = lookup;

  done:
    if (NS_FAILED(rv))
        delete lookup;
    return rv;
}

PRBool
nsDNSLookup::CompletedEntry(nsHashKey *aKey, void *aData, void* closure)
{
    nsresult rv;
    nsDNSLookup* lookup = (nsDNSLookup*)aData;
    nsresult* status = (nsresult*)closure;

    rv = lookup->CompletedLookup(*status);
    NS_ASSERTION(NS_SUCCEEDED(rv), "lookup Completed failed");
    return PR_TRUE;     // keep iterating
}

NS_IMETHODIMP
nsDNSService::Shutdown()
{
    nsresult rv = NS_OK;

    if (mThread == nsnull) return rv;

    {
        nsAutoMonitor mon(mMonitor);        // protect mLookups

        nsresult status = NS_BINDING_ABORTED;
        mLookups.Enumerate(nsDNSLookup::CompletedEntry, &status);
        mLookups.Reset();

        // XXX deallocate cache

#if defined(XP_MAC)

        mThreadRunning = PR_FALSE;

        // deallocate Open Transport Service Provider
        OSStatus status = OTCloseProvider((ProviderRef)mServiceRef);
        CloseOpenTransport();           // should be moved to terminate routine
        PRThread* dnsServiceThread;
        rv = mThread->GetPRThread(&dnsServiceThread);
        if (dnsServiceThread)
            PR_Mac_PostAsyncNotify(dnsServiceThread);
        rv = mThread->Join();

#elif defined(XP_PC)

        SendMessage(mDNSWindow, WM_DNS_SHUTDOWN, 0, 0);
        rv = mThread->Join();

#elif defined(XP_UNIX)
        // XXXX - ?
#endif
    }
    mThread = nsnull;
    PR_DestroyMonitor(mMonitor);
    mMonitor = nsnull;
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
