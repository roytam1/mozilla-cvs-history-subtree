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

#ifndef _LIVE_RANGE_H_
#define _LIVE_RANGE_H_

#include "Fundamentals.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Primitives.h"
#include "Instruction.h"
#include "RegisterAllocator.h"
#include "RegisterAllocatorTools.h"

template <class RegisterPressure>
struct LiveRange
{
	static void build(RegisterAllocator& registerAllocator);
};

template <class RegisterPressure>
void LiveRange<RegisterPressure>::build(RegisterAllocator& registerAllocator)
{
	// Intialize the lookup table.
	//
	Uint32 nameCount = registerAllocator.nameCount;
	RegisterName* nameTable = new(registerAllocator.pool) RegisterName[2*nameCount];
	RegisterName* rangeName = &nameTable[nameCount];

	init(rangeName, nameCount);

	// Walk the graph.
	//
	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	SparseSet destination(registerAllocator.pool, nameCount);

	for (Uint32 n = 0; n < nNodes; n++) {
		InstructionList& phiNodes = nodes[n]->getPhiNodeInstructions();

		destination.clear();
		for (InstructionList::iterator i = phiNodes.begin(); !phiNodes.done(i); i = phiNodes.advance(i)) {
			Instruction& phiNode = phiNodes.get(i);
			assert(phiNode.getInstructionDefineBegin() != phiNode.getInstructionDefineEnd() && phiNode.getInstructionDefineBegin()[0].isRegister());
			destination.set(findRoot(phiNode.getInstructionDefineBegin()[0].getRegisterName(), rangeName));
		}

		for (InstructionList::iterator p = phiNodes.begin(); !phiNodes.done(p); p = phiNodes.advance(p)) {
			Instruction& phiNode = phiNodes.get(p);

			assert(phiNode.getInstructionDefineBegin() != phiNode.getInstructionDefineEnd() && phiNode.getInstructionDefineBegin()[0].isRegister());
			RegisterName destinationName = phiNode.getInstructionDefineBegin()[0].getRegisterName();
			RegisterName destinationRoot = findRoot(destinationName, rangeName);

			InstructionUse* useEnd = phiNode.getInstructionUseEnd();
			for (InstructionUse* usePtr = phiNode.getInstructionUseBegin(); usePtr < useEnd; usePtr++) {
				assert(usePtr->isRegister());
				RegisterName sourceName = usePtr->getRegisterName();
				RegisterName sourceRoot = findRoot(sourceName, rangeName);

				if (sourceRoot != destinationRoot && !destination.test(sourceRoot))
					rangeName[sourceRoot] = destinationRoot;
			}
		}
	}

	registerAllocator.rangeCount = compress(registerAllocator.name2range, rangeName, nameCount, nameCount);
}

#endif // _LIVE_RANGE_H_
