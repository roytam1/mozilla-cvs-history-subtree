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

#ifndef _REGISTER_ALLOCATOR_TOOLS_H_
#define _REGISTER_ALLOCATOR_TOOLS_H_

#include "LogModule.h"
#include "RegisterTypes.h"
#include <string.h>

class RegisterAllocator;
class ControlGraph;
class InstructionEmitter;
class VirtualRegisterManager;

struct RegisterAllocatorTools
{
	//
	//
	static void insertPhiNodeInstructions(ControlGraph& controlGraph, InstructionEmitter& emitter);

	//
	//
	static void updateInstructionGraph(RegisterAllocator& registerAllocator);

	//
	//
	static void removeUnnecessaryCopies(RegisterAllocator& registerAllocator);

#ifdef DEBUG
	//
	//
	static void testTheInstructionGraph(ControlGraph& controlGraph, VirtualRegisterManager& vrManager);
#endif // DEBUG

#ifdef DEBUG_LOG
	//
	//
	static void printInstructions(RegisterAllocator& registerAllocator);

	//
	//
	static void printSpillCosts(RegisterAllocator& registerAllocator);

	//
	//
	static void printSplitCosts(RegisterAllocator& registerAllocator);
#endif // DEBUG_LOG
};

//
// FIX: this should go in a class (LookupTable ?)
//

inline RegisterName findRoot(RegisterName name, RegisterName* table)
{
	RegisterName* stack = table;
	RegisterName* stackPtr = stack;

	RegisterName newName;
	while((newName = table[name]) != name) {
		*--stackPtr = name;
		name = newName;
	}

	while (stackPtr != stack)
		table[*stackPtr++] = name;

	return name;
}

inline void init(RegisterName* table, Uint32 nameCount)
{
	for (RegisterName r = RegisterName(0); r < nameCount; r = RegisterName(r + 1))
		table[r] = r;
}

inline Uint32 compress(RegisterName* name2range, RegisterName* table, Uint32 nameCount, Uint32 tableSize)
{
	RegisterName* liveRange = new RegisterName[tableSize];
	memset(liveRange, '\0', tableSize * sizeof(RegisterName));

	// Update the lookup table.
	for (RegisterName r = RegisterName(1); r < tableSize; r = RegisterName(r + 1))
		findRoot(r, table);

	// Count the liveranges.
	Uint32 liveRangeCount = 1;
	for (RegisterName s = RegisterName(1); s < tableSize; s = RegisterName(s + 1))
		if (table[s] == s)
			liveRange[s] = RegisterName(liveRangeCount++);

	for (RegisterName t = RegisterName(1); t < nameCount; t = RegisterName(t + 1))
		name2range[t] = liveRange[table[name2range[t]]];

	return liveRangeCount;
}

inline double doLog10(Uint32 power)
{
	double log = 1.0;
	while (power--)
		log *= 10.0;
	return log;
}

#endif // _REGISTER_ALLOCATOR_TOOLS_H_
