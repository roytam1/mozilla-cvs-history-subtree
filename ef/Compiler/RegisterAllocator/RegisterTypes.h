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

#ifndef _REGISTER_TYPES_H_
#define _REGISTER_TYPES_H_

#include "Fundamentals.h"

//------------------------------------------------------------------------------
// RegisterName -
//

enum RegisterName {
	rnInvalid = 0,
};

//------------------------------------------------------------------------------
// RegisterClassKind -
//

enum RegisterClassKind {
	rckInvalid = 0,
	rckGeneral,
	rckStackSlot,

	nRegisterClassKind
};

//------------------------------------------------------------------------------
// RegisterID -
//

enum RegisterID {
	invalidID = 0
};

//------------------------------------------------------------------------------
// RegisterKind -
//

enum RegisterKind {
	rkCallerSave = 0,
	rkCalleeSave,
};

struct NameLinkedList {
	RegisterName name;
	NameLinkedList* next;
};

#ifdef DEBUG

const registerNameMask = 		0x03ffffff;
const coloredRegisterMask = 	0x04000000;
const machineRegisterMask = 	0x08000000;
const registerClassMask = 		0xf0000000;

const registerNameShift = 		0;
const coloredRegisterShift = 	26;
const machineRegisterShift = 	27;
const registerClassShift = 		28;

#else // DEBUG

const registerNameMask = 		0x0fffffff;
const registerClassMask = 		0xf0000000;

const registerNameShift = 		0;
const registerClassShift = 		28;

#endif // DEBUG


inline RegisterClassKind getClass(RegisterID registerID) {return RegisterClassKind((registerID & registerClassMask) >> registerClassShift);}
inline RegisterName getName(RegisterID registerID) {return RegisterName((registerID & registerNameMask) >> registerNameShift);}
inline void setClass(RegisterID& registerID, RegisterClassKind classKind) {registerID = RegisterID((registerID & ~registerClassMask) | ((classKind << registerClassShift) & registerClassMask));}
inline void setName(RegisterID& registerID, RegisterName name) {assert((name & ~registerNameMask) == 0); registerID = RegisterID((registerID & ~registerNameMask) | ((name << registerNameShift) & registerNameMask));}
inline RegisterID buildRegisterID(RegisterName name, RegisterClassKind classKind) {return RegisterID(((classKind << registerClassShift) & registerClassMask) | ((name << registerNameShift) & registerNameMask));}

#ifdef DEBUG

inline bool isMachineRegister(RegisterID rid) {return (rid & machineRegisterMask) != 0;}
inline void setMachineRegister(RegisterID& rid) {rid = RegisterID(rid | machineRegisterMask);}
inline bool isColoredRegister(RegisterID rid) {return (rid & coloredRegisterMask) != 0;}
inline void setColoredRegister(RegisterID& rid) {rid = RegisterID(rid | coloredRegisterMask);}

#endif // DEBUG

#endif // _REGISTER_TYPES_H_
