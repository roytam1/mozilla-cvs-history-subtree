/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla FastLoad code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (original author)
 */

#include "prtypes.h"
#include "prio.h"
#include "prtime.h"
#include "pldhash.h"

#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsFastLoadPtr.h"

#include "nsIComponentManager.h"
#include "nsIFastLoadService.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsISeekableStream.h"

PR_IMPLEMENT_DATA(nsIFastLoadService*) gFastLoadService_ = nsnull;

class NS_COM nsFastLoadService : public nsIFastLoadService
{
  public:
    nsFastLoadService();
    virtual ~nsFastLoadService();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIFASTLOADSERVICE

  private:
    PRLock*             mLock;
    PLDHashTable*       mFastLoadPtrMap;

    nsCOMPtr<nsIObjectInputStream>  mObjectInputStream;
    nsCOMPtr<nsIObjectOutputStream> mObjectOutputStream;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsFastLoadService, nsIFastLoadService)

nsFastLoadService::nsFastLoadService()
  : mLock(nsnull),
    mFastLoadPtrMap(nsnull)
{
    NS_INIT_REFCNT();

    gFastLoadService_ = this;
}

nsFastLoadService::~nsFastLoadService()
{
    gFastLoadService_ = nsnull;

    if (mLock)
        PR_DestroyLock(mLock);

    if (mFastLoadPtrMap)
        PL_DHashTableDestroy(mFastLoadPtrMap);
}

