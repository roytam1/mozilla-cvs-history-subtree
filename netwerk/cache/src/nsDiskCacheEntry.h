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
 * The Original Code is nsMemoryCacheDevice.cpp, released February 22, 2001.
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

#ifndef _nsDiskCacheEntry_h_
#define _nsDiskCacheEntry_h_

#include "nspr.h"
#include "nscore.h"
#include "nsError.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsICacheVisitor.h"

#include "nsCacheEntry.h"


/******************************************************************************
 *  MetaData
 *****************************************************************************/
struct MetaDataHeader {
    PRUint32        mHeaderSize;
//  PRUint32        mHashNumber;    // XXX
//  PRUint32        mMetaLocation;  // XXX
    PRInt32         mFetchCount;
    PRUint32        mLastFetched;
    PRUint32        mLastModified;
    PRUint32        mExpirationTime;
    PRUint32        mDataSize;
    PRUint32        mKeySize;
    PRUint32        mMetaDataSize;
//  void *          mKeyPtrSpace;       // XXX  don't need this
//  void *          mMetaDataPtrSpace;  // XXX  don't need this, we'll calculate it when necessary
    // followed by null-terminated key and metadata string values.

    MetaDataHeader()
        :   mHeaderSize(sizeof(MetaDataHeader)),
            mFetchCount(0),
            mLastFetched(0),
            mLastModified(0),
            mExpirationTime(0),
            mDataSize(0),
            mKeySize(0),
            mMetaDataSize(0)
    {
    }

    MetaDataHeader(nsCacheEntry* entry)
        :   mHeaderSize(sizeof(MetaDataHeader)),
            mFetchCount(entry->FetchCount()),
            mLastFetched(entry->LastFetched()),
            mLastModified(entry->LastModified()),
            mExpirationTime(entry->ExpirationTime()),
            mDataSize(entry->DataSize()),
            mKeySize(entry->Key()->Length() + 1),
            mMetaDataSize(0)
    {
    }
    
    void Swap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHeaderSize     = ::PR_htonl(mHeaderSize);
        mFetchCount     = ::PR_htonl(mFetchCount);
        mLastFetched    = ::PR_htonl(mLastFetched);
        mLastModified   = ::PR_htonl(mLastModified);
        mExpirationTime = ::PR_htonl(mExpirationTime);
        mDataSize       = ::PR_htonl(mDataSize);
        mKeySize        = ::PR_htonl(mKeySize);
        mMetaDataSize   = ::PR_htonl(mMetaDataSize);
#endif
    }
    
    void Unswap()
    {
#if defined(IS_LITTLE_ENDIAN)
        mHeaderSize     = ::PR_ntohl(mHeaderSize);
        mFetchCount     = ::PR_ntohl(mFetchCount);
        mLastFetched    = ::PR_ntohl(mLastFetched);
        mLastModified   = ::PR_ntohl(mLastModified);
        mExpirationTime = ::PR_ntohl(mExpirationTime);
        mDataSize       = ::PR_ntohl(mDataSize);
        mKeySize        = ::PR_ntohl(mKeySize);
        mMetaDataSize   = ::PR_ntohl(mMetaDataSize);
#endif
    }
};

struct MetaDataFile : MetaDataHeader {
    char*           mKey;
    char*           mMetaData;

    MetaDataFile()
        :   mKey(nsnull), mMetaData(nsnull)
    {
    }
    
    MetaDataFile(nsCacheEntry* entry)
        :   MetaDataHeader(entry),
            mKey(nsnull), mMetaData(nsnull)
    {
    }
    
    ~MetaDataFile()
    {
        delete[] mKey;
        delete[] mMetaData;
    }
    
    nsresult Init(nsCacheEntry* entry)
    {
        PRUint32 size = 1 + entry->Key()->Length();
        mKey = new char[size];
        if (!mKey) return NS_ERROR_OUT_OF_MEMORY;
        nsCRT::memcpy(mKey, entry->Key()->get(), size);
        return entry->FlattenMetaData(&mMetaData, &mMetaDataSize);
    }

    nsresult Read(nsIInputStream* input);
    nsresult Write(nsIOutputStream* output);
};


/******************************************************************************
 *  nsDiskCacheEntryInfo
 *****************************************************************************/
class nsDiskCacheEntryInfo : public nsICacheEntryInfo {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEENTRYINFO

    nsDiskCacheEntryInfo(const char * deviceID)
        : mDeviceID(deviceID)
    {
        NS_INIT_ISUPPORTS();
    }

    virtual ~nsDiskCacheEntryInfo() {}
    
    nsresult Read(nsIInputStream * input)
    {
        return mMetaDataFile.Read(input);
    }
    
    const char* Key() { return mMetaDataFile.mKey; }
    
private:
    const char * mDeviceID;
    MetaDataFile mMetaDataFile;
};


#endif /* _nsDiskCacheEntry_h_ */
