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
 *    Gordon Sheridan  <gordon@netscape.com>
 */

#include "nsDiskCacheMap.h"
#include "nsDiskCacheEntry.h"

#include "nsCRT.h"

#include <string.h>


/******************************************************************************
 *  nsDiskCacheBucket
 *****************************************************************************/

void
nsDiskCacheBucket::Swap()
{
    nsDiskCacheRecord * record = &mRecords[0];
    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (record->HashNumber() == 0)
            break;
        record->Swap();
    }
}

void
nsDiskCacheBucket::Unswap()
{
    nsDiskCacheRecord * record = &mRecords[0];
    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (record->HashNumber() == 0)
            break;
        record->Unswap();
    }
}


PRUint32
nsDiskCacheBucket::CountRecords()
{
    if (mRecords[0].HashNumber() == 0)  return 0;
    
    PRUint32 i      = kRecordsPerBucket << 1;
    PRUint32 offset = kRecordsPerBucket << 2;
    
    while (offset > 0) {
        if (mRecords[i].HashNumber())  i += offset;
        else                           i -= offset;
        offset <<= 1;
    }
    
    if (mRecords[i].HashNumber() != 0)
        ++i;
    
    return i;
}


PRInt32
nsDiskCacheBucket::VisitEachRecordInBucket(nsDiskCacheRecordVisitor * visitor,
                                           PRBool *                   dirty)
{
    PRInt32  count = CountRecords();
    *dirty = PR_FALSE;
    
    if (count == 0) return kVisitNextRecord;    // bucket is empty
    
    // XXX call visitor for each entry
    PRInt32 i = count - 1;
    PRInt32 result = visitor->VisitRecord(&mRecords[i]);
    
    return result;
}



/******************************************************************************
 *  nsDiskCacheMap
 *****************************************************************************/

/**
 *  File operations
 */

nsresult
nsDiskCacheMap::Open(nsILocalFile *  cacheDirectory)
{
    NS_ENSURE_ARG_POINTER(cacheDirectory);
    if (mMapFD)  return NS_ERROR_ALREADY_INITIALIZED;

    mCacheDirectory = cacheDirectory;   // save a reference for ourselves
    
    // create nsILocalFile for _CACHE_MAP_
    nsresult rv;
    nsCOMPtr<nsIFile> file;
    rv = cacheDirectory->Clone(getter_AddRefs(file));
    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(file, &rv));
    if (NS_FAILED(rv))  return rv;
    rv = localFile->Append("_CACHE_MAP_");
    if (NS_FAILED(rv))  return rv;
    
    // open the file
    rv = localFile->OpenNSPRFileDesc(PR_RDWR | PR_CREATE_FILE, 00666, &mMapFD);
    if (NS_FAILED(rv))  return rv;  // unable to open or create file
    
    // check size of map file
    PRUint32  mapSize = PR_Available(mMapFD);
    if (mapSize < 0) {
        rv = NS_ERROR_UNEXPECTED;
        goto error_exit;
    }
    
    if (mapSize == 0) {
        // create the file - initialize in memory
        mHeader.mVersion    = nsDiskCache::kCurrentVersion;
        mHeader.mDataSize   = 0;
        mHeader.mEntryCount = 0;
        mHeader.mIsDirty = PR_TRUE;

        nsCRT::zero(mHeader.reserved, nsDiskCacheHeader::kReservedBytes);
        nsCRT::zero(mBuckets, sizeof(nsDiskCacheBucket) * kBucketsPerTable);
        
    } else if (mapSize == kCacheMapSize) {
        // read it in
        PRUint32 bytesRead = PR_Read(mMapFD, &mHeader, kCacheMapSize);
        if (kCacheMapSize != bytesRead) {
            rv = NS_ERROR_UNEXPECTED;
            goto error_exit;
        }
        mHeader.Unswap();
        if (mHeader.mIsDirty || mHeader.mVersion != nsDiskCache::kCurrentVersion) {
            rv = NS_ERROR_FILE_CORRUPTED;
            goto error_exit;
        }
        
        // Unswap each bucket
        for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
            mBuckets[i].Unswap();
        }
        
        // XXX verify entry count, check size(?)
        
    } else {
        rv = NS_ERROR_FILE_CORRUPTED;
        goto error_exit;
    }

    rv = OpenBlockFiles();
    if (NS_FAILED(rv))  goto error_exit;

    // set dirty bit and flush header
    mHeader.mIsDirty    = PR_TRUE;
    rv = FlushHeader();
    if (NS_FAILED(rv))  goto error_exit;
    
    return NS_OK;
    
