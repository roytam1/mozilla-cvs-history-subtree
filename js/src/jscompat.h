/* -*- Mode: C; tab-width: 8 -*-
 * Copyright © 1996 Netscape Communications Corporation, All Rights Reserved.
 */
#ifndef jscompat_h___
#define jscompat_h___
/*
 * Compatibility glue for various NSPR versions.  We must always define int8,
 * int16, prword, and so on to minimize differences with js/ref, no matter what
 * the NSPR typedef names may be.
 */
#include "prtypes.h"
#include "prlong.h"
typedef PRIntn intN;
typedef PRUintn uintN;
/* Following are already available in compatibility mode of NSPR 2.0 */
#if 0
typedef PRInt64 int64;
typedef PRInt32 int32;
typedef PRInt16 int16;
typedef PRInt8 int8;
typedef uint64 uint64;
typedef uint32 uint32;
typedef uint16 uint16;
typedef uint8 uint8;
#endif
typedef PRUword pruword;
typedef PRWord prword;


typedef float float32;
#define allocPriv allocPool
#endif /* jscompat_h___ */
