/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsProgressManager.h"
#include "progress.h"
#include "xp.h"                  // for FE_* callbacks
#include "prprf.h"
#include "prmem.h"
#include "plstr.h"

#define OBJECT_TABLE_INIT_SIZE 32

static NS_DEFINE_IID(kITransferListenerIID, NS_ITRANSFERLISTENER_IID);
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

////////////////////////////////////////////////////////////////////////
//
// Debugging garbage
//
//

#if 0 /* defined(DEBUG) */
#define TRACE_PROGRESS(args) pm_TraceProgress args

static void
pm_TraceProgress(const char* fmtstr, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmtstr);
    PR_vsnprintf(buf, sizeof(buf), fmtstr, ap);
    va_end(ap);

#if defined(XP_WIN)
    OutputDebugString(buf);
#elif defined(XP_UNIX)
#elif defined(XP_MAC)
#endif
}

#else /* defined(DEBUG) */
#define TRACE_PROGRESS(args)
#endif /* defined(DEBUG) */


////////////////////////////////////////////////////////////////////////
//
// nsTransfer
//
//

/**
 * This class encapsulates the transfer state for each stream that will
 * be transferring data, is currently transferring data, or has previously
 * transferred data in the current context.
 */
class nsTransfer
{
protected:
    const URL_Struct* fURL;
    PRUint32          fBytesReceived;
    PRUint32          fContentLength;
    nsTime            fStart;
    char*             fStatus;
    PRBool            fComplete;
    PRInt32           fResultCode;

public:
    nsTransfer(const URL_Struct* url);
    virtual ~nsTransfer(void);

    NS_DECL_ISUPPORTS

    void         SetProgress(PRUint32 bytesReceived, PRUint32 contentLength);
    PRUint32     GetBytesReceived(void);
    PRUint32     GetContentLength(void);
    double       GetTransferRate(void);
    PRUint32     GetMSecRemaining(void);
    void         SetStatus(const char* message);
    const char*  GetStatus(void);
    void         MarkComplete(PRInt32 resultCode);
    PRBool       IsComplete(void);
};

////////////////////////////////////////////////////////////////////////

nsTransfer::nsTransfer(const URL_Struct* url)
    : fURL(url), fBytesReceived(0), fContentLength(0), fStart(PR_Now()),
      fStatus(NULL), fComplete(PR_FALSE), fResultCode(0)
{
}


nsTransfer::~nsTransfer(void)
{
    if (fStatus) {
        PL_strfree(fStatus);
        fStatus = NULL;
    }
}


NS_IMPL_ISUPPORTS(nsTransfer, kISupportsIID);


void
nsTransfer::SetProgress(PRUint32 bytesReceived, PRUint32 contentLength)
{
    fBytesReceived = bytesReceived;
    fContentLength = contentLength;
    if (fContentLength < fBytesReceived)
        fContentLength = fBytesReceived;
}


PRUint32
nsTransfer::GetBytesReceived(void)
{
    return fBytesReceived;
}


PRUint32
nsTransfer::GetContentLength(void)
{
    return fContentLength;
}


double
nsTransfer::GetTransferRate(void)
{
    if (fContentLength == 0 || fBytesReceived == 0)
        return 0;

    nsInt64 dt = nsTime(PR_Now()) - fStart;
    PRUint32 dtMSec = dt / nsInt64((PRUint32) PR_USEC_PER_MSEC);

    if (dtMSec == 0)
        return 0;

    return ((double) fBytesReceived) / ((double) dtMSec);
}

PRUint32
nsTransfer::GetMSecRemaining(void)
{
#define DEFAULT_MSEC_REMAINING 10000
    if (IsComplete())
        return 0;

    if (fContentLength == 0 || fBytesReceived == 0)
        // no content length and/or no bytes received
        return DEFAULT_MSEC_REMAINING;

    PRUint32 cbRemaining = fContentLength - fBytesReceived;
    if (cbRemaining == 0)
        // not complete, but content length == bytes received.
        return DEFAULT_MSEC_REMAINING;

    double bytesPerMSec = GetTransferRate();
    if (bytesPerMSec == 0)
        return DEFAULT_MSEC_REMAINING;

    PRUint32 msecRemaining = (PRUint32) (((double) cbRemaining) * bytesPerMSec);

    return msecRemaining;
}



void
nsTransfer::SetStatus(const char* message)
{
    if (fStatus)
        PL_strfree(fStatus);

    fStatus = PL_strdup(message);
}


