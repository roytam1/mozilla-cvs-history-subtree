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
 *                     Thom O'Connor <thom@sendmail.com>
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

#include <assert.h>
#include <string.h>
#include "xalloc.h"

#ifndef USE_SYSTEM_MALLOC

/* allocate memory and die if out of memory */
void *
xalloc(size_t size)
{
	void *ptr;
#ifdef _DEBUG_xalloc
	size += 3 * sizeof(int);
	size += size % sizeof(int);
#endif
	ptr = malloc(size);	
	assert(ptr != NULL);
#ifdef _DEBUG_xalloc
	{
		char *p = (char*)ptr;
		*(int*)p = size;
		*((int*)p + 1) = 0xf00dcafe;
		*((int*)(p + size - sizeof(int))) = 0xdeadbeef;
		ptr = (p + 2 * sizeof(int));
	}
#endif
	return(ptr);
}

void
xfree(void * ptr)
{
#ifdef _DEBUG_xalloc
	char *p = (char*)ptr;
	int size = *(int*)(p - 2 * sizeof(int));
	int magic = *(int*)(p - sizeof(int));
	assert(magic == 0xf00dcafe);
	magic = *(int*)(p + size - 3 * sizeof(int));
	assert(magic == 0xdeadbeef);
	memset(p - 2 * sizeof(int), 0, size);
	ptr = p - 2 * sizeof(int);
#endif
	free(ptr);
}

void *
xcalloc(size_t size)
{
	void *ret = xalloc(size);
	memset(ret, 0, size);
	return ret;
}

void *
xrealloc(void *ptr, size_t size)
{
#ifdef _DEBUG_xalloc
	if (ptr == NULL) {
		return xalloc(size);
	} else if (size == 0) {
		xfree(ptr);
		return NULL;
	} else {
		char *p = (char*)ptr;
		void *ret = xalloc(size);
		/* xalloc has already checked the magic */
		int oldsize = *(int*)(p - 2 * sizeof(int));
		oldsize -= 3 *sizeof(int);
		memcpy(ret, ptr, (oldsize>size)?size:oldsize);
		xfree(ptr);
		return ret;
	}
#else /* _DEBUG */
	ptr = realloc(ptr, size);
	assert(ptr != NULL);
	return ptr;
#endif /* _DEBUG */
}

char *
xstrdup(const char *cp)
{
	char *retcp;
	int len = strlen(cp);

	retcp = (char *)xalloc(len+1);
	strcpy(retcp, cp);
	return retcp;
}

#endif /* USE_SYSTEM_MALLOC */
