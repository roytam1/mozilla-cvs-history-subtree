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
 * The Original Code is nsDiskCacheMap.cpp, released March 23, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Patrick C. Beard <beard@netscape.com>
 */

#include "nsDiskCacheMap.h"
#include "nsIFileStreams.h"

#include <string.h>

nsDiskCacheMap::nsDiskCacheMap()
{
}

nsDiskCacheMap::~nsDiskCacheMap()
{
}

void nsDiskCacheMap::Reset()
{
    mHeader.mDataSize = 0;
    mHeader.mEntryCount = 0;
    mHeader.mIsDirty = PR_TRUE;

    for (PRUint32 i = 1; i < kBucketsPerTable; ++i) {
        nsDiskCacheBucket& bucket = mBuckets[i];
        ::memset(&bucket, 0, sizeof(nsDiskCacheBucket));
    }
}

nsDiskCacheRecord* nsDiskCacheMap::GetRecord(PRUint32 hashNumber)
{
    nsDiskCacheBucket& bucket = mBuckets[GetBucketIndex(hashNumber)];
    nsDiskCacheRecord* oldestRecord = &bucket.mRecords[0];

    for (int r = 0; r < kRecordsPerBucket; ++r) {
        nsDiskCacheRecord* record = &bucket.mRecords[r];
        if (record->HashNumber() == 0 || record->HashNumber() == hashNumber)
            return record;
        if (record->EvictionRank() < oldestRecord->EvictionRank())
            oldestRecord = record;
    }
    // if we don't find an empty record, return the oldest record for eviction.
    return oldestRecord;
}


void nsDiskCacheMap::DeleteRecord(nsDiskCacheRecord* deletedRecord)
{
    PRUint32 hashNumber = deletedRecord->HashNumber();
    nsDiskCacheBucket& bucket = mBuckets[GetBucketIndex(hashNumber)];
    NS_ASSERTION(deletedRecord >= &bucket.mRecords[0] &&
                 deletedRecord < &bucket.mRecords[kRecordsPerBucket],
                 "invalid record to delete.");
    nsDiskCacheRecord* limit = &bucket.mRecords[kRecordsPerBucket];
    nsDiskCacheRecord* lastRecord = nsnull;
    // XXX use binary search to find the end, much quicker.
    // find the last record, to fill in the deleted record.
    for (nsDiskCacheRecord* record = deletedRecord + 1; record < limit; ++record) {
        if (record->HashNumber() == 0) {
            lastRecord = record - 1;
            break;
        }
    }
    // copy the last record, to the newly deleted record.
    if (lastRecord && deletedRecord != lastRecord) {
        *deletedRecord = *lastRecord;
        deletedRecord = lastRecord;
    }
    // mark record as free.
    deletedRecord->SetHashNumber(0);
    // reduce the number of entries.
    mHeader.mEntryCount--;
}


nsresult nsDiskCacheMap::Read(nsIInputStream* input)
{
    nsresult rv;
    PRUint32 count;

    // seek to beginning of the file.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(input, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;

    // read the header.
    nsDiskCacheHeader header;
    rv = input->Read((char*)&header, sizeof(header), &count);
    if (count != sizeof(header)) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;
    header.Unswap();
    
    // validate the version.
    if (header.mVersion != nsDiskCacheHeader::kCurrentVersion) return NS_ERROR_FAILURE;
    mHeader = header;

    // seek to beginning of first bucket.
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, sizeof(nsDiskCacheBucket));
    if (NS_FAILED(rv)) return rv;

    // read the buckets.
    rv = input->Read((char*)&mBuckets[1], sizeof(mBuckets) - sizeof(nsDiskCacheBucket), &count);
    if (count != (sizeof(mBuckets) - sizeof(nsDiskCacheBucket))) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;
    
    // unswap all of the active records.
    for (int b = 1; b < kBucketsPerTable; ++b) {
        nsDiskCacheBucket& bucket = mBuckets[b];
        for (int r = 0; r < kRecordsPerBucket; ++r) {
            nsDiskCacheRecord* record = &bucket.mRecords[r];
            if (record->HashNumber() == 0)
                break;
            record->Unswap();
        }
    }
    
    return NS_OK;
}

nsresult nsDiskCacheMap::Write(nsIOutputStream* output)
{
    nsresult rv;
    PRUint32 count;
    int b;
    
    // swap all of the active records.
    for (b = 1; b < kBucketsPerTable; ++b) {
        nsDiskCacheBucket& bucket = mBuckets[b];
        for (int r = 0; r < kRecordsPerBucket; ++r) {
            nsDiskCacheRecord* record = &bucket.mRecords[r];
            if (record->HashNumber() == 0)
                break;
            record->Swap();
        }
    }
    
    // seek back to beginning of file.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(output, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;

    // write the buckets.
    rv = output->Write((char*)&mBuckets, sizeof(mBuckets), &count);

    // unswap all of the active records.
    for (b = 1; b < kBucketsPerTable; ++b) {
        nsDiskCacheBucket& bucket = mBuckets[b];
        for (int r = 0; r < kRecordsPerBucket; ++r) {
            nsDiskCacheRecord* record = &bucket.mRecords[r];
            if (record->HashNumber() == 0)
                break;
            record->Unswap();
        }
    }
    if (count != sizeof(mBuckets)) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;

    // seek back to beginning of file.
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;
    
    // write the header.
    nsDiskCacheHeader header = mHeader;
    header.Swap();
    rv = output->Write((char*)&header, sizeof(header), &count);
    if (count != sizeof(header)) return NS_ERROR_FAILURE;

    // flush the write.
    output->Flush();
    
    return rv;
}

nsresult nsDiskCacheMap::WriteBucket(nsIOutputStream* output, PRUint32 index)
{
    nsresult rv;
    
    // can only do this if the stream is seekable.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(output, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, index * sizeof(nsDiskCacheBucket));
    if (NS_FAILED(rv)) return rv;
    
    nsDiskCacheBucket& bucket = mBuckets[index];
    
    // swap all of the active records.
    {
        for (int r = 0; r < kRecordsPerBucket; ++r) {
            nsDiskCacheRecord* record = &bucket.mRecords[r];
            if (record->HashNumber() == 0)
                break;
            record->Swap();
        }
    }

    PRUint32 count;
    rv = output->Write((char*)&bucket, sizeof(nsDiskCacheBucket), &count);
    output->Flush();

    // unswap all of the active records.
    {
        for (int r = 0; r < kRecordsPerBucket; ++r) {
            nsDiskCacheRecord* record = &bucket.mRecords[r];
            if (record->HashNumber() == 0)
                break;
            record->Unswap();
        }
    }

    NS_ASSERTION(count == sizeof(nsDiskCacheBucket), "nsDiskCacheMap::WriteBucket failed");
    if (count != sizeof(nsDiskCacheBucket)) return NS_ERROR_FAILURE;
    
    return rv;
}

nsresult nsDiskCacheMap::WriteHeader(nsIOutputStream* output)
{
    nsresult rv;
    PRUint32 count;
    
    // can only do this if the stream is seekable.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(output, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;

    // write the header.
    nsDiskCacheHeader header = mHeader;
    header.Swap();
    rv = output->Write((char*)&header, sizeof(header), &count);
    output->Flush();
    if (count != sizeof(header)) return NS_ERROR_FAILURE;
    
    return rv;
}