const char*
nsTransfer::GetStatus(void)
{
    return fStatus;
}



void
nsTransfer::MarkComplete(PRInt32 resultCode)
{
    fComplete = PR_TRUE;
    fResultCode = resultCode;
}


PRBool
nsTransfer::IsComplete(void)
{
    return fComplete;
}



////////////////////////////////////////////////////////////////////////
//
// Hash table allocation routines
//
//

static PR_CALLBACK void*
AllocTable(void* pool, PRSize size)
{
    return PR_MALLOC(size);
}

static PR_CALLBACK void
FreeTable(void* pool, void* item)
{
    PR_DELETE(item);
}

static PR_CALLBACK PLHashEntry*
AllocEntry(void* pool, const void* key)
{
    return PR_NEW(PLHashEntry);
}

static PR_CALLBACK void
FreeEntry(void* pool, PLHashEntry* he, PRUintn flag)
{
    if (flag == HT_FREE_VALUE) {
        if (he->value) {
            nsTransfer* obj = (nsTransfer*) he->value;
            obj->Release();
        }
    }
    else if (flag == HT_FREE_ENTRY) {
        // we don't own the key, so leave it alone...

        if (he->value) {
            nsTransfer* obj = (nsTransfer*) he->value;
            obj->Release();
        }
        PR_DELETE(he);
    }
}


static PLHashAllocOps AllocOps = {
    AllocTable,
    FreeTable,
    AllocEntry,
    FreeEntry
};


static PLHashNumber
pm_HashURL(const void* key)
{
    return (PLHashNumber) key;
}


static int
pm_CompareURLs(const void* v1, const void* v2)
{
    return v1 == v2;
}


////////////////////////////////////////////////////////////////////////
//
// Hash table iterators
//
//

struct AggregateObjectInfo {
    PRUint32 ObjectCount;
    PRUint32 CompleteCount;
    PRUint32 MSecRemaining;
    PRUint32 BytesReceived;
    PRUint32 ContentLength;
};

static PRIntn
pm_AggregateObjectInfo(PLHashEntry* he, PRIntn i, void* closure)
{
    AggregateObjectInfo* objInfo = (AggregateObjectInfo*) closure;

    ++(objInfo->ObjectCount);
    if (he->value) {
        nsTransfer* transfer = (nsTransfer*) he->value;

        if (transfer->IsComplete())
            ++(objInfo->CompleteCount);

        objInfo->MSecRemaining    += transfer->GetMSecRemaining();
        objInfo->BytesReceived    += transfer->GetBytesReceived();
        objInfo->ContentLength    += transfer->GetContentLength();
    }

    return HT_ENUMERATE_NEXT;
}


////////////////////////////////////////////////////////////////////////
//
// nsTopProgressManager
//
//


/**
 * An <b>nsTopProgressManager</b> is a progress manager that is associated
 * with a top-level window context. It does the dirty work of managing the
 * progress bar and the status text.
 */
class nsTopProgressManager : public nsProgressManager
{
protected:
    /**
     * A table of URLs that we are monitoring progress for. The table is
     * keyed by <b>URL_Structs</b>, the values are <b>nsTransfer</b>
     * objects.
     */
    PLHashTable* fURLs;

    /**
     * The time at which the progress manager was created. Used to compute
     * elapsed time for the percentage bar.
     */
    nsTime fStart;

    /**
     * The current amount of progress shown on the chrome's progress bar
     */
    PRUint16 fProgress;

    /**
     * The default status message to display if nothing more appropriate
     * is around.
     */
    char* fDefaultStatus;

    /**
     * An FE timeout object that is used to periodically update the
     * status bar.
     */
    void* fTimeout;

    /**
     * The FE timeout callback.
     */
    static void TimeoutCallback(void* self);

    /**
     * Called to update the status bar.
     */
    virtual void Tick(void);

public:
    nsTopProgressManager(MWContext* context);
    virtual ~nsTopProgressManager(void);

    NS_IMETHOD
    OnStartBinding(const URL_Struct* url);

    NS_IMETHOD
    OnProgress(const URL_Struct* url, PRUint32 bytesReceived, PRUint32 contentLength);

    NS_IMETHOD
    OnStatus(const URL_Struct* url, const char* message);

    NS_IMETHOD
    OnStopBinding(const URL_Struct* url, PRInt32 status, const char* message);
};


////////////////////////////////////////////////////////////////////////


