/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Furman, fur@netscape.com
 */

#include "nsISupportsUtils.h"
#include "nsCachedNetData.h"
#include "nsCacheManager.h"
#include "nsCacheEntryChannel.h"
#include "nsINetDataCache.h"
#include "nsIStreamAsFile.h"
#include "nsIStringStream.h"
#include "nsIBinaryInputStream.h"
#include "nsIBinaryOutputStream.h"
#include "nsISupportsArray.h"
#include "nsCRT.h"

// Version of the cache record meta-data format.  If this version doesn't match
// the one in the database, an error is signaled when the record is read.
#define CACHE_MANAGER_VERSION 1

// Other than nsISupports methods, no public methods can operate on a deleted
// cache entry or one that has been Release'ed by everyone but the cache manager.
#define CHECK_AVAILABILITY()                                                  \
{                                                                             \
    if (GetFlag((Flag)(DELETED | DORMANT)))                                   \
        return NS_ERROR_NOT_AVAILABLE;                                        \
}

// Convert PRTime to unix-style time_t, i.e. seconds since the epoch
static PRUint32
convertPRTimeToSeconds(PRTime aTime64)
{
    double fpTime;
    LL_L2D(fpTime, aTime64);
    return (PRUint32)(fpTime * 1e-6 + 0.5);
}

// Convert unix-style time_t, i.e. seconds since the epoch, to PRTime
static PRTime
convertSecondsToPRTime(PRUint32 aSeconds)
{
    PRInt64 t64;
    LL_L2I(t64, aSeconds);
    LL_MUL(t64, t64, 1000000);
    return t64;
}

// One element in a linked list of nsIStreamAsFileObserver's
class StreamAsFileObserverClosure
{
public:
    StreamAsFileObserverClosure(nsIStreamAsFile *aStreamAsFile, nsIStreamAsFileObserver *aObserver):
        mStreamAsFile(aStreamAsFile), mObserver(aObserver), mNext(0) {}

    ~StreamAsFileObserverClosure() { delete mNext; }

    // Weak link to nsIStreamAsFile which, indirectly, holds a strong link to this
    nsIStreamAsFile*           mStreamAsFile;
    nsIStreamAsFileObserver*   mObserver;        

    // Link to next in list
    StreamAsFileObserverClosure* mNext;
};


// nsIStreamAsFile is implemented as an XPCOM tearoff to avoid the cost of an
// extra vtable pointer in nsCachedNetData
class StreamAsFile : public nsIStreamAsFile {
public:    
    StreamAsFile(nsCachedNetData* cacheEntry): mCacheEntry(cacheEntry) {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetFileSpec(nsIFileSpec* *aFileSpec) {
        return mCacheEntry->GetFileSpec(aFileSpec);
    }

    NS_IMETHOD AddObserver(nsIStreamAsFileObserver *aObserver) {
        return mCacheEntry->AddObserver(this, aObserver);
    }
    
    NS_IMETHOD RemoveObserver(nsIStreamAsFileObserver *aObserver) {
        return mCacheEntry->RemoveObserver(aObserver);
    } 

protected:
    nsCOMPtr<nsCachedNetData> mCacheEntry;
};

NS_IMPL_ADDREF(StreamAsFile)
NS_IMPL_RELEASE(StreamAsFile)

// QueryInterface delegates back to the nsICachedNetData that spawned this instance
NS_IMETHODIMP
StreamAsFile::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "QueryInterface requires a non-NULL destination!");
    if ( !aInstancePtr )
        return NS_ERROR_NULL_POINTER;
    nsISupports* foundInterface;

    if ( aIID.Equals(NS_GET_IID(nsIStreamAsFile)) ) {
        foundInterface = NS_STATIC_CAST(nsIStreamAsFile*, this);
        NS_ADDREF(foundInterface);
        *aInstancePtr = foundInterface;
        return NS_OK;
    } else {
        return mCacheEntry->QueryInterface(aIID, aInstancePtr);
    }
}

NS_IMPL_ADDREF(nsCachedNetData)
NS_IMETHODIMP
nsCachedNetData::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
    NS_ASSERTION(aInstancePtr, "QueryInterface requires a non-NULL destination!");
    if ( !aInstancePtr )
        return NS_ERROR_NULL_POINTER;
    nsISupports* foundInterface;

    if ( aIID.Equals(NS_GET_IID(nsICachedNetData)) ) {
         foundInterface = NS_STATIC_CAST(nsICachedNetData*, this);
    } else if ( aIID.Equals(NS_GET_IID(nsIStreamAsFile)) ) {
         foundInterface = new StreamAsFile(this);
         if (!foundInterface)
             return NS_ERROR_OUT_OF_MEMORY;
    } else if ( aIID.Equals(NS_GET_IID(nsISupports)) ) {
        foundInterface = NS_STATIC_CAST(nsISupports*, this);
    } else {
        foundInterface = 0;
    }

    nsresult status;
    if ( !foundInterface )
        status = NS_NOINTERFACE;
    else {
        NS_ADDREF(foundInterface);
        status = NS_OK;
    }
    *aInstancePtr = foundInterface;
    return status;
}

