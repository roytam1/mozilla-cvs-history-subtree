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

#ifndef _COALESCING_H_
#define _COALESCING_H_

#include "Fundamentals.h"
#include "Pool.h"
#include "RegisterPressure.h"
#include "InterferenceGraph.h"
#include "ControlGraph.h"
#include "ControlNodes.h"
#include "Instruction.h"
#include "SparseSet.h"
#include "RegisterAllocator.h"
#include "RegisterAllocatorTools.h"

#if 1
// Performing an ultra conservative coalescing meens that when we look at
// candidates (source,destination) for coalescing we need to make sure
// that the combined interference of the source and destination register
// will not exceed the total number of register available for the register
// class. 
#define ULTRA_CONSERVATIVE_COALESCING
#else
// If we are not doing an ultra conservative coalescing we have to make sure 
// that the total number of neighbor whose degree is greater than the total 
// number of register is not greater than the total number of register.
#undef ULTRA_CONSERVATIVE_COALESCING
#endif

template <class RegisterPressure>
struct Coalescing
{
	static bool coalesce(RegisterAllocator& registerAllocator);
};

template <class RegisterPressure>
bool Coalescing<RegisterPressure>::coalesce(RegisterAllocator& registerAllocator)
{
	Pool& pool = registerAllocator.pool;

   	// Initialize the lookup table
	//
	Uint32 rangeCount = registerAllocator.rangeCount;
	RegisterName* newRange = new RegisterName[2 * rangeCount];
	RegisterName* coalescedRange = &newRange[rangeCount];
	RegisterName* name2range = registerAllocator.name2range;

	init(coalescedRange, rangeCount);

	SparseSet interferences(pool, rangeCount);
	InterferenceGraph<RegisterPressure>& iGraph = registerAllocator.iGraph;
	bool removedInstructions = false;

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.lndList;
	Uint32 nNodes = controlGraph.nNodes;


	// Walk the nodes in the loop nesting depth list.
	for (Int32 n = nNodes - 1; n >= 0; n--) {
		InstructionList& instructions = nodes[n]->getInstructions();

		InstructionList::iterator it = instructions.begin();
		while (!instructions.done(it)) {
			Instruction& instruction = instructions.get(it);
			it = instructions.advance(it);

			if ((instruction.getFlags() & ifCopy) != 0) {
				assert(instruction.getInstructionUseBegin() != instruction.getInstructionUseEnd() && instruction.getInstructionUseBegin()[0].isRegister());
				assert(instruction.getInstructionDefineBegin() != instruction.getInstructionDefineEnd() && instruction.getInstructionDefineBegin()[0].isRegister());

				RegisterName source = findRoot(name2range[instruction.getInstructionUseBegin()[0].getRegisterName()], coalescedRange);
				RegisterName destination = findRoot(name2range[instruction.getInstructionDefineBegin()[0].getRegisterName()], coalescedRange);

				if (source == destination) {
					instruction.remove();
				} else if (!iGraph.interfere(source, destination)) {
					InterferenceVector* sourceVector = iGraph.getInterferenceVector(source);
					InterferenceVector* destinationVector = iGraph.getInterferenceVector(destination);

#ifdef ULTRA_CONSERVATIVE_COALESCING
					interferences.clear();

					InterferenceVector* vector;
					for (vector = sourceVector; vector != NULL; vector = vector->next) {
						RegisterName* neighbors = vector->neighbors;
						for (Uint32 i = 0; i < vector->count; i++)
							interferences.set(findRoot(neighbors[i], coalescedRange));
					}
					for (vector = destinationVector; vector != NULL; vector = vector->next) {
						RegisterName* neighbors = vector->neighbors;
						for (Uint32 i = 0; i < vector->count; i++)
							interferences.set(findRoot(neighbors[i], coalescedRange));
					}

					Uint32 count = interferences.getSize();
#else // ULTRA_CONSERVATIVE_COALESCING
					trespass("not implemented");
					Uint32 count = 0;
#endif // ULTRA_CONSERVATIVE_COALESCING

					if (count < 6 /* FIX: should get the number from the class */) {
						// Update the interferences vector.
						if (sourceVector == NULL) {
							iGraph.setInterferenceVector(source, destinationVector);
							sourceVector = destinationVector;
						} else if (destinationVector == NULL)
							iGraph.setInterferenceVector(destination, sourceVector);
						else {
							InterferenceVector* last = NULL;
							for (InterferenceVector* v = sourceVector; v != NULL; v = v->next)
								last = v;
							assert(last);
							last->next = destinationVector;
							iGraph.setInterferenceVector(destination, sourceVector);
						}
						// Update the interference matrix.
						for (InterferenceVector* v = sourceVector; v != NULL; v = v->next) {
							RegisterName* neighbors = v->neighbors;
							for (Uint32 i = 0; i < v->count; i++) {
								RegisterName neighbor = findRoot(neighbors[i], coalescedRange);
								iGraph.setInterference(neighbor, source);
								iGraph.setInterference(neighbor, destination);
							}
						}

						instruction.remove();
						coalescedRange[source] = destination;
						removedInstructions = true;
					}
				}
			}
		}
	}

	registerAllocator.rangeCount = compress(registerAllocator.name2range, coalescedRange, registerAllocator.nameCount, rangeCount);
	delete newRange;

	return removedInstructions;
}

#endif // _COALESCING_H_
