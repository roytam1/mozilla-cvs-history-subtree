/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Original Code is mozilla.org code, released
 * Jan 28, 2003.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2003 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 28-January-2003
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

#include "moz_shunt_ppc2002.h"

extern "C" {
#if 0
}
#endif

/*
**  Help figure the character count of a TCHAR array.
*/
#define wcharcount(array) (sizeof(array) / sizeof(TCHAR))


MOZ_SHUNT_PPC2002_API UINT GetWindowsDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        wcscpy(inBuffer, _T("\\WINDOWS"));
        retval = 8;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT GetSystemDirectoryW(LPWSTR inBuffer, UINT inSize)
{
    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        wcscpy(inBuffer, _T("\\WINDOWS"));
        retval = 8;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HANDLE OpenSemaphoreW(DWORD inDesiredAccess, BOOL inInheritHandle, LPCWSTR inName)
{
    HANDLE retval = NULL;
    HANDLE semaphore = NULL;

    semaphore = CreateSemaphoreW(NULL, 0, 0x7fffffff, inName);
    if(NULL != semaphore)
    {
        DWORD lastErr = GetLastError();
        
        if(ERROR_ALREADY_EXISTS != lastErr)
        {
            CloseHandle(semaphore);
        }
        else
        {
            retval = semaphore;
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetGlyphOutlineW(HDC inDC, WCHAR inChar, UINT inFormat, LPGLYPHMETRICS inGM, DWORD inBufferSize, LPVOID outBuffer, CONST LPMAT2 inMAT2)
{
    DWORD retval = GDI_ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetCurrentDirectoryW(DWORD inBufferLength, LPTSTR outBuffer)
{
    DWORD retval = 0;

    if(NULL != outBuffer && 0 < inBufferLength)
    {
        outBuffer[0] = _T('\0');
    }

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL SetCurrentDirectoryW(LPCTSTR inPathName)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


#if 0
{
#endif
} /* extern "C" */
