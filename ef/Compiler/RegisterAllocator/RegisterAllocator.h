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

#ifndef _REGISTER_ALLOCATOR_H_
#define _REGISTER_ALLOCATOR_H_

class Pool;
class ControlGraph;
class InstructionEmitter;
struct SpillCost;
struct SplitCost;

#include "Liveness.h"
#include "VirtualRegister.h"
#include "RegisterPressure.h" // This should included by Backend.cpp
#include "InterferenceGraph.h"
#include "LiveRangeGraph.h"

//template <class RegisterPressure>
class RegisterAllocator
{
public:

	Pool&									pool;			//
	ControlGraph&							controlGraph;	//
	VirtualRegisterManager&					vrManager;		//
	InstructionEmitter&						emitter;		//

	RegisterName*							name2range;		//
	RegisterName*							color;			//
	SpillCost*								spillCost;		//
	SparseSet*								willSpill;		//
	SplitCost*								splitCost;		//
	NameLinkedList**						splitAround;	//
	InterferenceGraph<LowRegisterPressure>	iGraph;			//
	LiveRangeGraph<LowRegisterPressure>		lGraph;			//
	LivenessInfo<LowRegisterPressure>		liveness;		//
	Uint32									nameCount;		//
	Uint32									rangeCount;		//
	bool									splitFound;		//

private:

	//
	//
	void doGraphColoring();

public:

	//
	//
	inline RegisterAllocator(Pool& pool, ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter);

	//
	//
	bool canInterfere(RegisterName /*name1*/, RegisterName /*name2*/) const {return true;}

	//
	//
	void initLiveRanges();

	//
	//
	static void allocateRegisters(Pool& pool, ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter);
};

//
//
inline RegisterAllocator::RegisterAllocator(Pool& pool, ControlGraph& controlGraph, VirtualRegisterManager& vrManager, InstructionEmitter& emitter)
	: pool(pool), controlGraph(controlGraph), vrManager(vrManager), emitter(emitter), iGraph(*this), lGraph(*this), nameCount(vrManager.getSize()) {}

#endif // _REGISTER_ALLOCATOR_H_

