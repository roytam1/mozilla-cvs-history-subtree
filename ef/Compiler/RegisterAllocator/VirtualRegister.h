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

#ifndef _VIRTUAL_REGISTER_H_
#define _VIRTUAL_REGISTER_H_

#include "Fundamentals.h"
#include "IndexedPool.h"
#include <string.h>

#include "RegisterTypes.h"
#include "RegisterClass.h"


//------------------------------------------------------------------------------
// VirtualRegister - 24b

class Instruction;

class VirtualRegister : public IndexedObject<VirtualRegister>
{
public:

	Instruction*					definingInstruction;	// Instruction defining this VR.

	// Initialize a VR of the given classKind.
	VirtualRegister(RegisterClassKind /*classKind*/) : definingInstruction(NULL) {}

	// Return the defining instruction for this VR.
	Instruction* getDefiningInstruction() const {return definingInstruction;}
	// Set the defining instruction.
	void setDefiningInstruction(Instruction& insn);
};

// Return true if the VirtualRegisters are equals. The only way 2 VRs can be equal is if
// they have the same index. If they have the same index then they are at the same
// address in the indexed pool.
//
inline bool operator == (const VirtualRegister& regA, const VirtualRegister& regB) {return &regA == &regB;}

//------------------------------------------------------------------------------
// VirtualRegisterManager -

struct PreColoredRegister
{
	RegisterID id;
	RegisterName color;
};

class VirtualRegisterManager
{
private:

	IndexedPool<VirtualRegister> registerPool;
	PreColoredRegister machineRegister[6];

public:
	VirtualRegisterManager()
		{
			for (Uint32 i = 0; i < 6; i++)
				machineRegister[i].id = invalidID;
		}

	// Return the VirtualRegister at the given index.
	VirtualRegister& getVirtualRegister(RegisterName name) const {return registerPool.get(name);}

	// Return a new VirtualRegister.
	RegisterID newVirtualRegister(RegisterClassKind classKind)
		{
			VirtualRegister& vReg = *new(registerPool) VirtualRegister(classKind);
			RegisterID rid;

			setName(rid, RegisterName(vReg.getIndex()));
			setClass(rid, classKind);
			return rid;
		}

	RegisterID newMachineRegister(RegisterName name, RegisterClassKind classKind)
		{
			RegisterID rid = machineRegister[name].id;

			if (rid == invalidID) {
				rid = newVirtualRegister(classKind);
				DEBUG_ONLY(setMachineRegister(rid));
				machineRegister[name].id = rid;
				machineRegister[name].color = name;
			}

			return rid;
		}

	PreColoredRegister* getMachineRegistersBegin() const {return (PreColoredRegister*) machineRegister;} // FIX
	PreColoredRegister* getMachineRegistersEnd() const {return (PreColoredRegister*) &machineRegister[6];} // FIX

	// Return the VirtualRegister universe size.
	Uint32 getSize() {return registerPool.getSize();}

	void setSize(Uint32 size) {registerPool.setSize(size);}
};

#endif // _VIRTUAL_REGISTER_H_
