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
 *  nsDiskCacheBindData
 *****************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheBindData);

PLDHashNumber
nsDiskCacheBindData::Hash(const char* key)
{
    PLDHashNumber h = 0;
    for (const PRUint8* s = (PRUint8*) key; *s != '\0'; ++s)
        h = (h >> (PL_DHASH_BITS - 4)) ^ (h << 4) ^ *s;
    return (h == 0 ? ULONG_MAX : h);
}


/******************************************************************************
 *  nsDiskCacheHashTable
 *****************************************************************************/

PLDHashTableOps nsDiskCacheHashTable::ops =
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


nsDiskCacheHashTable::nsDiskCacheHashTable()
    : initialized(PR_FALSE)
{
}


nsDiskCacheHashTable::~nsDiskCacheHashTable()
{
    if (initialized)
        PL_DHashTableFinish(&table);
}


nsresult
nsDiskCacheHashTable::Init()
{
    nsresult rv = NS_OK;
    initialized = PL_DHashTableInit(&table, &ops, nsnull,
                                    sizeof(HashTableEntry), 512);

    if (!initialized) rv = NS_ERROR_OUT_OF_MEMORY;
    
    return rv;
}


nsDiskCacheBindData *
nsDiskCacheHashTable::GetEntry(const char * key)
{
    return GetEntry(nsDiskCacheBindData::Hash(key));
}


nsDiskCacheBindData *
nsDiskCacheHashTable::GetEntry(PLDHashNumber key)
{
    nsDiskCacheBindData * result = nsnull;
    NS_ASSERTION(initialized, "nsDiskCacheHashTable not initialized");
    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry*) PL_DHashTableOperate(&table, (void*) key, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(hashEntry)) {
        result = hashEntry->mDiskCacheBindData;
    }
    return result;
}


nsresult
nsDiskCacheHashTable::AddEntry(nsDiskCacheBindData * entry)
{
    NS_ENSURE_ARG_POINTER(entry);
    NS_ASSERTION(initialized, "nsDiskCacheHashTable not initialized");

    HashTableEntry * hashEntry;
    hashEntry = (HashTableEntry *) PL_DHashTableOperate(&table,
                                                        (void*) entry->getHashNumber(),
                                                        PL_DHASH_ADD);
    if (!hashEntry) return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(hashEntry->mDiskCacheBindData = entry);

    return NS_OK;
}


void
nsDiskCacheHashTable::RemoveEntry(nsDiskCacheBindData * entry)
{
    NS_ASSERTION(initialized, "nsDiskCacheHashTable not initialized");
    NS_ASSERTION(entry, "### cacheEntry == nsnull");

    (void) PL_DHashTableOperate(&table, (void*) entry->getHashNumber(), PL_DHASH_REMOVE);
}


void
nsDiskCacheHashTable::VisitEntries(Visitor *visitor)
{
    PL_DHashTableEnumerate(&table, VisitEntry, visitor);
}


PLDHashOperator PR_CALLBACK
nsDiskCacheHashTable::VisitEntry(PLDHashTable *        table,
                                 PLDHashEntryHdr *     header,
                                 PRUint32              number,
                                 void *                arg)
{
    HashTableEntry* hashEntry = (HashTableEntry *) header;
    Visitor *visitor = (Visitor*) arg;
    return (visitor->VisitEntry(hashEntry->mDiskCacheBindData) ? PL_DHASH_NEXT : PL_DHASH_STOP);
}

/**
 *  hash table operation callback functions
 */
const void * PR_CALLBACK
nsDiskCacheHashTable::GetKey(PLDHashTable * /*table*/, PLDHashEntryHdr * header)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (void*) hashEntry->mDiskCacheBindData->getHashNumber();
}


PLDHashNumber PR_CALLBACK
nsDiskCacheHashTable::HashKey( PLDHashTable *table, const void *key)
{
    return (PLDHashNumber) key;
}


PRBool PR_CALLBACK
nsDiskCacheHashTable::MatchEntry(PLDHashTable *             /* table */,
                                 const PLDHashEntryHdr *       header,
                                 const void *                  key)
{
    HashTableEntry * hashEntry = (HashTableEntry *) header;
    return (hashEntry->mDiskCacheBindData->getHashNumber() == (PLDHashNumber) key);
}

void PR_CALLBACK
nsDiskCacheHashTable::MoveEntry(PLDHashTable *                  /* table */,
                                const PLDHashEntryHdr *            fromHeader,
                                PLDHashEntryHdr       *            toHeader)
{
    HashTableEntry * fromEntry = (HashTableEntry *) fromHeader;
    HashTableEntry * toEntry = (HashTableEntry *) toHeader;
    toEntry->keyHash = fromEntry->keyHash;
    toEntry->mDiskCacheBindData = fromEntry->mDiskCacheBindData;
    fromEntry->mDiskCacheBindData = nsnull;
}


void PR_CALLBACK
nsDiskCacheHashTable::ClearEntry(PLDHashTable *             /* table */,
                                 PLDHashEntryHdr *             header)
{
    HashTableEntry* hashEntry = (HashTableEntry *) header;
    hashEntry->keyHash = 0;
    NS_IF_RELEASE(hashEntry->mDiskCacheBindData);
}


void PR_CALLBACK
nsDiskCacheHashTable::Finalize(PLDHashTable * table)
{
    (void) PL_DHashTableEnumerate(table, FreeCacheEntries, nsnull);
}


PLDHashOperator PR_CALLBACK
nsDiskCacheHashTable::FreeCacheEntries(PLDHashTable *             /* table */,
                                       PLDHashEntryHdr *             header,
                                       PRUint32                      number,
                                       void *                        arg)
{
    HashTableEntry *entry = (HashTableEntry *) header;
    NS_IF_RELEASE(entry->mDiskCacheBindData);
    return PL_DHASH_NEXT;
}
