/* 
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
 * The Original Code is Mozilla Communicator client code.
 * 
 * The Initial Developer of the Original Code is James L. Nance
 * Portions created by James L. Nance are Copyright (C) 2001
 * James L. Nance.  All  Rights Reserved.
 * 
 * Contributor(s): Patricia Jewell Nance, Jesse Jacob Nance
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

// #define TEST_PATRICIA

#if defined(TEST_PATRICIA)
#   include "stdlib.h"
#   include "stdio.h"
#   include "string.h"
#   define PRBool bool
#   define PRUint32 unsigned int
#   define PRInt32  int
#   define PR_CALLBACK
#   define PLArenaPool int
#   define PL_FinishArenaPool(a)
#   define PL_InitArenaPool(a, b, c, d)
#   define PL_ArenaAllocate(a, size) malloc(size)
#   define PR_TRUE true
#   define PR_FALSE false

    struct nsCRT {
        static int strlen(const char *a) {return ::strlen(a);}
        static int memcmp(const void *a, const void *b, PRUint32 c) {
            return ::memcmp(a,b,c);
        }
    };
#else
#   include "nsCRT.h"
#endif

#include "nsStringMap.h"

const char nsStringMap::zero_str[] = "\0";

nsStringMap::~nsStringMap()
{
    // Get rid of the arena memory
    PL_FinishArenaPool(&mPool);
}

nsStringMap::nsStringMap() : numEntries(0)
{
    // Initialize the head
    head.l   = head.r = &head;
    head.bit = ~0;
    head.key = zero_str;
    head.len = 1;
    head.obj = 0;

    // Initialize the arena.  Guess that a 512 byte block size is good
    PL_InitArenaPool(&mPool, "nsStringMap", 512, sizeof(void*));
}

void
nsStringMap::Reset()
{
    // Initialize the head
    head.l   = head.r = &head;
    head.bit = ~0;
    head.key = zero_str;
    head.len = 1;
    head.obj = 0;

    // Reinitialize the Arena
    PL_FinishArenaPool(&mPool);
    PL_InitArenaPool(&mPool, "nsStringMap", 512, sizeof(void*));
}

void
nsStringMap::Reset(nsStringMapEnumFunc destroyFunc, void *aClosure)
{
    Enumerate(destroyFunc, aClosure);
    Reset();
}

nsStringMap::Patricia *
nsStringMap::newNode()
{
    return (Patricia*) PL_ArenaAllocate(&mPool, sizeof(Patricia));
}

nsStringMap::Patricia *
nsStringMap::searchDown(BitTester &key)
{
    // The head node only branches to the left, so we can optimize here.
    Patricia *x = head.l;

    PRUint32 lastBits;

    do {
        lastBits = x->bit;

        if(key.isset(lastBits))
            x = x->r;
        else
            x = x->l;

    } while(lastBits > x->bit);

    return x;
}

void*
nsStringMap::Get(const char *str, PRUint32 slen)
{
    BitTester key(str, slen);

    Patricia *t = searchDown(key);

    if(!key.memcmp(t->key, t->len)) {
        return t->obj;
    }

    return 0;
}

void*
nsStringMap::Get(const char *str)
{
    BitTester key(str);

    Patricia *t = searchDown(key);

    if(!key.memcmp(t->key, t->len)) {
        return t->obj;
    }

    return 0;
}

PRBool
nsStringMap::Put(const char *str, void *obj, PRBool copy)
{
    PRUint32 slen = nsCRT::strlen(str);
    return Put(str, slen, obj, copy);
}

PRBool
nsStringMap::Put(const char *str, PRUint32 slen, void *obj, PRBool copy)
{
    if(copy) {
        PRUint32 mask  = sizeof(double) - 1;
        PRUint32 asize = (slen+mask) & ~mask;
        char *tstr = (char*) PL_ArenaAllocate(&mPool, asize);
        memcpy(tstr, str, slen);
        str = tstr;
    }

    BitTester key(str, slen);

    Patricia *t = searchDown(key);

    if(!key.memcmp(t->key, t->len)) {
        t->obj = obj;
        return PR_TRUE;
    }

    // This is somewhat ugly.  We need to find the maximum bit position that
    // differs, but this is complicated by the fact that we have random length
    // data.  Assume that data past the end of the string is 0.
    const PRUint32 klen = key.datalen();
    const PRUint32 tlen = t->len;
    PRUint32   bpos;
    if(klen>tlen) {
        bpos = 8 * klen - 1;
        while(!BitTester::isset_checked(str, bpos)) --bpos;
    } else if(tlen>klen) {
        bpos = 8 * tlen - 1;
        while(!BitTester::isset_checked(t->key, bpos)) --bpos;
    } else /* equal */ {
        bpos = 8 * tlen - 1;
        while(BitTester::bitsequal(t->key, str, bpos)) --bpos;
    }

    Patricia *p, *x = &head;

    do {
        p = x;
        x = key.isset(x->bit) ? x->r : x->l;
    } while(x->bit > bpos && p->bit > x->bit);

    t = newNode();

    if(!t) {
        return PR_FALSE;
    }

    t->key = str;
    t->len = key.datalen();
    t->obj = obj;
    t->bit = bpos;

    if(key.isset(t->bit)) {
        t->r = t;
        t->l = x;
    } else {
        t->r = x;
        t->l = t;
    }

    if(key.isset(p->bit)) {
        p->r = t;
    } else {
        p->l = t;
    }

    return PR_TRUE;
}

void
nsStringMap::enumerate_recurse(
nsStringMapEnumFunc aEnumFunc, void* aClosure, Patricia *node)
{
    aEnumFunc(node->key, node->obj, aClosure);
    if(node->l && node->l->bit<node->bit)
        enumerate_recurse(aEnumFunc, aClosure, node->l);
    if(node->r && node->r->bit<node->bit)
        enumerate_recurse(aEnumFunc, aClosure, node->r);
}

void
nsStringMap::Enumerate(nsStringMapEnumFunc aEnumFunc, void *aClosure)
{
    // We dont want to process head, its a sentinal
    if(head.l && head.l->bit<head.bit)
        enumerate_recurse(aEnumFunc, aClosure, head.l);
    if(head.r && head.r->bit<head.bit)
        enumerate_recurse(aEnumFunc, aClosure, head.r);
}

#if defined(TEST_PATRICIA)

PRBool etest(const char *key, void *data, void *closure)
{
    printf("%s\n", key);
    return PR_TRUE;
}

int main()
{
    nsStringMap map;
    const char *strings[] = {
      "I am number 1 string",
      "I am number 2 string",
      "I am number 3 string",
      "a different string",
      "a similar string",
      "I am a very long string and I want to make sure we can handle this",
      "I am a very long string and I want to make sure we can handle this too",
      0
    };

    int idx;
    for(idx=0; strings[idx]; ++idx) {
        map.Put(strings[idx], (void*)(1+idx));
    }

    printf("Lookup Test\n");
    while(--idx>=0) {
        void *ptr = map.Get(strings[idx]);
        printf("%d: %s\n", (long)ptr, strings[idx]);
    }

    printf("\nEnumeration Test\n");

    map.Enumerate(etest, 0);

    return 0;
}

#endif
