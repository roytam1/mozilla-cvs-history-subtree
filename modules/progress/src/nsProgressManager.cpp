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
#include "xp.h"                  // for FE_* callbacks
#include "prprf.h"
#include "prmem.h"
#include "plstr.h"

#define OBJECT_TABLE_INIT_SIZE 32

static NS_DEFINE_IID(kITransferIID, NS_ITRANSFER_IID);
static NS_DEFINE_IID(kITransferObserverIID, NS_ITRANSFEROBSERVER_IID);

////////////////////////////////////////////////////////////////////////
//
// Allocation routines
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
            nsISupports* obj = (nsISupports*) he->value;
            obj->Release();
        }
    }
    else if (flag == HT_FREE_ENTRY) {
        if (he->key) {
            PL_strfree((char*) he->key);
        }
        if (he->value) {
            nsISupports* obj = (nsISupports*) he->value;
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



////////////////////////////////////////////////////////////////////////
//
// nsNullTransfer
//
//

class nsNullTransfer : public nsITransfer {
private:
    nsNullTransfer(void) {
        NS_INIT_REFCNT();
        AddRef(); // so we're never destroyed
    };

public:
    static nsNullTransfer Instance;

    virtual ~nsNullTransfer(void) {
        // PR_ASSERT(exiting);
    };

    NS_DECL_ISUPPORTS

    virtual const char* GetURL(void) {
        return NULL;
    };

    virtual State GetState(void) {
        return Error;
    };

    virtual PRUint32 GetTimeRemainingMSec(void) {
        return 0;
    };

    virtual void DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback) {
    };
};

NS_IMPL_ISUPPORTS(nsNullTransfer, kITransferIID);

nsNullTransfer nsNullTransfer::Instance;

////////////////////////////////////////////////////////////////////////
//
// nsProgressManager
//
//

nsProgressManager::nsProgressManager(MWContext* context, const char* url)
    : fContext(context), fTransfers(NULL), fTimeout(NULL), nsSimpleTransfer(url)
{
    NS_INIT_REFCNT();

    NS_NewISupportsArray(&fTransfers);
    PR_ASSERT(fTransfers);

    fURLs = PL_NewHashTable(OBJECT_TABLE_INIT_SIZE,
                            PL_HashString,
                            PL_CompareStrings,
                            PL_CompareValues,
                            &AllocOps,
                            NULL);

    // Start the progress manager
    fTimeout = FE_SetTimeout(nsProgressManager::TimeoutCallback, (void*) this, 500);

    fProgress = 1;
    FE_SetProgressBarPercent(context, fProgress);
}



nsProgressManager::~nsProgressManager(void)
{
    PR_ASSERT(NET_AreThereActiveConnectionsForWindow(fContext) == PR_FALSE);

    if (fTimeout) {
        FE_ClearTimeout(fTimeout);
        fTimeout = NULL;
    }

    if (fTransfers) {
        fTransfers->Release();
        fTransfers = NULL;
    }

    if (fURLs) {
        PL_HashTableDestroy(fURLs);
        fURLs = NULL;
    }

    FE_Progress(fContext, "Done");
}


NS_IMPL_ADDREF(nsProgressManager);
NS_IMPL_RELEASE(nsProgressManager);


nsresult
nsProgressManager::QueryInterface(const nsIID& iid, void** result)
{
    if (iid.Equals(kITransferObserverIID)) {
        (*result) = (void*) ((nsITransferObserver*) this);
        return NS_OK;
    }
    else if (iid.Equals(kITransferIID)) {
        (*result) = (void*) ((nsITransfer*) this);
        return NS_OK;
    }
    return NS_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////

void
nsProgressManager::Add(const char* url)
{
    // sanity check
    if (! fURLs)
        return;

    if (PL_HashTableLookup(fURLs, url))
        return; // already tracking it...

    char* key = PL_strdup(url);

    // Insert a dummy transfer as a placeholder.
    // XXX Is this necessary? I think a NULL might do the job...
    PL_HashTableAdd(fURLs, key, &nsNullTransfer::Instance);
    nsNullTransfer::Instance.AddRef();
}


void
nsProgressManager::NotifyBegin(nsITransfer* transfer)
{
    if (fTransfers) {
        if (fTransfers->IndexOf(transfer) == -1) {
            fTransfers->AppendElement(transfer);

            // Track this transfer as being the current transfer that's
            // out to download it's target object.
            const char* url = transfer->GetURL();

            if (! PL_HashTableLookup(fURLs, url))
                url = PL_strdup(url);

            PL_HashTableAdd(fURLs, url, transfer);
            transfer->AddRef();
        }
        else {
            // Shouldn't try to insert the same transfer twice...
            PR_ASSERT(0);
        }
    }
}


////////////////////////////////////////////////////////////////////////

struct AggregateTransferInfo {
    PRUint32 CompleteCount;
    PRUint32 ErrorCount;
    PRUint32 TimeRemaining;
};


static PRBool
pm_AggregateTransferInfo(nsISupports* obj, void* closure)
{
    nsITransfer* transfer;

    if (obj->QueryInterface(kITransferIID, (void**) &transfer) != NS_OK) {
        PR_ASSERT(0);
        return PR_FALSE;
    }

    AggregateTransferInfo* info = (AggregateTransferInfo*) closure;

    switch (transfer->GetState()) {
    case nsITransfer::State::Complete:
        ++info->CompleteCount;
        break;

    case nsITransfer::State::Error:
        ++info->ErrorCount;
        break;

    default:
        break;
    }

    info->TimeRemaining += transfer->GetTimeRemainingMSec();

    transfer->Release();
    return PR_TRUE;
}


////////////////////////////////////////////////////////////////////////

struct AggregateObjectInfo {
    PRUint32 ObjectCount;
    PRUint32 CompleteCount;
};

static PRIntn
pm_AggregateObjectInfo(PLHashEntry* he, PRIntn i, void* closure)
{
    AggregateObjectInfo* objInfo = (AggregateObjectInfo*) closure;

    ++(objInfo->ObjectCount);
    if (he->value) {
        nsITransfer* transfer = (nsITransfer*) he->value;

        if (transfer->GetState() == nsITransfer::State::Complete)
            ++(objInfo->CompleteCount);
    }

    return HT_ENUMERATE_NEXT;
}

////////////////////////////////////////////////////////////////////////

void
nsProgressManager::TimeoutCallback(void* closure)
{
    nsProgressManager* self = (nsProgressManager*) closure;
    self->Tick();
}

void
nsProgressManager::Tick(void)
{
    if (! fTransfers)
        return;

    // Reset the timeout to fire again...
    fTimeout = FE_SetTimeout(nsProgressManager::TimeoutCallback, (void*) this, 500);

    AggregateTransferInfo xferInfo = { 0, 0, 0 };
    fTransfers->EnumerateForwards(pm_AggregateTransferInfo, (void*) &xferInfo);

    AggregateObjectInfo objInfo = { 0, 0 };
    PL_HashTableEnumerateEntries(fURLs, pm_AggregateObjectInfo, (void*) &objInfo);

    if (fTransfers->Count() == 0) {
        // Not much to do...
        return;
    }
    else if (fTransfers->Count() == 1) {
        // We've got one transfer, so get some detailed status from it...
        nsISupports* obj = fTransfers->ElementAt(0);
        nsITransfer* transfer;

        if (obj->QueryInterface(kITransferIID, (void**) &transfer) != NS_OK) {
            // Huh? Somehow an object that doesn't support nsITransfer
            // got into the list...
            PR_ASSERT(0);
            return;
        }

        transfer->DisplayStatusMessage(this, nsProgressManager::DisplayStatusCallback);
        transfer->Release();
    }
    else {
        // XXX Need to add this to allxpstr.h, etc.
        char* buf = PR_smprintf("%ld of %ld objects loaded",
                                objInfo.CompleteCount,
                                objInfo.ObjectCount);

        if (buf) {
            FE_Progress(fContext, buf);
            PR_smprintf_free(buf);
        }
    }

    PRUint32 elapsed = GetElapsedTimeMSec();

    // XXX This is bogus, not monotonically increasing.
    fProgress = (100 * elapsed) / (elapsed + xferInfo.TimeRemaining);

    FE_SetProgressBarPercent(fContext, fProgress);
}

////////////////////////////////////////////////////////////////////////


void
nsProgressManager::DisplayStatusCallback(void* data, char* message)
{
    nsProgressManager* self = (nsProgressManager*) data;
    self->DisplayStatus(message);
}

void
nsProgressManager::DisplayStatus(char* message)
{
    FE_Progress(fContext, message);
}

////////////////////////////////////////////////////////////////////////



PRUint32
nsProgressManager::GetTimeRemainingMSec(void)
{
    // XXX go through and aggregate the time remaining for each of the
    // transfers that this progress manager owns.
    return 0;
}



////////////////////////////////////////////////////////////////////////

void
nsProgressManager::DisplayStatusMessage(void* closure, nsITransferDisplayStatusFunc callback)
{
}

