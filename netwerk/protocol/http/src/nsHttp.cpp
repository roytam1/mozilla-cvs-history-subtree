/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Gagan Saksena <gagan@netscape.com> (original author)
 *   Darin Fisher <darin@netscape.com>
 */
#include "nsHttp.h"
#include "nscore.h"
#include "plhash.h"
#include "nsCRT.h"
#include "nsPromiseFlatString.h"

#if defined(PR_LOGGING)
PRLogModuleInfo *gHttpLog = nsnull;
#endif

// define storage for all atoms
#define HTTP_ATOM(_name, _value) nsHttpAtom nsHttp::_name = { _value };
#include "nsHttpAtomList.h"
#undef HTTP_ATOM

static struct PLHashTable *gHttpAtomTable;

// Hash string ignore case, based on PL_HashString
static PLHashNumber
StringHash(const PRUint8 *key)
{
    PLHashNumber h;
    const PRUint8 *s;

    h = 0;
    for (s = key; *s; s++)
        h = (h >> 28) ^ (h << 4) ^ nsCRT::ToLower((char)*s);
    return h;
}

static PRIntn
StringCompare(const char *a, const char *b)
{
    return PL_strcasecmp(a, b) == 0;
}

#if 0
#define NBUCKETS(ht)    (1 << (PL_HASH_BITS - (ht)->shift))
static void
DumpAtomTable()
{
    if (gHttpAtomTable) {
        PLHashEntry *he, **hep;
        PRUint32 i, nbuckets = NBUCKETS(gHttpAtomTable);
        for (i=0; i<nbuckets; ++i) {
            printf("bucket %d: ", i);
            hep = &gHttpAtomTable->buckets[i];
            while ((he = *hep) != 0) {
                printf("(%s,%x,%x) ", (const char *) he->key, he->keyHash, he->value);
                hep = &he->next;
            }
            printf("\n");
        }
    }
}
#endif

// We put the atoms in a hash table for speedy lookup.. see ResolveAtom.
static nsresult
CreateAtomTable()
{
    LOG(("CreateAtomTable\n"));

    if (gHttpAtomTable)
        return NS_OK;

    gHttpAtomTable = PL_NewHashTable(128, (PLHashFunction) StringHash,
                                          (PLHashComparator) StringCompare,
                                          (PLHashComparator) 0, 0, 0);
    if (!gHttpAtomTable)
        return NS_ERROR_OUT_OF_MEMORY;

#define HTTP_ATOM(_name, _value) \
    PL_HashTableAdd(gHttpAtomTable, _value, (void *) nsHttp::_name.get());
#include "nsHttpAtomList.h"
#undef HTTP_ATOM

    //DumpAtomTable();
    return NS_OK;
}

void
nsHttp::DestroyAtomTable()
{
    if (gHttpAtomTable) {
        PL_HashTableDestroy(gHttpAtomTable);
        gHttpAtomTable = nsnull;
    }
}

nsHttpAtom
nsHttp::ResolveAtom(const nsACString &str)
{
    if (!gHttpAtomTable)
        CreateAtomTable();

    nsHttpAtom atom = { nsnull };

    if (gHttpAtomTable)
        atom._val = (const char *)
                PL_HashTableLookup(gHttpAtomTable, PromiseFlatCString(str).get());

    return atom;
}
