/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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

/*
 * This section typedefs the old 'native' types to the new PR<type>s.
 * These definitions are scheduled to be eliminated at the earliest
 * possible time. The NSPR API is implemented and documented using
 * the new definitions.
 */

#ifndef PROTYPES_H
#define PROTYPES_H

/* SVR4 typedef of uint is commonly found on UNIX machines. */
#ifndef XP_UNIX
typedef JSUintn uint;
#endif

typedef JSUintn uintn;
typedef JSUint64 uint64;
#if !defined(XP_MAC) && !defined(_WIN32) && !defined(XP_OS2)
typedef JSUint32 uint32;
#else
typedef unsigned long uint32;
#endif
typedef JSUint16 uint16;
typedef JSUint8 uint8;

#ifndef _XP_Core_
typedef JSIntn intn;
#endif

/*
 * On AIX 4.3, sys/inttypes.h (which is included by sys/types.h, a very
 * common header file) defines the types int8, int16, int32, and int64.
 * So we don't define these four types here to avoid conflicts in case
 * the code also includes sys/types.h.
 */
#ifdef AIX4_3
#include <sys/inttypes.h>
#else
typedef JSInt64 int64;

/* /usr/include/model.h on HP-UX defines int8, int16, and int32 */
#ifdef HPUX
#include <model.h>
#else
#if !defined(WIN32) || !defined(_WINSOCK2API_)  /* defines its own "int32" */
#if !defined(XP_MAC) && !defined(_WIN32) && !defined(XP_OS2)
typedef JSInt32 int32;
#else
typedef long int32;
#endif
#endif
typedef JSInt16 int16;
typedef JSInt8 int8;
#endif /* HPUX */
#endif /* AIX4_3 */

typedef JSFloat64 float64;

/* Re: jsbit.h */
#define TEST_BIT	JS_TEST_BIT
#define SET_BIT		JS_SET_BIT
#define CLEAR_BIT	JS_CLEAR_BIT

#ifdef XP_MAC
#ifndef TRUE				/* Mac standard is lower case true */
	#define TRUE 1
#endif
#ifndef FALSE				/* Mac standard is lower case false */
	#define FALSE 0
#endif
#endif

#endif /* !defined(PROTYPES_H) */
