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
 * The Original Code is nsDiskCacheEntry.cpp, released May 10, 2001.
 * 
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *    Gordon Sheridan  <gordon@netscape.com>
 *    Patrick C. Beard <beard@netscape.com>
 */

#include "nsDiskCacheEntry.h"
#include "nsCache.h"


nsresult MetaDataFile::Read(nsIInputStream* input)
{
    nsresult rv;
    PRUint32 count;
    
    // XXX  Is it less expensive to read the file in multiple parts, or
    // XXX  get the size and read it in one chunk?
    // read in the file header.
    rv = input->Read((char*)&mHeaderSize, sizeof(MetaDataHeader), &count);
    if (NS_FAILED(rv)) return rv;
    Unswap();
    
    // make sure it is self-consistent.
    if (mHeaderSize != sizeof(MetaDataHeader)) {
        NS_ERROR("### CACHE FORMAT CHANGED!!! PLEASE DELETE YOUR NewCache DIRECTORY!!! ###");
        return NS_ERROR_ILLEGAL_VALUE;
    }

    // read in the key.
    delete[] mKey;
    mKey = new char[mKeySize];
    if (!mKey) return NS_ERROR_OUT_OF_MEMORY;
    rv = input->Read(mKey, mKeySize, &count);
    if (NS_FAILED(rv)) return rv;

    // read in the metadata.
    delete mMetaData;
    mMetaData = nsnull;
    if (mMetaDataSize) {
        mMetaData = new char[mMetaDataSize];
        if (!mMetaData) return NS_ERROR_OUT_OF_MEMORY;
        rv = input->Read(mMetaData, mMetaDataSize, &count);
        if (NS_FAILED(rv)) return rv;
    }
    
    return NS_OK;
}

nsresult MetaDataFile::Write(nsIOutputStream* output)
{
    nsresult rv;
    PRUint32 count;
    
    // write the header to the file.
    Swap();
    rv = output->Write((char*)&mHeaderSize, sizeof(MetaDataHeader), &count);
    Unswap();
    if (NS_FAILED(rv)) return rv;
    
    // write the key to the file.
    rv = output->Write(mKey, mKeySize, &count);
    if (NS_FAILED(rv)) return rv;
    
    // write the flattened metadata to the file.
    if (mMetaDataSize) {
        rv = output->Write(mMetaData, mMetaDataSize, &count);
        if (NS_FAILED(rv)) return rv;
    }
    
    return NS_OK;
}


NS_IMPL_ISUPPORTS1(nsDiskCacheEntryInfo, nsICacheEntryInfo);

NS_IMETHODIMP nsDiskCacheEntryInfo::GetClientID(char ** clientID)
{
    NS_ENSURE_ARG_POINTER(clientID);
    return ClientIDFromCacheKey(nsLiteralCString(mMetaDataFile.mKey), clientID);
}

extern const char DISK_CACHE_DEVICE_ID[];
NS_IMETHODIMP nsDiskCacheEntryInfo::GetDeviceID(char ** deviceID)
{
    NS_ENSURE_ARG_POINTER(deviceID);
    *deviceID = nsCRT::strdup(mDeviceID);
    return *deviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP nsDiskCacheEntryInfo::GetKey(char ** clientKey)
{
    NS_ENSURE_ARG_POINTER(clientKey);
    return ClientKeyFromCacheKey(nsLiteralCString(mMetaDataFile.mKey), clientKey);
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetFetchCount(PRInt32 *aFetchCount)
{
    return *aFetchCount = mMetaDataFile.mFetchCount;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastFetched(PRUint32 *aLastFetched)
{
    *aLastFetched = mMetaDataFile.mLastFetched;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetLastModified(PRUint32 *aLastModified)
{
    *aLastModified = mMetaDataFile.mLastModified;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetExpirationTime(PRUint32 *aExpirationTime)
{
    *aExpirationTime = mMetaDataFile.mExpirationTime;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::IsStreamBased(PRBool *aStreamBased)
{
    *aStreamBased = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetDataSize(PRUint32 *aDataSize)
{
    *aDataSize = mMetaDataFile.mDataSize;
    return NS_OK;
}
