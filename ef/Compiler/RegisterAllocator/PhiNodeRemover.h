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

#ifndef _PHI_NODE_REMOVER_H_
#define _PHI_NODE_REMOVER_H_

#include "Fundamentals.h"
#include "Pool.h"
#include "ControlGraph.h"
#include "DominatorGraph.h"
#include "VirtualRegister.h"
#include "RegisterPressure.h"
#include "Liveness.h"
#include "Instruction.h"
#include "InstructionEmitter.h"
#include "SparseSet.h"
#include <string.h>

//------------------------------------------------------------------------------
// RegisterNameNode -

struct RegisterNameNode 
{
	RegisterNameNode*	next;
	RegisterName		newName;
	Uint32				nextPushed;
};

//------------------------------------------------------------------------------
// CopyData -

struct CopyData
{
	RegisterName		source;
	RegisterClassKind	classKind;
	Uint32				useCount;
	bool				isLiveOut;
	RegisterName		sourceNameToUse;
	RegisterName		temporaryName;
	RegisterNameNode*	newName;
};

//------------------------------------------------------------------------------
// PhiNodeRemover<RegisterPressure> -

template <class RegisterPressure>
struct PhiNodeRemover
{
	// Replace the phi nodes by copy instructions.
	static void replacePhiNodes(ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter);
};

// Split some of the critical edges and return true if there are still some
// in the graph after that.
//
static bool splitCriticalEdges(ControlGraph& /*cg*/)
{
	// FIX: not implemented.
	return true;
}

inline void pushName(Pool& pool, RegisterNameNode** stack, SparseSet& pushed, Uint32* nodeListPointer, RegisterName oldName, RegisterName newName)
{
	RegisterNameNode& newNode = *new(pool) RegisterNameNode();

	if (pushed.test(oldName))
		(*stack)->newName = newName;
	else {
		newNode.newName = newName;
		newNode.nextPushed = *nodeListPointer;
		*nodeListPointer = oldName;
		newNode.next = *stack;
		*stack = &newNode;
		pushed.set(oldName);
	}
}

