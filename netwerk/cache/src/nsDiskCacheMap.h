/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is nsDiskCacheMap.h, released March 23, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Patrick C. Beard <beard@netscape.com>
 *    Gordon Sheridan  <gordon@netscape.com>
 */

#ifndef _nsDiskCacheMap_h_
#define _nsDiskCacheMap_h_

#include "prtypes.h"
#include "prnetdb.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsILocalFile.h"
#include "nsANSIFileStreams.h"

class nsIInputStream;
class nsIOutputStream;

/**
 *   Cache Location Format
 *
 *    0011 0000 0000 0000 0000 0000 0000 0000 : File Selector (0 = separate file)
 *    0000 0011 0000 0000 0000 0000 0000 0000 : number of extra contiguous blocks 1-4
 *    1100 1100 0000 0000 0000 0000 0000 0000 : reserved bits
 *    0000 0000 1111 1111 1111 1111 1111 1111 : block#  0-16777216 (2^24)
 *
 *    0000 0000 1111 1111 1111 1111 0000 0000 : eFileReservedMask
 *    0000 0000 0000 0000 0000 0000 1111 1111 : eFileGenerationMask
 *
 *  File Selector:
 *      0 = separate file on disk
 *      1 = 256 byte block file
 *      2 = 1k block file
 *      3 = 4k block file
 */
 
 
/******************************************************************************
 *  nsDiskCacheRecord
 *****************************************************************************/
class nsDiskCacheRecord {

private:
    PRUint32    mHashNumber;
    PRUint32    mEvictionRank;
    PRUint32    mLocation;
    PRUint32    mMetaLocation;
 
    enum {
        eLocationSelectorMask   = 0x30000000,
        eLocationSelectorOffset = 28,
        
        eExtraBlocksMask        = 0x03000000,
        eExtraBlocksOffset      = 24,
        
        eReservedMask           = 0xCC000000,
        
        eBlockNumberMask        = 0x00FFFFFF,

        eFileReservedMask       = 0x00FFFF00,
        eFileGenerationMask     = 0x000000FF
    };

public:
    nsDiskCacheRecord()
        :   mHashNumber(0), mEvictionRank(0), mLocation(0), mMetaLocation(0)
    {
    }
    
    PRUint32   HashNumber() const
    {
        return mHashNumber;
    }
    
    void       SetHashNumber(PRUint32 hashNumber)
    {
        mHashNumber = hashNumber;
    }

    PRUint32   EvictionRank() const
    {
        return mEvictionRank;
    }

    void       SetEvictionRank(PRUint32 rank)
    {
        mEvictionRank = rank;
    }

    PRUint32   LocationSelector() const
    {
        return (PRUint32)(mLocation & eLocationSelectorMask) >> eLocationSelectorOffset;
    }

    void       SetLocationSelector(PRUint32 selector)
    {
        mLocation &= ~eLocationSelectorMask; // clear location selector bits
        mLocation |= (selector & eLocationSelectorMask) << eLocationSelectorOffset;
    }

    PRUint32   BlockCount() const
    {
        return (PRUint32)((mLocation & eExtraBlocksMask) >> eExtraBlocksOffset) + 1;
    }

    void       SetBlockCount(PRUint32 count)
    {
        NS_ASSERTION( (count>=1) && (count<=4),"invalid block count");
        count = --count;
        mLocation &= ~eExtraBlocksMask; // clear extra blocks bits
        mLocation |= (count & eExtraBlocksMask) << eExtraBlocksOffset;
    }

    PRUint32   BlockNumber() const
    {
        return (mLocation & eBlockNumberMask);
    }

    void       SetBlockNumber(PRUint32  blockNumber)
    {
        mLocation &= ~eBlockNumberMask;  // clear block number bits
        mLocation |= blockNumber & eBlockNumberMask;
    }

    PRUint16   FileGeneration() const
    {
        return (mLocation & eFileGenerationMask);
    }

    void       SetFileGeneration(PRUint16 generation)
    {
        mLocation &= ~eFileGenerationMask;  // clear file generation bits
        mLocation |= generation & eFileGenerationMask;
    }

    void        Swap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHashNumber   = ::PR_htonl(mHashNumber);
        mEvictionRank = ::PR_htonl(mEvictionRank);
        mLocation     = ::PR_htonl(mLocation);
        mMetaLocation = ::PR_htonl(mMetaLocation);
#endif
    }
    
    void        Unswap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHashNumber   = ::PR_ntohl(mHashNumber);
        mEvictionRank = ::PR_ntohl(mEvictionRank);
        mLocation     = ::PR_ntohl(mLocation);
        mMetaLocation = ::PR_ntohl(mMetaLocation);
#endif
    }

};


/******************************************************************************
 *  nsDiskCacheRecordVisitor
 *****************************************************************************/

