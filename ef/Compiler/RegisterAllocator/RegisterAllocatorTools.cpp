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
#include "LogModule.h"
#include "RegisterAllocatorTools.h"
#include "Pool.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Primitives.h"
#include "InstructionEmitter.h"
#include "Instruction.h"
#include "RegisterAllocator.h"
#include "Spilling.h"
#include "Splits.h"
#include "BitSet.h"

UT_EXTERN_LOG_MODULE(RegAlloc);

#ifdef DEBUG
void RegisterAllocatorTools::testTheInstructionGraph(ControlGraph& controlGraph, VirtualRegisterManager& vrManager)
{
	// Test the declared VirtualRegisters. The register allocator tries to condense the register universe.
	// Any gap in the VirtualRegister names will be a loss of efficiency !!!!

	Uint32 nameCount = vrManager.getSize();
	BitSet registerSeen(controlGraph.pool, nameCount);

	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {

		InstructionList& instructions = nodes[n]->getInstructions();
		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					registerSeen.set(usePtr->getRegisterName());

			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					registerSeen.set(definePtr->getRegisterName());
		}

		InstructionList& phiNodes = nodes[n]->getPhiNodeInstructions();
		for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
			Instruction& instruction = phiNodes.get(p);

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					registerSeen.set(usePtr->getRegisterName());

			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					registerSeen.set(definePtr->getRegisterName());
		}
	}

	bool renameRegisters = false;
	for (BitSet::iterator i = registerSeen.nextZero(0); !registerSeen.done(i); i = registerSeen.nextZero(i)) {
		renameRegisters = true;
		fprintf(stderr, 
				"WARNING: The VirtualRegister vr%d has been allocated during CodeGeneration but\n"
				"         is never used nor defined by any instruction in the instruction graph\n"
				"                                  PLEASE FIX                                  \n",
				i);
	}
	if (renameRegisters) {
		Instruction** definingInstruction = new Instruction*[nameCount];
		memset(definingInstruction, '\0', nameCount * sizeof(Instruction*));
		RegisterName* newName = new RegisterName[nameCount];
		memset(newName, '\0', nameCount * sizeof(RegisterName));
		RegisterName nextName = RegisterName(1);

		for (Uint32 n = 0; n < nNodes; n++) {

			InstructionList& instructions = nodes[n]->getInstructions();
			for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
				Instruction& instruction = instructions.get(i);

				InstructionUse* useEnd = instruction.getInstructionUseEnd();
				for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
					if (usePtr->isRegister()) {
						RegisterName name = usePtr->getRegisterName();
						if (newName[name] == rnInvalid) {
							newName[name] = nextName;
							definingInstruction[nextName] = vrManager.getVirtualRegister(name).getDefiningInstruction();
							nextName = RegisterName(nextName + 1);
						}
						usePtr->setRegisterName(newName[name]);
					}

				InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
				for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
					if (definePtr->isRegister()) {
						RegisterName name = definePtr->getRegisterName();
						if (newName[name] == rnInvalid) {
							newName[name] = nextName;
							definingInstruction[nextName] = vrManager.getVirtualRegister(name).getDefiningInstruction();
							nextName = RegisterName(nextName + 1);
						}
						definePtr->setRegisterName(newName[name]);
					}
			}

			InstructionList& phiNodes = nodes[n]->getPhiNodeInstructions();
			for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
				Instruction& instruction = phiNodes.get(p);

				InstructionUse* useEnd = instruction.getInstructionUseEnd();
				for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
					if (usePtr->isRegister()) {
						RegisterName name = usePtr->getRegisterName();
						if (newName[name] == rnInvalid) {
							newName[name] = nextName;
							definingInstruction[nextName] = vrManager.getVirtualRegister(name).getDefiningInstruction();
							nextName = RegisterName(nextName + 1);
						}
						usePtr->setRegisterName(newName[name]);
					}

				InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
				for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
					if (definePtr->isRegister()) {
						RegisterName name = definePtr->getRegisterName();
						if (newName[name] == rnInvalid) {
							newName[name] = nextName;
							definingInstruction[nextName] = vrManager.getVirtualRegister(name).getDefiningInstruction();
							nextName = RegisterName(nextName + 1);
						}
						definePtr->setRegisterName(newName[name]);
					}
			}
		}
		
		vrManager.setSize(nextName);

		for (RegisterName r = RegisterName(1); r < nextName; r = RegisterName(r + 1))
			vrManager.getVirtualRegister(r).definingInstruction = definingInstruction[r];

		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("RegisterMap:\n"));
		for (Uint32 i = 1; i < nameCount; i++)
			if (newName[i] != 0)
				UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\tvr%d becomes vr%d.\n", i, newName[i]));
			else
				UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("\tvr%d is dead.\n", i));


		delete newName;
		delete definingInstruction;
	}

}
#endif // DEBUG

