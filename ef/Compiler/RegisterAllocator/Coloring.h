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
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "RegisterAllocator.h"
#include "VirtualRegister.h"
#include "InterferenceGraph.h"
#include "SparseSet.h"
#include "Spilling.h"
#include "Splits.h"

UT_EXTERN_LOG_MODULE(RegAlloc);

template <class RegisterPressure>
class Coloring
{
private:
	static RegisterName* simplify(RegisterAllocator& registerAllocator, RegisterName* coloringStack);
	static bool select(RegisterAllocator& registerAllocator, RegisterName* coloringStack, RegisterName* coloringStackPtr);

public:
	static bool color(RegisterAllocator& registerAllocator);
	static void finalColoring(RegisterAllocator& registerAllocator);
};


template <class RegisterPressure>
void Coloring<RegisterPressure>::finalColoring(RegisterAllocator& registerAllocator)
{
	RegisterName* color = registerAllocator.color;
	RegisterName* name2range = registerAllocator.name2range;

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {
		InstructionList& instructions = nodes[n]->getInstructions();

		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					usePtr->setRegisterName(color[name2range[usePtr->getRegisterName()]]);
#ifdef DEBUG
					RegisterID rid = usePtr->getRegisterID();
					setColoredRegister(rid);
					usePtr->setRegisterID(rid);
#endif // DEBUG
				}

			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister()) {
					definePtr->setRegisterName(color[name2range[definePtr->getRegisterName()]]);
#ifdef DEBUG
					RegisterID rid = definePtr->getRegisterID();
					setColoredRegister(rid);
					definePtr->setRegisterID(rid);
#endif // DEBUG
				}
		}
	}
}

template <class RegisterPressure>
bool Coloring<RegisterPressure>::select(RegisterAllocator& registerAllocator, RegisterName* coloringStack, RegisterName* coloringStackPtr)
{
	Uint32 rangeCount = registerAllocator.rangeCount;
	RegisterName* color = new RegisterName[rangeCount];
	registerAllocator.color = color;

	for (Uint32 r = 1; r < rangeCount; r++)
		color[r] = RegisterName(6); // FIX;

	// Color the preColored registers.
	//
	VirtualRegisterManager& vrManager = registerAllocator.vrManager;
	RegisterName* name2range = registerAllocator.name2range;
	PreColoredRegister* machineEnd = vrManager.getMachineRegistersEnd();
	for (PreColoredRegister* machinePtr = vrManager.getMachineRegistersBegin(); machinePtr < machineEnd; machinePtr++)
		if (machinePtr->id != invalidID) {
			color[name2range[getName(machinePtr->id)]] = machinePtr->color;
			UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\twill preColor range %d as %d\n", name2range[getName(machinePtr->id)], machinePtr->color));
		}

	SpillCost* cost = registerAllocator.spillCost;
	Pool& pool = registerAllocator.pool;
	SparseSet& spill = *new(pool) SparseSet(pool, rangeCount);
	registerAllocator.willSpill = &spill;
	SparseSet neighborColors(pool, 6); // FIX
	InterferenceGraph<RegisterPressure>& iGraph = registerAllocator.iGraph;

	bool coloringFailed = false;
	while (coloringStackPtr > coloringStack) {
		RegisterName range = *--coloringStackPtr;

		if (!cost[range].infinite && cost[range].cost < 0) {
			coloringFailed = true;
			spill.set(range);
			UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\tfailed to color %d, will spill.\n", range));
		} else {
			neighborColors.clear();

			for (InterferenceVector* vector = iGraph.getInterferenceVector(range); vector != NULL; vector = vector->next)
				for (Int32 i = vector->count - 1; i >= 0; --i) {
					RegisterName neighborColor = color[vector->neighbors[i]];
					if (neighborColor < 6) // FIX
						neighborColors.set(neighborColor);
				}

			if (neighborColors.getSize() == 6) { // FIX
				coloringFailed = true;
				UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\tfailed to color %d, ", range));

				if (!Splits<RegisterPressure>::findSplit(registerAllocator, color, range)) {
					UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("will spill.\n"));
					spill.set(range);
				} else
					UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("will split.\n"));
			} else {
				for (Uint32 i = 0; i < 6; i++) // FIX
					if (!neighborColors.test(i)) {
						fprintf(stdout, "\twill color %d as %d\n", range, i);
						color[range] = RegisterName(i);
						break;
					}
			}
		}
	}

#ifdef DEBUG_LOG
	if (coloringFailed) {
		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("Coloring failed:\n"));
		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\twill spill: "));
		spill.printPretty(UT_LOG_MODULE(RegAlloc));
	} else {
		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("Coloring succeeded:\n"));
		for (Uint32 i = 1; i < rangeCount; i++)
			UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\trange %d colored as %d\n", i, color[i]));
	}
