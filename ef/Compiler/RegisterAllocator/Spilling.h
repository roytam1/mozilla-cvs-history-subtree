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

#ifndef _SPILLING_H_
#define _SPILLING_H_

#include "Fundamentals.h"
#include <string.h>
#include "RegisterAllocator.h"
#include "RegisterAllocatorTools.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "SparseSet.h"

template <class RegisterPressure>
class Spilling
{
private:
	static void insertStoreAfter(Instruction& instruction, RegisterName name);
	static void insertLoadBefore(Instruction& instruction, RegisterName name);

public:
	static void calculateSpillCosts(RegisterAllocator& registerAllocator);
	static void insertSpillCode(RegisterAllocator& registerAllocator);
};

struct SpillCost
{
	double loads;
	double stores;
	double copies;
	double cost;
	bool infinite;
};

template <class RegisterPressure>
void Spilling<RegisterPressure>::insertSpillCode(RegisterAllocator& registerAllocator)
{
	Uint32 rangeCount = registerAllocator.rangeCount;
	RegisterName* name2range = registerAllocator.name2range;

	Pool& pool = registerAllocator.pool;
	SparseSet currentLive(pool, rangeCount);
	SparseSet needLoad(pool, rangeCount);
	SparseSet mustSpill(pool, rangeCount);
	SparseSet& willSpill = *registerAllocator.willSpill;
	
	ControlGraph& controlGraph = registerAllocator.controlGraph;
	RegisterPressure::Set* liveOut = registerAllocator.liveness.liveOut;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {

		needLoad.clear();
		currentLive = liveOut[n];
		mustSpill = currentLive;

		InstructionList& instructions = nodes[n]->getInstructions();
		for (InstructionList::iterator i = instructions.end(); !instructions.done(i);) {
			Instruction& instruction = instructions.get(i);
			i = instructions.retreat(i);

			InstructionUse* useBegin = instruction.getInstructionUseBegin();
			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			InstructionUse* usePtr;
			InstructionDefine* defineBegin = instruction.getInstructionDefineBegin();
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			InstructionDefine* definePtr;

			bool foundLiveDefine = false;
			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister()) {
					if (currentLive.test(name2range[definePtr->getRegisterName()])) {
						foundLiveDefine = true;
						break;
					}
				} else {
					foundLiveDefine = true;
					break;
				}
			if (defineBegin != defineEnd && !foundLiveDefine) {
				fprintf(stderr, "!!! Removed instruction because it was only defining unused registers !!!\n");
				instruction.remove();
			}

 			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister()) {
					RegisterName range = name2range[definePtr->getRegisterName()];
#ifdef DEBUG
					if (needLoad.test(range))
						if (!mustSpill.test(range) && registerAllocator.spillCost[range].infinite && willSpill.test(range)) {
							fprintf(stderr, "Tried to spill a register with infinite spill cost\n");
							abort();
						}
#endif // DEBUG
					if (willSpill.test(range))
						insertStoreAfter(instruction, range);

					needLoad.clear(range);
				}

			if (instruction.getFlags() & ifCopy)
				for (usePtr = useBegin; usePtr < useEnd; usePtr++)
					if (usePtr->isRegister()) {
						RegisterName range = name2range[usePtr->getRegisterName()];
						if (!currentLive.test(range))
							for (SparseSet::iterator r = needLoad.begin(); !needLoad.done(r); r = needLoad.advance(r)) {
								RegisterName load = RegisterName(needLoad.get(r));
								if (willSpill.test(load))
									insertLoadBefore(instruction, load);
								mustSpill.set(load);
							}
						needLoad.clear();
					}

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister())
					currentLive.clear(name2range[definePtr->getRegisterName()]);


			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					RegisterName range = name2range[usePtr->getRegisterName()];
					currentLive.set(range);
					needLoad.set(range);
				}
		}

		for (SparseSet::iterator l = needLoad.begin(); !needLoad.done(l); l = needLoad.advance(l)) {
			RegisterName load = RegisterName(needLoad.get(l));
			if (willSpill.test(load))
				insertLoadBefore(instructions.first(), load);
		}		
	}
}

