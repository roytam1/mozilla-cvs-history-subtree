/*
 * @(#)jni_md.h	1.2 96/10/24
 *
 * Copyright (c) 1993-1996 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies.
 *
 * The Java source code is the confidential and proprietary information
 * of Sun Microsystems, Inc. ("Confidential Information").  You shall
 * not disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Sun.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef JNI_MD_H
#define JNI_MD_H

#ifdef XP_MAC

#define JNIEXPORT 
#define JNICALL

/*
	Types.h is part of the MacOS Universal Interfaces release.
	It defines SInt32, wide, and SInt8.
	
	We cannot use SInt64 for jlong because it maps to long long
	when the compiler supports it, but MRJ 2.0 was built assumming
	jlong was a struct which uses different calling conventions.
*/
#include <Types.h>

typedef SInt32 		jint;
typedef wide 		jlong;
typedef UInt8 		jbyte;

#else

/* Get the rest of the stuff from jri_md.h */
#include "jri_md.h"

#ifndef FAR		/* for non-Win16 */
#define FAR
#endif

#endif /* !XP_MAC */

#endif /* JNI_MD_H */
