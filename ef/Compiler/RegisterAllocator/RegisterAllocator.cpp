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
#include "RegisterAllocator.h"
#include "RegisterPressure.h"
#include "RegisterAllocatorTools.h"
#include "PhiNodeRemover.h"
#include "LiveRange.h"
#include "Liveness.h"
#include "InterferenceGraph.h"
#include "LiveRangeGraph.h"
#include "Coalescing.h"
#include "Spilling.h"
#include "Coloring.h"
#include "Splits.h"

class Pool;
class ControlGraph;
class VirtualRegisterManager;
class InstructionEmitter;

UT_DEFINE_LOG_MODULE(RegAlloc);

void RegisterAllocator::allocateRegisters(Pool& pool, ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter)
{
	// Insert the phi node instructions. We want to do this to have a single defined register per instruction.
	// If we keep the PhiNode (as a DataNode) and a PhiNode is of DoubleWordKind then we have to execute
	// some special code for the high word annotation.
	//
	RegisterAllocatorTools::insertPhiNodeInstructions(controlGraph, emitter);

	// Perform some tests on the instruction graph.
	//
	DEBUG_ONLY(RegisterAllocatorTools::testTheInstructionGraph(controlGraph, vrManager));

	// Replace the phi node instructions by their equivalent copy instructions.
	//
	PhiNodeRemover<LowRegisterPressure>::replacePhiNodes(controlGraph, vrManager, emitter);

	// Do the register allocation.
	//
	RegisterAllocator registerAllocator(pool, controlGraph, vrManager, emitter);
	registerAllocator.doGraphColoring();
}

void RegisterAllocator::doGraphColoring()
{
	// Initialize the liverange map.
	//
	initLiveRanges();

	// Build the live ranges. We do this to compress the number of RegisterNames 
	// used in the insterference graph.
	//
	LiveRange<LowRegisterPressure>::build(*this);

	// Remove unnecessary copies.
	//
	RegisterAllocatorTools::removeUnnecessaryCopies(*this);

	for (Uint8 loop = 0; loop < 10; loop++) {

		UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("********* RegisterAllocator loop %d *********\n", loop));

		while(true) {
			// Build the interference graph.
			//
			iGraph.build();

			// Coalesce the copy instructions.
			//
			if (!Coalescing<LowRegisterPressure>::coalesce(*this))
				break;
		}

		// Print the interference graph.
		//
		DEBUG_LOG_ONLY(iGraph.printPretty(UT_LOG_MODULE(RegAlloc)));

		// Calculate the spill costs.
		//
		Spilling<LowRegisterPressure>::calculateSpillCosts(*this);
		DEBUG_LOG_ONLY(RegisterAllocatorTools::printSpillCosts(*this));

		// Calculate the split costs.
		//
		Splits<LowRegisterPressure>::calculateSplitCosts(*this);
		DEBUG_LOG_ONLY(RegisterAllocatorTools::printSplitCosts(*this));

		// Build the live range graph.
		//
		lGraph.build();
		DEBUG_LOG_ONLY(lGraph.printPretty(UT_LOG_MODULE(RegAlloc)));

		// Color the graph. If it succeeds then we're done with the
		// register allocation.
		//
		if (Coloring<LowRegisterPressure>::color(*this)) {
			// Write the final colors in the instruction graph.
			//
			Coloring<LowRegisterPressure>::finalColoring(*this);

			UT_OBJECTLOG(UT_LOG_MODULE(RegAlloc), PR_LOG_ALWAYS, ("********** RegisterAllocator  done **********\n"));
			DEBUG_LOG_ONLY(RegisterAllocatorTools::printInstructions(*this));

			return;
		}

		// We need to spill some registers.
		//
		Spilling<LowRegisterPressure>::insertSpillCode(*this);

		// Insert the split instructions.
		//
		Splits<LowRegisterPressure>::insertSplitCode(*this);

		// Update the live ranges.
		// 
		// FIX
	}

#ifdef DEBUG_LOG
	RegisterAllocatorTools::updateInstructionGraph(*this);
	RegisterAllocatorTools::printInstructions(*this);
#endif
	fprintf(stderr, "!!! Coloring failed after 10 loops !!!\n");
	abort();
}

void RegisterAllocator::initLiveRanges()
{
	Uint32 count = this->nameCount;
	RegisterName* name2range = new(pool) RegisterName[nameCount];
	for (RegisterName r = RegisterName(1); r < count; r = RegisterName(r + 1))
		name2range[r] = r;
	this->name2range = name2range;
	rangeCount = count;
}
