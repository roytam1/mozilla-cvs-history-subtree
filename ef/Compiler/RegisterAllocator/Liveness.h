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

#ifndef _LIVENESS_H_
#define _LIVENESS_H_

#include "Fundamentals.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "RegisterTypes.h"

// ----------------------------------------------------------------------------
// LivenessInfo -

template <class RegisterPressure>
struct LivenessInfo
{
	RegisterPressure::Set*	liveIn;
	RegisterPressure::Set*	liveOut;
	DEBUG_LOG_ONLY(Uint32	size);

#ifdef DEBUG_LOG
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

// ----------------------------------------------------------------------------
// Liveness
//
// The liveness is defined by the following data-flow equations:
//
//		LiveIn(n) = LocalLive(n) U (LiveOut(n) - Killed(n)).
//		LiveOut(n) = U LiveIn(s) (s a successor of n).
//
//	where LocalLive(n) is the set of used registers in the block n, Killed(n)
//  is the set of defined registers in the block n, LiveIn(n) is the set of
//  live registers at the begining of the block n and LiveOut(n) is the set
//  of live registers at the end of the block n.
//
//
// We will compute the liveness analysis in two stages:
//
//	1- Build LocalLive(n) (wich is an approximation of LiveIn(n)) and Killed(n) 
//     for each block n.
//  2- Perform a backward data-flow analysis to propagate the liveness information
//     through the entire control-flow graph.
//

template <class RegisterPressure>
struct Liveness
{
	static LivenessInfo<RegisterPressure> analysis(ControlGraph& controlGraph, Uint32 rangeCount, const RegisterName* name2range);
	static LivenessInfo<RegisterPressure> analysis(ControlGraph& controlGraph, Uint32 nameCount);
};

template <class RegisterPressure>
LivenessInfo<RegisterPressure> Liveness<RegisterPressure>::analysis(ControlGraph& controlGraph, Uint32 rangeCount, const RegisterName* name2range)
{
	Pool& pool = controlGraph.pool;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	// Allocate the temporary sets.
	RegisterPressure::Set* killed = new(pool) RegisterPressure::Set[nNodes](pool, rangeCount);
	
	// Allocate the globals sets.
	RegisterPressure::Set* liveIn = new(pool) RegisterPressure::Set[nNodes](pool, rangeCount);
	RegisterPressure::Set* liveOut = new(pool) RegisterPressure::Set[nNodes](pool, rangeCount);

	// First stage of the liveness analysis: Compute the sets LocalLive(stored in LiveIn) and Killed.
	//
	for (Uint32 n = 0; n < (nNodes - 1); n++) {
		ControlNode& node = *nodes[n];

		RegisterPressure::Set& currentLocalLive = liveIn[n];
		RegisterPressure::Set& currentKilled = killed[n];
		
		// Find the instructions contributions to the sets LocalLive and Killed.
		//
		InstructionList& instructions = node.getInstructions();
		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			Instruction& instruction = instructions.get(i);

			// If a VirtualRegister is 'used' before being 'defined' then we add it to set LocalLive.

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					Uint32 index = name2range[usePtr->getRegisterName()];

					if (!currentKilled.test(index))
						currentLocalLive.set(index);
				}

			// If a Virtualregister is 'defined' then we add it to the set Killed.
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					currentKilled.set(name2range[definePtr->getRegisterName()]);
		}
	}

	// Second stage of the liveness analysis: We propagate the LiveIn & LiveOut through the entire 
	// control-flow graph.
	//
	RegisterPressure::Set temp(pool, rangeCount);

	bool changed;
	do {
		changed = false;

		// For all nodes is this graph except the endNode.
		for (Int32 n = (nNodes - 2); n >= 0; n--) {
			ControlNode& node = *nodes[n];

			RegisterPressure::Set& currentLiveIn = liveIn[n];
			RegisterPressure::Set& currentLiveOut = liveOut[n];

			// Compute temp = Union of LiveIn(s) (s a successor of this node) | usedByPhiNodes(n).
			// temp will be the new LiveOut(n).
			Uint32 nSuccessors = node.nSuccessors();
			if (nSuccessors != 0) {
				temp = liveIn[node.nthSuccessor(0).getTarget().dfsNum];
				for (Uint32 s = 1; s < nSuccessors; s++)
					temp |= liveIn[node.nthSuccessor(s).getTarget().dfsNum];
			} else 
				temp.clear();

			// If temp and LiveOut(n) differ then set LiveOut(n) = temp and recalculate the
			// new LiveIn(n).
			if (currentLiveOut != temp) {
				currentLiveOut = temp;
				temp -= killed[n]; // FIX: could be optimized with one call to unionDiff !
				temp |= currentLiveIn;
			
				if (currentLiveIn != temp) {
					currentLiveIn = temp;
					changed = true;
				}
			}
		}
	} while(changed);

	LivenessInfo<RegisterPressure> liveness;
	liveness.liveIn = liveIn;
	liveness.liveOut = liveOut;
	DEBUG_LOG_ONLY(liveness.size = nNodes);
	return liveness;
}