#if 0
NS_IMETHODIMP
nsFastLoadService::Init()
{
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::Open(const char* aFileName, PRBool *aReading)
{
    Close();

    nsresult rv;
    nsFastLoadHeader header;
    nsFastLoadFooter footer;

    PRIntn flags = PR_RDONLY;
    PRFileDesc* fd = PR_OpenFile(aFileName, flags, 0);

    if (fd) {
        PRFileInfo info;
        if (PR_GetOpenFileInfo(fd, &info) == PR_FAILURE ||
            info.size < NS_FAST_LOAD_MAGIC_SIZE ||
            NS_FAILED(ReadHeaderAndFooter(&header, &footer)) ||
            LL_CMP(info.modifyTime, <, footer.MaxDependencyModifyTime())) {
            // Something's corrupted or out of date -- let's start over.
            PR_Close(fd);
            fd = nsnull;
        }
    }

    if (!fd) {
        flags = PR_CREATE_FILE | PR_TRUNCATE | PR_WRONLY;
        fd = PR_OpenFile(aFileName, flags, PR_IRUSR | PR_IWUSR);
        if (!fd)
            return NS_ERROR_FAILURE;
    }

    // Only one thread should be calling open and close.
    nsAutoLock lock(mLock);
    if (mFD)
        return NS_ERROR_UNEXPECTED;

    mFD = fd;
    mHeader = header;
    mFooter = footer;

    *aReading = (flags == PR_RDONLY);
    if (*aReading) {
        mFastLoadPtrMap = PL_NewDHashTable(PL_DHashGetStubOps(), this,
                                           sizeof(nsFastLoadPtrMapEntry),
                                           PL_DHASH_MIN_SIZE);
        if (!mFastLoadPtrMap)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = NS_NewObjectInputStream(&mObjectInputStream, this);
        if (NS_FAILED(rv))
            return rv;

        mObjectOutputStream = nsnull;
    } else {
        rv = NS_NewObjectOutputStream(&mObjectOutputStream, this);
        if (NS_FAILED(rv))
            return rv;

        mObjectInputStream = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::Close()
{
    nsAutoLock lock(mLock);

    if (mFD) {
        PR_Close(mFD);
        mFD = nsnull;
    }

    if (mFastLoadPtrMap) {
        PL_DHashTableDestroy(mFastLoadPtrMap);
        mFastLoadPtrMap = nsnull;
    }

    mFooter.Clear();

    mObjectInputStream = nsnull;
    mObjectOutputStream = nsnull;
}

NS_IMETHODIMP
nsFastLoadService::AddDependency(const char* aFileName)
{
    return mFooter.AddDependency(aFileName);
}

nsresult
nsFastLoadFooter::AddDependency(const char* aFileName)
{
    if (!mDependencies) {
        mDependencies = new nsCStringArray();
        if (!mDependencies)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!mDependencies->AppendCString(aFileName))
        return NS_ERROR_FAILURE;
    return NS_OK;
}

PRTime
nsFastLoadFooter::MaxDependencyModifyTime()
{
    PRTime maxTime = LL_ZERO;

    if (mDependencies) {
        PRInt32 count = mDependencies->Count();
        for (PRInt32 i = 0; i < count; i++) {
            PRFileInfo info;
            if (PR_GetFileInfo(mDependencies[i], &info) == PR_SUCCESS &&
                LL_CMP(maxTime, <, info.modifyTime)) {
                maxTime = info.modifyTime;
            }
        }
    }

    return maxTime;
}
#endif

struct nsFastLoadPtrEntry : public PLDHashEntryHdr {
    nsISupports** mPtrAddr;     // key, must come first for PL_DHashGetStubOps
    PRUint32      mOffset;
};

NS_IMETHODIMP
nsFastLoadService::GetFastLoadReferent(nsISupports* *aPtrAddr)
{
    *aPtrAddr = nsnull;

    nsAutoLock lock(mLock);
    if (!mObjectInputStream)
        return NS_OK;

    nsFastLoadPtrEntry* entry =
        NS_STATIC_CAST(nsFastLoadPtrEntry*,
                       PL_DHashTableOperate(mFastLoadPtrMap, aPtrAddr,
                                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_FREE(entry))
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mObjectInputStream));

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, entry->mOffset);
    if (NS_FAILED(rv)) return rv;

    rv = mObjectInputStream->ReadObject(PR_TRUE, aPtrAddr);
    if (NS_FAILED(rv)) return rv;

    // Shrink the table if half the entries are removed sentinels.
    PRUint32 size = PR_BIT(mFastLoadPtrMap->sizeLog2);
    if (mFastLoadPtrMap->removedCount >= (size >> 2))
        PL_DHashTableOperate(mFastLoadPtrMap, entry, PL_DHASH_REMOVE);
    else
        PL_DHashTableRawRemove(mFastLoadPtrMap, entry);

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::ReadFastLoadPtr(nsIObjectInputStream* aInputStream,
                                   nsISupports* *aPtrAddr)
{
    // nsFastLoadPtrs self-construct to null, so if we have a non-null value
    // in our inout parameter, we must have been read already, alright!
    if (*aPtrAddr)
        return NS_OK;

    nsAutoLock lock(mLock);
    if (!mObjectInputStream)
        return NS_OK;

    nsresult rv;
    PRUint32 nextOffset;

    rv = mObjectInputStream->Read32(&nextOffset);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mObjectInputStream));
    PRUint32 thisOffset;

    rv = seekable->Tell(&thisOffset);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, nextOffset);
    if (NS_FAILED(rv)) return rv;

    nsFastLoadPtrEntry* entry =
        NS_STATIC_CAST(nsFastLoadPtrEntry*,
                       PL_DHashTableOperate(mFastLoadPtrMap, aPtrAddr,
                                            PL_DHASH_ADD));
    NS_ASSERTION(entry->mPtrAddr == nsnull, "duplicate nsFastLoadPtr?!");

    entry->mPtrAddr = aPtrAddr;
    entry->mOffset = thisOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::WriteFastLoadPtr(nsIObjectOutputStream* aOutputStream,
                                    nsISupports* aObject,
                                    const nsCID& aCID)
{
    NS_ASSERTION(aObject != nsnull, "writing an unread nsFastPtr?!");

    nsAutoLock lock(mLock);
    if (!mObjectOutputStream)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mObjectOutputStream));
    PRUint32 saveOffset, nextOffset;

    rv = seekable->Tell(&saveOffset);
    if (NS_FAILED(rv)) return rv;

    rv = mObjectOutputStream->Write32(0);       // nextOffset placeholder
    if (NS_FAILED(rv)) return rv;

    rv = mObjectOutputStream->WriteObject(aObject, aCID, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Tell(&nextOffset);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
    if (NS_FAILED(rv)) return rv;

    rv = mObjectOutputStream->Write32(nextOffset);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, nextOffset);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}