template <class RegisterPressure>
void Spilling<RegisterPressure>::insertLoadBefore(Instruction& /*instruction*/, RegisterName name)
{
	fprintf(stdout, "will insert load for range %d\n", name);
}

template <class RegisterPressure>
void Spilling<RegisterPressure>::insertStoreAfter(Instruction& /*instruction*/, RegisterName name)
{
	fprintf(stdout, "will insert store for range %d\n", name);
}

template <class RegisterPressure>
void Spilling<RegisterPressure>::calculateSpillCosts(RegisterAllocator& registerAllocator)
{
	Uint32 rangeCount = registerAllocator.rangeCount;
	RegisterName* name2range = registerAllocator.name2range;

	Pool& pool = registerAllocator.pool;
	SparseSet live(pool, rangeCount);
	SparseSet needLoad(pool, rangeCount);
	SparseSet mustSpill(pool, rangeCount);

	SparseSet alreadyStored(pool, rangeCount); // FIX: should get this from previous spilling.

	SpillCost* cost = new SpillCost[rangeCount];
	memset(cost, '\0', rangeCount * sizeof(SpillCost));

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	RegisterPressure::Set* liveOut = registerAllocator.liveness.liveOut;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {
		ControlNode& node = *nodes[n];

		double weight = doLog10(node.loopDepth);

		needLoad.clear();
		live = liveOut[n];
		mustSpill = live;

		InstructionList& instructions = nodes[n]->getInstructions();
		for (InstructionList::iterator i = instructions.end(); !instructions.done(i); i = instructions.retreat(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useBegin = instruction.getInstructionUseBegin();
			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			InstructionUse* usePtr;
			InstructionDefine* defineBegin = instruction.getInstructionDefineBegin();
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			InstructionDefine* definePtr;

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister()) {
					RegisterName range = name2range[definePtr->getRegisterName()];

					if (needLoad.test(range))
						if (!mustSpill.test(range))
							cost[range].infinite = true;

					if ((false /* !rematerializable(range) */ || !needLoad.test(range)) && !alreadyStored.test(range))
						cost[range].stores += weight;

					needLoad.clear(range);
				}

			if (instruction.getFlags() & ifCopy)
				for (usePtr = useBegin; usePtr < useEnd; usePtr++)
					if (usePtr->isRegister())
						if (!live.test(name2range[usePtr->getRegisterName()])) {
							for (SparseSet::iterator l = needLoad.begin(); !needLoad.done(l); l = needLoad.advance(l)) {
								Uint32 range = needLoad.get(l);
								cost[range].loads += weight;
								mustSpill.set(range);
							}
							needLoad.clear();
						}

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister())
					live.clear(name2range[definePtr->getRegisterName()]);

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					RegisterName range = name2range[usePtr->getRegisterName()];

					live.set(range);
					needLoad.set(range);
				}

			if (instruction.getFlags() & ifCopy) {
				assert(useBegin != useEnd && useBegin[0].isRegister());
				assert(defineBegin != defineEnd && defineBegin[0].isRegister());

				RegisterName source = name2range[useBegin[0].getRegisterName()];
				RegisterName destination = name2range[defineBegin[0].getRegisterName()];

				cost[source].copies += weight;
				cost[destination].copies += weight;
			}			
		}

		for (SparseSet::iterator s = needLoad.begin(); !needLoad.done(s); s = needLoad.advance(s))
			cost[needLoad.get(s)].loads += weight;
	}
	
	for (Uint32 r = 0; r < rangeCount; r++) {
		SpillCost& c = cost[r];
		c.cost = 2 * (c.loads + c.stores) - c.copies;
	}

	registerAllocator.spillCost = cost;
}

#endif // _SPILLING_H_
