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

#include "nsReplacementPolicy.h"
#include "nsCachedNetData.h"

#include "nsQuickSort.h"
#include "nsIAllocator.h"
#include "nsIEnumerator.h"
#include "prtime.h"
#include "prbit.h"
#include "nsCOMPtr.h"
#include <math.h>

// Constant used to estimate frequency of access to a document based on size
#define CACHE_CONST_B   1.35

// A cache whose space is managed by this replacement policy
class nsReplacementPolicy::CacheInfo {
public:
    CacheInfo(nsINetDataCache* aCache):mCache(aCache),mNext(0) {}

    nsINetDataCache* mCache;
    CacheInfo*       mNext;
};

nsReplacementPolicy::nsReplacementPolicy()
    : mRecordsRemovedSinceLastRanking(0), mNumEntries(0), mCaches(0),
      mCapacityRankedEntriesArray(0), mRankedEntries(0), mLastRankTime(0) {}

nsReplacementPolicy::~nsReplacementPolicy()
{
    delete mRankedEntries;
    if (mMapRecordIdToEntry)
        nsAllocator::Free(mMapRecordIdToEntry);
}

nsresult
nsReplacementPolicy::Init(PRUint32 aMaxCacheEntries)
{
    nsresult rv;

    rv = NS_NewHeapArena(getter_AddRefs(mArena), sizeof nsCachedNetData * 32);
    if (NS_FAILED(rv)) return rv;

    mMaxEntries = aMaxCacheEntries;

    mHashArrayLength = PR_CeilingLog2(aMaxCacheEntries) >> 3;
    mMapRecordIdToEntry = (nsCachedNetData**)nsAllocator::Alloc(mHashArrayLength);
    if (!mMapRecordIdToEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

nsresult
nsReplacementPolicy::AddCache(nsINetDataCache *aCache)
{
    CacheInfo *cacheInfo = new CacheInfo(aCache);
    if (!cacheInfo)
        return NS_ERROR_OUT_OF_MEMORY;
    cacheInfo->mNext = mCaches;
    mCaches = cacheInfo;
    return NS_OK;
}

PRUint32
nsReplacementPolicy::HashRecordID(PRInt32 aRecordID)
{
    return ((aRecordID >> 16) ^ aRecordID) & (mHashArrayLength - 1);
}

nsCachedNetData*
nsReplacementPolicy::FindCacheEntryByRecordID(PRInt32 aRecordID, nsINetDataCache *aCache)
{
    nsCachedNetData* cacheEntry;
    PRUint32 bucket = HashRecordID(aRecordID);

    cacheEntry = mMapRecordIdToEntry[bucket];
    while (cacheEntry) {
        if (cacheEntry && (cacheEntry->mRecordID == aRecordID) &&
            (cacheEntry->mCache == aCache))
            return cacheEntry;
        cacheEntry = cacheEntry->mNext;
    }

    return 0;
}

// Add a cache entry to the hash table that maps record ID to entries
void
nsReplacementPolicy::AddCacheEntry(nsCachedNetData* aCacheEntry, PRInt32 aRecordID)
{
    nsCachedNetData** cacheEntryp;
    PRUint32 bucket = HashRecordID(aRecordID);

    cacheEntryp = &mMapRecordIdToEntry[bucket];
    while (*cacheEntryp)
        cacheEntryp = &(*cacheEntryp)->mNext;
    *cacheEntryp = aCacheEntry;
    aCacheEntry->mNext = 0;
}

// Delete a cache entry from the hash table that maps record ID to entries
nsresult
nsReplacementPolicy::DeleteCacheEntry(nsCachedNetData* aCacheEntry)
{
    nsresult rv;
    PRInt32 recordID;
    rv = aCacheEntry->GetRecordID(&recordID);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bucket = HashRecordID(recordID);

    nsCachedNetData** cacheEntryp;
    cacheEntryp = &mMapRecordIdToEntry[bucket];
    while (*cacheEntryp) {
        if (*cacheEntryp == aCacheEntry) {
            *cacheEntryp = aCacheEntry->mNext;
            return NS_OK;
        }
        cacheEntryp = &(*cacheEntryp)->mNext;
    }
    
    NS_ASSERTION(0, "hash table inconsistency");
    return NS_ERROR_FAILURE;
}

nsresult
nsReplacementPolicy::AddAllRecordsInCache(nsINetDataCache *aCache)
{
    nsresult rv;
    nsCOMPtr<nsISimpleEnumerator> iterator;
    nsCOMPtr<nsISupports> iSupports;
    nsCOMPtr<nsINetDataCacheRecord> record;
    rv = aCache->NewCacheEntryIterator(getter_AddRefs(iterator));
    if (!NS_SUCCEEDED(rv)) return rv;

    while (1) {
        PRBool notDone;
        nsCachedNetData *cacheEntry;

        rv = iterator->HasMoreElements(&notDone);
        if (NS_FAILED(rv)) return rv;
        if (!notDone)
            break;

        rv = iterator->GetNext(getter_AddRefs(iSupports));
        if (!NS_SUCCEEDED(rv)) return rv;
        record = do_QueryInterface(iSupports);
        
        rv = AddCacheRecord(record, aCache, &cacheEntry);
        if (!NS_SUCCEEDED(rv)) return rv;
    }

    return NS_OK;
}

// Get current time and convert to seconds since the epoch
static PRUint32
now32()
{
    double nowFP;
    PRInt64 now64 = PR_Now();
    LL_L2D(nowFP, now64);
    PRUint32 now = (PRUint32)(nowFP * 1e-6);
    return now;
}

void
nsCachedNetData::NoteDownloadTime(PRTime start, PRTime end)
{
    double startFP, endFP, rate, duration;

    LL_L2D(startFP, start);
    LL_L2D(endFP, end);

    duration = endFP - startFP;
    // Sanity-check
    if (!duration)
        return;

    // Compute download rate in kB/s
    rate = mLogicalLength / (duration * (1e-6 /1024.0));
    
    // Exponentially smooth download rate
    const double alpha = 0.5;
    mDownloadRate = (float)(mDownloadRate * alpha + rate * (1.0 - alpha));
}

// 1 hour
#define MIN_HALFLIFE (60 * 60)

// 1 week
#define TYPICAL_HALFLIFE (7 * 24 * 60 * 60)

/**
 * Estimate the profit that would be lost if the given cache entry was evicted
 * from the cache.  Profit is defined as the future expected download delay per
 * byte of cached content.  The profit computation is made based on projected
 * frequency of access, prior download performance and a heuristic staleness
 * criteria.  The technique used is a variation of that described in the
 * following paper:
 *
 *    "A Case for Delay-Conscious Caching of Web Documents"
 *    http://www.eecs.nwu.edu/EXTERNAL/dbwww/papers/www97/www97.html
 *
 * Briefly, expected profit is:
 *
 *   (projected frequency of access) * (download time per byte) * (probability freshness)
 */
void
nsCachedNetData::ComputeProfit(PRUint32 aNow)
{
    PRUint32 K, now;

    if (aNow)
        now = aNow;
    else
        now = now32();
    
    K = PR_MIN(MAX_K, mNumAccesses);
    NS_ASSERTION(K, "Internal inconsistency");

    // Compute time, in seconds, since k'th most recent access
    double timeSinceKthAccess = now - mAccessTime[K];
    if (timeSinceKthAccess <= 0.0)    // Sanity check
        timeSinceKthAccess = 1.0;

    // Estimate frequency of future document access based on past
    //  access frequency
    double frequencyAccess = K / timeSinceKthAccess;

    // If we don't have much historical data on access frequency
    //   use a heuristic based on document size as an estimate
    if (mLogicalLength) {
        if (K == 1) {
            frequencyAccess /= pow(mLogicalLength, CACHE_CONST_B);
        } else if (K == 2) {
            frequencyAccess /= pow(mLogicalLength, CACHE_CONST_B / 2);
        }
    }

    // Estimate likelihood that data in cache is fresh, i.e.
    //  that it corresponds to the document on the server
    double probabilityFreshness;
    PRInt32 halfLife, age, docTime;
    bool potentiallyStale;

    docTime = GetFlag(LAST_MODIFIED_KNOWN) ? mLastModifiedTime : mLastUpdateTime;
    age = now - docTime;

    probabilityFreshness = 1.0; // Optimistic

    if (GetFlag(EXPIRATION_KNOWN)) {
        potentiallyStale = now > mExpirationTime;
        halfLife = mExpirationTime - mLastModifiedTime;
    } else if (GetFlag(STALE_TIME_KNOWN)) {
        potentiallyStale = true;
        halfLife = mStaleTime - docTime;
    } else {
        potentiallyStale = true;
        halfLife = TYPICAL_HALFLIFE;
    }
    
    if (potentiallyStale) {
        if (halfLife < MIN_HALFLIFE)
            halfLife = MIN_HALFLIFE;

        probabilityFreshness = pow(0.5, (double)age / (double)halfLife);
    }

    mProfit = (float)(frequencyAccess * probabilityFreshness * mDownloadRate);
}

// Number of entries to grow mRankedEntries array when it's full
#define STATS_GROWTH_INCREMENT  256


// Sorting predicate for NS_Quicksort
int
nsCachedNetData::Compare(const void *a, const void *b, void *unused)
{
    nsCachedNetData& entryA = *(nsCachedNetData*)a;
    nsCachedNetData& entryB = *(nsCachedNetData*)a;

    // Percolate deleted entries to the end of the mRankedEntries array, so that they
    // can be recycled.
    if (entryA.GetFlag(DELETED)) {
        if (entryB.GetFlag(DELETED))
            return 0;
        else 
            return +1;
    } 
    if (entryB.GetFlag(DELETED))
        return -1;

    // Evicted entries (those with no content data) and active entries (those
    // currently being updated) are collected towards the end of the sorted
    // array just prior to the deleted cache entries, since evicted entries
    // can't be re-evicted.
    if (entryA.GetFlag(UNAVAILABLE)) {
        if (entryB.GetFlag(UNAVAILABLE))
            return 0;
        else 
            return +1;
    } 
    if (entryB.GetFlag(UNAVAILABLE))
        return -1;

    // Order cache entries by the number of times they've been accessed
    if (entryA.mNumAccesses < entryB.mNumAccesses)
        return -1;
    if (entryA.mNumAccesses > entryB.mNumAccesses)
        return +1;

    /*
     * Among records that have been accessed an equal number of times, order
     * them by profit.
     */
    if (entryA.mProfit > entryB.mProfit)
        return +1;
    if (entryA.mProfit < entryB.mProfit)
        return -1;
    return 0;
}

/**
 * Rank cache entries in terms of their elegibility for eviction.
 */
nsresult
nsReplacementPolicy::RankRecords()
{
    PRUint32 i, now;

    // Init replacement policy if this is the first ranking
    if (!mLastRankTime) {
        nsresult rv;
        CacheInfo *cacheInfo;

        cacheInfo = mCaches;
        while (cacheInfo) {
            rv = AddAllRecordsInCache(cacheInfo->mCache);
            if (NS_FAILED(rv)) return rv;

            cacheInfo = cacheInfo->mNext;
        }
    }

    // Get current time and convert to seconds since the epoch
    now = now32();

    // Recompute profit for every known cache record, except deleted/evicted ones
    for (i = 0; i < mNumEntries; i++) {
        nsCachedNetData* entry = mRankedEntries[i];
        if (!entry->GetFlag(nsCachedNetData::UNAVAILABLE))
            entry->ComputeProfit(now);
    }
    NS_QuickSort(mRankedEntries, mNumEntries, sizeof *mRankedEntries, nsCachedNetData::Compare, 0);

    mNumEntries -= mRecordsRemovedSinceLastRanking;
    mRecordsRemovedSinceLastRanking = 0;
    mLastRankTime = now;
    return NS_OK;
}

// A heuristic policy to avoid the cost of re-ranking cache records by
// profitability every single time space must be made available in the cache.
void
nsReplacementPolicy::MaybeRerankRecords()
{
    // Rank at most once per minute
    PRUint32 now = now32();
    if ((now - mLastRankTime) >= 60)
        RankRecords();
}

void
nsReplacementPolicy::CompactRankedEntriesArray()
{
    if (mRecordsRemovedSinceLastRanking || !mLastRankTime)
        RankRecords();
}

nsresult
nsReplacementPolicy::CheckForTooManyCacheEntries()
{
    if (mCapacityRankedEntriesArray == mMaxEntries) {
        return DeleteOneEntry(0);
    } else {
        nsresult rv;
        CacheInfo *cacheInfo;

        cacheInfo = mCaches;
        while (cacheInfo) {
            PRUint32 numEntries, maxEntries;

            rv = cacheInfo->mCache->GetNumEntries(&numEntries);
            if (NS_FAILED(rv)) return rv;

            rv = cacheInfo->mCache->GetMaxEntries(&maxEntries);
            if (NS_FAILED(rv)) return rv;

            if (numEntries == maxEntries)
                return DeleteOneEntry(cacheInfo->mCache);

            cacheInfo = cacheInfo->mNext;
        }
    }
    return NS_OK;
}


/** 
 * Add a cache record to the set of entries eligible for eviction from the cache.
 * This would typically be done when the cache entry is created.
 */
nsresult
nsReplacementPolicy::AddCacheRecord(nsINetDataCacheRecord *aRecord,
                                    nsINetDataCache* aCache,
                                    nsCachedNetData** aResult)
{
    nsCachedNetData* cacheEntry;
    nsresult rv;

    // First, see if the record is already known to the replacement policy
    PRInt32 recordID;
    rv = aRecord->GetRecordID(&recordID);
    if (NS_FAILED(rv)) return rv;
    cacheEntry = FindCacheEntryByRecordID(recordID, aCache);
    if (cacheEntry) {
        *aResult = cacheEntry;
        return NS_OK;
    }

    // Compact the array of cache entry statistics, so that free entries appear
    // at the end, for possible reuse.
    if (mNumEntries && (mNumEntries == mCapacityRankedEntriesArray))
        CompactRankedEntriesArray();

    // If compaction doesn't yield available entries in the
    // mRankedEntries array, then extend the array.
    if (mNumEntries == mCapacityRankedEntriesArray) {
        PRUint32 newCapacity;

        rv = CheckForTooManyCacheEntries();
        if (NS_FAILED(rv)) return rv;

        newCapacity = mCapacityRankedEntriesArray + STATS_GROWTH_INCREMENT;
        if (newCapacity > mMaxEntries)
            newCapacity = mMaxEntries;

        nsCachedNetData** newStatsArray; 
        PRUint32 numBytes = sizeof(nsCachedNetData*) * newCapacity;
        newStatsArray = 
            (nsCachedNetData**)nsAllocator::Realloc(mRankedEntries, numBytes);
        if (!newStatsArray)
            return NS_ERROR_OUT_OF_MEMORY;
        
        mRankedEntries = newStatsArray;
        mCapacityRankedEntriesArray = newCapacity;

        PRUint32 i;
        for (i = mNumEntries; i < newCapacity; i++)
            mRankedEntries[i] = 0;
    }

    // Recycle the record after the last in-use record in the array
    nsCachedNetData *entry = mRankedEntries[mNumEntries];
    NS_ASSERTION(!entry || !entry->GetFlag(nsCachedNetData::DELETED),
                 "Only deleted cache entries should appear at end of array");

    if (!entry) {
        entry = (nsCachedNetData*)mArena->Alloc(sizeof nsCachedNetData);
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;
        nsCRT::zero(entry, sizeof nsCachedNetData);
    }

    entry->Init(aRecord, aCache);
    
    // Add one reference to the cache entry from the cache manager
    NS_ADDREF(entry);

    *aResult = entry;
    mNumEntries++;
    return NS_OK;
}

/**
 * Delete the least desirable record from the cache database.  This is used
 * when the addition of another record would exceed either the cache manager or
 * the cache's maximum permitted number of records.
 */
nsresult
nsReplacementPolicy::DeleteOneEntry(nsINetDataCache *aCache)
{
    PRUint32 i;
    nsresult rv;
    nsCachedNetData *entry;

    i = 0;
    while (1) {
        MaybeRerankRecords();
        for (; i < mNumEntries; i++) {
            entry = mRankedEntries[i];
            if (entry->GetFlag(nsCachedNetData::DELETED))
                continue;
            if (!aCache || (entry->mCache == aCache))
                break;
        }

        // Report error if no record found to delete
        if (i == mNumEntries)
            return NS_ERROR_FAILURE;
        rv = entry->Delete();
        if (NS_SUCCEEDED(rv)) {
            rv = DeleteCacheEntry(entry);
            return rv;
        }
    }
}

nsresult
nsReplacementPolicy::GetStorageInUse(PRUint32* aStorageInUse)
{
    nsresult rv;
    CacheInfo *cacheInfo;

    *aStorageInUse = 0;
    cacheInfo = mCaches;
    while (cacheInfo) {
        PRUint32 cacheStorage;
        rv = cacheInfo->mCache->GetStorageInUse(&cacheStorage);
        if (NS_FAILED(rv)) return rv;

        *aStorageInUse += cacheStorage;
        cacheInfo = cacheInfo->mNext;
    }
    return NS_OK;
}

/**
 * Delete the least desirable records from the cache until the occupancy of the
 * cache has been reduced by the given number of KB.  This is used when the
 * addition of more cache data would exceed the cache's capacity. 
 */
nsresult
nsReplacementPolicy::Evict(PRUint32 aTargetOccupancy)
{
    PRUint32 i;
    nsCachedNetData *entry;
    nsresult rv;
    PRUint32 occupancy, truncatedLength;

    MaybeRerankRecords();
    for (i = 0; i < mNumEntries; i++) {
        rv = GetStorageInUse(&occupancy);
        if (!NS_SUCCEEDED(rv)) return rv;
        
        if (occupancy <= aTargetOccupancy)
            return NS_OK;

        entry = mRankedEntries[i];

        // Skip deleted cache entries and ones that have already been evicted
        if (entry->GetFlag(nsCachedNetData::UNAVAILABLE))
            continue;

        if (entry->GetFlag(nsCachedNetData::ALLOW_PARTIAL)) {
            truncatedLength = aTargetOccupancy - occupancy;
            if (truncatedLength < 0)
                truncatedLength = 0;
        } else {
            truncatedLength = 0;
        }
        rv = entry->Evict(truncatedLength);
    }
    return NS_ERROR_FAILURE;
}
