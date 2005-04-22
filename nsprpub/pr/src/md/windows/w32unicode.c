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
 *  Garrett Arch Blythe, 02/05/2002
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

/*
 * w32unicode.c
 *
 * This file exists mainly to provide easy ways to convert internal
 *  multibyte string representations into their wide character
 *  counterparts.
 *
 * FYI:
 *
 * WinNT functions as a UNICODE Win32 API with automatic conversions
 *  functioning in the funcA (using funcW would be faster for NT).
 * Win95 functions as a multibyte Win32 API with automatic conversions
 *  functioning in the funcW (using funcA would be faster for win9x).
 */

#include "primpl.h"

/*
 * _PR_MD_MALLOC_A2W
 *
 * Automatically PR_Malloc a wide char string and return it based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * Caller must PR_Free the return value if non-NULL.
 */
LPWSTR _PR_MD_MALLOC_A2W(LPCSTR inString)
{
    LPWSTR retval = NULL;

    if(NULL != inString)
    {
        int neededWChars = 0;

        neededWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, NULL, 0);
        if(0 < neededWChars)
        {
            LPWSTR wstr = NULL;

            wstr = (LPWSTR)PR_Malloc(sizeof(WCHAR) * neededWChars);
            if(NULL != wstr)
            {
                int convertRes = 0;

                convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, wstr, neededWChars);
                if(0 == convertRes)
                {
                    PR_Free(wstr);
                }
                else
                {
                    retval = wstr;
                }
            }
        }
    }

    return retval;
}

/*
 * _PR_MD_A2W
 *
 * Non-mallocing version to return a wide char string based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPWSTR _PR_MD_A2W(LPCSTR inString, LPWSTR outWideString, int inWideStringChars)
{
    LPWSTR retval = outWideString;

    if(NULL != outWideString)
    {
        int convertRes = 0;

        convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, outWideString, inWideStringChars);
        if(0 == convertRes)
        {
            retval = NULL;
        }
    }

    return retval;
}

/*
 * _PR_MD_W2A
 *
 * Non-mallocing fucntion to return a ANSI (multi byte, ansi code page)
 *  string based on the wide char string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPSTR _PR_MD_W2A(LPCWSTR inWideString, LPSTR outString, int inStringChars)
{
    LPSTR retval = outString;

    if(NULL != outString)
    {
        int convertRes = 0;

        convertRes = WideCharToMultiByte(
            CP_ACP,
            WC_COMPOSITECHECK,
            inWideString,
            -1,
            outString,
            inStringChars,
            NULL,
            NULL
            );
        if(0 == convertRes)
        {
            retval = NULL;
        }
    }

    return retval;
}

#if defined(WINCE)


#endif /* WINCE */
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
 *  Garrett Arch Blythe, 02/05/2002
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

/*
 * w32unicode.c
 *
 * This file exists mainly to provide easy ways to convert internal
 *  multibyte string representations into their wide character
 *  counterparts.
 *
 * FYI:
 *
 * WinNT functions as a UNICODE Win32 API with automatic conversions
 *  functioning in the funcA (using funcW would be faster for NT).
 * Win95 functions as a multibyte Win32 API with automatic conversions
 *  functioning in the funcW (using funcA would be faster for win9x).
 */

#include "primpl.h"

/*
 * _PR_MD_MALLOC_A2W
 *
 * Automatically PR_Malloc a wide char string and return it based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * Caller must PR_Free the return value if non-NULL.
 */
LPWSTR _PR_MD_MALLOC_A2W(LPCSTR inString)
{
    LPWSTR retval = NULL;

    if(NULL != inString)
    {
        int neededWChars = 0;

        neededWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, NULL, 0);
        if(0 < neededWChars)
        {
            LPWSTR wstr = NULL;

            wstr = (LPWSTR)PR_Malloc(sizeof(WCHAR) * neededWChars);
            if(NULL != wstr)
            {
                int convertRes = 0;

                convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, wstr, neededWChars);
                if(0 == convertRes)
                {
                    PR_Free(wstr);
                }
                else
                {
                    retval = wstr;
                }
            }
        }
    }

    return retval;
}

/*
 * _PR_MD_A2W
 *
 * Non-mallocing version to return a wide char string based on the
 *  ANSI (multi byte, ansi code page) string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPWSTR _PR_MD_A2W(LPCSTR inString, LPWSTR outWideString, int inWideStringChars)
{
    LPWSTR retval = outWideString;

    if(NULL != outWideString)
    {
        int convertRes = 0;

        convertRes = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, inString, -1, outWideString, inWideStringChars);
        if(0 == convertRes)
        {
            retval = NULL;
        }
    }

    return retval;
}

/*
 * _PR_MD_W2A
 *
 * Non-mallocing fucntion to return a ANSI (multi byte, ansi code page)
 *  string based on the wide char string passed in.
 *
 * NOTE:  inWideStringChars is number of wide characters in outWideString,
 *          NOT the number of bytes....
 */
LPSTR _PR_MD_W2A(LPCWSTR inWideString, LPSTR outString, int inStringChars)
{
    LPSTR retval = outString;

    if(NULL != outString)
    {
        int convertRes = 0;

        convertRes = WideCharToMultiByte(
            CP_ACP,
            WC_COMPOSITECHECK,
            inWideString,
            -1,
            outString,
            inStringChars,
            NULL,
            NULL
            );
        if(0 == convertRes)
        {
            retval = NULL;
        }
    }

    return retval;
}

#if defined(WINCE)


#endif /* WINCE */
