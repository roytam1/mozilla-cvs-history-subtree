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

#ifndef _SPLITS_H_
#define _SPLITS_H_

#include "Fundamentals.h"
#include <string.h>
#include "Pool.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "RegisterAllocator.h"
#include "RegisterAllocatorTools.h"

UT_EXTERN_LOG_MODULE(RegAlloc);

template <class RegisterPressure>
struct Splits
{
	static void calculateSplitCosts(RegisterAllocator& registerAllocator);
	static bool findSplit(RegisterAllocator& registerAllocator, RegisterName* color, RegisterName range);
	static void insertSplitCode(RegisterAllocator& registerAllocator);
};

struct SplitCost
{
	double loads;
	double stores;
};

template <class RegisterPressure>
void Splits<RegisterPressure>::insertSplitCode(RegisterAllocator& /*registerAllocator*/)
{
	// FIX
}

template <class RegisterPressure>
bool Splits<RegisterPressure>::findSplit(RegisterAllocator& registerAllocator, RegisterName* color, RegisterName range)
{
	Pool& pool = registerAllocator.pool;
	NameLinkedList** neighborsWithColor = new(pool) NameLinkedList*[6]; // FIX
	memset(neighborsWithColor, '\0', 6 * sizeof(NameLinkedList*));

	InterferenceGraph<RegisterPressure>& iGraph = registerAllocator.iGraph;

	for (InterferenceVector* vector = iGraph.getInterferenceVector(range); vector != NULL; vector = vector->next)
		for (Int32 i = vector->count - 1; i >=0; --i) {
			RegisterName neighbor = vector->neighbors[i];
			RegisterName c = color[neighbor];

			if (c < 6) { // FIX
				NameLinkedList* node = new(pool) NameLinkedList();
				node->name = neighbor;
				node->next = neighborsWithColor[c];
				neighborsWithColor[c] = node;
			}
		}

	bool splitAroundName = true;

	LiveRangeGraph<RegisterPressure>& lGraph = registerAllocator.lGraph;
	RegisterName bestColor = RegisterName(6); // FIX
	double bestCost = registerAllocator.spillCost[range].cost;
	SplitCost* splitCost = registerAllocator.splitCost;

	for (RegisterName i = RegisterName(0); i < 6; i = RegisterName(i + 1)) { // FIX

		double splitAroundNameCost = 0.0;
		bool canSplitAroundName = true;

		SplitCost& sCost = splitCost[range];
		double addedCost = 2.0 * (sCost.stores + sCost.loads);

		for (NameLinkedList* node = neighborsWithColor[i]; node != NULL; node = node->next) {
			RegisterName neighbor = node->name;
			if (lGraph.haveEdge(neighbor, range)) {
				canSplitAroundName = false;
				break;
			} else
				splitAroundNameCost += addedCost;
		}
		if (canSplitAroundName && splitAroundNameCost < bestCost) {
			bestCost = splitAroundNameCost;
			bestColor = i;
			splitAroundName = true;
		}

		double splitAroundColorCost = 0.0;
		bool canSplitAroundColor = true;

		for (NameLinkedList* node = neighborsWithColor[i]; node != NULL; node = node->next) {
			RegisterName neighbor = node->name;
			if (lGraph.haveEdge(range, neighbor)) {
				canSplitAroundColor = false;
				break;
			} else {
				SplitCost& sCost = splitCost[neighbor];
				double addedCost = 2.0 * (sCost.stores + sCost.loads);
				splitAroundColorCost += addedCost;
			}
		}
		if (canSplitAroundColor && splitAroundColorCost < bestCost) {
			bestCost = splitAroundColorCost;
			bestColor = i;
			splitAroundName = false;
		}
 	}
	if (bestColor < RegisterName(6)) {
		color[range] = bestColor;
		registerAllocator.splitFound = true;

		NameLinkedList** splitAround = registerAllocator.splitAround;

		if (splitAroundName)
			for (NameLinkedList* node = neighborsWithColor[bestColor]; node != NULL; node = node->next) {
				NameLinkedList* newNode = new(pool) NameLinkedList();
				newNode->name = node->name;
				newNode->next = splitAround[range];
				splitAround[range] = newNode;
			}
		else
			for (NameLinkedList* node = neighborsWithColor[bestColor]; node != NULL; node = node->next) {
				NameLinkedList* newNode = new(pool) NameLinkedList();
				RegisterName neighbor = node->name;
				newNode->name = range;
				newNode->next = splitAround[neighbor];
				splitAround[neighbor] = newNode;
			}

		trespass("Found a split");
		return true;
	}

	return false;
}

template <class RegisterPressure>
void Splits<RegisterPressure>::calculateSplitCosts(RegisterAllocator& registerAllocator)
{
	Pool& pool = registerAllocator.pool;
	Uint32 rangeCount = registerAllocator.rangeCount;
	RegisterName* name2range = registerAllocator.name2range;

	SplitCost* splitCost = new(pool) SplitCost[rangeCount];
	memset(splitCost, '\0', rangeCount * sizeof(SplitCost));

	SparseSet live(pool, rangeCount);
	RegisterPressure::Set* liveIn = registerAllocator.liveness.liveIn;
	RegisterPressure::Set* liveOut = registerAllocator.liveness.liveOut;

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {
		ControlNode& node = *nodes[n];
		double weight = doLog10(node.loopDepth);

		live = liveOut[n];

		ControlEdge* successorsEnd = node.getSuccessorsEnd();
		for (ControlEdge* successorsPtr = node.getSuccessorsBegin(); successorsPtr < successorsEnd; successorsPtr++) {
			ControlNode& successor = successorsPtr->getTarget();

			if (successor.getControlKind() != ckEnd) {
				RegisterPressure::Set& successorLiveIn = liveIn[successor.dfsNum];

				for (SparseSet::iterator i = live.begin(); !live.done(i); i = live.advance(i)) {
					RegisterName name = RegisterName(live.get(i));
					if (!successorLiveIn.test(name))
						splitCost[name].loads += doLog10(successor.loopDepth);
				}
			}
		}

		InstructionList& instructions = node.getInstructions();
		for (InstructionList::iterator i = instructions.end(); !instructions.done(i); i = instructions.retreat(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useBegin = instruction.getInstructionUseBegin();
			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			InstructionUse* usePtr;
			InstructionDefine* defineBegin = instruction.getInstructionDefineBegin();
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			InstructionDefine* definePtr;

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister())
					splitCost[name2range[definePtr->getRegisterName()]].stores += weight;

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					RegisterName range = name2range[usePtr->getRegisterName()];
					if (!live.test(range)) {
						if (&instruction != &instructions.last())
							splitCost[range].loads += weight;
						else {
							ControlEdge* successorsEnd = node.getSuccessorsEnd();
							for (ControlEdge* successorsPtr = node.getSuccessorsBegin(); successorsPtr < successorsEnd; successorsPtr++)
								splitCost[range].loads += doLog10(successorsPtr->getTarget().loopDepth);
						}
					}
				}

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++) 
				if (definePtr->isRegister())
					live.clear(name2range[definePtr->getRegisterName()]);

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					live.set(name2range[usePtr->getRegisterName()]);
		}
	}

	NameLinkedList** splitAround = new(pool) NameLinkedList*[rangeCount];
	memset(splitAround, '\0', rangeCount * sizeof(NameLinkedList*));
	registerAllocator.splitAround = splitAround;

	registerAllocator.splitCost = splitCost;
	registerAllocator.splitFound = false;
}

#endif // _SPLITS_H_