// A customized version of Release() that slims down the memory consumption
// when the ref-count drops to one, i.e. only the cache manager holds a
// reference to the cache entry.
NS_IMETHODIMP_(nsrefcnt)
nsCachedNetData::Release(void)
{
    NS_PRECONDITION(1 != mRefCnt, "dup release");
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsCachedNetData");
    if (mRefCnt == 1) {

        nsCacheManager::NoteDormant(this);

        // Clear flag, in case the protocol handler forgot to
        mFlags &= ~UPDATE_IN_PROGRESS;

        // First, flush any altered cache entry data to the database
        Commit();
        
        SetFlag(DORMANT);
        
        // Free up some storage
        delete mObservers;
        mObservers = 0;
        PRInt32 recordID;
        mRecord->GetRecordID(&recordID);
        mRecord = 0;
        mRecordID = recordID;
        if (mProtocolData) {
            nsAllocator::Free((void*)mProtocolData);
            mProtocolData = 0;
        }
    }
    return mRefCnt;
}

nsresult 
nsCachedNetData::GetRecord(nsINetDataCacheRecord* *aRecord)
{
    if (GetFlag(DORMANT)) {
        return mCache->GetCachedNetDataByID(mRecordID, aRecord);
    } else {
        NS_ADDREF(mRecord);
        *aRecord = mRecord;
        return NS_OK;
    }
}

nsresult 
nsCachedNetData::GetRecordID(PRInt32 *aRecordID)
{
    if (GetFlag(DORMANT)) {
        *aRecordID = mRecordID;
        return NS_OK;
    } else {
        return mRecord->GetRecordID(aRecordID);
    }
}

void
nsCachedNetData::NoteAccess()
{
    PRUint32 now;

    now = convertPRTimeToSeconds(PR_Now());

    // Don't record accesses that occur more than once per second
    if (mAccessTime[0] == now)
        return;

    // Saturate access count at 16-bit limit
    if (mNumAccesses < 0xFFFF)
        mNumAccesses++;

    // Update array of recent access times
    for (int i = MAX_K - 1; i >= 1; i--) {
        mAccessTime[i] = mAccessTime[i - 1];
    }
    mAccessTime[0] = now;
}

void
nsCachedNetData::NoteUpdate()
{
    // We only keep track of last-modified time or update time, not both
    if (GetFlag(LAST_MODIFIED_KNOWN))
        return;
    mLastUpdateTime = convertPRTimeToSeconds(PR_Now());
}

nsresult
nsCachedNetData::Init(nsINetDataCacheRecord *aRecord, nsINetDataCache *aCache)
{
    mRecord = aRecord;
    mCache = aCache;
    
    nsresult rv = Deserialize();
    ComputeProfit(0);
    return rv;
}

