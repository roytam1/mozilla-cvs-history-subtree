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
 *    Garrett Arch Blythe, 03/18/2002
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

/*
**  apiA2W.c
**
**  This file exists to provide a conversion layer from ansi code page
**      multibyte Windows functions to their unicode wide character
**      equivalents.
**
**  Why?  Simple, really.  WinCE is a unicode only operating system, as
**      is mostly WinNT.  On WinNT there is an automatic conversion layer from
**      each windows api that converts to the perferred underlying format,
**      however on WinCE we have had to roll our own.
**  Just FYI, Win9x is mostly vice versa with regards to format (i.e. is
**      ansi code page multibyte software).
**  As for trying to explain just why there is no inherent A2W layer for
**      WinCE, I can not other than this does cost stack/heap space and time.
**
**  But shouldn't we just back up and make the product support wide
**      characters everywhere, and it will just work?  IMO, no, because
**      such a project might never finish due to red tape and insufficient
**      knowledge of the entire product being ported by me.  IMO, you
**      could only possibly succeed if across the board wide character
**      support was a goal from the beginning.  Enough of MO.
**  This is not to say that full support of unicode can't be done where
**      appropriate.
**
**  WARNING:
**      Only use this for WinCE.
**      The function declarations may or may not be out of date, so there
**          may be some inconsistencies.
**      Good luck....
*/


#include <windows.h>

#include "a2ww2a.h"


/*
**  Help figure the character count of a WCHAR array.
*/
#define wcharcount(array) (sizeof(array) / sizeof(WCHAR))



A2WW2A_API DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
    WCHAR wideStr[MAX_PATH];

    return w2a_buffer(
        wideStr,
        GetModuleFileNameW(hModule, wideStr, wcharcount(wideStr)),
        lpFilename,
        nSize
        );
}


A2WW2A_API BOOL CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL retval = FALSE;
    WCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpPathName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = CreateDirectoryW(wideStr, lpSecurityAttributes);
    }

    return retval;
}


A2WW2A_API BOOL MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
    BOOL retval = FALSE;
    WCHAR wideStr[2][MAX_PATH];

    if(
        a2w_buffer(lpExistingFileName, -1, wideStr[0], wcharcount(wideStr[0])) &&
        a2w_buffer(lpNewFileName, -1, wideStr[1], wcharcount(wideStr[1]))
        )
    {
        retval = MoveFileW(wideStr[0], wideStr[1]);
    }

    return retval;
}


A2WW2A_API BOOL CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists)
{
    BOOL retval = FALSE;
    WCHAR wideStr[2][MAX_PATH];

    if(
        a2w_buffer(lpExistingFileName, -1, wideStr[0], wcharcount(wideStr[0])) &&
        a2w_buffer(lpNewFileName, -1, wideStr[1], wcharcount(wideStr[1]))
        )
    {
        retval = CopyFileW(wideStr[0], wideStr[1], bFailIfExists);
    }

    return retval;
}


A2WW2A_API HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE retval = INVALID_HANDLE_VALUE;
    WCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpFileName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = CreateFileW(wideStr, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    return retval;
}


A2WW2A_API DWORD GetFileAttributesA(LPCSTR lpFileName)
{
    DWORD retval = (DWORD)-1;
    WCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpFileName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = GetFileAttributesW(wideStr);
    }

    return retval;
}


A2WW2A_API BOOL CreateProcessA_CE(LPCSTR pszImageName, LPCSTR pszCmdLine, LPSECURITY_ATTRIBUTES psaProcess, LPSECURITY_ATTRIBUTES psaThread, BOOL fInheritHandles, DWORD fdwCreate, LPVOID pvEnvironment, LPSTR pszCurDir, LPSTARTUPINFO psiStartInfo, LPPROCESS_INFORMATION pProcInfo)
{
    BOOL retval = FALSE;
    WCHAR pszImageNameW[MAX_PATH];

    if(a2w_buffer(pszImageName, -1, pszImageNameW, wcharcount(pszImageNameW)))
    {
        LPWSTR pszCmdLineW = NULL;

        pszCmdLineW = a2w_malloc(pszCmdLine, -1, NULL);
        if(NULL != pszCmdLineW || NULL == pszCmdLine)
        {
            retval = CreateProcessW(pszImageNameW, pszCmdLineW, NULL, NULL, FALSE, fdwCreate, NULL, NULL, NULL, pProcInfo);

            if(NULL != pszCmdLineW)
            {
                free(pszCmdLineW);
            }
        }
    }

    return retval;
}

A2WW2A_API int GetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
    int retval = 0;
    int neededChars = 0;

    neededChars = GetLocaleInfoW(Locale, LCType, NULL, 0);
    if(0 != neededChars)
    {
        LPWSTR buffer = NULL;

        buffer = (LPWSTR)malloc(neededChars * sizeof(WCHAR));
        if(NULL != buffer)
        {
            int gotChars = 0;

            gotChars = GetLocaleInfoW(Locale, LCType, buffer, neededChars);
            if(0 != gotChars)
            {
                if(0 == cchData)
                {
                    retval = WideCharToMultiByte(
                        CP_ACP,
                        WC_COMPOSITECHECK,
                        buffer,
                        neededChars,
                        NULL,
                        0,
                        NULL,
                        NULL
                        );

                }
                else
                {
                    retval = w2a_buffer(buffer, neededChars, lpLCData, cchData);
                }
            }

            free(buffer);
        }
    }

    return retval;
}

