/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape Portable Runtime (NSPR).
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *  Garrett Arch Blythe 01/15/2002
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifndef wince_libc___
#define wince_libc___

/*
 * This file contains various #defines, typedefs and functions that are
 * mising from the WinCE libc implementation.
 *
 * They are provided here for compatability with other systems.
 */

#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(_NSPR_BUILD_)
#define _NSPRIMP __declspec(dllexport)
#else
#define _NSPRIMP __declspec(dllimport)
#endif

/* The following definitions are generally expected in <stddef.h> */

typedef int ptrdiff_t; /* pointer difference */


/* The following definitions are generally expected in <errno.h> */

#define ERANGE 34

#define errno (*PR_GetOSErrorAddress())


/* The following definitions are generally expected in <time.h> */

#ifndef _TM_DEFINED
#define _TM_DEFINED
/*
 * struct tm
 *
 * And related windows specific functions to mimic LIBC's tm funcs.
 */
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

/* WinCE defines time_t in <stdlib.h> */

/*
 * subset of the function prototypes provided by <time.h>
 */

_NSPRIMP struct tm* gmtime(const time_t* inTimeT);
_NSPRIMP struct tm* localtime(const time_t* inTimeT);
_NSPRIMP time_t mktime(struct tm* inTM);
#endif
_NSPRIMP size_t strftime(char *strDest, size_t maxsize, const char *format,
                         const struct tm *timeptr);


#ifdef  __cplusplus
}
#endif

#endif /* wince_libc___ */
