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
#include "nsDiskCacheBinding.h"
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
    
    PRUint32 i      = kRecordsPerBucket >> 1;
    PRUint32 offset = kRecordsPerBucket >> 2;
    
    while (offset > 0) {
        if (mRecords[i].HashNumber())  i += offset;
        else                           i -= offset;
        offset >>= 1;
    }
    
    if (mRecords[i].HashNumber() != 0)
        ++i;
    
    return i;
}


PRInt32
nsDiskCacheBucket::VisitEachRecord(nsDiskCacheRecordVisitor *  visitor,
                                   PRBool *                    dirty)
{
    PRInt32  rv = kVisitNextRecord;
    PRInt32  i = CountRecords();
    
    *dirty = PR_FALSE;
       
    // call visitor for each entry
    while (i--) {
        rv = visitor->VisitRecord(&mRecords[i]);
        if (rv == kVisitNextRecord) continue;
        
        if (rv == kDeleteRecordAndContinue) {
            mRecords[i].SetHashNumber(0);
            *dirty = PR_TRUE;
            continue;
        }

        return kStopVisitingRecords;  // rv == kStopVisitingRecords
    }
    
    return rv;
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
    (void) CloseBlockFiles();
    
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
    nsresult  rv = CloseBlockFiles();
    if (NS_FAILED(rv)) goto exit;  // this is going to be a mess...
    
    // write map record buckets
    rv = FlushBuckets(PR_FALSE);   // don't bother swapping buckets back
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
    nsresult            rv;
    PRUint32            hashNumber = mapRecord->HashNumber();
    nsDiskCacheBucket * bucket;

   oldRecord->SetHashNumber(0);  // signify no record

    rv = GetBucketForHashNumber(hashNumber, &bucket);
    if (NS_FAILED(rv))  return rv;
    
    nsDiskCacheRecord * mostEvictable = &bucket->mRecords[0];
    for (int i = 0; i < kRecordsPerBucket; ++i) {
        if (bucket->mRecords[i].HashNumber() == 0) {
            // stick the new record here
            bucket->mRecords[i] = *mapRecord;
            ++mHeader.mEntryCount;
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
    for (PRUint32 i = 0; i < count; ++i) {        
        if (bucket->mRecords[i].HashNumber() == mapRecord->HashNumber()) {
            // found it, now delete it.
            if (i != (count - 1)) { // if not the last record, shift last record into opening
                bucket->mRecords[i] = bucket->mRecords[count - 1];
            }
            bucket->mRecords[count - 1].SetHashNumber(0);   // clear last record
            mHeader.mEntryCount--;
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
        PRBool continueFlag = mBuckets[i].VisitEachRecord(visitor, &dirty);
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
    
    diskEntry->Unswap();    // disk to memory
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
nsDiskCacheMap::WriteDiskCacheEntry(nsDiskCacheBinding *  binding)
{
    nsresult            rv        = NS_OK;
    nsDiskCacheEntry *  diskEntry =  CreateDiskCacheEntry(binding);
    if (!diskEntry)  rv = NS_ERROR_UNEXPECTED;
    
    PRUint32    size = diskEntry->Size();
    PRUint32    fileIndex;
    PRUint32    blocks;
    
    if (size < 1024) {             // block size 256
        fileIndex = 1;
        blocks    = size / 256 + 1;
    } else if (size < 4096) {      // block size 1024
        fileIndex = 2;
        blocks    = size / 1024 + 1;
    } else if (size < 16384) {     // block size 4096
        fileIndex = 3;
        blocks    = size / 4096 + 1;
    } else {                       // separate file
        fileIndex = 0;
    }
    
   PRInt32   startBlock = binding->mRecord.MetaStartBlock();
   PRInt32   blockCount = binding->mRecord.MetaBlockCount();
   PRUint32  metaFile   = binding->mRecord.MetaFile();

    // Deallocate old blocks if necessary    
    if (binding->mRecord.MetaLocationInitialized() &&  // no old blocks
       (metaFile != 0)) {               // separate file, no old blocks

        // deallocate previously used blocks        
        rv = mBlockFile[metaFile - 1].DeallocateBlocks(startBlock, blockCount);
        if (NS_FAILED(rv))  return rv;
    }
        
    if (fileIndex == 0) {
        // Write entry data to separate file
        nsCOMPtr<nsILocalFile> localFile;
        binding->mRecord.SetMetaFileGeneration(binding->mGeneration);
        rv = GetLocalFileForDiskCacheRecord(&binding->mRecord,
                                            nsDiskCache::kMetaData,
                                            getter_AddRefs(localFile));
        if (NS_FAILED(rv))  return rv;
        
        // open the file
        PRFileDesc * fd;
        rv = localFile->OpenNSPRFileDesc(PR_RDWR | PR_TRUNCATE | PR_CREATE_FILE, 00666, &fd);
        if (NS_FAILED(rv))  return rv;  // unable to open or create file

        // write the file
        diskEntry->Swap();
        PRInt32 bytesWritten = PR_Write(fd, diskEntry, size);
        
        PRStatus err = PR_Close(mMapFD);
        if ((bytesWritten != size) || (err != PR_SUCCESS))  return NS_ERROR_UNEXPECTED;
        
    } else {
        // write entry data to disk cache block file
        startBlock = mBlockFile[fileIndex - 1].AllocateBlocks(blocks);
        if (startBlock < 0)  return NS_ERROR_UNEXPECTED;
        
        // update binding so caller can update cache map
        binding->mRecord.SetMetaBlocks(fileIndex, startBlock, blocks);
        // XXX we should probably write out bucket ourselves

        // write data
        rv = mBlockFile[fileIndex - 1].WriteBlocks(diskEntry, startBlock, blocks);
        if (NS_FAILED(rv))  return rv;
    }

    return NS_OK;
}


nsresult
nsDiskCacheMap::DoomRecord(nsDiskCacheRecord * record)
{
    nsresult  rv = DeleteRecord(record);
    // XXX future: add record to doomed record journal

    return rv;
}


nsresult
nsDiskCacheMap::DeleteStorage(nsDiskCacheRecord * record)
{
    nsresult  rv1 = DeleteStorage(record, nsDiskCache::kData);
    nsresult  rv2 = DeleteStorage(record, nsDiskCache::kMetaData);
    return NS_FAILED(rv1) ? rv1 : rv2;
}


nsresult
nsDiskCacheMap::DeleteStorage(nsDiskCacheRecord * record, PRBool metaData)
{
    nsresult    rv;
    PRUint32    fileIndex = metaData ? record->MetaFile() : record->DataFile();
    nsCOMPtr<nsILocalFile> file;
    
    if (fileIndex == 0) {
        // delete the file
        rv = GetLocalFileForDiskCacheRecord(record, metaData, getter_AddRefs(file));
        if (NS_SUCCEEDED(rv)) {
            rv = file->Delete(PR_FALSE);    // false == non-recursive
        }
        
    } else if (fileIndex < 4) {
        // deallocate blocks
        PRInt32  startBlock = metaData ? record->MetaStartBlock() : record->DataStartBlock();
        PRInt32  blockCount = metaData ? record->MetaBlockCount() : record->DataBlockCount();
        
        rv = mBlockFile[fileIndex - 1].DeallocateBlocks(startBlock, blockCount);
    }
    
    return rv;
}


nsresult
nsDiskCacheMap::DeleteRecordAndStorage(nsDiskCacheRecord * record)
{
    nsresult  rv1 = DeleteStorage(record);
    nsresult  rv2 = DeleteRecord(record);
    return NS_FAILED(rv1) ? rv1 : rv2;
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
    
    NS_IF_ADDREF(*result = file);
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
