/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2002 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 04/10/2002
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */
#if !defined(__shunt_h__)
#define __shunt_h__

/*
**  shunt.h
**
**  This file exists mainly to divert/shunt commonly expected APIs to
**      their equivalents on the appropriate platform.
**  The specific instance the file was first created as to redefine
**      strdup as _strdup for WINCE as there is no OLDNAMES.LIB which
**      would have done the job.
*/

/*
**  Windows only.
*/
#if defined(_WIN32)

#if defined(__cplusplus)
extern "C" {
#endif
#if 0
}
#endif


/*
**  WINCE only section.
*/
#if defined(WINCE)

#define stricmp             _stricmp
#define strcmpi             _stricmp
#define strdup              _strdup

#endif /* WINCE */


#if 0
{
#endif
#if defined(__cplusplus)
}   /* extern "C" */
#endif

#endif /* _WIN32 */
#endif /* __shunt_h__ */
