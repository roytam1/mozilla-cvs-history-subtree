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
 *  Garrett Arch Blythe 02/05/2002
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

#ifndef nspr_win32_unicode_h___
#define nspr_win32_unicode_h___

#include <windows.h>

/*
 * _PR_MD_MALLOC_A2W
 *
 * Automatically PR_Malloc a wide char string and return it based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * Caller must PR_Free the return value if non-NULL.
 */
LPWSTR _PR_MD_MALLOC_A2W(LPCSTR inString);

/*
 * _PR_MD_A2W
 *
 * Non-mallocing/faster version to return a wide char string based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPWSTR _PR_MD_A2W(LPCSTR inString, LPWSTR outWideString, int inWideStringChars);

/*
 * _PR_MD_W2A
 *
 * Non-mallocing fucntion to return a ANSI (multi byte, ansi code page)
 *  string based on the wide char string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPSTR _PR_MD_W2A(LPCWSTR inWideString, LPSTR outString, int inStringChars);

#if defined(WINCE)
/*
 * WinCE Only
 *
 * Some of the functions we intend to proxy to get around multi-byte API
 *  assumptions are imported from DLLs.  I would rather not fake out
 *  the linkage by prepending _imp_ ## functioname to make things happen.
 * We need to provide our own linkage symbol for all this to work out.
 */

HANDLE
WINAPI
_MD_CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

BOOL
WINAPI
_MD_DeleteFileA(
    LPCSTR lpFileName
    );

BOOL
WINAPI
_MD_MoveFileA(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    );

DWORD
WINAPI
_MD_GetFileAttributesA(
    LPCSTR lpFileName
    );

BOOL
WINAPI
_MD_CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

BOOL
WINAPI
_MD_RemoveDirectoryA(
    LPCSTR lpPathName
    );

HANDLE
WINAPI
_MD_CreateSemaphoreA(
    IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCSTR lpName
    );

HANDLE
WINAPI
_MD_CreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );

#endif /* WINCE */

#endif /* nspr_win32_unicode_h___ */