void RegisterAllocatorTools::removeUnnecessaryCopies(RegisterAllocator& registerAllocator)
{
	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;
	RegisterName* name2range = registerAllocator.name2range;

	for (Uint32 n = 0; n < nNodes; n++) {
		InstructionList& instructions = nodes[n]->getInstructions();
		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i);) {
			Instruction& instruction = instructions.get(i);
			i = instructions.advance(i);

			if (instruction.getFlags() & ifCopy) {
				assert(instruction.getInstructionUseBegin() != instruction.getInstructionUseEnd() && instruction.getInstructionUseBegin()[0].isRegister());
				assert(instruction.getInstructionDefineBegin() != instruction.getInstructionDefineEnd() && instruction.getInstructionDefineBegin()[0].isRegister());
				
				RegisterName source = name2range[instruction.getInstructionUseBegin()[0].getRegisterName()];
				RegisterName destination = name2range[instruction.getInstructionDefineBegin()[0].getRegisterName()];

				if (source == destination)
					instruction.remove();
			}
		}
	}
}


void RegisterAllocatorTools::updateInstructionGraph(RegisterAllocator& registerAllocator)
{
	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;
	RegisterName* name2range = registerAllocator.name2range;

	for (Uint32 n = 0; n < nNodes; n++) {
		InstructionList& instructions = nodes[n]->getInstructions();
		for (InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			Instruction& instruction = instructions.get(i);

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					usePtr->setRegisterName(name2range[usePtr->getRegisterName()]);

			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					definePtr->setRegisterName(name2range[definePtr->getRegisterName()]);
		}

		InstructionList& phiNodes = nodes[n]->getPhiNodeInstructions();
		for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
			Instruction& instruction = phiNodes.get(p);

			InstructionUse* useEnd = instruction.getInstructionUseEnd();
			for (InstructionUse* usePtr = instruction.getInstructionUseBegin(); usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					usePtr->setRegisterName(name2range[usePtr->getRegisterName()]);

			InstructionDefine* defineEnd = instruction.getInstructionDefineEnd();
			for (InstructionDefine* definePtr = instruction.getInstructionDefineBegin(); definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					definePtr->setRegisterName(name2range[definePtr->getRegisterName()]);
		}
	}
}


void RegisterAllocatorTools::insertPhiNodeInstructions(ControlGraph& controlGraph, InstructionEmitter& emitter)
{
	Pool& pool = controlGraph.pool;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;
	
	for (Uint32 n = 0; n < nNodes; n++) {
		ControlNode& node = *nodes[n];
		DoublyLinkedList<PhiNode>& phiNodes = node.getPhiNodes();

		if (!phiNodes.empty()) {

			// Set the index of the incoming edges.
			Uint32 index = 0;
			const DoublyLinkedList<ControlEdge>& predecessors = node.getPredecessors();
			for (DoublyLinkedList<ControlEdge>::iterator p = predecessors.begin(); !predecessors.done(p); p = predecessors.advance(p))
				predecessors.get(p).setIndex(index++);

			// Insert the phi node instruction in the instruction list.
			for (DoublyLinkedList<PhiNode>::iterator i = phiNodes.begin(); !phiNodes.done(i); i = phiNodes.advance(i)) {
				PhiNode& phiNode = phiNodes.get(i);
				ValueKind kind = phiNode.getKind();

				if (!isStorableKind(kind))
					continue;

				RegisterClassKind classKind = rckGeneral; // FIX: get class kind from phi node kind.
				Uint32 nInputs = phiNode.nInputs();

				PhiNodeInstruction& phiNodeInstruction = *new(pool) PhiNodeInstruction(&phiNode, pool, nInputs);

				emitter.defineProducer(phiNode, phiNodeInstruction, 0, classKind, drLow);
				for (Uint32 whichInput = 0; whichInput < nInputs; whichInput++)
					emitter.useProducer(phiNode.nthInputVariable(whichInput), phiNodeInstruction, whichInput, classKind, drLow);

				node.addPhiNodeInstruction(phiNodeInstruction);

				if (isDoublewordKind(kind)) {
					PhiNodeInstruction& phiNodeInstruction = *new(pool) PhiNodeInstruction(&phiNode, pool, nInputs);

					emitter.defineProducer(phiNode, phiNodeInstruction, 0, classKind, drHigh);
					for (Uint32 whichInput = 0; whichInput < nInputs; whichInput++)
						emitter.useProducer(phiNode.nthInputVariable(whichInput), phiNodeInstruction, whichInput, classKind, drHigh);

					node.addPhiNodeInstruction(phiNodeInstruction);
				}
			}
		}
	}
}

#ifdef DEBUG_LOG

void RegisterAllocatorTools::printSpillCosts(RegisterAllocator& registerAllocator)
{
	LogModuleObject log = UT_LOG_MODULE(RegAlloc);
	Uint32 rangeCount = registerAllocator.rangeCount;
	SpillCost* cost = registerAllocator.spillCost;

	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Spill costs:\n"));
	for (Uint32 i = 1; i < rangeCount; i++) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\trange %d : ", i));
		if (cost[i].infinite)
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("infinite\n"));
		else
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("%f\n", cost[i].cost));
	}
}

