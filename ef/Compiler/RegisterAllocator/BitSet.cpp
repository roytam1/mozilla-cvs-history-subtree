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

#include "Fundamentals.h"
#include "BitSet.h"

// Return the next bit after index set to true or -1 if none.
//
Int32 BitSet::nextOne(Int32 pos) const
{
	++pos;

	if (pos < 0 || Uint32(pos) >= universeSize)
		return -1;

	Uint32 offset = getWordOffset(pos);
	Uint8 index = getBitOffset(pos);
	Word* ptr = &word[offset];
	Word currentWord = *ptr++ >> index;
	
	if (currentWord != Word(0)) {
		while ((currentWord & Word(1)) == 0) {
			++index;
			currentWord >>= 1;
		}
		return (offset << nBitsInWordLog2) + index;
	}

	Word* limit = &word[getSizeInWords(universeSize)];
	while (ptr < limit) {
		++offset;
		currentWord = *ptr++;
		if (currentWord != Word(0)) {
			index = 0;
			while ((currentWord & Word(1)) == 0) {
				++index;
				currentWord >>= 1;
			}
			return (offset << nBitsInWordLog2) + index;
		}
	}
	return -1;
}

// Return the next bit after index set to false or -1 if none.
//
Int32 BitSet::nextZero(Int32 pos) const
{
	++pos;

	if (pos < 0 || Uint32(pos) >= universeSize)
		return -1;

	Uint32 offset = getWordOffset(pos);
	Uint8 index = getBitOffset(pos);
	Word* ptr = &word[offset];
	Word currentWord = *ptr++ >> index;
	
	if (currentWord != Word(~0)) {
		for (; index < nBitsInWord; ++index) {
			if ((currentWord & Word(1)) == 0) {
				Int32 ret = (offset << nBitsInWordLog2) + index;
				return (Uint32(ret) < universeSize) ? ret : -1;
			}
			currentWord >>= 1;
		}
	}

	Word* limit = &word[getSizeInWords(universeSize)];
	while (ptr < limit) {
		++offset;
		currentWord = *ptr++;
		if (currentWord != Word(~0)) {
			for (index = 0; index < nBitsInWord; ++index) {
				if ((currentWord & Word(1)) == 0) {
					Int32 ret = (offset << nBitsInWordLog2) + index;
					return (Uint32(ret) < universeSize) ? ret : -1;
				}
				currentWord >>= 1;
			}
		}
	}
	return -1;
}

#ifdef DEBUG_LOG

// Print the set.
//
void BitSet::printPretty(LogModuleObject log)
{
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("[ "));

	for (Int32 i = firstOne(); i != -1; i = nextOne(i)) {
		Int32 currentBit = i;
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("%d", currentBit));

		Int32 nextBit = nextOne(currentBit);
		if (nextBit != currentBit + 1) {
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, (" "));
			continue;
		}
		  
		while ((nextBit != -1) && (nextBit == (currentBit + 1))) {
			currentBit = nextBit;
			nextBit = nextOne(nextBit);
		}

		if (currentBit > (i+1))
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("-%d ", currentBit));
		else
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, (" %d ", currentBit));

		i = currentBit;
	}
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("]\n"));
}

#endif // DEBUG_LOG
