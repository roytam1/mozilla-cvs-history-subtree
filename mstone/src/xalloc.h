/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Netscape Mailstone utility,
 * released March 17, 2000.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1999-2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):     Sean O'Rourke <sean@sendmail.com>
 *                     Sendmail, Inc.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License Version 2 or later (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of
 * those above.  If you wish to allow use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the NPL or the GPL.
 */

#ifndef _xalloc_h
#define _xalloc_h

#include <stdlib.h>

/* xalloc.h -- consolidate memory allocation */

#define XALLOC(T) (T*)xalloc(sizeof (T))
#define XCALLOC(T) (T*)xcalloc(sizeof (T))

#ifdef USE_SYSTEM_MALLOC
#define xalloc(X) malloc(X)
#define xcalloc(X) calloc(1, X)
#define xrealloc(A, B) realloc(A, B)
#define xstrdup(X) strdup(X)
#define xfree(X) free(X)
#else /* USE_SYSTEM_MALLOC */
void *xalloc(size_t size);
void *xcalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *cp);
void xfree(void *ptr);
#endif /* USE_SYSTEM_MALLOC */

#endif /* _xalloc_h */
