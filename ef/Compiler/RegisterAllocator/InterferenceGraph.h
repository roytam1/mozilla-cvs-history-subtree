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

#ifndef _INTERFERENCE_GRAPH_H_
#define _INTERFERENCE_GRAPH_H_

#include "Fundamentals.h"
#include "ControlGraph.h"
#include "Primitives.h"
#include "Instruction.h"
#include "VirtualRegister.h"
#include "RegisterPressure.h"
#include "SparseSet.h"
#include <string.h>

struct InterferenceVector
{
	Uint32				count;
	InterferenceVector*	next;
	RegisterName*		neighbors;

	InterferenceVector() : count(0), next(NULL) {}
};

class RegisterAllocator;

template <class RegisterPressure>
class InterferenceGraph
{
private:

	RegisterAllocator&			registerAllocator;

	RegisterPressure::Set*		interferences;
	InterferenceVector**		vector;
	Uint32*						offset;
	Uint32 						rangeCount;

private:

	// No copy constructor.
	InterferenceGraph(const InterferenceGraph&);
	// No copy operator.
	void operator = (const InterferenceGraph&);

	// Check if reg is a member of the universe.
	void checkMember(RegisterName name) {assert(name < rangeCount);}
	// Return the edge index for the interference between name1 and name2.
	Uint32 getEdgeIndex(RegisterName name1, RegisterName name2);

public:
	InterferenceGraph(RegisterAllocator& registerAllocator) : registerAllocator(registerAllocator) {}

	// Calculate the interferences.
	void build();
	// Return true if reg1 and reg2 interfere.
	bool interfere(RegisterName name1, RegisterName name2);
	// Return the interference vector for the given register or NULL if there is none.
	InterferenceVector* getInterferenceVector(RegisterName name) {return vector[name];}
	// Set the interference between name1 and name2.
	void setInterference(RegisterName name1, RegisterName name2);
	// Set the interference vector for the given register.
	void setInterferenceVector(RegisterName name, InterferenceVector* v) {vector[name] = v;}


#ifdef DEBUG_LOG
	// Print the interferences.
	void printPretty(LogModuleObject log);
#endif // DEBUG_LOG
};