error_exit:
    // XXX close block files

    if (mMapFD) {
        (void) PR_Close(mMapFD);
        mMapFD = nsnull;
    }
    
    return rv;
}


nsresult
nsDiskCacheMap::Close()
{
    if (!mMapFD)  return NS_OK;

    // close block files
    for (int i = 0; i < 3; ++i) {
        (void) mBlockFile[i].Close();       // XXX report rv
    }

    // write map record buckets
    nsresult rv = FlushBuckets(PR_FALSE);   // don't bother swapping buckets back
    if (NS_FAILED(rv))  goto exit;
    
    // clear dirty bit
    mHeader.mIsDirty    = PR_FALSE;

    rv = FlushHeader();

exit:
    PRStatus err = PR_Close(mMapFD);
    mMapFD = nsnull;
    
    if (NS_FAILED(rv))  return rv;
    return err == PR_SUCCESS ? NS_OK : NS_ERROR_UNEXPECTED;
}


nsresult
nsDiskCacheMap::FlushHeader()
{
    if (!mMapFD)  return NS_ERROR_NOT_AVAILABLE;
    
    // seek to beginning of cache map
    PRInt32 filePos = PR_Seek(mMapFD, 0, PR_SEEK_SET);
    if (filePos != 0)  return NS_ERROR_UNEXPECTED;
    
    // write the header
    mHeader.Swap();
    PRInt32 bytesWritten = PR_Write(mMapFD, &mHeader, sizeof(nsDiskCacheHeader));
    mHeader.Unswap();
    if (sizeof(nsDiskCacheHeader) != bytesWritten) {
        return NS_ERROR_UNEXPECTED;
    }
    
    return NS_OK;
}


nsresult
nsDiskCacheMap::FlushBuckets(PRBool unswap)
{
    if (!mMapFD)  return NS_ERROR_NOT_AVAILABLE;
    
    // seek to beginning of buckets
    PRInt32 filePos = PR_Seek(mMapFD, sizeof(nsDiskCacheHeader), PR_SEEK_SET);
    if (filePos != sizeof(nsDiskCacheHeader))  return NS_ERROR_UNEXPECTED;
    
    // Swap each bucket
    for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
        mBuckets[i].Swap();
    }
    
    PRInt32 bytesWritten = PR_Write(mMapFD, &mBuckets, sizeof(nsDiskCacheBucket) * kBucketsPerTable);

    if (unswap) {
        // Unswap each bucket
        for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
            mBuckets[i].Unswap();
        }
    }

    if (sizeof(nsDiskCacheHeader) != bytesWritten) {
        return NS_ERROR_UNEXPECTED;
    }
    
    return NS_OK;
}


/**
 *  Record operations
 */