enum {  kDeleteRecordAndContinue = -1,
        kStopVisitingRecords     =  0,
        kVisitNextRecord         =  1
};

class nsDiskCacheRecordVisitor {
    PRInt32  VisitRecord(nsDiskCacheRecord *  mapRecord);
};


/******************************************************************************
 *  nsDiskCacheBucket
 *****************************************************************************/
enum {
    kRecordsPerBucket = 256
};

struct nsDiskCacheBucket {
    nsDiskCacheRecord   mRecords[kRecordsPerBucket];
};


/******************************************************************************
 *  nsDiskCacheHeader
 *****************************************************************************/
enum { kCurrentVersion = 0x00010002 };

struct nsDiskCacheHeader {
    PRUint32    mVersion;                           // cache version.
    PRUint32    mDataSize;                          // size of cache in bytes.
    PRUint32    mEntryCount;                        // number of entries stored in cache.
    PRUint32    mIsDirty;                           // dirty flag.
    
    // pad to blocksize
    PRUint8     reserved[sizeof(nsDiskCacheBucket) - 4 * sizeof(PRUint32)];

    // XXX need a bitmap?
    
    nsDiskCacheHeader()
        :   mVersion(kCurrentVersion), mDataSize(0),
            mEntryCount(0), mIsDirty(PR_TRUE)
    {
    }

    void        Swap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mVersion    = ::PR_htonl(mVersion);
        mDataSize   = ::PR_htonl(mDataSize);
        mEntryCount = ::PR_htonl(mEntryCount);
        mIsDirty    = ::PR_htonl(mIsDirty);
#endif
    }
    
    void        Unswap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mVersion    = ::PR_ntohl(mVersion);
        mDataSize   = ::PR_ntohl(mDataSize);
        mEntryCount = ::PR_ntohl(mEntryCount);
        mIsDirty    = ::PR_ntohl(mIsDirty);
#endif
    }
};


/******************************************************************************
 *  nsDiskCacheMap
 *
 *  // XXX initial capacity, enough for 8192 distinct entries.
 *****************************************************************************/

enum {
    kBucketsPerTable = (1 << 5),                 // must be a power of 2!
    kCacheMapSize = sizeof(nsDiskCacheHeader) +
                    kBucketsPerTable * sizeof(nsDiskCacheBucket)
};


class nsDiskCacheMap {
public:
    nsDiskCacheMap();
    ~nsDiskCacheMap();

/**
 *  Open
 *
 *  Creates a new cache map file if one doesn't exist.
 *  Returns error if it detects change in format or cache wasn't closed.
 */
//  nsresult Open(nsIFile *  cacheDirectory);
    nsresult Open(nsILocalFile * mapFile);
    nsresult Close();

//  nsresult Flush();

/**
 *  Record operations
 *
 *  AddRecord - 
 */
    nsresult AddRecord( nsDiskCacheRecord *  mapRecord, nsDiskCacheRecord * oldRecord);
    nsresult UpdateRecord( nsDiskCacheRecord *  mapRecord);
    nsresult FindRecord( PRUint32  hashNumber, nsDiskCacheRecord *  mapRecord);
    nsresult DeleteRecord2( nsDiskCacheRecord *  mapRecord);
    nsresult EvictRecords( nsDiskCacheRecordVisitor *  visitor);

//private:


    void Reset();
    
    PRUint32& DataSize()   { return mHeader.mDataSize; }
    PRUint32& EntryCount() { return mHeader.mEntryCount; }
    PRUint32& IsDirty()    { return mHeader.mIsDirty; }
    
    nsDiskCacheRecord* GetRecord(PRUint32 hashNumber);
    void DeleteRecord(nsDiskCacheRecord* record);
    
    nsresult GetBucketForHashNumber(PRUint32  hashNumber, nsDiskCacheBucket ** result);

    nsDiskCacheRecord* GetBucket(PRUint32 index)
    {
        return mBuckets[index].mRecords;
    }
    
    PRUint32 GetBucketIndex(PRUint32 hashNumber)
    {
        return (hashNumber & (kBucketsPerTable - 1));
    }
    
    PRUint32 GetBucketIndex(nsDiskCacheRecord* record)
    {
        return GetBucketIndex(record->HashNumber());
    }
    
    nsresult Read(nsIInputStream* input);
    nsresult Write(nsIOutputStream* output);
    
    nsresult ReadBucket(nsIInputStream* input, PRUint32 index);
    nsresult WriteBucket(nsIOutputStream* output, PRUint32 index);
    
    nsresult ReadHeader(nsIInputStream* input);
    nsresult WriteHeader(nsIOutputStream* output);
    
private:
    nsANSIFileStream *      mStream;    
    nsDiskCacheHeader       mHeader;
    nsDiskCacheBucket       mBuckets[kBucketsPerTable];
};

#endif // _nsDiskCacheMap_h_
