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

#ifndef _BITSET_H_
#define _BITSET_H_

#include "Fundamentals.h"
#include "LogModule.h"
#include "Pool.h"
#include <string.h>

//------------------------------------------------------------------------------
// BitSet -

class BitSet
{
private:

#if (PR_BITS_PER_WORD == 64)
	typedef Uint64 Word;
#elif (PR_BITS_PER_WORD == 32)
	typedef Uint32 Word;
#endif

	static const nBitsInWord = 		PR_BITS_PER_WORD;
	static const nBytesInWord = 	PR_BYTES_PER_WORD;
	static const nBitsInWordLog2 =	PR_BITS_PER_WORD_LOG2;
	static const nBytesInWordLog2 = PR_BYTES_PER_WORD_LOG2;

	// Return the number of Word need to store the universe.
	static Uint32 getSizeInWords(Uint32 sizeOfUniverse) {return (sizeOfUniverse + (nBitsInWord - 1)) >> nBitsInWordLog2;}
	// Return the given element offset in its containing Word.
	static Uint32 getBitOffset(Uint32 element) {return element & (nBitsInWord - 1);}
	// Return the Word offset for the given element int the universe.
	static Uint32 getWordOffset(Uint32 element) {return element >> nBitsInWordLog2;}
	// Return the mask for the given bit index.
	static Word getMask(Uint8 index) {return Word(1) << index;}
	
private:

	Uint32							universeSize;			// Size of the universe
	Word*							word;					// universe memory.

private:

	// No copy constructor.
	BitSet(const BitSet&);

	// Check if the given set's universe is of the same size than this universe.
	void checkUniverseCompatibility(const BitSet& set) const {assert(set.universeSize == universeSize);}
	// Check if pos is valid for this set's universe.
	void checkMember(Int32 pos) const {assert(pos >=0 && Uint32(pos) < universeSize);}

public:

	// Create a bitset of universeSize bits.
	BitSet(Pool& pool, Uint32 universeSize) : universeSize(universeSize) {word = new(pool) Word[getSizeInWords(universeSize)]; clear();}

	// Return the size of this bitset.
	Uint32 getSize() const {return universeSize;}

	// Clear the bitset.
	void clear() {memset(word, 0x00, getSizeInWords(universeSize) << nBytesInWordLog2);}
	// Clear the bit at index.
	void clear(Uint32 index) {checkMember(index); word[getWordOffset(index)] &= ~getMask(index);}
	// Set the bitset.
	void set() {memset(word, 0xFF, getSizeInWords(universeSize) << nBytesInWordLog2);}
	// Set the bit at index.
	void set(Uint32 index) {checkMember(index); word[getWordOffset(index)] |= getMask(index);}
	// Return true if the bit at index is set.
	bool test(Uint32 index) const {checkMember(index); return (word[getWordOffset(index)] & getMask(index)) != 0;}
	// Union with the given bitset.
	inline void or(const BitSet& set);
	// Intersection with the given bitset.
	inline void and(const BitSet& set);
	// Difference with the given bitset.
	inline void difference(const BitSet& set);
	// Copy set.
	inline BitSet& operator = (const BitSet& set);
	// Return true if the bitset are identical.
	friend bool operator == (const BitSet& set1, const BitSet& set2);
	// Return true if the bitset are different.
	friend bool operator != (const BitSet& set1, const BitSet& set2);

	// Logical operators.
	BitSet& operator |= (const BitSet& set) {or(set); return *this;}
	BitSet& operator &= (const BitSet& set) {and(set); return *this;}
	BitSet& operator -= (const BitSet& set) {difference(set); return *this;}

	// Return the first bit at set to true or -1 if none.
	Int32 firstOne() const {return nextOne(-1);}
	// Return the next bit after index set to true or -1 if none.
	Int32 nextOne(Int32 pos) const;
	// Return the first bit at set to false or -1 if none.
	Int32 firstZero() const {return nextZero(-1);}
	// Return the next bit after index set to false or -1 if none.
	Int32 nextZero(Int32 pos) const;

	// Iterator to conform with the set API.
	typedef Int32 iterator;
	// Return true if the walk is ordered.
	static bool isOrdered() {return true;}
	// Return the iterator for the first element of this set.
	iterator begin() const {return firstOne();}
	// Return the next iterator.
	iterator advance(iterator pos) const {return nextOne(pos);}
	// Return true if the iterator is at the end of the set.
	bool done(iterator pos) const {return pos == -1;}
	// Return the element corresponding to the given iterator.
	Uint32 get(iterator pos) const {return pos;}

#ifdef DEBUG_LOG
	// Print the set.
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

// Union with the given bitset.
//
inline void  BitSet::or(const BitSet& set)
{
	checkUniverseCompatibility(set);
	Word* src = set.word;
	Word* dst = word;
	Word* limit = &src[getSizeInWords(universeSize)];

	while (src < limit)
		*dst++ |= *src++;
}

// Intersection with the given bitset.
//
inline void  BitSet::and(const BitSet& set)
{
	checkUniverseCompatibility(set);
	Word* src = set.word;
	Word* dst = word;
	Word* limit = &src[getSizeInWords(universeSize)];

	while (src < limit)
		*dst++ &= *src++;
}

// Difference with the given bitset.
//
inline void BitSet::difference(const BitSet& set)
{
	checkUniverseCompatibility(set);
	Word* src = set.word;
	Word* dst = word;
	Word* limit = &src[getSizeInWords(universeSize)];

	while (src < limit)
		*dst++ &= ~*src++;
}

// Copy the given set into this set.
//
inline BitSet& BitSet::operator = (const BitSet& set)
{
	checkUniverseCompatibility(set);
	if (this != &set)
		memcpy(word, set.word, getSizeInWords(universeSize) << nBytesInWordLog2);
	return *this;
}

// Return true if the given set is identical to this set.
inline bool operator == (const BitSet& set1, const BitSet& set2)
{
	set1.checkUniverseCompatibility(set2);

	if (&set1 == &set2)
		return true;

	return memcmp(set1.word, set2.word, BitSet::getSizeInWords(set1.universeSize) << BitSet::nBytesInWordLog2) == 0;
}

inline bool operator != (const BitSet& set1, const BitSet& set2) {return !(set1 == set2);}

#endif // _BITSET_H
