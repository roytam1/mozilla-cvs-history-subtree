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

#ifndef JSplhash_h___
#define JSplhash_h___
/*
 * API to portable hash table code.
 */
#include <stddef.h>
#include <stdio.h>
#include "jsprtypes.h"

JSPR_BEGIN_EXTERN_C

typedef struct JSPRHashEntry  JSPRHashEntry;
typedef struct JSPRHashTable  JSPRHashTable;
typedef JSPRUint32 JSPRHashNumber;
#define JSPR_HASH_BITS 32
typedef JSPRHashNumber (JSCALLBACK *JSPRHashFunction)(const void *key);
typedef JSPRIntn (JSCALLBACK *JSPRHashComparator)(const void *v1, const void *v2);
typedef JSPRIntn (JSCALLBACK *JSPRHashEnumerator)(JSPRHashEntry *he, JSPRIntn i, void *arg);

/* Flag bits in JSPRHashEnumerator's return value */
#define HT_ENUMERATE_NEXT       0       /* continue enumerating entries */
#define HT_ENUMERATE_STOP       1       /* stop enumerating entries */
#define HT_ENUMERATE_REMOVE     2       /* remove and free the current entry */
#define HT_ENUMERATE_UNHASH     4       /* just unhash the current entry */

typedef struct JSPRHashAllocOps {
    void *              (JSCALLBACK *allocTable)(void *pool, JSPRSize size);
    void                (JSCALLBACK *freeTable)(void *pool, void *item);
    JSPRHashEntry *       (JSCALLBACK *allocEntry)(void *pool, const void *key);
    void                (JSCALLBACK *freeEntry)(void *pool, JSPRHashEntry *he, JSPRUintn flag);
} JSPRHashAllocOps;

#define HT_FREE_VALUE   0               /* just free the entry's value */
#define HT_FREE_ENTRY   1               /* free value and entire entry */

struct JSPRHashEntry {
    JSPRHashEntry         *next;          /* hash chain linkage */
    JSPRHashNumber        keyHash;        /* key hash function result */
    const void          *key;           /* ptr to opaque key */
    void                *value;         /* ptr to opaque value */
};

struct JSPRHashTable {
    JSPRHashEntry         **buckets;      /* vector of hash buckets */
    JSPRUint32              nentries;       /* number of entries in table */
    JSPRUint32              shift;          /* multiplicative hash shift */
    JSPRHashFunction      keyHash;        /* key hash function */
    JSPRHashComparator    keyCompare;     /* key comparison function */
    JSPRHashComparator    valueCompare;   /* value comparison function */
    JSPRHashAllocOps      *allocOps;      /* allocation operations */
    void                *allocPriv;     /* allocation private data */
#ifdef HASHMETER
    JSPRUint32              nlookups;       /* total number of lookups */
    JSPRUint32              nsteps;         /* number of hash chains traversed */
    JSPRUint32              ngrows;         /* number of table expansions */
    JSPRUint32              nshrinks;       /* number of table contractions */
#endif
};

/*
 * Create a new hash table.
 * If allocOps is null, use default allocator ops built on top of malloc().
 */
JSEXTERN(JSPRHashTable *)
JSPR_NewHashTable(JSPRUint32 n, JSPRHashFunction keyHash,
                JSPRHashComparator keyCompare, JSPRHashComparator valueCompare,
                JSPRHashAllocOps *allocOps, void *allocPriv);

JSEXTERN(void)
JSPR_HashTableDestroy(JSPRHashTable *ht);

/* Low level access methods */
JSEXTERN(JSPRHashEntry **)
JSPR_HashTableRawLookup(JSPRHashTable *ht, JSPRHashNumber keyHash, const void *key);

JSEXTERN(JSPRHashEntry *)
JSPR_HashTableRawAdd(JSPRHashTable *ht, JSPRHashEntry **hep, JSPRHashNumber keyHash,
                   const void *key, void *value);

JSEXTERN(void)
JSPR_HashTableRawRemove(JSPRHashTable *ht, JSPRHashEntry **hep, JSPRHashEntry *he);

/* Higher level access methods */
JSEXTERN(JSPRHashEntry *)
JSPR_HashTableAdd(JSPRHashTable *ht, const void *key, void *value);

JSEXTERN(JSPRBool)
JSPR_HashTableRemove(JSPRHashTable *ht, const void *key);

JSEXTERN(JSPRIntn)
JSPR_HashTableEnumerateEntries(JSPRHashTable *ht, JSPRHashEnumerator f, void *arg);

JSEXTERN(void *)
JSPR_HashTableLookup(JSPRHashTable *ht, const void *key);

JSEXTERN(JSPRIntn)
JSPR_HashTableDump(JSPRHashTable *ht, JSPRHashEnumerator dump, FILE *fp);

/* General-purpose C string hash function. */
JSEXTERN(JSPRHashNumber)
JSPR_HashString(const void *key);

/* Compare strings using strcmp(), return true if equal. */
JSEXTERN(int)
JSPR_CompareStrings(const void *v1, const void *v2);

/* Stub function just returns v1 == v2 */
JSEXTERN(JSPRIntn)
JSPR_CompareValues(const void *v1, const void *v2);

JSPR_END_EXTERN_C

#endif /* JSplhash_h___ */