nsresult
nsDiskCacheMap::AddRecord( nsDiskCacheRecord *  mapRecord,
                           nsDiskCacheRecord *  oldRecord)
{
    PRUint32            hashNumber = mapRecord->HashNumber();
    nsDiskCacheBucket * bucket;
    nsresult            rv = GetBucketForHashNumber(hashNumber, &bucket);
    if (NS_FAILED(rv))  return rv;
    
    nsDiskCacheRecord * mostEvictable = &bucket->mRecords[0];
    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (bucket->mRecords[i].HashNumber() == 0) {
            // stick the new record here
            bucket->mRecords[i] = *mapRecord;
            oldRecord->SetHashNumber(0);  // signify no record
            return NS_OK;
        }
        
        if (bucket->mRecords[i].EvictionRank() > mostEvictable->EvictionRank())
            mostEvictable = &bucket->mRecords[i];
    }
    
    *oldRecord = *mostEvictable;    // i == kRecordsPerBucket, so evict the mostEvictable
    *mostEvictable = *mapRecord;    // replace it with the new record
    
    // XXX recalc mostEvictable
    return NS_OK;
}


nsresult
nsDiskCacheMap::UpdateRecord( nsDiskCacheRecord *  mapRecord)
{
    PRUint32            hashNumber = mapRecord->HashNumber();
    nsDiskCacheBucket * bucket;
    nsresult            rv = GetBucketForHashNumber(hashNumber, &bucket);
    if (NS_FAILED(rv))  return rv;

    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (bucket->mRecords[i].HashNumber() == mapRecord->HashNumber()) {
            // stick the new record here
            bucket->mRecords[i] = *mapRecord;
            return NS_OK;
        }
    }
    return NS_ERROR_UNEXPECTED;
}


nsresult
nsDiskCacheMap::FindRecord( PRUint32  hashNumber, nsDiskCacheRecord *  result)
{
    nsDiskCacheBucket * bucket;
    nsresult rv = GetBucketForHashNumber(hashNumber, &bucket);
    if (NS_FAILED(rv))  return rv;
    
    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (bucket->mRecords[i].HashNumber() == 0)  break;
            
        if (bucket->mRecords[i].HashNumber() == hashNumber) {
            *result = bucket->mRecords[i];    // copy the record
            return NS_OK;
        }
    }
    return NS_ERROR_CACHE_KEY_NOT_FOUND;
}


nsresult
nsDiskCacheMap::DeleteRecord( nsDiskCacheRecord *  mapRecord)
{
    nsDiskCacheBucket * bucket;
    nsresult rv = GetBucketForHashNumber(mapRecord->HashNumber(), &bucket);
    if (NS_FAILED(rv))  return rv;
    
    PRUint32 count = bucket->CountRecords();
    for (int i = 0; i < count; ++i) {        
        if (bucket->mRecords[i].HashNumber() == mapRecord->HashNumber()) {
            // found it, now delete it.
            if (i != (count - 1)) { // if not the last record, shift last record into opening
                bucket->mRecords[i] = bucket->mRecords[count - 1];
            }
            bucket->mRecords[count - 1].SetHashNumber(0);   // clear last record
            return NS_OK;
        }
    }
    return NS_ERROR_UNEXPECTED;
}


nsresult
nsDiskCacheMap::VisitRecords( nsDiskCacheRecordVisitor *  visitor)
{
    for (PRUint32 i = 0; i < kBucketsPerTable; ++i) {
        // get bucket
        PRBool dirty;
        PRBool continueFlag = mBuckets[i].VisitEachRecordInBucket(visitor, &dirty);
        if (dirty) {
            // XXX write bucket
        }
    }
    
    return NS_OK;
}


nsresult
nsDiskCacheMap::OpenBlockFiles()
{
    // create nsILocalFile for block file
    nsCOMPtr<nsILocalFile> blockFile;
    nsresult rv;
    
    for (int i = 0; i < 3; ++i) {
        rv = GetBlockFileForIndex(i, getter_AddRefs(blockFile));
        if (NS_FAILED(rv))  goto error_exit;
    
        PRUint32 blockSize = GetBlockSizeForIndex(i);
        rv = mBlockFile[i].Open(blockFile, blockSize);
        if (NS_FAILED(rv)) goto error_exit;
    }
    return NS_OK;

error_exit:
    (void)CloseBlockFiles();        // we already have an error to report
    return rv;
}


