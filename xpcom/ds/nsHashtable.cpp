/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

#include "prmem.h"
#include "nsHashtable.h"

//
// Key operations
//

static PR_CALLBACK PLHashNumber _hashValue(const void *key) 
{
  return ((nsHashKey *) key)->HashValue();
}

static PR_CALLBACK PRIntn _hashKeyCompare(const void *key1, const void *key2) {
  return ((nsHashKey *) key1)->Equals((nsHashKey *) key2);
}

static PR_CALLBACK PRIntn _hashValueCompare(const void *value1,
                                            const void *value2) {
  // We're not going to make any assumptions about value equality
  return 0;
}

//
// Memory callbacks
//

static PR_CALLBACK void *_hashAllocTable(void *pool, PRSize size) {
  return PR_MALLOC(size);
}

static PR_CALLBACK void _hashFreeTable(void *pool, void *item) {
  PR_DELETE(item);
}

static PR_CALLBACK PLHashEntry *_hashAllocEntry(void *pool, const void *key) {
  return PR_NEW(PLHashEntry);
}

static PR_CALLBACK void _hashFreeEntry(void *pool, PLHashEntry *entry, 
                                       PRUintn flag) {
  if (flag == HT_FREE_ENTRY) {
    delete (nsHashKey *) (entry->key);
    PR_DELETE(entry);
  }
}

#ifdef XP_OS2_VACPP
/* VACPP on OS/2 requires explicit _Optlink on the declaration, or a cast. */
/* We fake out the cast by redefining the function names.                  */
#define _hashValue        (PLHashNumber(*_Optlink)(const void*))(_hashValue)
#define _hashKeyCompare   (PRIntn(*_Optlink)(const void*,const void*))(_hashKeyCompare)
#define _hashValueCompare (PRIntn(*_Optlink)(const void*,const void*))(_hashValueCompare)
#define _hashAllocTable (void*(*_Optlink)(void*,PRSize))(_hashAllocTable)
#define _hashFreeTable  (void(*_Optlink)(void*,void*))(_hashFreeTable)
#define _hashAllocEntry (PLHashEntry*(*_Optlink)(void*,const void*))(_hashAllocEntry)
#define _hashFreeEntry (void(*_Optlink)(void*,PLHashEntry*,PRUintn))(_hashFreeEntry)
#endif

static PLHashAllocOps _hashAllocOps = {
    _hashAllocTable, _hashFreeTable,
    _hashAllocEntry, _hashFreeEntry
};

//
// Enumerator callback
//

static PR_CALLBACK PRIntn _hashEnumerate(PLHashEntry *he, PRIntn i, void *arg)
{
  return ((nsHashtableEnumFunc) arg)((nsHashKey *) he->key, he->value) ? 
    HT_ENUMERATE_NEXT : 
    HT_ENUMERATE_STOP;
}

#ifdef XP_OS2_VACPP
/* See comment above. */
#define _hashEnumerate (PRIntn(*_Optlink)(PLHashEntry*,PRIntn,void*))(_hashEnumerate)
#endif

nsHashtable::nsHashtable(PRUint32 aInitSize) {
  hashtable = PL_NewHashTable(aInitSize,
                              _hashValue,
                              _hashKeyCompare,
                              _hashValueCompare,
                              &_hashAllocOps,
                              NULL);
}

nsHashtable::~nsHashtable() {
  PL_HashTableDestroy(hashtable);
}

void *nsHashtable::Put(nsHashKey *aKey, void *aData) {
  void *res =  NULL;
  PLHashNumber hash = aKey->HashValue();
  PLHashEntry *he;
  PLHashEntry **hep = PL_HashTableRawLookup(hashtable, hash, (void *) aKey);

  if ((he = *hep) != NULL) {
    res = he->value;
    he->value = aData;
  } else {
    PL_HashTableRawAdd(hashtable, hep, hash,
                       (void *) aKey->Clone(), aData);
  }

  return res;
}

void *nsHashtable::Get(nsHashKey *aKey) {
  return PL_HashTableLookup(hashtable, (void *) aKey);
}

void *nsHashtable::Remove(nsHashKey *aKey) {
  PLHashNumber hash = aKey->HashValue();
  PLHashEntry *he;
  PLHashEntry **hep = PL_HashTableRawLookup(hashtable, hash, (void *) aKey);
  void *res = NULL;
  
  if ((he = *hep) != NULL) {
    res = he->value;
    PL_HashTableRawRemove(hashtable, hep, he);
  }

  return res;
}

void nsHashtable::Enumerate(nsHashtableEnumFunc aEnumFunc) {
  PL_HashTableEnumerateEntries(hashtable, _hashEnumerate, aEnumFunc);
}
