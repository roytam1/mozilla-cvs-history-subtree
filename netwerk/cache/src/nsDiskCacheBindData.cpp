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
 * The Original Code is nsDiskCacheBindData.cpp, released May 10, 2001.
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

#include <limits.h>

#include "nsDiskCacheBindData.h"


/******************************************************************************
 *  Utility Functions
 *****************************************************************************/

nsDiskCacheBindData *
GetBindDataFromCacheEntry(nsCacheEntry * entry)
{
    nsCOMPtr<nsISupports> data;
    nsresult rv = entry->GetData(getter_AddRefs(data));
    if (NS_FAILED(rv))  return nsnull;
    
    return (nsDiskCacheBindData *)data.get();
}


/******************************************************************************
 *  nsDiskCacheBindData
 *****************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheBindData);

nsDiskCacheBindData::nsDiskCacheBindData(nsCacheEntry* entry)
    :   mCacheEntry(entry)
{
    NS_INIT_ISUPPORTS();
    PR_INIT_CLIST(this);
    mRecord.SetHashNumber(nsDiskCache::Hash(entry->Key()->get()));
}

nsDiskCacheBindData::~nsDiskCacheBindData()
{
    // XXX if PR_CLIST_IS_EMPTY(this) then remove entry from hashtable
    PR_REMOVE_LINK(this);       // XXX why are we still on a list?
}


/******************************************************************************
 *  nsDiskCacheBindery
 *
 *  Keeps track of bound disk cache entries to detect for collisions.
 *
 *****************************************************************************/

PLDHashTableOps nsDiskCacheBindery::ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    GetKey,
    HashKey,
    MatchEntry,
    MoveEntry,
    ClearEntry,
    Finalize
};


nsDiskCacheBindery::nsDiskCacheBindery()
    : initialized(PR_FALSE)
{
}


nsDiskCacheBindery::~nsDiskCacheBindery()
{
    if (initialized)
        PL_DHashTableFinish(&table);
}


nsresult
nsDiskCacheBindery::Init()
{
    nsresult rv = NS_OK;
    initialized = PL_DHashTableInit(&table, &ops, nsnull,
                                    sizeof(HashTableEntry), 512);

    if (!initialized) rv = NS_ERROR_OUT_OF_MEMORY;
    
    return rv;
}


// XXX need to have nsDiskCacheRecord passed in
nsDiskCacheBindData *
nsDiskCacheBindery::CreateBindDataForCacheEntry(nsCacheEntry * entry)
{
    nsCOMPtr<nsISupports> data;
    nsresult rv = entry->GetData(getter_AddRefs(data));
    if (NS_FAILED(rv) || data) {
        NS_ASSERTION(!data, "cache entry already has bind data");
        return nsnull;
    }
    
    nsDiskCacheBindData * bindData = new nsDiskCacheBindData(entry);
    if (!bindData)  return nsnull;
    
    data = bindData; // add ref
    entry->SetData(data.get());     // XXX why .get() ?
    
    // XXX add bindData to collision detection system

    return bindData;
}


// XXX if we read an entry off of disk (FindEntry)
// XXX      - it may already have a generation number
// XXX      - generation number conflict is an error
// XXX new entries (BindEntry)
// XXX      - assign generation number

// XXX FindActiveBindData(hashNumber) // there can be only one
// XXX FindBindData(hashNumber, generation)

// XXX UnbindEntry(nsDiskCacheBindData * bindData); // called from DeactivateEntry()

nsDiskCacheBindData *
nsDiskCacheBindery::GetEntry(const char * key)
{
    return GetEntry(nsDiskCache::Hash(key));
}


nsDiskCacheBindData *
nsDiskCacheBindery::GetEntry(PLDHashNumber key)
{
    nsDiskCacheBindData * result = nsnull;
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry*) PL_DHashTableOperate(&table, (void*) key, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        result = hashEntry->mBindData;
    }
    return result;
}


nsresult
nsDiskCacheBindery::AddEntry(nsDiskCacheBindData * bindData)
{
    NS_ENSURE_ARG_POINTER(bindData);
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");

    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table,
                                                        (void*) bindData->mRecord.HashNumber(),
                                                        PL_DHASH_ADD);
    if (!hashEntry) return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(hashEntry->mBindData = bindData);

    return NS_OK;
}


void
nsDiskCacheBindery::RemoveEntry(nsDiskCacheBindData * bindData)
{
    NS_ASSERTION(initialized, "nsDiskCacheBindery not initialized");
    NS_ASSERTION(bindData, "### bindData == nsnull");

    (void) PL_DHashTableOperate(&table, (void*) bindData->mRecord.HashNumber(), PL_DHASH_REMOVE);
}


void
nsDiskCacheBindery::VisitEntries(Visitor *visitor)
{
    PL_DHashTableEnumerate(&table, VisitEntry, visitor);
}


PLDHashOperator PR_CALLBACK
nsDiskCacheBindery::VisitEntry(PLDHashTable *        table,
                               PLDHashEntryHdr *     header,
                               PRUint32              number,
                               void *                arg)
{
    HashTableEntry* hashEntry = (HashTableEntry *) header;
    Visitor *visitor = (Visitor*) arg;
    return (visitor->VisitEntry(hashEntry->mBindData) ? PL_DHASH_NEXT : PL_DHASH_STOP);
}

/**
 *  hash table operation callback functions
 */
const void * PR_CALLBACK
nsDiskCacheBindery::GetKey(PLDHashTable * /*table*/, PLDHashEntryHdr * header)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (void*) hashEntry->mBindData->mRecord.HashNumber();
}


PLDHashNumber PR_CALLBACK
nsDiskCacheBindery::HashKey( PLDHashTable *table, const void *key)
{
    return (PLDHashNumber) key;
}


PRBool PR_CALLBACK
nsDiskCacheBindery::MatchEntry(PLDHashTable *             /* table */,
                               const PLDHashEntryHdr *       header,
                               const void *                  key)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (hashEntry->mBindData->mRecord.HashNumber() == (PLDHashNumber) key);
}

void PR_CALLBACK
nsDiskCacheBindery::MoveEntry(PLDHashTable *                  /* table */,
                              const PLDHashEntryHdr *            fromHeader,
                              PLDHashEntryHdr       *            toHeader)
{
    HashTableEntry * fromEntry = (HashTableEntry *) fromHeader;
    HashTableEntry * toEntry = (HashTableEntry *) toHeader;
    toEntry->keyHash = fromEntry->keyHash;
    toEntry->mBindData = fromEntry->mBindData;
    fromEntry->mBindData = nsnull;
}


void PR_CALLBACK
nsDiskCacheBindery::ClearEntry(PLDHashTable *             /* table */,
                               PLDHashEntryHdr *             header)
{
    HashTableEntry* hashEntry = (HashTableEntry *) header;
    hashEntry->keyHash = 0;
    NS_IF_RELEASE(hashEntry->mBindData);
}


void PR_CALLBACK
nsDiskCacheBindery::Finalize(PLDHashTable * table)
{
    (void) PL_DHashTableEnumerate(table, FreeCacheEntries, nsnull);
}


PLDHashOperator PR_CALLBACK
nsDiskCacheBindery::FreeCacheEntries(PLDHashTable *             /* table */,
                                     PLDHashEntryHdr *             header,
                                     PRUint32                      number,
                                     void *                        arg)
{
    HashTableEntry *entry = (HashTableEntry *) header;
    NS_IF_RELEASE(entry->mBindData);
    return PL_DHASH_NEXT;
}