nsresult
nsDiskCacheMap::CloseBlockFiles()
{
    nsresult rv, rv2 = NS_OK;
    for (int i=0; i < 3; ++i) {
        rv = mBlockFile[i].Close();
        if (NS_FAILED(rv))  rv2 = rv;   // if one or more errors, report at least one
    }
    return rv2;
}


nsresult
nsDiskCacheMap::ReadDiskCacheEntry(nsDiskCacheRecord * record, nsDiskCacheEntry ** result)
{
    nsresult            rv;
    nsDiskCacheEntry *  diskEntry  = nsnull;
    PRUint16            generation = record->Generation();
    PRUint32            hashNumber = record->HashNumber();
    PRUint32            metaFile   = record->MetaFile();
    PRFileDesc *        fd         = nsnull;
    *result = nsnull;
    
    if (metaFile == 0) {  // entry/metadata stored in separate file
        // open and read the file
        nsCOMPtr<nsILocalFile> file;
        rv = GetLocalFileForDiskCacheRecord(record, nsDiskCache::kMetaData, getter_AddRefs(file));
        if (NS_FAILED(rv))  return rv;

        PRFileDesc * fd = nsnull;
        nsresult rv = file->OpenNSPRFileDesc(PR_RDONLY, 00666, &fd);
        if (NS_FAILED(rv))  return rv;
        
        PRInt32 fileSize = PR_Available(fd);
        if (fileSize < 0) {
            // XXX an error occurred. We could call PR_GetError(), but how would that help?
            rv = NS_ERROR_UNEXPECTED;
            goto exit;
        }

        diskEntry = (nsDiskCacheEntry *) new char[fileSize];
        if (!diskEntry) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto exit;
        }
        
        PRInt32 bytesRead = PR_Read(fd, diskEntry, fileSize);
        if (bytesRead < fileSize) {
            rv = NS_ERROR_UNEXPECTED;
            goto exit;
        }

    } else if (metaFile < 4) {  // XXX magic number: use constant
        // entry/metadata stored in cache block file
        
        // allocate buffer
        PRUint32 blockSize  = GetBlockSizeForIndex(metaFile - 1);
        PRUint32 blockCount = record->MetaBlockCount();
        diskEntry = (nsDiskCacheEntry *) new char[blockSize * blockCount];
        
        // read diskEntry
        rv = mBlockFile[metaFile - 1].ReadBlocks((char *)diskEntry,
                                                 record->MetaStartBlock(),
                                                 blockCount);
        if (NS_FAILED(rv))  goto exit;
    }
    
    
    // pass ownership to caller
    *result = diskEntry;
    diskEntry = nsnull;

exit:
    // XXX auto ptr would be nice
    if (fd) (void) PR_Close(fd);
    delete diskEntry;
    return rv;
}


nsresult
nsDiskCacheMap::WriteDiskCacheEntry(nsDiskCacheEntry *  diskEntry,
                                       nsDiskCacheBindData * bindData)
{
    nsresult rv = NS_OK;
    
    if (diskEntry->Size() < 1024) {             // block size 256
    
    } else if (diskEntry->Size() < 4096) {      // block size 1024
    
    } else if (diskEntry->Size() < 16384) {     // block size 4096
    
    } else {                                    // separate file
    
    }
    
    // XXX if block file != reallocate
    // XXX if block size != reallocate
    // XXX deallocate previously used blocks
    // XXX allocate new blocks
    // XXX update bindData so caller can update cache map
    // XXX write data
    
    return NS_OK;
}


nsresult
nsDiskCacheMap::GetFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                             PRBool              meta,
                                             nsIFile **          result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    PRInt16 generation = record->Generation();
    char name[32];
    ::sprintf(name, "%08x%c%02x", record->HashNumber(),  (meta ? 'M' : 'D'), generation);
    rv = file->Append(name);
    if (NS_FAILED(rv))  return rv;
    
    return rv;
}

