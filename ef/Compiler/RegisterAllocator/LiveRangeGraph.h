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

#ifndef _LIVE_RANGE_GRAPH_
#define _LIVE_RANGE_GRAPH_

#include "Fundamentals.h"
#include "Pool.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "RegisterTypes.h"

class RegisterAllocator;

template <class RegisterPressure>
class LiveRangeGraph
{
private:

	RegisterAllocator&		registerAllocator;

	RegisterPressure::Set*	edges;
	Uint32					rangeCount;

public:
	//
	//
	LiveRangeGraph(RegisterAllocator& registerAllocator) : registerAllocator(registerAllocator) {}

	//
	//
	void build();

	//
	//
	void addEdge(RegisterName name1, RegisterName name2);

	//
	//
	bool haveEdge(RegisterName name1, RegisterName name2);

#ifdef DEBUG_LOG
	//
	//
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

template <class RegisterPressure>
void LiveRangeGraph<RegisterPressure>::build()
{
	Pool& pool = registerAllocator.pool;
	Uint32 rangeCount = registerAllocator.rangeCount;
	this->rangeCount = rangeCount;

	edges = new(pool) RegisterPressure::Set(pool, rangeCount * rangeCount);

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	RegisterName* name2range = registerAllocator.name2range;
	LivenessInfo<RegisterPressure>& liveness = registerAllocator.liveness;
	SparseSet currentLive(pool, rangeCount);

	for (Uint32 n = 0; n < nNodes; n++) {
		ControlNode& node = *nodes[n];
		currentLive = liveness.liveOut[n];

		InstructionList& instructions = node.getInstructions();
		for (InstructionList::iterator i = instructions.end(); !instructions.done(i); i = instructions.retreat(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useBegin = instruction.getInstructionUseBegin();
			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			InstructionUse* usePtr;
			InstructionDefine* defineBegin = instruction.getInstructionDefineBegin();
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			InstructionDefine* definePtr;

			if ((instruction.getFlags() & ifCopy) != 0) {
				assert(useBegin != useEnd && useBegin[0].isRegister());
				currentLive.clear(name2range[useBegin[0].getRegisterName()]);
			}

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister()) {
					RegisterName define = name2range[definePtr->getRegisterName()];

					for (SparseSet::iterator l = currentLive.begin(); !currentLive.done(l); l = currentLive.advance(l)) {
						RegisterName live = RegisterName(currentLive.get(l));
						if (define != live && registerAllocator.canInterfere(define, live))
							addEdge(define, live);
					}
				}

			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					currentLive.clear(name2range[definePtr->getRegisterName()]);

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					currentLive.set(name2range[usePtr->getRegisterName()]);

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					RegisterName use = name2range[usePtr->getRegisterName()];

					for (SparseSet::iterator l = currentLive.begin(); !currentLive.done(l); l = currentLive.advance(l)) {
						RegisterName live = RegisterName(currentLive.get(l));
						if (use != live && registerAllocator.canInterfere(use, live))
							addEdge(use, live);
					}
				}
		}
	}
}

template <class RegisterPressure>
void LiveRangeGraph<RegisterPressure>::addEdge(RegisterName name1, RegisterName name2)
{
	assert(name1 != name2);
	edges->set(name1 * rangeCount + name2);
}

template <class RegisterPressure>
bool LiveRangeGraph<RegisterPressure>::haveEdge(RegisterName name1, RegisterName name2)
{
	assert(name1 != name2);
	return edges->test(name1 * rangeCount + name2);
}

#ifdef DEBUG_LOG
template <class RegisterPressure>
void LiveRangeGraph<RegisterPressure>::printPretty(LogModuleObject log)
{
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Live ranges graph:\n"));
	for (RegisterName name1 = RegisterName(1); name1 < rangeCount; name1 = RegisterName(name1 + 1)) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\t%d:\t", name1));
		for (RegisterName name2 = RegisterName(1); name2 < rangeCount; name2 = RegisterName(name2 + 1))
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("%c", ((name1 != name2) && haveEdge(name1, name2)) ? '1' : '0'));
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
	}
}
#endif // DEBUG_LOG

#endif // _LIVE_RANGE_GRAPH_
