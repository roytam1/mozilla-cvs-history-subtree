/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

#ifndef _HASH_SET_H_
#define _HASH_SET_H_

#include "Fundamentals.h"
#include "Pool.h"
#include <string.h>

struct HashSetElement
{
	Uint32 index;
	HashSetElement* next;
};

class HashSet
{
private:

	static const hashSize = 64;

	// Return the hash code for the given element index.
	static Uint32 getHashCode(Uint32 index) {return index & (hashSize - 1);} // Could be better !

private:

	Pool&					allocationPool;
	HashSetElement**		bucket;
	HashSetElement*			free;

private:

	// No copy constructor.
	HashSet(const HashSet&);
	// No copy operator.
	void operator = (const HashSet&);

public:

	// Create a new HashSet.
	inline HashSet(Pool& pool, Uint32 universeSize);

	// Clear the hashset.
	void clear();
	// Clear the element for the given index.
	void clear(Uint32 index);
	// Set the element for the given index.
	void set(Uint32 index);
	// Return true if the element at index is a member.
	bool test(Uint32 index) const;
	// Union with the given hashset.
	inline void or(const HashSet& set);
	// Intersection with the given hashset.
	inline void and(const HashSet& set);
	// Difference with the given hashset.
	inline void difference(const HashSet& set);

	// Logical operators.
	HashSet& operator |= (const HashSet& set) {or(set); return *this;}
	HashSet& operator &= (const HashSet& set) {and(set); return *this;}
	HashSet& operator -= (const HashSet& set) {difference(set); return *this;}

	// Iterator to conform with the set API.
	typedef HashSetElement* iterator;
	// Return the iterator for the first element of this set.
	iterator begin() const;
	// Return the next iterator.
	iterator advance(iterator pos) const;
	// Return true if the iterator is at the end of the set.
	bool done(iterator pos) const {return pos == NULL;}
};


inline HashSet::HashSet(Pool& pool, Uint32 /*universeSize*/)
	: allocationPool(pool), free(NULL)
{
	bucket = new(pool) HashSetElement*[hashSize];
	memset(bucket, '\0', sizeof(HashSetElement*));
}

#endif // _HASH_SET_H_
