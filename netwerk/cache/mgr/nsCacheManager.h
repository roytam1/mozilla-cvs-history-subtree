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

#ifndef _nsCacheManager_h_
#define _nsCacheManager_h_

#include "nsINetDataCacheManager.h"
#include "nsCOMPtr.h"

class nsHashtable;
class nsReplacementPolicy;
class nsCachedNetData;

class nsCacheManager : public nsINetDataCacheManager {

    nsCacheManager();
    ~nsCacheManager();

    NS_METHOD Init();

    // nsISupports methods
    NS_DECL_ISUPPORTS

    // nsINetDataCacheManager methods
    NS_DECL_NSINETDATACACHEMANAGER

private:
	
    // Mapping from cache key to nsCachedNetData, but only for those cache
    // entries with external references, i.e. those referred to outside the
    // cache manager
    nsHashtable*              mActiveCacheRecords;

    // Memory cache
    nsCOMPtr<nsINetDataCache> mMemCache;

    // Flat-file database cache; All content aggregated into single disk file
    nsCOMPtr<nsINetDataCache> mFlatCache;

    // stream-as-file cache
    nsCOMPtr<nsINetDataCache> mFileCache;

    // Unified replacement policy for flat-cache and file-cache
    nsReplacementPolicy*      mDiskSpaceManager;
    
    // Replacement policy for memory cache
    nsReplacementPolicy*      mMemSpaceManager;

    // List of caches in search order
    nsINetDataCache*          mCacheSearchChain;

    // Combined file/flat cache capacity, in KB
    PRUint32                  mDiskCacheCapacity;

    // Memory cache capacity, in KB
    PRUint32                  mMemCacheCapacity;

protected:
    static nsresult NoteDormant(nsCachedNetData* aEntry);
    static nsresult LimitCacheSize();
    static nsresult LimitMemCacheSize();
    static nsresult LimitDiskCacheSize();

    friend class nsCachedNetData;
};

#endif // _nsCacheManager_h_
