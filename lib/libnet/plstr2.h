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
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef plstr2_h__
#define plstr2_h__

PR_BEGIN_EXTERN_C

/*
 * PL_strtok
 *
 * A string tokenizer.
 */

PR_EXTERN(char *)
PL_strtok(char *s, const char *tokens);

/*
 * PL_strtok_r
 *
 * A thread safe string tokenizer.
 */

PR_EXTERN(char *)
PL_strtok_r(char *s, const char *tokens, char **next);

/*
 * The PL_mem* functions are reliable replacements for the standard 
 * mem* functions. In most cases the platform mem* functions are
 * optimized and should be used instead. If a platform requires an 
 * implementation it should set PL_MEM_IMPLEMENT. Otherwise the PL_mem* 
 * functions will be mapped onto the mem* functions with macros. 
 */

#ifdef PL_MEM_IMPLEMENT

/*
 * PL_memcmp
 *
 * Like PL_strcmp, but uses count instead of null termination
 */

PR_EXTERN(int)
PL_memcmp(const void *s1, const void *s2, size_t count);

/*
 * PL_memcpy
 *
 * Like PL_strcpy, but uses count instead of null termination
 * NOTE: Doesn't handle overlapping dest and src in all cases
 *       Use PL_memcpy if this is required
 */

PR_EXTERN(void *)
PL_memcpy(void *dest, const void *src, size_t count);

/*
 * PL_memmove
 *
 * Copies count bytes from src to dest. If the regions overlap,
 * it is handled correctly.
 */

PR_EXTERN(void *)
PL_memmove(void *dest, const void *src, size_t count);

/*
 * PL_memset
 *
 * Sets count bytes of memory to value c starting at dest
 */

PR_EXTERN(void *)
PL_memset(void *dest, int c, size_t count);

#else /* !PL_MEM_IMPLEMENT */

#include <string.h>

#define PL_memcmp(s1, s2, count)     memcmp(s1, s2, count)
#define PL_memcpy(dest, src, count)  memcpy(dest, src, count)
#define PL_memmove(dest, src, count) memmove(dest, src, count)
#define PL_memset(dest, c, count)    memset(dest, c, count)

#endif /* !PL_MEM_IMPLEMENT */

/*
 * PL_bcmp
 *
 * Compares two regions of bytes at b1 and b2 of size count.
 * Returns 0 if they are the same, non-zero if they are not.
 * (uses PL_memcmp).
 */

PR_EXTERN(int)
PL_bcmp(const void *b1, const void *b2, size_t count);

/*
 * PL_bcopy
 *
 * Copies count bytes from src to dest. If the regions overlap,
 * they may not be copied correctly.
 * (uses PL_memcpy)
 */

PR_EXTERN(void)
PL_bcopy(void *dest, void *src, size_t count);

/*
 * PL_bzero
 *
 * Zeros count bytes of memory starting at dest.
 * (uses PL_memset)
 */

PR_EXTERN(void)
PL_bzero(void *dest, size_t count);

PR_END_EXTERN_C

#endif /* plstr2_h__ */

