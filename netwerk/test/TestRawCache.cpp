/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#include "nsIStreamListener.h"
#include "nsIStreamObserver.h"
#include "nsIServiceManager.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include <stdio.h>

#include "nsINetDataCache.h"
#include "nsINetDataCacheRecord.h"
#include "nsMemCacheCID.h"

// Number of test entries to be placed in the cache
#define NUM_CACHE_ENTRIES  250

// Cache content stream length will have random length between zero and
// MAX_CONTENT_LENGTH bytes
#define MAX_CONTENT_LENGTH 20000

static NS_DEFINE_CID(kMemCacheCID, NS_MEM_CACHE_FACTORY_CID);
static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

// Mapping from test case number to RecordID
static PRInt32 recordID[NUM_CACHE_ENTRIES];

static PRInt32
mapRecordIdToTestNum(PRInt32 aRecordID)
{
    int i;
    for (i = 0; i < NUM_CACHE_ENTRIES; i++) {
        if (recordID[i] == aRecordID)
            return i;
    }
    return -1;
}

// Supply a reproducible stream of random data.
class RandomStream {
public:
    RandomStream(PRUint32 aSeed) {
        mStartSeed = mSeed = aSeed;
    }
    
    PRUint32 GetStartSeed() {
        return mStartSeed;
    }
    
    PRUint32 Next() {
        mSeed = 1103515245 * mSeed + 12345;
        return mSeed;
    }

    void Read(char* aBuf, PRUint32 aCount) {
        PRUint32 i;
        for (i = 0; i < aCount; i++) {
            *aBuf++ = Next();
        }
    }

    PRBool
    Match(char* aBuf, PRUint32 aCount) {
        PRUint32 i;
        for (i = 0; i < aCount; i++) {
            if (*aBuf++ != (char)(Next() & 0xff))
                return PR_FALSE;
        }
        return PR_TRUE;
    }

protected:
    
    PRUint32 mSeed;
    PRUint32 mStartSeed;
};

static int gNumReaders = 0;
static PRUint32 gTotalBytesRead = 0;
static PRUint32 gTotalDuration = 0;

class nsReader : public nsIStreamListener {
public:
    NS_DECL_ISUPPORTS

    nsReader()
        : mStartTime(0), mBytesRead(0)
    {
        NS_INIT_REFCNT();
        gNumReaders++;
    }

    virtual ~nsReader() {
        delete mRandomStream;
        gNumReaders--;
    }

    nsresult 
    Init(nsIChannel *aChannel, RandomStream *aRandomStream, PRUint32 aExpectedStreamLength) {
        mChannel = aChannel;
        mRandomStream = aRandomStream;
        mExpectedStreamLength = aExpectedStreamLength;
        mRefCnt = 1;
        return NS_OK;
    }

    NS_IMETHOD OnStartRequest(nsIChannel* channel,
                              nsISupports* context) {
        mStartTime = PR_IntervalNow();
        return NS_OK;
    }

    NS_IMETHOD OnDataAvailable(nsIChannel* channel, 
                               nsISupports* context,
                               nsIInputStream *aIStream, 
                               PRUint32 aSourceOffset,
                               PRUint32 aLength) {
        char buf[1025];
        while (aLength > 0) {
            PRUint32 amt;
            PRBool match;
            aIStream->Read(buf, sizeof buf, &amt);
            if (amt == 0) break;
            aLength -= amt;
            mBytesRead += amt;
            match = mRandomStream->Match(buf, amt);
            NS_ASSERTION(match, "Stored data was corrupted on read");
        }
        return NS_OK;
    }

