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


MOZ_SHUNT_PPC2002_API DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
    TCHAR wideStr[MAX_PATH];

    return w2a_buffer(
        wideStr,
        GetModuleFileNameW(hModule, wideStr, wcharcount(wideStr)),
        lpFilename,
        nSize
        );
}


MOZ_SHUNT_PPC2002_API BOOL CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    BOOL retval = FALSE;
    TCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpPathName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = CreateDirectoryW(wideStr, lpSecurityAttributes);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
    BOOL retval = FALSE;
    TCHAR wideStr[2][MAX_PATH];

    if(
        a2w_buffer(lpExistingFileName, -1, wideStr[0], wcharcount(wideStr[0])) &&
        a2w_buffer(lpNewFileName, -1, wideStr[1], wcharcount(wideStr[1]))
        )
    {
        retval = MoveFileW(wideStr[0], wideStr[1]);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists)
{
    BOOL retval = FALSE;
    TCHAR wideStr[2][MAX_PATH];

    if(
        a2w_buffer(lpExistingFileName, -1, wideStr[0], wcharcount(wideStr[0])) &&
        a2w_buffer(lpNewFileName, -1, wideStr[1], wcharcount(wideStr[1]))
        )
    {
        retval = CopyFileW(wideStr[0], wideStr[1], bFailIfExists);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE retval = INVALID_HANDLE_VALUE;
    TCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpFileName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = CreateFileW(wideStr, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetFileAttributesA(LPCSTR lpFileName)
{
    DWORD retval = (DWORD)-1;
    TCHAR wideStr[MAX_PATH];

    if(a2w_buffer(lpFileName, -1, wideStr, wcharcount(wideStr)))
    {
        retval = GetFileAttributesW(wideStr);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL CreateProcessA_CE(LPCSTR pszImageName, LPCSTR pszCmdLine, LPSECURITY_ATTRIBUTES psaProcess, LPSECURITY_ATTRIBUTES psaThread, BOOL fInheritHandles, DWORD fdwCreate, LPVOID pvEnvironment, LPSTR pszCurDir, LPSTARTUPINFO psiStartInfo, LPPROCESS_INFORMATION pProcInfo)
{
    BOOL retval = FALSE;
    TCHAR pszImageNameW[MAX_PATH];

    if(a2w_buffer(pszImageName, -1, pszImageNameW, wcharcount(pszImageNameW)))
    {
        LPTSTR pszCmdLineW = NULL;

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

MOZ_SHUNT_PPC2002_API int GetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
    int retval = 0;
    int neededChars = 0;

    neededChars = GetLocaleInfoW(Locale, LCType, NULL, 0);
    if(0 != neededChars)
    {
        LPTSTR buffer = NULL;

        buffer = (LPTSTR)malloc(neededChars * sizeof(TCHAR));
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
                        0,
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


MOZ_SHUNT_PPC2002_API UINT GetWindowsDirectoryA(LPSTR inBuffer, UINT inSize)
{
    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        strcpy(inBuffer, "\\WINDOWS");
        retval = 8;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT GetSystemDirectoryA(LPSTR inBuffer, UINT inSize)
{
    UINT retval = 0;

    if(inSize < 9)
    {
        retval = 9;
    }
    else
    {
        strcpy(inBuffer, "\\WINDOWS");
        retval = 8;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API LONG RegOpenKeyExA(HKEY inKey, LPCSTR inSubKey, DWORD inOptions, REGSAM inSAM, PHKEY outResult)
{
    LONG retval = ERROR_GEN_FAILURE;

    LPTSTR wSubKey = a2w_malloc(inSubKey, -1, NULL);
    if(NULL != wSubKey)
    {
        retval = RegOpenKeyEx(inKey, wSubKey, inOptions, inSAM, outResult);
        free(wSubKey);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API LONG RegQueryValueExA(HKEY inKey, LPCSTR inValueName, LPDWORD inReserved, LPDWORD outType, LPBYTE inoutBData, LPDWORD inoutDData)
{
    LONG retval = ERROR_GEN_FAILURE;

    LPTSTR wName = a2w_malloc(inValueName, -1, NULL);
    if(NULL != wName)
    {
        DWORD tempSize = *inoutDData * sizeof(TCHAR); /* in bytes */
        LPTSTR tempData = (LPTSTR)malloc(tempSize);
        if(NULL != tempData)
        {
            retval = RegQueryValueEx(inKey, wName, inReserved, outType, (LPBYTE)tempData, &tempSize);

            /*
            **  Convert to ANSI if a string....
            */
            if(ERROR_SUCCESS == retval && (
                REG_EXPAND_SZ == *outType ||
                REG_MULTI_SZ == *outType ||
                REG_SZ == *outType
                ))
            {
                *inoutDData = (DWORD)WideCharToMultiByte(CP_ACP, 0, tempData, tempSize / sizeof(TCHAR), (LPSTR)inoutBData, *inoutDData, NULL, NULL);
            }
            else
            {
                memcpy(inoutBData, tempData, tempSize);
                *inoutDData = tempSize;
            }

            free(tempData);
        }

        free(wName);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API int _MessageBoxA(HWND inWnd, LPCSTR inText, LPCSTR inCaption, UINT uType)
{
    int retval = 0;
    LPTSTR wCaption = a2w_malloc(inCaption, -1, NULL);

    if(NULL != wCaption)
    {
        LPTSTR wText = a2w_malloc(inText, -1, NULL);

        if(NULL != wText)
        {
            retval = MessageBox(inWnd, wText, wCaption, uType);
            free(wText);
        }
        free(wCaption);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HANDLE OpenSemaphoreA(DWORD inDesiredAccess, BOOL inInheritHandle, LPCSTR inName)
{
    HANDLE retval = NULL;
    LPTSTR wName = a2w_malloc(inName, -1, NULL);

    if(NULL != wName)
    {
        retval = OpenSemaphoreW(inDesiredAccess, inInheritHandle, wName);
        free(wName);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HDC _CreateDCA(LPCSTR inDriver, LPCSTR inDevice, LPCSTR inOutput, CONST DEVMODEA* inInitData)
{
    HDC retval = NULL;

    LPTSTR wDriver = a2w_malloc(inDriver, -1, NULL);
    LPTSTR wDevice = a2w_malloc(inDevice, -1, NULL);
    LPTSTR wOutput = a2w_malloc(inOutput, -1, NULL);

    DEVMODE wInitData;
    memset(&wInitData, 0, sizeof(wInitData));

    wInitData.dmSpecVersion = inInitData->dmSpecVersion;
    wInitData.dmDriverVersion = inInitData->dmDriverVersion;
    wInitData.dmSize = inInitData->dmSize;
    wInitData.dmDriverExtra = inInitData->dmDriverExtra;
    wInitData.dmFields = inInitData->dmFields;
    wInitData.dmOrientation = inInitData->dmOrientation;
    wInitData.dmPaperSize = inInitData->dmPaperSize;
    wInitData.dmPaperLength = inInitData->dmPaperLength;
    wInitData.dmPaperWidth = inInitData->dmPaperWidth;
    wInitData.dmScale = inInitData->dmScale;
    wInitData.dmCopies = inInitData->dmCopies;
    wInitData.dmDefaultSource = inInitData->dmDefaultSource;
    wInitData.dmPrintQuality = inInitData->dmPrintQuality;
    wInitData.dmColor = inInitData->dmColor;
    wInitData.dmDuplex = inInitData->dmDuplex;
    wInitData.dmYResolution = inInitData->dmYResolution;
    wInitData.dmTTOption = inInitData->dmTTOption;
    wInitData.dmCollate = inInitData->dmCollate;
    wInitData.dmLogPixels = inInitData->dmLogPixels;
    wInitData.dmBitsPerPel = inInitData->dmBitsPerPel;
    wInitData.dmPelsWidth = inInitData->dmPelsWidth;
    wInitData.dmPelsHeight = inInitData->dmPelsHeight;
    wInitData.dmDisplayFlags = inInitData->dmDisplayFlags;
    wInitData.dmDisplayFrequency = inInitData->dmDisplayFrequency;

    a2w_buffer((LPCSTR)inInitData->dmDeviceName, -1, wInitData.dmDeviceName, sizeof(wInitData.dmDeviceName) / sizeof(TCHAR));
    a2w_buffer((LPCSTR)inInitData->dmFormName, -1, wInitData.dmFormName, sizeof(wInitData.dmFormName) / sizeof(TCHAR));

    retval = CreateDC(wDriver, wDevice, wOutput, &wInitData);

    if(NULL != wDriver)
    {
        free(wDriver);
        wDriver = NULL;
    }
    if(NULL != wDevice)
    {
        free(wDevice);
        wDevice = NULL;
    }
    if(NULL != wOutput)
    {
        free(wOutput);
        wOutput = NULL;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HDC _CreateDCA2(LPCSTR inDriver, LPCSTR inDevice, LPCSTR inOutput, CONST DEVMODE* inInitData)
{
    HDC retval = NULL;

    LPTSTR wDriver = a2w_malloc(inDriver, -1, NULL);
    LPTSTR wDevice = a2w_malloc(inDevice, -1, NULL);
    LPTSTR wOutput = a2w_malloc(inOutput, -1, NULL);

    retval = CreateDC(wDriver, wDevice, wOutput, inInitData);

    if(NULL != wDriver)
    {
        free(wDriver);
        wDriver = NULL;
    }
    if(NULL != wDevice)
    {
        free(wDevice);
        wDevice = NULL;
    }
    if(NULL != wOutput)
    {
        free(wOutput);
        wOutput = NULL;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL _GetTextExtentExPointA(HDC inDC, LPCSTR inStr, int inLen, int inMaxExtent, LPINT outFit, LPINT outDx, LPSIZE inSize)
{
    BOOL retval = FALSE;

    int wLen = 0;
    LPTSTR wStr = a2w_malloc(inStr, inLen, &wLen);

    if(NULL != wStr)
    {
        retval = GetTextExtentExPoint(inDC, wStr, wLen, inMaxExtent, outFit, outDx, inSize);

        free(wStr);
        wStr = NULL;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL _ExtTextOutA(HDC inDC, int inX, int inY, UINT inOptions, const LPRECT inRect, LPCSTR inString, UINT inCount, const LPINT inDx)
{
    BOOL retval = false;

    int wLen = 0;
    LPTSTR wStr = a2w_malloc(inString, inCount, &wLen);

    if(NULL != wStr)
    {
        retval = ExtTextOut(inDC, inX, inY, inOptions, inRect, wStr, wLen, inDx);

        free(wStr);
        wStr = NULL;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetGlyphOutlineA(HDC inDC, CHAR inChar, UINT inFormat, LPGLYPHMETRICS inGM, DWORD inBufferSize, LPVOID outBuffer, CONST LPMAT2 inMAT2)
{
    DWORD retval = GDI_ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetCurrentDirectoryA(DWORD inBufferLength, LPSTR outBuffer)
{
    DWORD retval = 0;

    if(NULL != outBuffer && 0 < inBufferLength)
    {
        outBuffer[0] = '\0';
    }

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL SetCurrentDirectoryA(LPCSTR inPathName)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_NOT_SUPPORTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API LONG RegEnumKeyExA(HKEY inKey, DWORD inIndex, LPSTR outName, LPDWORD inoutName, LPDWORD inReserved, LPSTR outClass, LPDWORD inoutClass, PFILETIME inLastWriteTime)
{
    LONG retval = ERROR_NOT_ENOUGH_MEMORY;

    LPTSTR wName = (LPTSTR)malloc(sizeof(TCHAR) * *inoutName);
    DWORD wNameChars = *inoutName;
    if(NULL != wName)
    {
        LPTSTR wClass = NULL;
        DWORD wClassChars = 0;

        if(NULL != outClass)
        {
            wClass = (LPTSTR)malloc(sizeof(TCHAR) * *inoutClass);
            wClassChars = *inoutClass;
        }

        if(NULL == outClass || NULL != wClass)
        {
            retval = RegEnumKeyEx(inKey, inIndex, wName, &wNameChars, inReserved, wClass, &wClassChars, inLastWriteTime);
            if(ERROR_SUCCESS == retval)
            {
                *inoutName = w2a_buffer(wName, wNameChars + 1, outName, *inoutName);
                if(NULL != wClass)
                {
                    *inoutClass = w2a_buffer(wClass, wClassChars + 1, outClass, *inoutClass);
                }
            }
        }

        if(NULL != wClass)
        {
            free(wClass);
        }
        free(wName);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL GetFileVersionInfoA(LPSTR inFilename, DWORD inHandle, DWORD inLen, LPVOID outData)
{
    BOOL retval = FALSE;
    TCHAR wPath[MAX_PATH];

    if(0 != a2w_buffer(inFilename, -1, wPath, sizeof(wPath) / sizeof(TCHAR)))
    {
        retval = GetFileVersionInfo(wPath, inHandle, inLen, outData);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetFileVersionInfoSizeA(LPSTR inFilename, LPDWORD outHandle)
{
    DWORD retval = 0;

    TCHAR wPath[MAX_PATH];

    if(0 != a2w_buffer(inFilename, -1, wPath, sizeof(wPath) / sizeof(TCHAR)))
    {
        retval = GetFileVersionInfoSize(wPath, outHandle);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL VerQueryValueA(const LPVOID inBlock, LPSTR inSubBlock, LPVOID *outBuffer, PUINT outLen)
{
    BOOL retval = FALSE;
    LPTSTR wBlock = NULL;

    wBlock = a2w_malloc(inSubBlock, -1, NULL);
    if(NULL != wBlock)
    {
        retval = VerQueryValue(inBlock, wBlock, outBuffer, outLen);
        free(wBlock);
        wBlock = NULL;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API int _LoadStringA(HINSTANCE inInstance, UINT inID, LPSTR outBuffer, int inBufferMax)
{
    int retval = 0;

    if(NULL != outBuffer && 0 < inBufferMax)
    {
        LPTSTR wBuffer = (LPTSTR)malloc(sizeof(TCHAR) * inBufferMax);
        if(NULL != wBuffer)
        {
            retval = LoadString(inInstance, inID, wBuffer, inBufferMax);
            if(0 < retval)
            {
                retval = w2a_buffer(wBuffer, retval + 1, outBuffer, inBufferMax);
            }
            free(wBuffer);
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API VOID _OutputDebugStringA(LPCSTR inOutputString)
{
    LPTSTR wideStr = NULL;

    wideStr = a2w_malloc(inOutputString, -1, NULL);
    if(NULL != wideStr)
    {
        OutputDebugString(wideStr);
        free(wideStr);
    }
}


MOZ_SHUNT_PPC2002_API int _DrawTextA(HDC inDC, LPCSTR inString, int inCount, LPRECT inRect, UINT inFormat)
{
    int retval = 0;
    int wStringLen = 0;
    LPTSTR wString = a2w_malloc(inString, inCount, &wStringLen);
    if(NULL != wString)
    {
        retval = DrawText(inDC, wString, wStringLen, inRect, inFormat);
        free(wString);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL _SetDlgItemTextA(HWND inDlg, int inIDDlgItem, LPCSTR inString)
{
    BOOL retval = FALSE;
    LPTSTR wString = a2w_malloc(inString, -1, NULL);
    if(NULL != wString)
    {
        retval = SetDlgItemText(inDlg, inIDDlgItem, wString);
        free(wString);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HANDLE _LoadImageA(HINSTANCE inInst, LPCSTR inName, UINT inType, int inCX, int inCY, UINT inLoad)
{
    HANDLE retval = NULL;

    LPTSTR wName = a2w_malloc(inName, -1, NULL);
    if(NULL != wName)
    {
        retval = LoadImage(inInst, wName, inType, inCX, inCY, inLoad);
        free(wName);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HWND _FindWindowA(LPCSTR inClass, LPCSTR inWindow)
{
    HWND retval = NULL;

    LPTSTR wClass = a2w_malloc(inClass, -1, NULL);
    if(NULL != wClass)
    {
        if(NULL == inWindow)
        {
            retval = FindWindow(wClass, NULL);
        }
        else
        {
            LPTSTR wWindow = a2w_malloc(inWindow, -1, NULL);
            if(NULL != wWindow)
            {
                retval = FindWindow(wClass, wWindow);
                free(wWindow);
            }
        }
        free(wClass);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT _RegisterClipboardFormatA(LPCSTR inFormat)
{
    UINT retval = 0;

    LPTSTR wFormat = a2w_malloc(inFormat, -1, NULL);
    if(NULL != wFormat)
    {
        retval = RegisterClipboardFormat(wFormat);
        free(wFormat);
    }

    return retval;
}


#if 0
{
#endif
} /* extern "C" */