template <class RegisterPressure>
void InterferenceGraph<RegisterPressure>::build()
{
	Pool& pool = registerAllocator.pool;
	Uint32 rangeCount = registerAllocator.rangeCount;
	this->rangeCount = rangeCount;

	// Initialize the structures.
	//
	offset = new(pool) Uint32[rangeCount + 1];
	vector = new(pool) InterferenceVector*[rangeCount];
	memset(vector, '\0', sizeof(InterferenceVector*) * rangeCount);

	Uint32 o = 0;
	offset[0] = 0;
	for (Uint32 i = 1; i <= rangeCount; ++i) {
		offset[i] = o;
		o += i;
	}
	
	interferences = new(pool) RegisterPressure::Set(pool, (rangeCount * rangeCount) / 2);

	ControlGraph& controlGraph = registerAllocator.controlGraph;
	ControlNode** nodes = controlGraph.dfsList;
	Uint32 nNodes = controlGraph.nNodes;

	RegisterName* name2range = registerAllocator.name2range;
	LivenessInfo<RegisterPressure> liveness = Liveness<RegisterPressure>::analysis(controlGraph, rangeCount, name2range);
	registerAllocator.liveness = liveness;
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

			// Handle the copy instruction to avoid unnecessary interference between the 2 registers.
			if ((instruction.getFlags() & ifCopy) != 0) {
				assert(useBegin != useEnd && useBegin[0].isRegister());
				currentLive.clear(name2range[useBegin[0].getRegisterName()]);
			}

			// Create the interferences.
			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister()) {
					RegisterName define = name2range[definePtr->getRegisterName()];

					for (SparseSet::iterator e = currentLive.begin(); !currentLive.done(e); e = currentLive.advance(e)) {
						RegisterName live = RegisterName(currentLive.get(e));

						if ((live != define) && !interfere(live, define) && registerAllocator.canInterfere(live, define)) {

							if (vector[define] == NULL)
								vector[define] = new(pool) InterferenceVector();
							vector[define]->count++;

							if (vector[live] == NULL)
								vector[live] = new(pool) InterferenceVector();
							vector[live]->count++;

							setInterference(live, define);
						}
					}
				}

			// Now update the liveness.
			//
			for (definePtr = defineBegin; definePtr < defineEnd; definePtr++)
				if (definePtr->isRegister())
					currentLive.clear(name2range[definePtr->getRegisterName()]);

			for (usePtr = useBegin; usePtr < useEnd; usePtr++)
				if (usePtr->isRegister())
					currentLive.set(name2range[usePtr->getRegisterName()]);
		}
	}

	// Allocate the memory to store the interferences.
	//
	for (Uint32 e = 0; e < rangeCount; e++)
		if (vector[e] != NULL) {
			InterferenceVector& v = *vector[e];
			v.neighbors = new(pool) RegisterName[v.count];
			v.count = 0;
		}

	// Initialize the edges.
	//
	if (RegisterPressure::Set::isOrdered()) {
		RegisterName name1 = RegisterName(0);

		for (RegisterPressure::Set::iterator i = interferences->begin(); !interferences->done(i); i = interferences->advance(i)) {
			Uint32 interferenceIndex = interferences->get(i);

			while(interferenceIndex >= offset[name1 + 1])
				name1 = RegisterName(name1 + 1);

			assert((interferenceIndex >= offset[name1]) && (interferenceIndex < offset[name1 + 1]));

			RegisterName name2 = RegisterName(interferenceIndex - offset[name1]);

			assert(interfere(name1, name2));

			InterferenceVector& vector1 = *vector[name1];
			vector1.neighbors[vector1.count++] = name2;
		
			InterferenceVector& vector2 = *vector[name2];
			vector2.neighbors[vector2.count++] = name1;
		}
	} else {
		trespass("not Implemented"); // FIX: need one more pass to initialize the vectors.
	}
}

template <class RegisterPressure>
Uint32 InterferenceGraph<RegisterPressure>::getEdgeIndex(RegisterName name1, RegisterName name2)
{
	checkMember(name1); checkMember(name2);
	assert(name1 != name2); // This is not possible.
	return (name1 < name2) ? offset[name2] + name1 : offset[name1] + name2;
}

template <class RegisterPressure>
void InterferenceGraph<RegisterPressure>::setInterference(RegisterName name1, RegisterName name2)
{
	interferences->set(getEdgeIndex(name1, name2));
}

template <class RegisterPressure>
bool InterferenceGraph<RegisterPressure>::interfere(RegisterName name1, RegisterName name2)
{
	return interferences->test(getEdgeIndex(name1, name2));
}

#ifdef DEBUG_LOG
template <class RegisterPressure>
void InterferenceGraph<RegisterPressure>::printPretty(LogModuleObject log)
{
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Interference Vectors:\n"));
	for (Uint32 i = 1; i < rangeCount; i++) {
		if (vector[i] != NULL) {
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\tvr%d: (", i));
			for (InterferenceVector* v = vector[i]; v != NULL; v = v->next)
				for (Uint32 j = 0; j < v->count; j++) {
					UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("%d", v->neighbors[j]));
					if (v->next != NULL || j != (v->count - 1))
						UT_OBJECTLOG(log, PR_LOG_ALWAYS, (","));
				}
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, (")\n"));
		}
	}
	UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("Interference Matrix:\n"));
	for (RegisterName name1 = RegisterName(1); name1 < rangeCount; name1 = RegisterName(name1 + 1)) {
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\t%d:\t", name1));
		for (RegisterName name2 = RegisterName(1); name2 < rangeCount; name2 = RegisterName(name2 + 1))
			UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("%c", ((name1 != name2) && interfere(name1, name2)) ? '1' : '0'));
		UT_OBJECTLOG(log, PR_LOG_ALWAYS, ("\n"));
	}
}
#endif // DEBUG_LOG

#endif // _INTERFERENCE_GRAPH_H_
