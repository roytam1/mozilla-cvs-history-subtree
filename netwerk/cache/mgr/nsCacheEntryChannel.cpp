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
 * Copyright (C) 1999 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Furman, fur@netscape.com
 */

#include "nsCacheManager.h"
#include "nsCacheEntryChannel.h"
#include "nsIOutputStream.h"

NS_IMPL_ISUPPORTS3(nsCacheEntryChannel, nsISupports, nsIChannel, nsIRequest)

// A proxy for nsIOutputStream
class CacheOutputStream : public nsIOutputStream {

public:
    CacheOutputStream(nsIOutputStream *aOutputStream, nsCachedNetData *aCacheEntry):
        mOutputStream(aOutputStream), mCacheEntry(aCacheEntry), mStartTime(PR_Now())
        { NS_INIT_REFCNT(); }

    ~CacheOutputStream() {}

    NS_DECL_ISUPPORTS

    NS_IMETHOD Close() {
        mCacheEntry->NoteDownloadTime(mStartTime, PR_Now());
        return mOutputStream->Close();
    }

    NS_IMETHOD Flush() { return mOutputStream->Flush(); }

    NS_IMETHOD
    Write(const char *aBuf, PRUint32 aCount, PRUint32 *aActualBytes) {
        nsresult rv;
    
        *aActualBytes = 0;
        rv = mOutputStream->Write(aBuf, aCount, aActualBytes);
        mCacheEntry->mLogicalLength += *aActualBytes;
        if (NS_FAILED(rv)) return rv;
        nsCacheManager::LimitCacheSize();
        return rv;
    }
    
protected:
    nsCOMPtr<nsIOutputStream> mOutputStream;
    nsCOMPtr<nsCachedNetData> mCacheEntry;

    // Time at which stream was opened
    PRTime mStartTime;
};

NS_IMPL_ISUPPORTS(CacheOutputStream, NS_GET_IID(nsIOutputStream))

NS_IMETHODIMP
nsCacheEntryChannel::OpenOutputStream(PRUint32 aStartPosition, nsIOutputStream* *aOutputStream)
{
    nsresult rv;
    nsCOMPtr<nsIOutputStream> baseOutputStream;
    
    rv = mChannel->OpenOutputStream(aStartPosition, getter_AddRefs(baseOutputStream));
    if (NS_FAILED(rv)) return rv;

    mCacheEntry->NoteUpdate();
    mCacheEntry->NoteAccess();
    mCacheEntry->mLogicalLength = aStartPosition;

    *aOutputStream = new CacheOutputStream(baseOutputStream, mCacheEntry);
    if (!*aOutputStream)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aOutputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsCacheEntryChannel::OpenInputStream(PRUint32 aStartPosition, PRInt32 aReadCount,
				     nsIInputStream* *aInputStream)
{
    mCacheEntry->NoteAccess();
    return mChannel->OpenInputStream(aStartPosition, aReadCount, aInputStream);
}

NS_IMETHODIMP
nsCacheEntryChannel::AsyncRead(PRUint32 aStartPosition, PRInt32 aReadCount,
			       nsISupports *aContext, nsIStreamListener *aListener)
{
    mCacheEntry->NoteAccess();
    return mChannel->AsyncRead(aStartPosition, aReadCount, aContext, aListener);
}

// No async writes allowed to the cache yet
NS_IMETHODIMP
nsCacheEntryChannel::AsyncWrite(nsIInputStream *aFromStream, PRUint32 aStartPosition,
				PRInt32 aWriteCount, nsISupports *aContext,
				nsIStreamObserver *aObserver)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