#endif

	return !coloringFailed;
}

template <class RegisterPressure>
RegisterName* Coloring<RegisterPressure>::simplify(RegisterAllocator& registerAllocator, RegisterName* coloringStack)
{
	InterferenceGraph<RegisterPressure>& iGraph = registerAllocator.iGraph;
	SpillCost* spillCost = registerAllocator.spillCost;
	Uint32 rangeCount = registerAllocator.rangeCount;

	Uint32* degree = new Uint32[rangeCount];
	for (RegisterName i = RegisterName(1); i < rangeCount; i = RegisterName(i + 1)) {
		InterferenceVector* vector = iGraph.getInterferenceVector(i);
		degree[i] = (vector != NULL) ? vector->count : 0;
	}

	Pool& pool = registerAllocator.pool;
	SparseSet low(pool, rangeCount);
	SparseSet high(pool, rangeCount);
	SparseSet highInfinite(pool, rangeCount);
	SparseSet preColored(pool, rangeCount);

	// Get the precolored registers.
	//
	VirtualRegisterManager& vrManager = registerAllocator.vrManager;
	RegisterName* name2range = registerAllocator.name2range;
	PreColoredRegister* machineEnd = vrManager.getMachineRegistersEnd();
	for (PreColoredRegister* machinePtr = vrManager.getMachineRegistersBegin(); machinePtr < machineEnd; machinePtr++)
		if (machinePtr->id != invalidID)
			preColored.set(name2range[getName(machinePtr->id)]);

	// Insert the live ranges in the sets.
	//
	for (Uint32 range = 1; range < rangeCount; range++)
		if (!preColored.test(range))
			if (degree[range] < 6) // FIX
				low.set(range);
			else if (!spillCost[range].infinite)
				high.set(range);
			else
				highInfinite.set(range);

#ifdef DEBUG_LOG
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("Coloring sets:\n\tlow = "));
	low.printPretty(UT_LOG_MODULE(RegAlloc));
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\thigh = "));
	high.printPretty(UT_LOG_MODULE(RegAlloc));
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\thighInfinite = "));
	highInfinite.printPretty(UT_LOG_MODULE(RegAlloc));
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\tpreColored = "));
	preColored.printPretty(UT_LOG_MODULE(RegAlloc));
#endif // DEBUG_LOG

	RegisterName* coloringStackPtr = coloringStack;

	while (low.getSize() != 0 || high.getSize() != 0) {
		while (low.getSize() != 0) {
			RegisterName range = RegisterName(low.getOne());
			low.clear(range);
			*coloringStackPtr++ = range;

			for (InterferenceVector* vector = iGraph.getInterferenceVector(range); vector != NULL; vector = vector->next)
				for (Int32 i = (vector->count - 1); i >= 0; --i) {
					RegisterName neighbor = vector->neighbors[i];
					degree[neighbor]--;

					if (degree[neighbor] < 6) // FIX
						if (high.test(neighbor)) {
							high.clear(neighbor);
							low.set(neighbor);
						} else if (highInfinite.test(neighbor)) {
							highInfinite.clear(neighbor);
							low.set(neighbor);
						}
				}
		}

		if (high.getSize() != 0) {
			RegisterName best = RegisterName(high.getOne());
			double bestCost = spillCost[best].cost;
			double bestDegree = degree[best];

			// Choose the next best candidate.
			//
			for (SparseSet::iterator i = high.begin(); !high.done(i); i = high.advance(i)) {
				RegisterName range = RegisterName(high.get(i));
				double thisCost = spillCost[range].cost;
				double thisDegree = degree[range];

				if (thisCost * bestDegree < bestCost * thisDegree) {
					best = range;
					bestCost = thisCost;
					bestDegree = thisDegree;
				}
			}
			
			high.clear(best);
			low.set(best);
		}
 	}
	assert(highInfinite.getSize() == 0);

	delete degree;

#ifdef DEBUG_LOG
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("Coloring stack:\n\t"));
	for (RegisterName* sp = coloringStack; sp < coloringStackPtr; ++sp)
		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("%d ", *sp));
	UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\n"));
#endif // DEBUG_LOG

	return coloringStackPtr;
}


template <class RegisterPressure>
bool Coloring<RegisterPressure>::color(RegisterAllocator& registerAllocator)
{
	RegisterName* coloringStack = new RegisterName[registerAllocator.rangeCount];
	return select(registerAllocator, coloringStack, simplify(registerAllocator, coloringStack));
}