nsTopProgressManager::nsTopProgressManager(MWContext* context)
    : nsProgressManager(context), fStart(PR_Now()), fDefaultStatus(NULL)
{
    fURLs = PL_NewHashTable(OBJECT_TABLE_INIT_SIZE,
                            pm_HashURL,
                            pm_CompareURLs,
                            PL_CompareValues,
                            &AllocOps,
                            NULL);

    // Start the progress manager
    fTimeout = FE_SetTimeout(nsTopProgressManager::TimeoutCallback, (void*) this, 500);
    PR_ASSERT(fTimeout);

    // to avoid "strobe" mode...
    fProgress = 1;
    FE_SetProgressBarPercent(fContext, fProgress);
}


nsTopProgressManager::~nsTopProgressManager(void)
{
    if (fDefaultStatus) {
        PL_strfree(fDefaultStatus);
        fDefaultStatus = NULL;
    }

    if (fURLs) {
        PL_HashTableDestroy(fURLs);
        fURLs = NULL;
    }

    if (fTimeout) {
        FE_ClearTimeout(fTimeout);
        fTimeout = NULL;
    }

    // XXX Needs to go to allxpstr.h
    FE_Progress(fContext, "Done.");
}


////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsTopProgressManager::OnStartBinding(const URL_Struct* url)
{
    PR_ASSERT(url);
    if (! url)
        return NS_ERROR_NULL_POINTER;

    PR_ASSERT(fURLs);
    if (! fURLs)
        return NS_ERROR_NULL_POINTER;

    TRACE_PROGRESS(("OnStartBinding(%s)\n", url->address));

    PL_HashTableAdd(fURLs, url, new nsTransfer(url));
    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressManager::OnProgress(const URL_Struct* url,
                                 PRUint32 bytesReceived,
                                 PRUint32 contentLength)
{

    // Some sanity checks...
    PR_ASSERT(url);
    if (! url)
        return NS_ERROR_NULL_POINTER;

    TRACE_PROGRESS(("OnProgress(%s, %ld, %ld)\n", url->address, bytesReceived, contentLength));

    PR_ASSERT(fURLs);
    if (! fURLs)
        return NS_ERROR_NULL_POINTER;

    nsTransfer* transfer = (nsTransfer*) PL_HashTableLookup(fURLs, url);

    PR_ASSERT(transfer);
    if (!transfer)
        return NS_ERROR_NULL_POINTER;

    transfer->SetProgress(bytesReceived, contentLength);
    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressManager::OnStatus(const URL_Struct* url, const char* message)
{
    TRACE_PROGRESS(("OnStatus(%s, %s)\n", (url ? url->address : NULL), message));

    // There are cases when transfer may be null, and that's ok.
    if (url) {
        PR_ASSERT(fURLs);
        if (! fURLs)
            return NS_ERROR_NULL_POINTER;

        nsTransfer* transfer = (nsTransfer*) PL_HashTableLookup(fURLs, url);

        PR_ASSERT(transfer);
        if (transfer)
            transfer->SetStatus(message);
    }

    if (fDefaultStatus)
        PL_strfree(fDefaultStatus);

    fDefaultStatus = PL_strdup(message);
    return NS_OK;
}

NS_IMETHODIMP
nsTopProgressManager::OnStopBinding(const URL_Struct* url,
                                    PRInt32 status,
                                    const char* message)
{
    PR_ASSERT(url);
    if (! url)
        return NS_ERROR_NULL_POINTER;

    TRACE_PROGRESS(("OnStatus(%s, %d, %s)\n", url->address, status, message));

    PR_ASSERT(fURLs);
    if (! fURLs)
        return NS_ERROR_NULL_POINTER;

    nsTransfer* transfer = (nsTransfer*) PL_HashTableLookup(fURLs, url);

    PR_ASSERT(transfer);
    if (!transfer)
        return NS_ERROR_NULL_POINTER;

    transfer->MarkComplete(status);
    transfer->SetStatus(message);

    return NS_OK;
}



////////////////////////////////////////////////////////////////////////

void
nsTopProgressManager::TimeoutCallback(void* closure)
{
    nsTopProgressManager* self = (nsTopProgressManager*) closure;
    self->Tick();
}

void
nsTopProgressManager::Tick(void)
{
    TRACE_PROGRESS(("nsProgressManager.Tick: aggregating information for active objects\n"));

    AggregateObjectInfo info = { 0, 0, 0, 0, 0 };
    PL_HashTableEnumerateEntries(fURLs, pm_AggregateObjectInfo, (void*) &info);

    PR_ASSERT(info.ObjectCount > 0);
    if (info.ObjectCount == 0)
        return;

    if (info.ObjectCount == 1 || info.CompleteCount == 0) {
        // If we only have one object that we're transferring, or if
        // nothing has completed yet, show the default status message
        FE_Progress(fContext, fDefaultStatus);
    }
    else {
        // Display the overall progress: number of object complete out
        // of number of known objects

        // XXX Need to add this to allxpstr.h, etc.
        char* buf = PR_smprintf("%ld of %ld objects loaded\n",
                                info.CompleteCount,
                                info.ObjectCount);

        if (buf) {
            FE_Progress(fContext, buf);
            PR_smprintf_free(buf);
        }
    }

    // Compute how much time has elapsed
    nsInt64 dt = nsTime(PR_Now()) - fStart;
    PRUint32 elapsed = dt / nsInt64((PRUint32) PR_USEC_PER_MSEC);

    TRACE_PROGRESS(("nsProgressManager.Tick: %ld of %ld objects complete, "
                    "%ldms left, "
                    "%ld of %ld bytes xferred\n",
                    info.CompleteCount, info.ObjectCount,
                    info.MSecRemaining,
                    info.BytesReceived, info.ContentLength));

    if (info.MSecRemaining != 0) {
        // Compute the percent complete, that is, elapsed / (elapsed + remaining)
        double pctComplete = ((double) elapsed) / ((double) (elapsed + info.MSecRemaining));

        // This hackery is a kludge to make the progress bar
        // monotonically increase rather than slipping backwards as we
        // discover that there's more content to download. It works by
        // adjusting the progress manager's start time backwards to
        // make the elapsed time (time we've waited so far) seem
        // larger in proportion to the amount of time that appears to
        // be left.
        if (((PRUint32) (pctComplete * 100.0)) < fProgress) {
            PRUint32 newElapsed =
                (PRUint32) ((pctComplete * ((double) info.MSecRemaining))
                            / (1.0 - pctComplete));

            PRUint32 dMSec = newElapsed - elapsed;
            fStart -= nsInt64(dMSec * ((PRUint32) PR_USEC_PER_MSEC));
        } else {
            fProgress = (PRUint32) (100.0 * pctComplete);
            FE_SetProgressBarPercent(fContext, fProgress);
        }
    }

    // Check to see if we're done.
    if (info.CompleteCount == info.ObjectCount) {
        TRACE_PROGRESS(("Complete: %ld/%ld objects loaded\n",
                        info.CompleteCount,
                        info.ObjectCount));

        // XXX needs to go to allxpstr.h
        FE_Progress(fContext, "Done.");

        PL_HashTableDestroy(fURLs);
        fURLs = NULL;

        fTimeout = NULL;
    }
    else {
        // Reset the timeout to fire again...
        fTimeout = FE_SetTimeout(nsTopProgressManager::TimeoutCallback,
                                 (void*) this, 500);
    }
}



////////////////////////////////////////////////////////////////////////
//
// nsSubProgressManager
//
//

/**
 * The <b>nsSubProgressManager</b> is a progress manager that gets created
 * in a grid (frame cell) context. It simply delegates all of the operations
 * in the <b>nsITransferListener</b> interface to it's grid parent's
 * progress manager.
 */
class nsSubProgressManager : public nsProgressManager
{
public:
    nsSubProgressManager(MWContext* context);
    virtual ~nsSubProgressManager(void);

    NS_IMETHOD
    OnStartBinding(const URL_Struct* url);

    NS_IMETHOD
    OnProgress(const URL_Struct* url, PRUint32 bytesReceived, PRUint32 contentLength);

    NS_IMETHOD
    OnStatus(const URL_Struct* url, const char* message);

    NS_IMETHOD
    OnStopBinding(const URL_Struct* url, PRInt32 status, const char* message);
};

////////////////////////////////////////////////////////////////////////


nsSubProgressManager::nsSubProgressManager(MWContext* context)
    : nsProgressManager(context)
{
    // Grab a reference to the parent context's progress manager
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    PR_ASSERT(pm);
    if (pm)
        pm->AddRef();
}


nsSubProgressManager::~nsSubProgressManager(void)
{
    // Release the reference on the parent context's progress manager
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    if (pm)
        pm->Release();
}


////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsSubProgressManager::OnStartBinding(const URL_Struct* url)
{
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    PR_ASSERT(pm);
    if (! pm)
        return NS_ERROR_NULL_POINTER;

    return pm->OnStartBinding(url);
}


NS_IMETHODIMP
nsSubProgressManager::OnProgress(const URL_Struct* url,
                                 PRUint32 bytesReceived,
                                 PRUint32 contentLength)
{
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    PR_ASSERT(pm);
    if (! pm)
        return NS_ERROR_NULL_POINTER;

    return pm->OnProgress(url, bytesReceived, contentLength);
}


NS_IMETHODIMP
nsSubProgressManager::OnStatus(const URL_Struct* url, const char* message)
{
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    PR_ASSERT(pm);
    if (! pm)
        return NS_ERROR_NULL_POINTER;

    return pm->OnStatus(url, message);
}


NS_IMETHODIMP
nsSubProgressManager::OnStopBinding(const URL_Struct* url,
                                    PRInt32 status,
                                    const char* message)
{
    nsITransferListener* pm = fContext->grid_parent->progressManager;
    PR_ASSERT(pm);
    if (! pm)
        return NS_ERROR_NULL_POINTER;

    return pm->OnStopBinding(url, status, message);
}



////////////////////////////////////////////////////////////////////////
//
// nsProgressManager
//
//

/**
 * <p>
 * This unholiness is used to keep reference counts straight for
 * all the wild and wooly ways that a progress manager can get
 * created. It recursively creates and installs progress managers,
 * walking the grid hierarchy up to the root, if necessary. Recursion
 * terminates as soon as a context is found with a progress manager
 * already installed.
 * </p>
 *
 * <p>
 * Just for posterity, there are several ways that I can think of
 * that a progress manager might be born. We need to survive all
 * of them:
 * </p>
 *
 * <ol>
 * <li>
 * A progress manager can be created in a simple window context
 * </li>
 *
 * <li>
 * A progress manager can be created in a top-level grid context,
 * which will quickly receive an <b>AllConnectionsComplete</b>
 * as soon as it reads the HTML for the frameset.
 * </li>
 *
 * <li>
 * A progress manager can be created in a child grid context, in
 * a frameset.
 * </li>
 *
 * <li>
 * A progress manager can be created in a child grid context, in
 * response to somebody clicking a link on a page in the child
 * context
 * </li>
 * </ol>
 *
 * <p>
 * In each case, we need to make sure that we properly create
 * A top-level grid context and that everything gets refcounted
 * nicely. Could be nasty situations like, I'm loading a link
 * on a left frame, and the user gets bored and clicks a link
 * in the right frame! This should take care of that, too.
 * </p>
 */

static void
pm_RecursiveEnsure(MWContext* context)
{
    if (! context)
        return;

    if (context->progressManager)
        return;

    if (context->grid_parent) {
        pm_RecursiveEnsure(context->grid_parent);
        context->progressManager =
            new nsSubProgressManager(context);
    }
    else {   
        context->progressManager =
            new nsTopProgressManager(context);
    }
}

void
nsProgressManager::Ensure(MWContext* context)
{
    pm_RecursiveEnsure(context);

    if (context->progressManager)
        context->progressManager->AddRef();
}


void
nsProgressManager::Release(MWContext* context)
{
    if (! context)
        return;

    // Note that we DO NOT set the context's progressManager
    // slot to NULL. This is due to the obscene reference
    // counting that we do: the slot will be set to NULL by
    // the progressManager's destructor.
    if (context->progressManager)
        context->progressManager->Release();
}

nsProgressManager::nsProgressManager(MWContext* context)
    : fContext(context)
{
    NS_INIT_REFCNT();
}



nsProgressManager::~nsProgressManager(void)
{
    // HERE is where we null the context's progressManager slot. We
    // wait until the progress manager is actually destroyed, because
    // there are several ways that a reference to the progress manager
    // might be held, and unfortunately, the only public way to access
    // the progress manager is through the contexts.
    if (fContext) {
        fContext->progressManager = NULL;
        fContext = NULL;
    }
}

////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(nsProgressManager);
NS_IMPL_RELEASE(nsProgressManager);

nsresult
nsProgressManager::QueryInterface(const nsIID& iid, void** result)
{
    if (iid.Equals(kITransferListenerIID)) {
        (*result) = (void*) ((nsITransferListener*) this);
        AddRef();
        return NS_OK;
    }
    else if (iid.Equals(kISupportsIID)) {
        (*result) = (void*) ((nsISupports*) this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

