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
 * The Original Code is nsDiskCacheBindData.h, released May 10, 2001.
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


#ifndef _nsDiskCacheBindData_h_
#define _nsDiskCacheBindData_h_

#include "nspr.h"
#include "pldhash.h"

#include "nsISupports.h"
#include "nsCacheEntry.h"

#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
#include "nsITransport.h"
#endif

#include "nsDiskCacheMap.h"


/******************************************************************************
 *  nsDiskCacheBindData
 *
 *  Created for disk cache specific data and stored in nsCacheEntry.mData as
 *  an nsISupports.  Also stored in nsDiskCacheHashTable, with collisions
 *  linked by the PRCList.
 *
 *****************************************************************************/

class nsDiskCacheBindData : public nsISupports, public PRCList {
public:
    NS_DECL_ISUPPORTS

    nsDiskCacheBindData(nsCacheEntry* entry);
    virtual ~nsDiskCacheBindData();

#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
    /**
     * Maps a cache access mode to a cached nsITransport for that access
     * mode. We keep these cached to avoid repeated trips to the
     * file transport service.
     */
    nsCOMPtr<nsITransport>& getTransport(nsCacheAccessMode mode)
    {
        return mTransports[mode - 1];
    }
#endif


// XXX make friends
public:
    nsDiskCacheRecord       mRecord;
    PRInt32                 mGeneration;    // possibly just reservation
    nsCacheEntry*           mCacheEntry;    // back pointer to parent nsCacheEntry

private:
#ifdef MOZ_NEW_CACHE_REUSE_TRANSPORTS
    nsCOMPtr<nsITransport>  mTransports[3];
#endif
};


/******************************************************************************
 *  Utility Functions
 *****************************************************************************/

nsDiskCacheBindData *   GetBindDataFromCacheEntry(nsCacheEntry * entry);



/******************************************************************************
 *  nsDiskCacheBindery
 *
 *  Used to keep track of nsDiskCacheBindData associated with active/bound (and
 *  possibly doomed) entries.  Lookups on 4 byte disk hash to find collisions
 *  (which need to be doomed, instead of just evicted.  Collisions are linked
 *  using a PRCList to keep track of current generation number.
 *
 *  Used to detect hash number collisions, and find available generation numbers.
 *
 *  Not all nsDiskCacheBindData have a generation number.
 *
 *  Generation numbers may be aquired late, or lost (when data fits in block file)
 *
 *  Collisions can occur:
 *      BindEntry()       - hashnumbers collide (possibly different keys)
 *
 *  Generation number required:
 *      DeactivateEntry() - metadata written to disk, may require file
 *      GetFileForEntry() - force data to require file
 *      writing to stream - data size may require file
 *
 *  BindData can be kept in PRCList in order of generation numbers.
 *  BindData with no generation number can be Appended to PRCList (last).
 *
 *****************************************************************************/

class nsDiskCacheBindery {
public:
    nsDiskCacheBindery();
    ~nsDiskCacheBindery();

    nsresult                Init();

    nsDiskCacheBindData *   CreateBindDataForCacheEntry(nsCacheEntry * entry);

    nsDiskCacheBindData *   GetEntry(const char *              key);
    nsDiskCacheBindData *   GetEntry(PLDHashNumber             key);
    nsresult                AddEntry(nsDiskCacheBindData *     entry);
    void                    RemoveEntry(nsDiskCacheBindData *  entry);
    
    class Visitor {
    public:
        virtual PRBool      VisitEntry(nsDiskCacheBindData *       entry) = 0;
    };
    
    void                    VisitEntries(Visitor *              visitor);
    
private:
    struct HashTableEntry : PLDHashEntryHdr {
        nsDiskCacheBindData *  mBindData;
    };

    // PLDHashTable operation callbacks
    static const void *     PR_CALLBACK GetKey(PLDHashTable *               table,
                                               PLDHashEntryHdr *            entry);

    static PLDHashNumber    PR_CALLBACK HashKey(PLDHashTable *              table,
                                                const void *                key);

    static PRBool           PR_CALLBACK MatchEntry(PLDHashTable *           table,
                                                   const PLDHashEntryHdr *  entry,
                                                   const void *             key);

    static void             PR_CALLBACK MoveEntry(PLDHashTable *            table,
                                                  const PLDHashEntryHdr *   from,
                                                  PLDHashEntryHdr       *   to);

    static void             PR_CALLBACK ClearEntry(PLDHashTable *           table,
                                                   PLDHashEntryHdr *        entry);

    static void             PR_CALLBACK Finalize(PLDHashTable *table);

    static
    PLDHashOperator         PR_CALLBACK FreeCacheEntries(PLDHashTable *     table,
                                                         PLDHashEntryHdr *  hdr,
                                                         PRUint32           number,
                                                         void *             arg);
    static
    PLDHashOperator         PR_CALLBACK VisitEntry(PLDHashTable *           table,
                                                   PLDHashEntryHdr *        hdr,
                                                   PRUint32                 number,
                                                   void *                   arg);
                                     
    // member variables
    static PLDHashTableOps ops;
    PLDHashTable           table;
    PRBool                 initialized;
};

#endif /* _nsDiskCacheBindData_h_ */