nsresult
nsDiskCacheMap::GetLocalFileForDiskCacheRecord(nsDiskCacheRecord * record,
                                                  PRBool              meta,
                                                  nsILocalFile **     result)
{
    nsCOMPtr<nsIFile> file;
    nsresult rv = GetFileForDiskCacheRecord(record, meta, getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    if (NS_FAILED(rv))  return rv;
    
    NS_IF_ADDREF(*result = localFile);
    return rv;
}


nsresult
nsDiskCacheMap::GetBlockFileForIndex(PRUint32 index, nsILocalFile ** result)
{
    if (!mCacheDirectory)  return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsIFile> file;
    nsresult rv = mCacheDirectory->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))  return rv;
    
    char name[32];
    ::sprintf(name, "_CACHE_%03d_", index + 1);
    rv = file->Append(name);
    if (NS_FAILED(rv))  return rv;
    
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
    NS_IF_ADDREF(*result = localFile);

    return rv;
}


PRUint32
nsDiskCacheMap::GetBlockSizeForIndex(PRUint32 index)
{
    return 256 << (2 * (index));  // XXX magic numbers

}


#if 0
/******************************************************************************
 *  old code
 *****************************************************************************/
#ifdef XP_MAC
#pragma mark -
#pragma mark OLD CODE
#endif

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
    if (header.mVersion != kCurrentVersion) return NS_ERROR_FAILURE;
    mHeader = header;

    // seek to beginning of first bucket.
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, sizeof(nsDiskCacheBucket));
    if (NS_FAILED(rv)) return rv;

    // read the buckets.
    rv = input->Read((char*)&mBuckets, sizeof(mBuckets), &count);
    if (count != sizeof(mBuckets)) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;
    
    // unswap all of the active records.
    for (int b = 0; b < kBucketsPerTable; ++b) {
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

    // seek to beginning of the file.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(output, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv)) return rv;
    
    // write the header.
    nsDiskCacheHeader header = mHeader;
    header.Swap();
    rv = output->Write((char*)&header, sizeof(header), &count);
    if (count != sizeof(header)) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;
    
    // pad the rest of the header to sizeof(nsDiskCacheBucket).
/*
    char padding[sizeof(nsDiskCacheBucket) - sizeof(nsDiskCacheHeader)];
    ::memset(padding, 0, sizeof(padding));
    rv = output->Write(padding, sizeof(padding), &count);
    if (count != sizeof(padding)) return NS_ERROR_FAILURE;
    if (NS_FAILED(rv)) return rv;
*/
    // swap all of the active records.
    {
        for (int b = 0; b < kBucketsPerTable; ++b) {
            nsDiskCacheBucket& bucket = mBuckets[b];
            for (int r = 0; r < kRecordsPerBucket; ++r) {
                nsDiskCacheRecord* record = &bucket.mRecords[r];
                if (record->HashNumber() == 0)
                    break;
                record->Swap();
            }
        }
    }
        
    // write the buckets.
    rv = output->Write((char*)&mBuckets, sizeof(mBuckets), &count);
    output->Flush();

    // unswap all of the active records.
    {
        for (int b = 0; b < kBucketsPerTable; ++b) {
            nsDiskCacheBucket& bucket = mBuckets[b];
            for (int r = 0; r < kRecordsPerBucket; ++r) {
                nsDiskCacheRecord* record = &bucket.mRecords[r];
                if (record->HashNumber() == 0)
                    break;
                record->Unswap();
            }
        }
    }

    if (count != sizeof(mBuckets)) return NS_ERROR_FAILURE;
    return rv;
}

nsresult nsDiskCacheMap::WriteBucket(nsIOutputStream* output, PRUint32 index)
{
    nsresult rv;
    
    // can only do this if the stream is seekable.
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(output, &rv);
    if (NS_FAILED(rv)) return rv;

    // seek to the offset of this bucket, (index + 1) to skip the header.
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, (index + 1) * sizeof(nsDiskCacheBucket));
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

#endif

