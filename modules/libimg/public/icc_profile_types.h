#if defined (COLORSYNC)
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */

/* This file contains external references for ICC Profiles. */
#ifndef _icc_profile_types_h
#define _icc_profile_types_h

/*	---------------------------------------------------------------------------
	Constants
*/

/*
	These are IDs for standard profiles.
	They can be used in place of an actual profile ref
	to signal to the front end to use a known profile.
*/
#define		kICCProfileRef_DefaultProfile	0xFFFFFFFE
#define		kICCProfileRef_SystemProfile	0xFFFFFFFF
#define		kICCProfileRef_NoProfile		0x0
#define		kICCProfileRef_AVID				0x00000001
#define		kICCProfileRefConstants			0x00000001

#endif	/*	_icc_profile_types_h */
#endif /* (COLORSYNC) */
