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

#include "nsMemCache.h"
#include "nsMemCacheRecord.h"
#include "nsMemCacheChannel.h"
#include "nsIAllocator.h"
#include "nsStorageStream.h"

static NS_DEFINE_IID(kINetDataCacheRecord, NS_INETDATACACHERECORD_IID);

nsMemCacheRecord::nsMemCacheRecord()
    : mKey(0), mKeyLength(0), mMetaData(0), mMetaDataLength(0), mNumChannels(0)
{
    NS_INIT_REFCNT();
}

nsMemCacheRecord::~nsMemCacheRecord()
{
    if (mMetaData)
        delete[] mMetaData;
    if (mKey)
        delete[] mKey;
}	
     
NS_IMPL_ISUPPORTS(nsMemCacheRecord, NS_GET_IID(nsINetDataCacheRecord))

NS_IMETHODIMP
nsMemCacheRecord::GetKey(PRUint32 *aLength, char **aResult)
{
    NS_ENSURE_ARG(aResult);
    *aResult = (char *)nsAllocator::Alloc(mKeyLength);
    if (!*aResult)
        return NS_ERROR_OUT_OF_MEMORY;
    memcpy(*aResult, mKey, mKeyLength);
    *aLength = mKeyLength;
    return NS_OK;
}

nsresult
nsMemCacheRecord::Init(const char *aKey, PRUint32 aKeyLength,
                       PRUint32 aRecordID, nsMemCache *aCache)
{
    nsresult rv;

    NS_ASSERTION(!mKey, "Memory cache record key set multiple times");

    mStorageStream = new nsStorageStream;
    rv = mStorageStream->Initialize(MEM_CACHE_SEGMENT_SIZE,
                                    MEM_CACHE_MAX_ENTRY_SIZE,
                                    0);
    if (NS_FAILED(rv)) return rv;

    mKey = new char[aKeyLength];
    if (!mKey)
        return NS_ERROR_OUT_OF_MEMORY;
    memcpy(mKey, aKey, aKeyLength);
    mKeyLength = aKeyLength;
    mRecordID = aRecordID;
    mCache = aCache;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheRecord::GetRecordID(PRInt32 *aRecordID)
{
    NS_ENSURE_ARG(aRecordID);
    *aRecordID = mRecordID;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheRecord::GetMetaData(PRUint32 *aLength, char **aResult)
{
    NS_ENSURE_ARG(aResult);

    *aResult = 0;
    if (mMetaDataLength) {
        *aResult = (char*)nsAllocator::Alloc(mMetaDataLength);
        if (!*aResult)
            return NS_ERROR_OUT_OF_MEMORY;
        memcpy(*aResult, mMetaData, mMetaDataLength);
    }
    *aLength = mMetaDataLength;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheRecord::SetMetaData(PRUint32 aLength, const char *aData)
{
    if (mMetaData)
        delete[] mMetaData;
    mMetaData = new char[aLength];
    if (!mMetaData)
        return NS_ERROR_OUT_OF_MEMORY;
    memcpy(mMetaData, aData, aLength);
    mMetaDataLength = aLength;
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheRecord::GetStoredContentLength(PRUint32 *aStoredContentLength)
{
    NS_ENSURE_ARG(aStoredContentLength);
    return mStorageStream->GetLength(aStoredContentLength);
}

NS_IMETHODIMP
nsMemCacheRecord::SetStoredContentLength(PRUint32 aStoredContentLength)
{
    PRUint32 before, after;
    mStorageStream->GetLength(&before);
    nsresult rv = mStorageStream->SetLength(aStoredContentLength);
    if (NS_FAILED(rv)) return rv;
    mStorageStream->GetLength(&after);
    mCache->mOccupancy -= (before - after);
    return NS_OK;
}

NS_IMETHODIMP
nsMemCacheRecord::Delete(void)
{
    if (mNumChannels)
        return NS_ERROR_NOT_AVAILABLE;

    return mCache->Delete(this);
}

NS_IMETHODIMP
nsMemCacheRecord::GetFilename(nsIFileSpec* *aFilename)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMemCacheRecord::NewChannel(nsILoadGroup *aLoadGroup, nsIChannel* *aResult)
{
    NS_ENSURE_ARG(aResult);

    nsMemCacheChannel* channel = new nsMemCacheChannel(this, aLoadGroup);
    if (!channel)
        return NS_ERROR_OUT_OF_MEMORY;

    channel->AddRef();
    *aResult = NS_STATIC_CAST(nsIChannel*, channel);
    return NS_OK;

}