void RegisterAllocatorTools::printSplitCosts(RegisterAllocator& registerAllocator)
{
	LogModuleObject log = UT_LOG_MODULE(RegAlloc);
	Uint32 rangeCount = registerAllocator.rangeCount;
	SplitCost* cost = registerAllocator.splitCost;

	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Split costs:\n"));
	for (Uint32 i = 1; i < rangeCount; i++) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\trange %d : loads = %f stores = %f\n", i, cost[i].loads, cost[i].stores));
	}
}

void RegisterAllocatorTools::printInstructions(RegisterAllocator& registerAllocator)
{
	LogModuleObject log = UT_LOG_MODULE(RegAlloc);
	ControlNode** nodes = registerAllocator.controlGraph.dfsList;
	Uint32 nNodes = registerAllocator.controlGraph.nNodes;

	for (Uint32 n = 0; n < nNodes; n++) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("N%d:\n", n));

		InstructionList& phiNodes = nodes[n]->getPhiNodeInstructions();
		InstructionList& instructions = nodes[n]->getInstructions();

		if (!phiNodes.empty()) {
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, (" PhiNodes:\n", n));
			for(InstructionList::iterator i = phiNodes.begin(); !phiNodes.done(i); i = phiNodes.advance(i)) {
				phiNodes.get(i).printPretty(log);
				UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
			}
			if (!instructions.empty())
				UT_OBJECTLOG(log, PR_LOG_ALWAYS, (" Instructions:\n", n));
		}

		for(InstructionList::iterator i = instructions.begin(); !instructions.done(i); i = instructions.advance(i)) {
			instructions.get(i).printPretty(log);
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
		}
	}
}
#endif // DEBUG_LOG
