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
**  This tool exists so that a host computer can push a file to the
**      connected CE device.  Useful for scripting if the files are
**      small enough.
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
    bool    mUpdate;
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
        _tprintf(_T("Usage: %s [OPTIONS] SOURCE DESTINATION\n"), inArg0);
        _tprintf(_T("   -h          This help text.\n"));
        _tprintf(_T("   -u          Update, push only if size differs or SOURCE newer.\n"));
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
HRESULT myWork(OptionState& inOS, LPCTSTR inSource, LPCTSTR inTarget)
{
    HRESULT retval = NOERROR;

    /*
    **  Open source file.
    */
    HANDLE source = CreateFile(inSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if(INVALID_HANDLE_VALUE != source)
    {
        bool doCopy = true;

        if(false != inOS.mUpdate)
        {
            /*
            **  Try to open target in read only mode.
            **  We need to check the dates and size.
            */
            HANDLE rTarget = CeCreateFile(inTarget, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            ABSORB_CE_ERROR(INVALID_HANDLE_VALUE == rTarget);

            if(INVALID_HANDLE_VALUE != rTarget)
            {
                DWORD size;
                DWORD rSize;
                
                /*
                **  Check size.
                */
                size = GetFileSize(source, NULL);
                rSize = CeGetFileSize(rTarget, NULL);
                ABSORB_CE_ERROR((DWORD)-1 == rSize);
                
                if((DWORD)-1 != size && size == rSize)
                {
                    FILETIME lastWrite;
                    FILETIME rLastWrite;
                    
                    BOOL getTime = GetFileTime(source, NULL, NULL, &lastWrite);
                    BOOL rGetTime = CeGetFileTime(rTarget, NULL, NULL, &rLastWrite);
                    ABSORB_CE_ERROR(FALSE == rGetTime);
                    
                    if(FALSE != getTime && FALSE != rGetTime)
                    {
                        if(0 >= CompareFileTime(&lastWrite, &rLastWrite))
                        {
                            /*
                            **  Skip copying this file since the remote file
                            **      is comparably the same.
                            */
                            doCopy = false;
                        }
                    }
                }

                CeCloseHandle(rTarget);
            }
        }

        if(false != doCopy)
        {
            /*
            **  Open target file.
            */
            HANDLE target = CeCreateFile(inTarget, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            ABSORB_CE_ERROR(INVALID_HANDLE_VALUE == target);

            if(INVALID_HANDLE_VALUE != target)
            {
                BOOL readRes = FALSE;
                BOOL writeRes = FALSE;
                DWORD bytes = 0;
                DWORD wroteBytes = 0;
                BYTE buffer[0x4000];
                
                /*
                **  Write all we've got.
                */
                do
                {
                    bytes = 0;
                    wroteBytes = 0;
                    
                    readRes = ReadFile(source, buffer, sizeof(buffer), &bytes, NULL);
                    if(FALSE != readRes && 0 != bytes)
                    {
                        writeRes = CeWriteFile(target, buffer, bytes, &wroteBytes, NULL);
                        ABSORB_CE_ERROR(FALSE == writeRes);
                    }
                }
                while(readRes && writeRes && 0 != bytes);
                
                if(!(readRes && writeRes))
                {
                    _ftprintf(stderr, _T("Unable to finish copying!\n"));
                    retval = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                {
                    FILETIME time;

                    /*
                    **  Set the file times to be the same as source file.
                    */
                    BOOL getTimes = GetFileTime(source, NULL, NULL, &time);
                    if(FALSE != getTimes)
                    {
                        BOOL setTimes = CeSetFileTime(target, NULL, NULL, &time);
                        ABSORB_CE_ERROR(FALSE == setTimes);

                        if(FALSE == setTimes)
                        {
                            _ftprintf(stderr, _T("Unable to set file times for %s\n"), inTarget);
                            retval = HRESULT_FROM_WIN32(GetLastError());
                        }
                    }
                    else
                    {
                        _ftprintf(stderr, _T("Unable to get file times for %s\n"), inSource);
                        retval = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                
                CeCloseHandle(target);
            }
            else
            {
                _ftprintf(stderr, _T("Unable to open destination file %s\n"), inTarget);
                retval = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        CloseHandle(source);
    }
    else
    {
        _ftprintf(stderr, _T("Unable to open source file %s\n"), inSource);
        retval = HRESULT_FROM_WIN32(GetLastError());
    }

    /*
    ** TODO
    */

    return retval;
}


/*
**  charswap
**
**  Replace one char with another.
**  Likely broken for full MBCS case, but do not care.
*/
void charswap(LPTSTR inString, TCHAR inFrom, TCHAR inTo)
{
    while(NULL != (inString = _tcschr(inString, inFrom)))
    {
        _tcsncpy(inString, &inTo, 1);
        inString = _tcsinc(inString);
    }
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
                    else if(0 == _tcsncmp(_T("u"), option, 1))
                    {
                        isOption = true;

                        os.mUpdate = true;
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
        **  We should have exactly two arguments left.
        **  This left inside of loop in case I change my mind to support
        **      more args.
        */
        if(SUCCEEDED(retval) && ((inArgc - 2) == loop))
        {
            LPTSTR dupSource = NULL;
            
            
            dupSource = _tcsdup(inArgv[loop]);
            loop++;
            if(NULL != dupSource)
            {
                LPTSTR dupDestination = NULL;

                dupDestination = _tcsdup(inArgv[loop]);
                if(NULL != dupDestination)
                {
                    charswap(dupSource, _T('/'), _T('\\'));
                    charswap(dupDestination, _T('/'), _T('\\'));
                    
                    retval = myWork(os, dupSource, dupDestination);
                    
                    if(FAILED(retval))
                    {
                        _ftprintf(stderr, _T("Problem performing copy.\n"));
                    }

                    free(dupDestination);
                }
                else
                {
                    _ftprintf(stderr, _T("Unable to allocated duplicate string.\n"));
                    retval = E_OUTOFMEMORY;
                }
                
                free(dupSource);
            }
            else
            {
                _ftprintf(stderr, _T("Unable to allocated duplicate string.\n"));
                retval = E_OUTOFMEMORY;
            }
        }
        else
        {
            _ftprintf(stderr, _T("Wrong number of arguments.\n"));
            retval = E_INVALIDARG;
        }
        
        /*
        **  Break loop.
        */
        break;
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