    NS_IMETHOD OnStopRequest(nsIChannel* channel, 
                             nsISupports* context,
                             nsresult aStatus,
                             const PRUnichar* aMsg) {
        PRIntervalTime endTime;
        PRIntervalTime duration;
        
        endTime = PR_IntervalNow();
        duration = (endTime - mStartTime);

        if (NS_FAILED(aStatus)) printf("channel failed.\n");
        //        printf("read %d bytes\n", mBytesRead);

        NS_ASSERTION(mBytesRead == mExpectedStreamLength,
                     "Stream in cache is wrong length");

        gTotalBytesRead += mBytesRead;
        gTotalDuration += duration;

        // Release channel
        mChannel = 0;
        return NS_OK;
    }

protected:
    PRIntervalTime       mStartTime;
    PRUint32             mBytesRead;
    RandomStream*        mRandomStream;
    PRUint32             mExpectedStreamLength;
    nsCOMPtr<nsIChannel> mChannel;
};

NS_IMPL_ISUPPORTS2(nsReader, nsIStreamListener, nsIStreamObserver)

static nsIEventQueue* eventQueue;

nsresult
InitQueue() {
    nsresult rv;

    NS_WITH_SERVICE(nsIEventQueueService, eventQService, kEventQueueServiceCID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get event queue service");

    rv = eventQService->CreateThreadEventQueue();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create event queue");
  
    rv = eventQService->GetThreadEventQueue(PR_CurrentThread(), &eventQueue);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get event queue for main thread");

    return NS_OK;
}

nsresult
WaitForEvents() {
    while (gNumReaders) {
        eventQueue->ProcessPendingEvents();
    }
    return NS_OK;
}

nsresult
TestReadStream(nsINetDataCacheRecord *record, RandomStream *randomStream,
               PRUint32 expectedStreamLength)
{
    nsCOMPtr<nsIChannel> channel;
    nsresult rv;
    PRUint32 actualContentLength;

    rv = record->NewChannel(0, getter_AddRefs(channel));
    NS_ASSERTION(NS_SUCCEEDED(rv), " ");

    rv = record->GetStoredContentLength(&actualContentLength);
    NS_ASSERTION(NS_SUCCEEDED(rv), " ");
    NS_ASSERTION(actualContentLength == expectedStreamLength,
                 "nsINetDataCacheRecord::GetContentLength() busted ?");
    
    nsReader *reader = new nsReader;
    reader->AddRef();
    rv = reader->Init(channel, randomStream, expectedStreamLength);
    NS_ASSERTION(NS_SUCCEEDED(rv), " ");
    
    rv = channel->AsyncRead(0, -1, 0, reader);
    NS_ASSERTION(NS_SUCCEEDED(rv), " ");
    reader->Release();

    return NS_OK;
}

nsresult
TestRecordID(nsINetDataCache *cache)
{
    nsresult rv;
    nsCOMPtr<nsINetDataCacheRecord> record;
    RandomStream *randomStream;
    PRUint32 metaDataLength;
    char cacheKey[15];
    char *metaData;
    PRUint32 testNum;
    PRBool match;

    for (testNum = 0; testNum < NUM_CACHE_ENTRIES; testNum++) {
        randomStream = new RandomStream(testNum);
        randomStream->Read(cacheKey, sizeof cacheKey);

        rv = cache->GetCachedNetDataByID(recordID[testNum], getter_AddRefs(record));
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't obtain record using record ID");

        // Match against previously stored meta-data
        rv = record->GetMetaData(&metaDataLength, &metaData);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get record meta-data");
        match = randomStream->Match(metaData, metaDataLength);
        NS_ASSERTION(match, "Meta-data corrupted or incorrect");

        nsAllocator::Free(metaData);
        delete randomStream;
    }
    return NS_OK;
}

nsresult
TestEnumeration(nsINetDataCache *cache)
{
    nsresult rv;
    nsCOMPtr<nsINetDataCacheRecord> record;
    nsCOMPtr<nsISupports> tempISupports;
    nsCOMPtr<nsISimpleEnumerator> iterator;
    RandomStream *randomStream;
    PRUint32 metaDataLength;
    char cacheKey[15];
    char *metaData;
    PRUint32 testNum;
    PRBool match;
    PRInt32 recID;

    int numRecords = 0;

    // Iterate over all records in the cache
    rv = cache->NewCacheEntryIterator(getter_AddRefs(iterator));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create new cache entry iterator");

    PRBool notDone;
    while (1) {

        // Done iterating ?
        rv = iterator->HasMoreElements(&notDone);
        if (NS_FAILED(rv)) return rv;
        if (!notDone)
            break;

        // Get next record in iteration
        rv = iterator->GetNext(getter_AddRefs(tempISupports));
        NS_ASSERTION(NS_SUCCEEDED(rv), "iterator bustage");
        record = do_QueryInterface(tempISupports);

        numRecords++;

        // Get record ID
        rv = record->GetRecordID(&recID);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get Record ID");
        testNum = mapRecordIdToTestNum(recID);
        NS_ASSERTION(testNum != -1, "Corrupted Record ID ?");

        // Erase mapping from table, so that duplicate enumerations are detected
        recordID[testNum] = -1;

        // Make sure stream matches test data
        randomStream = new RandomStream(testNum);
        randomStream->Read(cacheKey, sizeof cacheKey);

        // Match against previously stored meta-data
        rv = record->GetMetaData(&metaDataLength, &metaData);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get record meta-data");
        match = randomStream->Match(metaData, metaDataLength);
        NS_ASSERTION(match, "Meta-data corrupted or incorrect");
        nsAllocator::Free(metaData);

        delete randomStream;
    }

    NS_ASSERTION(numRecords == NUM_CACHE_ENTRIES, "Iteration bug");

    return NS_OK;
}

nsresult
TestRead(nsINetDataCache *cache)
{
    nsresult rv;
    PRBool inCache;
    nsCOMPtr<nsINetDataCacheRecord> record;
    RandomStream *randomStream;
    PRUint32 metaDataLength;
    char cacheKey[15];
    char *metaData, *storedCacheKey;
    PRUint32 testNum, storedCacheKeyLength;
    PRBool match;

    for (testNum = 0; testNum < NUM_CACHE_ENTRIES; testNum++) {
        randomStream = new RandomStream(testNum);
        randomStream->Read(cacheKey, sizeof cacheKey);

        // Ensure that entry is in the cache
        rv = cache->Contains(cacheKey, sizeof cacheKey, &inCache);
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        NS_ASSERTION(inCache, "nsINetDataCache::Contains error");
        
        rv = cache->GetCachedNetData(cacheKey, sizeof cacheKey, getter_AddRefs(record));
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");

        // Match against previously stored meta-data
        match = record->GetMetaData(&metaDataLength, &metaData);
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        match = randomStream->Match(metaData, metaDataLength);
        NS_ASSERTION(match, "Meta-data corrupted or incorrect");
        nsAllocator::Free(metaData);

        // Test GetKey() method
        rv = record->GetKey(&storedCacheKeyLength, &storedCacheKey);
        NS_ASSERTION(NS_SUCCEEDED(rv) &&
                     (storedCacheKeyLength == sizeof cacheKey) &&
                     !memcmp(storedCacheKey, &cacheKey[0], sizeof cacheKey),
                     "nsINetDataCacheRecord::GetKey failed");
        nsAllocator::Free(storedCacheKey);

        PRUint32 expectedStreamLength = randomStream->Next() & 0xffff;

        TestReadStream(record, randomStream, expectedStreamLength);
    }

    WaitForEvents();

    // Compute rate in MB/s
    double rate = gTotalBytesRead / PR_IntervalToMilliseconds(gTotalDuration);
    rate *= 1000;
    rate /= (1024 * 1024);
    printf("Read %d bytes at a rate of %5.1f MB per second \n",
           gTotalBytesRead, rate);

    return NS_OK;
}

nsresult
FillCache(nsINetDataCache *cache)
{
    nsresult rv;
    PRBool inCache;
    nsCOMPtr<nsINetDataCacheRecord> record;
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIOutputStream> outStream;
    char buf[1000];
    PRUint32 metaDataLength;
    char cacheKey[15];
    char metaData[100];
    PRUint32 testNum;
    char *data;
    RandomStream *randomStream;

    PRIntervalTime startTime = PR_IntervalNow();
    
    for (testNum = 0; testNum < NUM_CACHE_ENTRIES; testNum++) {
        randomStream = new RandomStream(testNum);
        randomStream->Read(cacheKey, sizeof cacheKey);

        // No entry should be in cache until we add it
        rv = cache->Contains(cacheKey, sizeof cacheKey, &inCache);
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        NS_ASSERTION(!inCache, "nsINetDataCache::Contains error");
        
        rv = cache->GetCachedNetData(cacheKey, sizeof cacheKey, getter_AddRefs(record));
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't access record via opaque cache key");

        rv = record->GetRecordID(&recordID[testNum]);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get Record ID");

        PRUint32 numEntries;

        numEntries = (PRUint32)-1;
        rv = cache->GetNumEntries(&numEntries);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get number of cache entries");
        NS_ASSERTION(numEntries == testNum + 1, "GetNumEntries failure");

        // MetaData should be initially empty
        rv = record->GetMetaData(&metaDataLength, &data);
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        if ((metaDataLength != 0) || (data != 0))
            return NS_ERROR_FAILURE;

        // For lack of a better source of random data, store the
        //  last buffer-full of data as the record meta-data
        randomStream->Read(metaData, sizeof metaData);
        record->SetMetaData(sizeof metaData, metaData);

        rv = record->NewChannel(0, getter_AddRefs(channel));
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");

        rv = channel->OpenOutputStream(0, getter_AddRefs(outStream));
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        
        int streamLength = randomStream->Next() & 0xffff;
        int remaining = streamLength;
        while (remaining) {
            PRUint32 numWritten;
            int amount = PR_MIN(sizeof buf, remaining);
            randomStream->Read(buf, amount);

            rv = outStream->Write(buf, amount, &numWritten);
            NS_ASSERTION(NS_SUCCEEDED(rv), " ");
            NS_ASSERTION(numWritten == (PRUint32)amount, "Write() bug?");
            
            remaining -= amount;
        }
        outStream->Close();

        // *Now* there should be an entry in the cache
        rv = cache->Contains(cacheKey, sizeof cacheKey, &inCache);
        NS_ASSERTION(NS_SUCCEEDED(rv), " ");
        NS_ASSERTION(inCache, "nsINetDataCache::Contains error");

        delete randomStream;
    }

    PRIntervalTime endTime = PR_IntervalNow();

    return NS_OK;
}

nsresult NS_AutoregisterComponents()
{
  nsresult rv = nsComponentManager::AutoRegister(nsIComponentManager::NS_Startup,
                                                 NULL /* default */);
  return rv;
}

int
main(int argc, char* argv[])
{
    nsresult rv;
    nsCOMPtr<nsINetDataCache> cache;

    rv = NS_AutoregisterComponents();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't register XPCOM components");

    rv = nsComponentManager::CreateInstance(kMemCacheCID, nsnull,
                                            NS_GET_IID(nsINetDataCache),
                                            getter_AddRefs(cache));
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't create memory cache factory");

    InitQueue();

    PRUnichar* description;
    rv = cache->GetDescription(&description);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get cache description");
    nsCAutoString descStr(description);
    printf("Testing: %s\n", descStr.GetBuffer());

    rv = cache->RemoveAll();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't clear cache");

    PRUint32 numEntries = (PRUint32)-1;
    rv = cache->GetNumEntries(&numEntries);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get number of cache entries");
    NS_ASSERTION(numEntries == 0, "Couldn't clear cache");

    rv = FillCache(cache);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't fill cache with random test data");

    rv = TestRead(cache);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't read random test data from cache");

    rv = TestRecordID(cache);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't index records using record ID");

    rv = TestEnumeration(cache);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't successfully truncate records");

    return 0;
}

