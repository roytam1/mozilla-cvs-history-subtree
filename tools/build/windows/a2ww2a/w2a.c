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
**  w2a.c
**
**  This file exists to provide some simple APIs to convert
**      from unicode wide characters to ansi code page multibyte
**      characters using the appropriate win32 APIs.
*/

#include <windows.h>

#include <string.h>
#include <stdlib.h>

#include "a2ww2a.h"

A2WW2A_API int w2a_buffer(LPCWSTR inWideString, int inWideChars, LPSTR outACPString, int inACPChars)
{
    int retval = 0;

    /*
    **  Start off by terminating the out argument if appropriate.
    */
    if(NULL != outACPString && 0 != inACPChars)
    {
        *outACPString = '\0';
    }

    /*
    **  Sanity check arguments.
    */
    if(NULL != inWideString && 0 != inWideChars && (0 == inACPChars || NULL != outACPString))
    {
        /*
        **  Attempt the conversion.
        */
        retval = WideCharToMultiByte(
            CP_ACP,
            WC_COMPOSITECHECK,
            inWideString,
            inWideChars,
            outACPString,
            inACPChars,
            NULL,
            NULL
            );
    }

    return retval;
}


A2WW2A_API LPSTR w2a_malloc(LPCWSTR inWideString, int inWideChars, int* outACPChars)
{
    LPSTR retval = NULL;

    /*
    **  Initialize any out arguments.
    */
    if(NULL != outACPChars)
    {
        *outACPChars = 0;
    }

    /*
    **  Initialize the wide char length if requested.
    **  We do this here to avoid doing it twice in calls to w2a_buffer.
    */
    if(-1 == inWideChars)
    {
        if(NULL != inWideString)
        {
            /*
            **  Plus one so the terminating character is included.
            */
            inWideChars = (int)wcslen(inWideString) + 1;
        }
        else
        {
            inWideChars = 0;
        }
    }

    /*
    **  Sanity check arguments.
    */
    if(NULL != inWideString && 0 != inWideChars)
    {
        int charsRequired = 0;

        /*
        **  Determine the size of buffer required for the conversion.
        */
        charsRequired = w2a_buffer(inWideString, inWideChars, NULL, 0);
        if(0 != charsRequired)
        {
            LPSTR heapBuffer = NULL;

            heapBuffer = (LPSTR)malloc((size_t)charsRequired * sizeof(CHAR));
            if(NULL != heapBuffer)
            {
                int acpChars = 0;

                /*
                **  Real thing this time.
                */
                acpChars = w2a_buffer(inWideString, inWideChars, heapBuffer, charsRequired);
                if(0 != acpChars)
                {
                    retval = heapBuffer;
                    if(NULL != outACPChars)
                    {
                        *outACPChars = acpChars;
                    }
                }
                else
                {
                    /*
                    **  Something wrong.
                    **  Clean up.
                    */
                    free(heapBuffer);
                }
            }
        }
    }

    return retval;
}
