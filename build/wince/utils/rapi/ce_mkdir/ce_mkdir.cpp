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

#include <windows.h>
#include <rapi.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

/*
**  This tool exists so that the host computer can script the creation
**      of a directory on the connected CE device.
*/


/*
**  ABSORB_CE_ERROR
**
**  Represent any remote CE error as a local error for simpler code paths.
*/
#define ABSORB_CE_ERROR(condition) \
    do \
    { \
        if(condition) \
        { \
            if(FAILED(CeRapiGetError())) \
            { \
                SetLastError(RPC_S_COMM_FAILURE); \
            } \
            else \
            { \
                SetLastError(CeGetLastError()); \
            } \
        } \
    } \
    while(0)


typedef struct __struct_OptionState
{
    bool    mFinishedOptions;
    bool    mCreateParents;
    bool    mIgnoreExisting;
}
OptionState;


/*
**  ce_tool_version
**
**  Lay claim, inform.
*/
HRESULT ce_tool_version(LPCTSTR inArg0)
{
    HRESULT retval = NOERROR;

    if(NULL != inArg0 && 0 != _tcslen(inArg0))
    {
        _tprintf(_T("%s\n"), inArg0);
        _tprintf(_T("Written by:     Garrett Arch Blythe\n"));
        _tprintf(_T("Last compiled:  %s\n"), _T(__TIMESTAMP__));
    }
    else
    {
        _ftprintf(stderr, _T("No progam name, invalid environment!\n"));
        retval = HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT);
    }

    return retval;
}


/*
**  ce_tool_help
**
**  Output whatever help we can.
*/
HRESULT ce_tool_help(LPCTSTR inArg0)
{
    HRESULT retval = NOERROR;

    if(NULL != inArg0 && 0 != _tcslen(inArg0))
    {
        _tprintf(_T("Usage: %s [OPTIONS] DIRECTORY...\n"), inArg0);
        _tprintf(_T("   -h          This help text.\n"));
        _tprintf(_T("   -p          Make parent directories as needed.\n"));
        _tprintf(_T("               No error if directories exist.\n"));
        _tprintf(_T("   -v          Print version information.\n"));
    }
    else
    {
        _ftprintf(stderr, _T("No progam name, invalid environment!\n"));
        retval = HRESULT_FROM_WIN32(ERROR_BAD_ENVIRONMENT);
    }

    return retval;
}


/*
**  myWork
**
**  Atomic step of what we are trying to do.
*/
HRESULT myWork(OptionState& inOS, LPCTSTR inDirectory)
{
    HRESULT retval = NOERROR;

    BOOL createRes = CeCreateDirectory(inDirectory, NULL);
    ABSORB_CE_ERROR(FALSE == createRes);
    if(FALSE == createRes)
    {
        DWORD ceErr = GetLastError();
        
        switch(ceErr)
        {
        case ERROR_ALREADY_EXISTS:
            if(false != inOS.mIgnoreExisting)
            {
                break;
            }
        default:
            retval = HRESULT_FROM_WIN32(ceErr);
            break;
        }
    }

    return retval;
}


/*
**  ce_tool
**
**  Perform actual actions need to fulfill tool requirements.
*/
HRESULT ce_tool(int inArgc, LPTSTR* inArgv)
{
    HRESULT retval = NOERROR;

    OptionState os;
    memset(&os, 0, sizeof(os));

    LPTSTR freeMe = NULL;
    LPTSTR slash = NULL;
    int loop = 0;
    for(loop = 1; loop < inArgc && SUCCEEDED(retval); loop++)
    {
        if(false == os.mFinishedOptions)
        {
            bool isOption = false;

            if(0 == _tcsnccmp(_T("-"), inArgv[loop], 1))
            {
                LPTSTR option = NULL;
                for(option = _tcsinc(inArgv[loop]); 0 != _tcsncmp(option, _T(""), 1); option = _tcsinc(option))
                {
                    if(0 == _tcsncmp(_T("h"), option, 1))
                    {
                        retval = ce_tool_help(inArgv[0]);
                        return retval; // early return
                    }
                    else if(0 == _tcsncmp(_T("p"), option, 1))
                    {
                        isOption = true;

                        /*
                        **  Modify our option state.
                        */
                        os.mCreateParents = true;
                        os.mIgnoreExisting = true;
                    }
                    else if(0 == _tcsncmp(_T("v"), option, 1))
                    {
                        retval = ce_tool_version(inArgv[0]);
                        return retval; // early return
                    }
                    else
                    {
                        /*
                        **  If we aren't processing an option, kick out on an
                        **      unknown.
                        **  Otherwise, inform of ignore.
                        */
                        if(false == isOption)
                        {
                            break;
                        }
                        else
                        {
                            _ftprintf(stderr, _T("Ignoring unknown option '%c'.\n"), *option);
                        }
                    }
                }
            }

            if(false == isOption)
            {
                os.mFinishedOptions = true;
            }
            else
            {
                /*
                **  Found an option, just continue.
                */
                continue;
            }
        }

        /*
        ** Dup for conversion.
        */
        freeMe = _tcsdup(inArgv[loop]);
        if(NULL != freeMe)
        {
            /*
            **  Are we creating parents?
            */
            if(false != os.mCreateParents)
            {
                /*
                **  Convert backslashes to forward slashes.
                */
                slash = freeMe;
                while(NULL != (slash = _tcschr(slash, _T('\\'))))
                {
                    _tcsncpy(slash, _T("/"), 1);
                    slash = _tcsinc(slash);
                }

                /*
                **  Loop over each slash, attempting to create the directory.
                */
                slash = freeMe;
                while(NULL != (slash = _tcschr(slash, _T('/'))))
                {
                    if(slash != freeMe)
                    {
                        /*
                        **  Remove the slash.
                        **  We will replace it with a backslash when done.
                        */
                        _tcsncpy(slash, _T(""), 1);
                        
                        retval = myWork(os, freeMe);
                    }

                    _tcsncpy(slash, _T("\\"), 1);
                    slash = _tcsinc(slash);
                }
            }
            else
            {
                /*
                **  Convert forward slashes to backslashes.
                */
                slash = freeMe;
                while(NULL != (slash = _tcschr(slash, _T('/'))))
                {
                    _tcsncpy(slash, _T("\\"), 1);
                    slash = _tcsinc(slash);
                }
            }

            retval = myWork(os, freeMe);

            if(FAILED(retval))
            {
                _ftprintf(stderr, _T("Problem creating directory %s\n"), freeMe);
            }

            free(freeMe);
        }
        else
        {
            _ftprintf(stderr, _T("Unable to allocated duplicate string.\n"));
            retval = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return retval;
}


/*
**  main
**
**  Init environment then pass on for real execution.
**  Control return to OS.
*/
int 
#if defined(_UNICODE)
wmain
#else
main
#endif
(int inArgc, LPTSTR* inArgv, LPTSTR* inEnv)
{
    HRESULT retval = 0;

    HRESULT ceInit = CeRapiInit();
    if(SUCCEEDED(ceInit))
    {
        if(1 >= inArgc)
        {
            retval = ce_tool_help(inArgv[0]);
        }
        else
        {
            retval = ce_tool(inArgc, inArgv);
        }

        HRESULT ceUninit = CeRapiUninit();
        if(FAILED(ceUninit))
        {
            _ftprintf(stderr, _T("Failed uninitialize CE device via ActiveSync.\n"));
            retval = ceUninit;
        }
    }
    else
    {
        _ftprintf(stderr, _T("Failed initialize CE device via ActiveSync.\n"));
        retval = ceInit;
    }

    return (int)retval;
}