// Retrieve the opaque meta-data stored in the cache database record, and
// extract its components, namely the protocol-specific meta-data and the
// protocol-independent cache manager meta-data.
nsresult
nsCachedNetData::Deserialize()
{
    nsresult rv;
    PRUint32 metaDataLength;
    char* metaData;

    rv = mRecord->GetMetaData(&metaDataLength, &metaData);
    if (NS_FAILED(rv)) return rv;

    // FIXME
    nsIInputStream* stringStream;

    nsCOMPtr<nsIBinaryInputStream> binaryStream;
    rv = NS_NewBinaryInputStream(getter_AddRefs(binaryStream), stringStream);
    if (NS_FAILED(rv)) return rv;

    // Verify that the record meta-data was serialized by this version of the
    // cache manager code.
    PRUint8 version;
    rv = binaryStream->Read8(&version);
    if (NS_FAILED(rv)) return rv;
    if (version != CACHE_MANAGER_VERSION)
        return NS_ERROR_FAILURE;

    rv = binaryStream->Read32(&mProtocolDataLength);
    if (NS_FAILED(rv)) return rv;
    
    rv = binaryStream->ReadBytes(&mProtocolData, mProtocolDataLength);
    if (NS_FAILED(rv)) return rv;

    rv = binaryStream->Read16(&mFlags);
    if (NS_FAILED(rv)) return rv;

    rv = binaryStream->Read16(&mNumAccesses);
    if (NS_FAILED(rv)) return rv;

    for (int i = 0; i < MAX_K; i++) {
        rv = binaryStream->Read32(&mAccessTime[i]);
        if (NS_FAILED(rv)) return rv;
    }

    rv = binaryStream->Read32(&mLastUpdateTime);
    if (NS_FAILED(rv)) return rv;

    rv = binaryStream->Read32(&mExpirationTime);
    if (NS_FAILED(rv)) return rv;

    rv = binaryStream->ReadFloat(&mDownloadRate);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetUriSpec(char* *aUriSpec)
{
    CHECK_AVAILABILITY();

    char* key;
    PRUint32 keyLength;
    nsresult rv;

    rv = mRecord->GetKey(&keyLength, &key);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(keyLength >=1, "Bogus record key");

    // The URI spec is stored as the first of the two components that make up
    // the nsINetDataCacheRecord key and is separated from the second component
    // by a NUL character, so we can use plain 'ol strdrup().
    *aUriSpec = nsCRT::strdup(key);
    if (!*aUriSpec)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetSecondaryKey(PRUint32 *aLength, char **aSecondaryKey)
{
    CHECK_AVAILABILITY();

    char* key;
    char* secondaryKey;
    PRUint32 keyLength;
    nsresult rv;

    *aSecondaryKey = 0;

    rv = mRecord->GetKey(&keyLength, &key);
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(keyLength >=1, "Bogus record key");

    // The URI spec is stored as the second of the two components that make up
    // the nsINetDataCacheRecord key and is separated from the first component
    // by a NUL character.
    for(secondaryKey = key; *secondaryKey; secondaryKey++)
        keyLength--;

    // Account for NUL character
    keyLength--;
    
    if (keyLength) {
        char* copy = (char*)nsAllocator::Alloc(keyLength);
        if (!copy)
            return NS_ERROR_OUT_OF_MEMORY;
        memcpy(copy, secondaryKey, keyLength);
        *aSecondaryKey = copy;
    }

    nsAllocator::Free(key);
    *aLength = keyLength;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetAllowPartial(PRBool *aAllowPartial)
{
    return GetFlag(ALLOW_PARTIAL);
}

NS_IMETHODIMP
nsCachedNetData::SetAllowPartial(PRBool aAllowPartial)
{
    return SetFlag(aAllowPartial, ALLOW_PARTIAL);
}

NS_IMETHODIMP
nsCachedNetData::GetPartial(PRBool *aPartial)
{
    return GetFlag(aPartial, TRUNCATED_CONTENT);
}

NS_IMETHODIMP
nsCachedNetData::GetUpdateInProgress(PRBool *aUpdateInProgress)
{
    return GetFlag(aUpdateInProgress, UPDATE_IN_PROGRESS);
}

NS_IMETHODIMP
nsCachedNetData::SetUpdateInProgress(PRBool aUpdateInProgress)
{
    return SetFlag(aUpdateInProgress, UPDATE_IN_PROGRESS);
}

NS_IMETHODIMP
nsCachedNetData::GetLastModifiedTime(PRTime *aLastModifiedTime)
{
    CHECK_AVAILABILITY();
    NS_ENSURE_ARG_POINTER(aLastModifiedTime);
    if (GetFlag(LAST_MODIFIED_KNOWN))
        *aLastModifiedTime = convertSecondsToPRTime(mLastModifiedTime);
    else
        *aLastModifiedTime = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::SetLastModifiedTime(PRTime aLastModifiedTime)
{
    CHECK_AVAILABILITY();
    mLastModifiedTime = convertPRTimeToSeconds(aLastModifiedTime);
    SetDirty();
    SetFlag(LAST_MODIFIED_KNOWN);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetExpirationTime(PRTime *aExpirationTime)
{
    CHECK_AVAILABILITY();
    NS_ENSURE_ARG_POINTER(aExpirationTime);
    if (GetFlag(EXPIRATION_KNOWN))
        *aExpirationTime = convertSecondsToPRTime(mExpirationTime);
    else
        *aExpirationTime = 0;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::SetExpirationTime(PRTime aExpirationTime)
{
    CHECK_AVAILABILITY();

    // Only expiration time or stale time can be set, not both
    if (GetFlag(STALE_TIME_KNOWN))
        return NS_ERROR_NOT_AVAILABLE;

    mExpirationTime = convertPRTimeToSeconds(aExpirationTime);
    SetDirty();
    SetFlag(EXPIRATION_KNOWN);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetStaleTime(PRTime *aStaleTime)
{
    CHECK_AVAILABILITY();
    NS_ENSURE_ARG_POINTER(aStaleTime);
    if (GetFlag(STALE_TIME_KNOWN)) {
        *aStaleTime = convertSecondsToPRTime(mStaleTime);
    } else {
        *aStaleTime = 0;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::SetStaleTime(PRTime aStaleTime)
{
    CHECK_AVAILABILITY();

    // Only expiration time or stale time can be set, not both
    if (GetFlag(EXPIRATION_KNOWN))
        return NS_ERROR_NOT_AVAILABLE;

    mStaleTime = convertPRTimeToSeconds(aStaleTime);
    SetDirty(); 
    SetFlag(STALE_TIME_KNOWN);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetNumberAccesses(PRUint16 *aNumberAccesses)
{
    CHECK_AVAILABILITY();
    NS_ENSURE_ARG_POINTER(aNumberAccesses);
    *aNumberAccesses = mNumAccesses;
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::GetLastAccessTime(PRTime *aLastAccessTime)
{
    CHECK_AVAILABILITY();
    NS_ENSURE_ARG_POINTER(aLastAccessTime);
    *aLastAccessTime = convertSecondsToPRTime(mAccessTime[0]);
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::Commit(void)
{
    CHECK_AVAILABILITY();

#ifdef DEBUG
    if (GetFlag(EXPIRATION_KNOWN))
        NS_ASSERTION(GetFlag(LAST_MODIFIED_KNOWN), "Protocol handler error");
    NS_ASSERTION(!GetFlag(UPDATE_IN_PROGRESS),
                 "Protocol handler forgot to clear UPDATE_IN_PROGRESS flag");
#endif

    // Check to see if any data changed.  If not, nothing to do.
    if (!GetFlag(DIRTY))
        return NS_OK;
    ClearFlag(DIRTY);

    int i;
    nsresult rv;

    // FIXME
    nsIOutputStream* stringStream;

    nsCOMPtr<nsIBinaryOutputStream> binaryStream;
    rv = NS_NewBinaryOutputStream(getter_AddRefs(binaryStream), stringStream);
    if (NS_FAILED(rv)) goto error;

    // Verify that the record meta-data was serialized by this version of the
    // cache manager code.
    rv = binaryStream->Write8(CACHE_MANAGER_VERSION);
    if (NS_FAILED(rv)) goto error;

    rv = binaryStream->Write32(mProtocolDataLength);
    if (NS_FAILED(rv)) goto error;
    
    rv = binaryStream->WriteBytes(mProtocolData, mProtocolDataLength);
    if (NS_FAILED(rv)) goto error;

    rv = binaryStream->Write16(mFlags);
    if (NS_FAILED(rv)) goto error;

    rv = binaryStream->Write16(mNumAccesses);
    if (NS_FAILED(rv)) goto error;

    for (i = 0; i < MAX_K; i++) {
        rv = binaryStream->Write32(mAccessTime[i]);
        if (NS_FAILED(rv)) goto error;
    }

    rv = binaryStream->Write32(mLastUpdateTime);
    if (NS_FAILED(rv)) goto error;

    rv = binaryStream->Write32(mExpirationTime);
    if (NS_FAILED(rv)) goto error;

    rv = binaryStream->WriteFloat(mDownloadRate);
    if (NS_FAILED(rv)) goto error;

    // FIXME - Store string in MetaData
    return NS_OK;

 error:
    SetDirty();
    return rv;
}

NS_IMETHODIMP
nsCachedNetData::GetProtocolPrivate(PRUint32* aProtocolDataLength, char* *aProtocolData)
{
    CHECK_AVAILABILITY();

    NS_ENSURE_ARG_POINTER(aProtocolDataLength);
    NS_ENSURE_ARG_POINTER(aProtocolData);

    *aProtocolData = mProtocolData;
    *aProtocolDataLength = mProtocolDataLength;
    
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::SetProtocolPrivate(PRUint32 aLength, const char *aProtocolData)
{
    CHECK_AVAILABILITY();

    char* newProtocolData = 0;

    if (aProtocolData) {
        newProtocolData = (char*)nsAllocator::Alloc(aLength);
        if (!mProtocolData)
            return NS_ERROR_OUT_OF_MEMORY;
        memcpy(newProtocolData, aProtocolData, aLength);
    }
        
    if (mProtocolData)
        nsAllocator::Free(mProtocolData);
    mProtocolData = newProtocolData;
    mProtocolDataLength = aLength;
    SetDirty();
    return NS_OK;
}

nsresult
nsCachedNetData::AddObserver(nsIStreamAsFile* aStreamAsFile, nsIStreamAsFileObserver* aObserver) 
{
    StreamAsFileObserverClosure *closure;

    CHECK_AVAILABILITY();
    NS_ENSURE_ARG(aObserver);
    
    closure = new StreamAsFileObserverClosure(aStreamAsFile, aObserver);
    if (!closure)
        return NS_ERROR_OUT_OF_MEMORY;

    closure->mNext = mObservers;
    mObservers = closure;
    return NS_OK;
}

nsresult
nsCachedNetData::RemoveObserver(nsIStreamAsFileObserver *aObserver)
{
    StreamAsFileObserverClosure** closurep;
    StreamAsFileObserverClosure* closure;

    CHECK_AVAILABILITY();
    NS_ENSURE_ARG(aObserver);
    if (!mObservers)
        return NS_ERROR_FAILURE;
    
    for (closurep = &mObservers; closure = *closurep; closurep = &(*closurep)->mNext) {
        if (closure->mObserver == aObserver) {
            *closurep = closure->mNext;
            closure->mNext = 0;
            delete closure;
        }
    }
    
    return NS_ERROR_FAILURE;
} 

NS_IMETHODIMP
nsCachedNetData::GetStoredContentLength(PRUint32 *aStoredContentLength)
{
    CHECK_AVAILABILITY();
    return mRecord->GetStoredContentLength(aStoredContentLength);
}

NS_IMETHODIMP
nsCachedNetData::SetStoredContentLength(PRUint32 aStoredContentLength)
{
    CHECK_AVAILABILITY();
    return mRecord->SetStoredContentLength(aStoredContentLength);
}

// Notify all cache entry observers
nsresult
nsCachedNetData::Notify(PRUint32 aMessage, nsresult aError)
{
    nsresult rv;
    StreamAsFileObserverClosure *closure;
    nsIStreamAsFileObserver *observer;
    closure = mObservers;
    while (closure) {
        observer = closure->mObserver;
        rv = observer->ObserveStreamAsFile(closure->mStreamAsFile, aMessage, aError);
        if (NS_FAILED(rv)) return rv;
        closure = closure->mNext;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsCachedNetData::Delete(void)
{
    if (GetFlag(DELETED))
        return NS_OK;

    // Tell observers about the deletion, so that they can release their
    // references to this cache object.
    Notify(nsIStreamAsFileObserver::REQUEST_DELETION, NS_OK);

    // Can only delete if all references are dropped excepting, of course, the
    // one from the cache manager and the caller of this method.
    if (mRefCnt <= 2) {
        nsresult rv;
        nsCOMPtr<nsINetDataCacheRecord> record;

        rv = GetRecord(getter_AddRefs(record));
        if (NS_FAILED(rv)) return rv;

        rv = record->Delete();
        if (NS_FAILED(rv)) return rv;

        // Now record is available for recycling
        SetFlag(DELETED);
        return NS_OK;
    }
        
    // Unable to delete because cache entry is still active
    return NS_ERROR_FAILURE;
}

// Truncate the content data for a cache entry, so as to make space in the
// cache for incoming data for another entry.
nsresult
nsCachedNetData::Evict(PRUint32 aTruncatedContentLength)
{
    // Tell observers about the eviction, so that they can release their
    // references to this cache object.
    Notify(nsIStreamAsFileObserver::REQUEST_DELETION, NS_OK);

    // Can only delete if all references are dropped excepting, of course, the
    // one from the cache manager.
    if (mRefCnt == 1) {
        nsresult rv = mRecord->SetStoredContentLength(aTruncatedContentLength);
        if (NS_FAILED(rv)) return rv;

        if (aTruncatedContentLength == 0) {
            SetFlag(EVICTED);
            // FIXME - reset flags, delete meta-data ?
        } else {
            SetFlag(TRUNCATED_CONTENT);
        }
        return NS_OK;
    }

    // Unable to delete
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCachedNetData::NewChannel(nsILoadGroup* loadGroup, nsIChannel* *aChannel)
{
    nsresult rv;
    nsCOMPtr<nsIChannel> channel;

    CHECK_AVAILABILITY();

    rv =  mRecord->NewChannel(loadGroup, getter_AddRefs(channel));
    if (NS_FAILED(rv)) return rv;

    *aChannel = new nsCacheEntryChannel(this, channel);
    if (!*aChannel)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

nsresult
nsCachedNetData::GetFileSpec(nsIFileSpec* *aFileSpec)
{
    NS_ENSURE_ARG_POINTER(aFileSpec);
    return mRecord->GetFilename(aFileSpec);
}
