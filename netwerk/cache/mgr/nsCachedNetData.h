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

#ifndef _nsCachedNetData_h_
#define _nsCachedNetData_h_

#include "nsICachedNetData.h"
#include "nsCOMPtr.h"
#include "nsINetDataCacheRecord.h"

class nsINetDataCache;
class nsIStreamAsFileObserver;
class nsIStreamAsFile;
class StreamAsFileObserverClosure;

// Number of recent access times recorded
#define MAX_K  3

/**
 * FIXME - add comment.  There are a lot of these data structures resident in
 * memory, so be careful about adding members unnecessarily.
 */
class nsCachedNetData : public nsICachedNetData {

public:
    ~nsCachedNetData();

    NS_DECL_ISUPPORTS

    // nsICachedNetData methods
    NS_DECL_NSICACHEDNETDATA

    NS_METHOD Init(nsINetDataCacheRecord *aRecord, nsINetDataCache *aCache);

protected:

    // Bits for mFlags, below
    typedef enum {
        ALLOW_PARTIAL       = 1 << 0, // Protocol supports partial fetching
        TRUNCATED_CONTENT   = 1 << 1, // Cache manager truncated content
        UPDATE_IN_PROGRESS  = 1 << 2, // Protocol handler is modifying cache data
        DIRTY               = 1 << 3, // Cache entry data needs to be flushed to database

        DELETED             = 1 << 4, // Associated database record is deleted;  This
                                      //   cache entry is available for recycling
        EVICTED             = 1 << 5, // All cache entry content data has been purged,
                                      //   though cache entry remains
        DORMANT             = 1 << 6, // No references to this cache entry, except the
                                      //   cache manager itself

        LAST_MODIFIED_KNOWN = 1 << 7, // Protocol has called SetLastModifiedTime()
        EXPIRATION_KNOWN    = 1 << 8, // Protocol has called SetExpirationTime()
        STALE_TIME_KNOWN    = 1 << 9, // Protocol has called SetStaleTime()

        UNAVAILABLE         = DELETED | EVICTED | UPDATE_IN_PROGRESS
    } Flag;

    PRBool GetFlag(Flag aFlag) {
        if (mFlags & DELETED)
            return NS_ERROR_NOT_AVAILABLE;
        return (mFlags & aFlag) != 0;
    }
    nsresult GetFlag(PRBool *aResult, Flag aFlag) { *aResult = GetFlag(aFlag); return NS_OK; }

    // Set a boolean flag for the cache entry
    nsresult SetFlag(PRBool aValue, Flag aFlag) {
        if (mFlags & DELETED)
            return NS_ERROR_NOT_AVAILABLE;
        PRUint8 newFlags = mFlags | (-(aValue != 0) & aFlag);
        if (newFlags != mFlags)
            newFlags |= DIRTY;
        mFlags = newFlags;
        return NS_OK;
    }

    nsresult SetFlag(Flag aFlag)   { return SetFlag(PR_TRUE,  aFlag); }
    nsresult ClearFlag(Flag aFlag) { return SetFlag(PR_FALSE, aFlag); }

    void ComputeProfit(PRUint32 aCurrentTime);
    static int Compare(const void *a, const void *b, void *unused);

    void NoteAccess();
    void NoteUpdate();

    // Get underlying raw cache database record.
    nsresult GetRecord(nsINetDataCacheRecord* *aRecord);

    nsresult GetRecordID(PRInt32 *aRecordID);

    nsresult Evict(PRUint32 aTruncatedContentLength);
    
    nsresult GetFileSpec(nsIFileSpec* *aFileSpec);

    void NoteDownloadTime(PRTime start, PRTime end);

    friend class nsReplacementPolicy;
    friend class nsCacheManager;
    friend class StreamAsFile;
    friend class nsCacheEntryChannel;
    friend class CacheOutputStream;

private:

    // Constructor should never be invoked since this class is arena-allocated 
    nsCachedNetData();

    // Initialize internal fields of this nsCachedNetData instance from the
    // underlying raw cache database record.
    nsresult Deserialize(void);

    // Notify observers about change in cache entry status
    nsresult Notify(PRUint32 aMessage, nsresult aError);

    nsresult AddObserver(nsIStreamAsFile *aStreamAsFile, nsIStreamAsFileObserver* aObserver);
    nsresult RemoveObserver(nsIStreamAsFileObserver* aObserver);

    void SetDirty() { mFlags |= DIRTY; }

private:
    
    // List of nsIStreamAsFileObserver's that will receive notification events
    // when the cache manager or a client desires to delete/truncate a cache
    // entry file.
    StreamAsFileObserverClosure* mObservers;

    // Protocol-specific meta-data, opaque to the cache manager
    char*       mProtocolData;
    PRUint32    mProtocolDataLength;

    // Next in chain for a single bucket in the replacement policy hash table
    // that maps from record ID to nsCachedNetData
    nsCachedNetData* mNext;
    
    // See flag bits, above
    // NOTE: 16 bit member is combined with 16-bit mNumAccesses below for
    //       struct packing efficiency.  Do not change order of members!
    PRUint16    mFlags;

protected:

    // Below members are statistics kept per cache-entry, used to decide how
    // profitable it will be to evict a record from the cache relative to other
    // existing records.  Note: times are measured in *seconds* since the
    // 1/1/70 epoch, same as a unix time_t.

    // Number of accesses for this cache record
    // NOTE: 16 bit member is combined with 16-bit mFlags above for
    //       struct packing efficiency.  Do not change order of members!
    PRUint16    mNumAccesses;

    // A reference to the underlying, raw cache database record, either as a
    // pointer to an in-memory object or as a database identifier
    union {
        nsINetDataCacheRecord* mRecord;

        // Database record ID of associated cache record.  See
        // nsINetDataCache::GetRecordByID().
        PRInt32                mRecordID;
    };

    // Weak link to parent cache
    nsINetDataCache* mCache;

    // Length of stored content, which may be less than storage consumed if
    // compression is used
    PRUint32 mLogicalLength;

    // Most recent cache entry access times, used to compute access frequency
    PRUint32 mAccessTime[MAX_K];
    
    // We use modification time of the original document for replacement policy
    // computations, i.e. to compute a document's age, but if we don't know it,
    // we use the time that the document was last written to the cache.
    union {
        // Document modification time, if known.
        PRUint32 mLastModifiedTime;

        // Time of last cache update for this doc
        PRUint32 mLastUpdateTime;
    };

    union {
      // Time until which document is fresh, i.e. does not have to be validated
      // with server and, therefore, data in cache is guaranteed usable
      PRUint32 mExpirationTime;

      // Heuristic time at which cached document is likely to be out-of-date
      // with respect to canonical copy on server.  Used for cache replacement
      // policy, not for validation.
      PRUint32 mStaleTime;
    };

    // Download time per byte, measure roughly in units of KB/s
    float mDownloadRate;
    
    // Heuristic estimate of cache entry future benefits, based on above values
    float mProfit;
};

#endif // _nsCachedNetData_h_