template <class RegisterPressure>
LivenessInfo<RegisterPressure> Liveness<RegisterPressure>::analysis(ControlGraph& controlGraph, Uint32 nameCount)
{
	Pool& pool = controlGraph.pool;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	// Allocate the temporary sets.
	RegisterPressure::Set* killed = new(pool) RegisterPressure::Set[nNodes](pool, nameCount);
	RegisterPressure::Set* usedByPhiNodes = NULL;
	
	// Allocate the globals sets.
	RegisterPressure::Set* liveIn = new(pool) RegisterPressure::Set[nNodes](pool, nameCount);
	RegisterPressure::Set* liveOut = new(pool) RegisterPressure::Set[nNodes](pool, nameCount);

	// First stage of the liveness analysis: Compute the sets LocalLive(stored in LiveIn) and Killed.
	//
	for (Uint32 n = 0; n < (nNodes - 1); n++) {
		ControlNode& node = *nodes[n];

		RegisterPressure::Set& currentLocalLive = liveIn[n];
		RegisterPressure::Set& currentKilled = killed[n];
		
		InstructionList& phiNodes = node.getPhiNodeInstructions();

		if ((usedByPhiNodes == NULL) && !phiNodes.empty())
			usedByPhiNodes = new(pool) RegisterPressure::Set[nNodes](pool, nameCount);

		for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
			Instruction& phiNode = phiNodes.get(p);

			InstructionDefine& define = phiNode.getInstructionDefineBegin()[0];
			currentKilled.set(define.getRegisterName());

			typedef DoublyLinkedList<ControlEdge> ControlEdgeList;
			const ControlEdgeList& predecessors = node.getPredecessors();
			ControlEdgeList::iterator p = predecessors.begin();
			InstructionUse* useEnd = phiNode.getInstructionUseEnd();
			for (InstructionUse* usePtr = phiNode.getInstructionUseBegin(); usePtr < useEnd; usePtr++, p = predecessors.advance(p))
				if (usePtr->isRegister())
					usedByPhiNodes[predecessors.get(p).getSource().dfsNum].set(usePtr->getRegisterName());
		}

		// Find the instructions contributions to the sets LocalLive and Killed.
		//
		InstructionList& instructions = node.getInstructions();
		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			Instruction& instruction = instructions.get(i);

			// If a VirtualRegister is 'used' before being 'defined' then we add it to set LocalLive.

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister()) {
					Uint32 index = usePtr->getRegisterName();

					if (!currentKilled.test(index))
						currentLocalLive.set(index);
				}

			// If a Virtualregister is 'defined' then we add it to the set Killed.
			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					currentKilled.set(definePtr->getRegisterName());
		}
	}

	// Second stage of the liveness analysis: We propagate the LiveIn & LiveOut through the entire 
	// control-flow graph.
	//
	RegisterPressure::Set temp(pool, nameCount);

	bool changed;
	do {
		changed = false;

		// For all nodes is this graph except the endNode.
		for (Int32 n = (nNodes - 2); n >= 0; n--) {
			ControlNode& node = *nodes[n];

			RegisterPressure::Set& currentLiveIn = liveIn[n];
			RegisterPressure::Set& currentLiveOut = liveOut[n];

			// Compute temp = Union of LiveIn(s) (s a successor of this node) | usedByPhiNodes(n).
			// temp will be the new LiveOut(n).
			Uint32 nSuccessors = node.nSuccessors();
			if (nSuccessors != 0) {
				temp = liveIn[node.nthSuccessor(0).getTarget().dfsNum];
				for (Uint32 s = 1; s < nSuccessors; s++)
					temp |= liveIn[node.nthSuccessor(s).getTarget().dfsNum];
			} else 
				temp.clear();

			// Insert the phiNodes contribution.
			if (usedByPhiNodes != NULL)
				temp |= usedByPhiNodes[n];

			// If temp and LiveOut(n) differ then set LiveOut(n) = temp and recalculate the
			// new LiveIn(n).
			if (currentLiveOut != temp) {
				currentLiveOut = temp;
				temp -= killed[n]; // FIX: could be optimized with one call to unionDiff !
				temp |= currentLiveIn;
			
				if (currentLiveIn != temp) {
					currentLiveIn = temp;
					changed = true;
				}
			}
		}
	} while(changed);

	LivenessInfo<RegisterPressure> liveness;
	liveness.liveIn = liveIn;
	liveness.liveOut = liveOut;
	DEBUG_LOG_ONLY(liveness.size = nNodes);
	return liveness;
}

#ifdef DEBUG_LOG
template <class RegisterPressure>
void LivenessInfo<RegisterPressure>::printPretty(LogModuleObject log)
{
	for (Uint32 n = 0; n < size; n++) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Node N%d:\n\tliveIn = ", n));
		liveIn[n].printPretty(log);
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\tliveOut = "));
		liveOut[n].printPretty(log);
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
	}
}
#endif // DEBUG_LOG

#endif // _LIVENESS_H_
