//	-*- mode:C++; tab-width:4; truncate-lines:t -*-
//
//           CONFIDENTIAL AND PROPRIETARY SOURCE CODE OF
//              NETSCAPE COMMUNICATIONS CORPORATION
// Copyright © 1996, 1997 Netscape Communications Corporation.  All Rights
// Reserved.  Use of this Source Code is subject to the terms of the
// applicable license agreement from Netscape Communications Corporation.
// The copyright notice(s) in this Source Code does not indicate actual or
// intended publication of this Source Code.
//
// $Id$
//

#ifndef _SPARSE_SET_H_
#define _SPARSE_SET_H_

#include "Fundamentals.h"
#include "Pool.h"
#include "LogModule.h"
#include "BitSet.h"

class SparseSet
{
private:

	struct Node {
		Uint32		element;
		Uint32		stackIndex;
	};

	Node*			node;
	Uint32			count;
	Uint32			universeSize;

private:

	// No copy constructor.
	SparseSet(const SparseSet&);

	// Check if the given set's universe is of the same size than this universe.
	void checkUniverseCompatibility(const SparseSet& set) const {assert(set.universeSize == universeSize);}
	// Check if pos is valid for this set's universe.
	void checkMember(Int32 pos) const {assert(pos >=0 && Uint32(pos) < universeSize);}

public:

	SparseSet(Pool& pool, Uint32 universeSize) : universeSize(universeSize) {node = new(pool) Node[universeSize]; clear();}

	// Clear the sparse set.
	void clear() {count = 0;}
	// Clear the element at index.
	inline void clear(Uint32 index);
	// Set the element at index.
	inline void set(Uint32 index);
	// Return true if the element at index is set.
	inline bool test(Uint32 index) const;
	// Union with the given sparse set.
	inline void or(const SparseSet& set);
	// Intersection with the given sparse set.
	inline void and(const SparseSet& set);
	// Difference with the given sparse set.
	inline void difference(const SparseSet& set);
	// Copy set.
	inline SparseSet& operator = (const SparseSet& set);
	inline SparseSet& operator = (const BitSet& set);
	// Return true if the sparse sets are identical.
	friend bool operator == (const SparseSet& set1, const SparseSet& set2);
	// Return true if the sparse sets are different.
	friend bool operator != (const SparseSet& set1, const SparseSet& set2);

	// Logical operators.
	SparseSet& operator |= (const SparseSet& set) {or(set); return *this;}
	SparseSet& operator &= (const SparseSet& set) {and(set); return *this;}
	SparseSet& operator -= (const SparseSet& set) {difference(set); return *this;}

	// Iterator to conform with the set API.
	typedef Int32 iterator;
	// Return the iterator for the first element of this set.
	iterator begin() const {return count - 1;}
	// Return the next iterator.
	iterator advance(iterator pos) const {return --pos;}
	// Return true if the iterator is at the end of the set.
	bool done(iterator pos) const {return pos < 0;}
	// Return the element for the given iterator;
	Uint32 get(iterator pos) const {return node[pos].element;}
	// Return one element of this set.
	Uint32 getOne() const {assert(count > 0); return node[0].element;}
	// Return the size of this set.
	Uint32 getSize() const {return count;}

#ifdef DEBUG_LOG
	// Print the set.
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

inline void SparseSet::clear(Uint32 element)
{
	checkMember(element);
	Uint32 count = this->count;
	Node* node = this->node;

	Uint32 stackIndex = node[element].stackIndex;

	if ((stackIndex < count) && (node[stackIndex].element == element)) {
		Uint32 stackTop = node[count - 1].element;

		node[stackIndex].element = stackTop;
		node[stackTop].stackIndex = stackIndex;
		this->count = count - 1;
	}
}

inline void SparseSet::set(Uint32 element)
{
	checkMember(element);
  	Uint32 count = this->count;
	Node* node = this->node;

	Uint32 stackIndex = node[element].stackIndex;

	if ((stackIndex >= count) || (node[stackIndex].element != element)) {
		node[count].element = element;
		node[element].stackIndex = count;
		this->count = count + 1;
	}
}

inline bool SparseSet::test(Uint32 element) const
{
	checkMember(element);
	Node* node = this->node;

	Uint32 stackIndex = node[element].stackIndex;
	return ((stackIndex < count) && (node[stackIndex].element == element));
}

inline SparseSet& SparseSet::operator = (const SparseSet& set)
{
	checkUniverseCompatibility(set);
	Uint32 sourceCount = set.getSize();
	Node* node = this->node;

	memcpy(node, set.node, sourceCount * sizeof(Node));

	for (Uint32 i = 0; i < sourceCount; i++) {
		Uint32 element = node[i].element;
		node[element].stackIndex = i;
	}

	count = sourceCount;

	return *this;
}


inline SparseSet& SparseSet::operator = (const BitSet& set)
{
	// FIX: there's room for optimization here.
	assert(universeSize == set.getSize());

	clear();
	for (Int32 i = set.firstOne(); i != -1; i = set.nextOne(i))
		this->set(i);
	return *this;
}

#endif // _SPARSE_SET_H_