template <class RegisterPressure>
void PhiNodeRemover<RegisterPressure>::replacePhiNodes(ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter)
{
	Pool& pool = controlGraph.pool;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	// Initialize the local variables.
	//

	// When we insert the copies we will also need to create new VirtualRegisters for
	// the insertion of temporaries. The maximum number of temporary register will not
	// exceed the number of phiNodes in the primitive graph.
	Uint32 nameCount = vrManager.getSize();
	Uint32 maxNameCount = nameCount;
	for (Uint32 n = 0; n < nNodes; n++)
		maxNameCount += nodes[n]->getPhiNodes().length();

	// If the CFG contains some critical edges (backward edge which source has more than one
	// outgoing edge and destination has more than one incomimg edge) then we need the liveness
	// information to be able to insert temporary copies.
	RegisterPressure::Set* liveOut = NULL;
	if (splitCriticalEdges(controlGraph))
		liveOut = Liveness<LowRegisterPressure>::analysis(controlGraph, nameCount).liveOut;

	DominatorGraph dGraph(controlGraph);

	SparseSet pushed(pool, maxNameCount);
	SparseSet destinationList(pool, maxNameCount);
	SparseSet workList(pool, maxNameCount);

	CopyData* copyStats = new(pool) CopyData[maxNameCount];
	memset(copyStats, '\0', maxNameCount*sizeof(CopyData));

	struct NodeStack {
		Uint32* next;
		Uint32* limit;
		Uint32 pushedList;
	};

	// Allocate the node stack and initialize the node stack pointer.
	NodeStack* nodeStack = new(pool) NodeStack[nNodes + 1];
	NodeStack* nodeStackPtr = nodeStack;

	// We start by the begin node. 
	Uint32 startNode = 0;
	Uint32* next = &startNode;
	Uint32* limit = &startNode + 1;

	while (true) {

		if (next == limit) {
			// If there are no more node in the sibling, we have to pop the current
			// frame from the stack and update the copyStats of the pushed nodes.
			//
			if (nodeStackPtr == nodeStack)
				// We are at the bottom of the stack and there are no more nodes
				// to look at. We are done !
				break;

			--nodeStackPtr;
			// We are done with all the children of this node in the dominator tree.
			// We need to update the copy information of all the new names pushed
			// during the walk over this node.
			Uint32 pushedList = nodeStackPtr->pushedList;
			while (pushedList != 0) {
				Uint32 nextName = copyStats[pushedList].newName->nextPushed;
				copyStats[pushedList].newName = copyStats[pushedList].newName->next;
				pushedList = nextName;
			}

			// restore the previous frame.
			next = nodeStackPtr->next;
			limit = nodeStackPtr->limit;
		} else {
			Uint32 currentNode = *next++;
			Uint32 pushedList = 0;


			// Initialize the sets.
			pushed.clear();
			destinationList.clear();

			// STEP1:
			//  Walk the instruction list and to replace all the instruction uses with their new name. 
			//  If the instruction is a phi node and its defined register is alive at the end of this 
			//  block then we push the defined register into the stack.
			//
			ControlNode& node = *nodes[currentNode];
			RegisterPressure::Set* currentLiveOut = (liveOut != NULL) ? &liveOut[currentNode] : (RegisterPressure::Set*) 0;

			InstructionList& phiNodes = node.getPhiNodeInstructions();
			for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
				Instruction& phiNode = phiNodes.get(p);

				InstructionUse* useEnd = phiNode.getInstructionUseEnd();
				for (InstructionUse* usePtr = phiNode.getInstructionUseBegin(); usePtr < useEnd; usePtr++) {
					assert(usePtr->isRegister());
					RegisterName name = usePtr->getRegisterName();

					if (copyStats[name].newName != NULL && copyStats[name].newName->newName != name)
						usePtr->setRegisterName(copyStats[name].newName->newName);
				}

				if (currentLiveOut != NULL) {
					// This is a phi node and we have to push its defined name if it is live
					// at the end of the node. We only need to do this if the CFG has critical edges.
					assert(phiNode.getInstructionDefineBegin() != phiNode.getInstructionDefineEnd() && phiNode.getInstructionDefineBegin()[0].isRegister());
					RegisterName name = phiNode.getInstructionDefineBegin()[0].getRegisterName();

					if (currentLiveOut->test(name))
						pushName(pool, &(copyStats[name].newName), pushed, &pushedList, name, name);
				}
				
			}

			InstructionList& instructions = node.getInstructions();
			for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
				Instruction& instruction = instructions.get(i);

				InstructionUse* useEnd = instruction.getInstructionUseEnd();
				for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
					if (usePtr->isRegister()) {
						RegisterName name = usePtr->getRegisterName();

						if (copyStats[name].newName != NULL && copyStats[name].newName->newName != name)
							usePtr->setRegisterName(copyStats[name].newName->newName);
					}
			}

			// STEP2:
			//  Look at this node's successors' phiNodes. We keep track of the number of time
			//  a VR will be used by another copy instruction and insert each definition into the 
			//  destinationList. This is the only pass over this node's successors as we will
			//  get all the information we need in the CopyData structures.
			//
			ControlEdge* successorEdgeEnd = node.getSuccessorsEnd();
			for (ControlEdge* successorEdgePtr = node.getSuccessorsBegin(); successorEdgePtr < successorEdgeEnd; successorEdgePtr++) {
				Uint32 useIndex = successorEdgePtr->getIndex();
				ControlNode& successor = successorEdgePtr->getTarget();

				// Look at its phi nodes. The phi nodes are at the top of the instruction list. We exit
				// as soon as we find an instruction which is not a phi node
				InstructionList& phiNodes = successor.getPhiNodeInstructions();
				for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
					Instruction& phiNode = phiNodes.get(p);
					
					assert((phiNode.getInstructionUseBegin() + useIndex) < phiNode.getInstructionUseEnd());
					assert(phiNode.getInstructionDefineBegin() != phiNode.getInstructionDefineEnd());

					InstructionUse& source = phiNode.getInstructionUseBegin()[useIndex];
					InstructionDefine& destination = phiNode.getInstructionDefineBegin()[0];

					assert(source.isRegister() && destination.isRegister());

					RegisterName sourceName = source.getRegisterName();
					RegisterName destinationName = destination.getRegisterName();

					// Get the correct name for the source.
					if (copyStats[sourceName].newName != NULL)
						sourceName = copyStats[sourceName].newName->newName;

					// Update the CopyData structures.
					if ((sourceName != rnInvalid) && (sourceName != destinationName)) {
						copyStats[destinationName].source = sourceName;
						copyStats[destinationName].classKind = destination.getRegisterClass();
						copyStats[destinationName].isLiveOut = (currentLiveOut != NULL) ? currentLiveOut->test(destinationName) : false;
						copyStats[destinationName].sourceNameToUse = destinationName;
						copyStats[sourceName].sourceNameToUse = sourceName;
						copyStats[sourceName].useCount++;
						destinationList.set(destinationName);
					}
				}
			}

			// STEP3:
			//  Insert into the worklist only the destination registers that will be not used in 
			//  another copy instruction in this block.
			//
			assert(workList.getSize() == 0);
			for (SparseSet::iterator d = destinationList.begin(); !destinationList.done(d); d = destinationList.advance(d)) {
				Uint32 dest = destinationList.get(d);
				if (copyStats[dest].useCount == 0)
					workList.set(dest);
			}

			// STEP4:
			//  Insert the copy instructions.
			//
			Uint32 destinationListSize = destinationList.getSize();
			InstructionList::iterator endOfTheNode = instructions.end();

			// Find the right place to insert the copy instructions.
			if (destinationListSize != 0)
				while (instructions.get(endOfTheNode).getFlags() & ifControl)
					endOfTheNode = instructions.retreat(endOfTheNode);

			while (destinationListSize != 0) {
				while(workList.getSize()) {
					RegisterName destinationName = RegisterName(workList.getOne());
					RegisterName sourceName = copyStats[destinationName].source;

					workList.clear(destinationName);
					if (copyStats[destinationName].isLiveOut && !copyStats[destinationName].temporaryName) {
						// Lost copy problem.
						copyStats[destinationName].isLiveOut = false;

						RegisterName sourceName = destinationName;
						RegisterClassKind classKind = copyStats[sourceName].classKind;
						RegisterName destinationName = getName(vrManager.newVirtualRegister(classKind));
						assert(destinationName < maxNameCount);

						copyStats[destinationName].classKind = classKind;
						copyStats[sourceName].useCount = 0;
				
						// We need to insert a copy to a temporary register to keep the
						// source register valid at the end of the node defining it.
						// This copy will be inserted right after the phi node defining it.
						RegisterName from = copyStats[sourceName].sourceNameToUse;
						Instruction* definingPhiNode = vrManager.getVirtualRegister(from).getDefiningInstruction();
						assert(definingPhiNode && (definingPhiNode->getFlags() & ifPhiNode) != 0);

						RegisterID fromID = buildRegisterID(from, classKind);
						RegisterID toID = buildRegisterID(destinationName, classKind);
						Instruction& copy = emitter.newCopy(*definingPhiNode->getPrimitive(), fromID, toID);
						vrManager.getVirtualRegister(destinationName).setDefiningInstruction(copy);
						definingPhiNode->getPrimitive()->getContainer()->getInstructions().addFirst(copy);

						copyStats[sourceName].temporaryName = destinationName;
						copyStats[sourceName].sourceNameToUse = destinationName;
						pushName(pool, &(copyStats[sourceName].newName), pushed, &pushedList, sourceName, destinationName);
					}

					// Insert the copy instruction at the end of the current node.
					RegisterName from = copyStats[sourceName].sourceNameToUse;

					RegisterClassKind classKind = copyStats[destinationName].classKind;
					RegisterID fromID = buildRegisterID(from, classKind);
					RegisterID toID = buildRegisterID(destinationName, classKind);
					Instruction& copy = emitter.newCopy(*vrManager.getVirtualRegister(from).getDefiningInstruction()->getPrimitive(), fromID, toID);
					instructions.insertAfter(copy, endOfTheNode);
					endOfTheNode = instructions.advance(endOfTheNode);

					copyStats[sourceName].useCount = 0;
					if (destinationList.test(sourceName) && copyStats[sourceName].isLiveOut)
						pushName(pool, &(copyStats[sourceName].newName), pushed, &pushedList, sourceName, destinationName);
					copyStats[sourceName].isLiveOut = false;
					copyStats[sourceName].sourceNameToUse = destinationName;

					if (destinationList.test(sourceName))
						workList.set(sourceName);
					destinationList.clear(destinationName);
				}

				destinationListSize = destinationList.getSize();
				if (destinationListSize != 0) {
					RegisterName sourceName = RegisterName(destinationList.getOne());
					RegisterName destinationName;

					if (!copyStats[sourceName].temporaryName) {
						// Cycle problem.
						RegisterClassKind classKind = copyStats[sourceName].classKind;
						destinationName = getName(vrManager.newVirtualRegister(classKind));
						assert(destinationName < maxNameCount);
				
						copyStats[destinationName].classKind = classKind;
						copyStats[sourceName].temporaryName = destinationName;

						// Insert the copy instruction at the end of the current node.
						RegisterName from = copyStats[sourceName].sourceNameToUse;

						RegisterID fromID = buildRegisterID(from, classKind);
						RegisterID toID = buildRegisterID(destinationName, classKind);
						Instruction& copy = emitter.newCopy(*vrManager.getVirtualRegister(from).getDefiningInstruction()->getPrimitive(), fromID, toID);
						vrManager.getVirtualRegister(destinationName).setDefiningInstruction(copy);
						instructions.insertAfter(copy, endOfTheNode);
						endOfTheNode = instructions.advance(endOfTheNode);
					} else 
						destinationName = copyStats[sourceName].temporaryName;

					copyStats[sourceName].useCount = 0;
					copyStats[sourceName].isLiveOut = false;
					copyStats[sourceName].sourceNameToUse = destinationName;
					pushName(pool, &(copyStats[sourceName].newName), pushed, &pushedList, sourceName, destinationName);

					workList.set(sourceName);
				}
			}

			nodeStackPtr->pushedList = pushedList;
			nodeStackPtr->next = next;
			nodeStackPtr->limit = limit;
			++nodeStackPtr;
			next = dGraph.getSuccessorsBegin(currentNode);
			limit = dGraph.getSuccessorsEnd(currentNode);
		}
	}
}

#endif // _PHI_NODE_REMOVER_H_
