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

MOZ_SHUNT_PPC2002_API int MulDiv(int inNumber, int inNumerator, int inDenominator)
{
    int retval = -1;

    retval = (int)(((INT64)inNumber * (INT64)inNumerator) / (INT64)inDenominator);

    return retval;
}


MOZ_SHUNT_PPC2002_API int GetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, LPVOID inBits, LPBITMAPINFO inInfo, UINT inUsage)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int SetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, CONST LPVOID inBits, CONST LPBITMAPINFO inInfo, UINT inUsage)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API HBITMAP CreateDIBitmap(HDC inDC, CONST BITMAPINFOHEADER *inBMIH, DWORD inInit, CONST VOID *inBInit, CONST BITMAPINFO *inBMI, UINT inUsage)
{
    HBITMAP retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int StretchDIBits(HDC inDC, int inXDest, int inYDest, int inDestWidth, int inDestHeight, int inXSrc, int inYSrc, int inSrcWidth, int inSrcHeight, CONST VOID *inBits, CONST BITMAPINFO *inBitsInfo, UINT inUsage, DWORD inRop)
{
    int retval = GDI_ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int SetPolyFillMode(HDC inDC, int inPolyFillMode)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int SetStretchBltMode(HDC inDC, int inStretchMode)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT SetTextAlign(HDC inDC, UINT inMode)
{
    int retval = GDI_ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int ExtSelectClipRgn(HDC inDC, HRGN inRGN, int inMode)
{
    int retval = ERROR;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL LineDDA(int inXStart, int inYStart, int inXEnd, int inYEnd, LINEDDAPROC inLineFunc, LPARAM inData)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL MoveToEx(HDC inDC, int inX, int inY, LPPOINT outPoint)
{
    BOOL retval = FALSE;

    if(NULL != outPoint)
    {
        memset(outPoint, 0, sizeof(POINT));
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL LineTo(HDC inDC, int inXEnd, int inYEnd)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int FrameRect(HDC inDC, CONST RECT *inRect, HBRUSH inBrush)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL InvertRect(HDC inDC, CONST RECT *inRect)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int SetArcDirection(HDC inDC, int inArcDirection)
{
    int retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL Arc(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXStartArc, int inYStartArc, int inXEndArc, int inYEndArc)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL Pie(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXRadial1, int inYRadial1, int inXRadial2, int inYRadial2)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetFontData(HDC inDC, DWORD inTable, DWORD inOffset, LPVOID outBuffer, DWORD inData)
{
    DWORD retval = GDI_ERROR;

    if(NULL != outBuffer && 0 < inData)
    {
        memset(outBuffer, 0, inData);
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT GetTextCharset(HDC inDC)
{
    UINT retval = DEFAULT_CHARSET;

    TEXTMETRIC tm;
    if(GetTextMetrics(inDC, &tm))
    {
        retval = tm.tmCharSet;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API UINT GetTextCharsetInfo(HDC inDC, LPFONTSIGNATURE outSig, DWORD inFlags)
{
    // A broken implementation.
    if(NULL != outSig)
    {
        memset(outSig, 0, sizeof(FONTSIGNATURE));
    }

    return GetTextCharset(inDC);
}


MOZ_SHUNT_PPC2002_API UINT GetOutlineTextMetrics(HDC inDC, UINT inData, LPOUTLINETEXTMETRIC outOTM)
{
    UINT retval = 0;

    if(NULL != outOTM)
    {
        memset(outOTM, 0, sizeof(OUTLINETEXTMETRIC));
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


#define FACENAME_MAX 128
typedef struct __struct_CollectFaces
{
    UINT    mCount;
    LPTSTR  mNames[FACENAME_MAX];
}
CollectFaces;

static int CALLBACK collectProc(CONST LOGFONT* inLF, CONST TEXTMETRIC* inTM, DWORD inFontType, LPARAM inParam)
{
    int retval = 0;
    CollectFaces* collection = (CollectFaces*)inParam;

    if(FACENAME_MAX > collection->mCount)
    {
        retval = 1;

        collection->mNames[collection->mCount] = _tcsdup(inLF->lfFaceName);
        if(NULL != collection->mNames[collection->mCount])
        {
            collection->mCount++;
        }
    }

    return retval;
}

MOZ_SHUNT_PPC2002_API int EnumFontFamiliesEx(HDC inDC, LPLOGFONT inLogfont, FONTENUMPROC inFunc, LPARAM inParam, DWORD inFlags)
{
    int retval = 0;

    //  We support only one case.
    //  Callback should be oldstyle EnumFonts.
    if(DEFAULT_CHARSET == inLogfont->lfCharSet)
    {
        CollectFaces collection;
        collection.mCount = 0;

        EnumFonts(inDC, NULL, collectProc, (LPARAM)&collection);

        UINT loop;
        for(loop = 0; loop < collection.mCount; loop++)
        {
            retval = EnumFonts(inDC, collection.mNames[loop], inFunc, inParam);
        }

        for(loop = 0; loop < collection.mCount; loop++)
        {
            free(collection.mNames[loop]);
        }
    }
    else
    {
        SetLastError(ERROR_NOT_SUPPORTED);
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API int GetMapMode(HDC inDC)
{
    int retval = MM_TEXT;

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL GetIconInfo(HICON inIcon, PICONINFO outIconinfo)
{
    BOOL retval = FALSE;

    if(NULL != outIconinfo)
    {
        memset(outIconinfo, 0, sizeof(ICONINFO));
    }

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL LPtoDP(HDC inDC, LPPOINT inoutPoints, int inCount)
{
    BOOL retval = TRUE;

    return retval;
}


MOZ_SHUNT_PPC2002_API LONG RegCreateKey(HKEY inKey, LPCTSTR inSubKey, PHKEY outResult)
{
    LONG retval = ERROR_SUCCESS;
    DWORD disp = 0;

    retval = RegCreateKeyEx(inKey, inSubKey, 0, NULL, 0, 0, NULL, outResult, &disp);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL WaitMessage(VOID)
{
    BOOL retval = TRUE;

    HANDLE hThread = GetCurrentThread();
    DWORD waitRes = MsgWaitForMultipleObjectsEx(1, &hThread, INFINITE, QS_ALLEVENTS, 0);
    if((DWORD)-1 == waitRes)
    {
        retval = FALSE;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL FlashWindow(HWND inWnd, BOOL inInvert)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


#define ECW_SIZEBY  0x100
typedef struct __struct_ECWWindows
{
    DWORD       mCount;
    DWORD       mCapacity;
    HWND*       mArray;
}
ECWWindows;

static BOOL ECWHelper(HWND inParent, ECWWindows* inChildren, BOOL inRecurse)
{
    BOOL retval = TRUE;

    HWND child = GetWindow(inParent, GW_CHILD);
    while(NULL != child && FALSE != retval)
    {
        if(inChildren->mCount >= inChildren->mCapacity)
        {
            void* moved = realloc(inChildren->mArray, sizeof(HWND) * (ECW_SIZEBY + inChildren->mCapacity));
            if(NULL != moved)
            {
                inChildren->mCapacity += ECW_SIZEBY;
                inChildren->mArray = (HWND*)moved;
            }
            else
            {
                retval = FALSE;
                break;
            }
        }

        inChildren->mArray[inChildren->mCount] = child;
        inChildren->mCount++;
        if(FALSE != inRecurse)
        {
            retval = ECWHelper(child, inChildren, inRecurse);
        }

        child = GetWindow(child, GW_HWNDNEXT);
    }

    return retval;
}

MOZ_SHUNT_PPC2002_API BOOL EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam)
{
    BOOL retval = FALSE;

    if(NULL != inFunc)
    {
        if(NULL == inParent)
        {
            inParent = GetDesktopWindow();
        }

        ECWWindows children;
        memset(&children, 0, sizeof(children));
        children.mArray = (HWND*)malloc(sizeof(HWND) * ECW_SIZEBY);
        if(NULL != children.mArray)
        {
            children.mCapacity = ECW_SIZEBY;

            BOOL helperRes = ECWHelper(inParent, &children, TRUE);
            if(FALSE != helperRes)
            {
                DWORD loop = 0;
                for(loop = 0; loop < children.mCount; loop++)
                {
                    if(IsWindow(children.mArray[loop])) // validate
                    {
                        if(FALSE == inFunc(children.mArray[loop], inParam))
                        {
                            break;
                        }
                    }
                }
            }

            free(children.mArray);
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL EnumThreadWindows(DWORD inThreadID, WNDENUMPROC inFunc, LPARAM inParam)
{
    BOOL retval = FALSE;

    if(NULL != inFunc)
    {
        ECWWindows children;
        memset(&children, 0, sizeof(children));
        children.mArray = (HWND*)malloc(sizeof(HWND) * ECW_SIZEBY);
        if(NULL != children.mArray)
        {
            children.mCapacity = ECW_SIZEBY;

            BOOL helperRes = ECWHelper(GetDesktopWindow(), &children, FALSE);
            if(FALSE != helperRes)
            {
                DWORD loop = 0;
                for(loop = 0; loop < children.mCount; loop++)
                {
                    if(IsWindow(children.mArray[loop]) && inThreadID == GetWindowThreadProcessId(children.mArray[loop], NULL)) // validate
                    {
                        if(FALSE == inFunc(children.mArray[loop], inParam))
                        {
                            break;
                        }
                    }
                }
            }

            free(children.mArray);
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL IsIconic(HWND inWnd)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL OpenIcon(HWND inWnd)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL InvalidateRgn(HWND inWnd, HRGN inRgn, BOOL inErase)
{
    BOOL retval = FALSE;

    RECT box;
    int boxRes = GetRgnBox(inRgn, &box);
    if(0 != boxRes)
    {
        if(NULLREGION != boxRes)
        {
            retval = InvalidateRect(inWnd, &box, inErase);
        }
        else
        {
            retval = TRUE;
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HHOOK SetWindowsHookEx(int inType, HOOKPROC inFunc, HINSTANCE inMod, DWORD inThreadId)
{
    HHOOK retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL UnhookWindowsHookEx(HHOOK inHook)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API LRESULT CallNextHookEx(HHOOK inHook, int inCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL InvertRgn(HDC inDC, HRGN inRGN)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API int GetScrollPos(HWND inWnd, int inBar)
{
    int retval = 0;
    SCROLLINFO info;

    if(GetScrollInfo(inWnd, inBar, &info))
    {
        return info.nPos;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL GetScrollRange(HWND inWnd, int inBar, LPINT outMinPos, LPINT outMaxPos)
{
    BOOL retval = FALSE;
    SCROLLINFO info;

    if((retval = GetScrollInfo(inWnd, inBar, &info)))
    {
        if(NULL != outMinPos)
        {
            *outMinPos = info.nMin;
        }
        if(NULL != outMaxPos)
        {
            *outMaxPos = info.nMax;
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HRESULT _CoLockObjectExternal(IUnknown* inUnk, BOOL inLock, BOOL inLastUnlockReleases)
{
    HRESULT retval = S_OK;

    if(NULL != inUnk)
    {
        if(FALSE == inLock)
        {
            inUnk->Release();
        }
        else
        {
            inUnk->AddRef();
        }
    }
    else
    {
        retval = E_INVALIDARG;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API HRESULT _OleSetClipboard(IDataObject* inDataObj)
{
    HRESULT retval = E_NOTIMPL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API HRESULT _OleGetClipboard(IDataObject** outDataObj)
{
    HRESULT retval = E_NOTIMPL;

    if(NULL != outDataObj)
    {
        *outDataObj = NULL;
    }
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API HRESULT _OleFlushClipboard(void)
{
    HRESULT retval = E_NOTIMPL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API HRESULT _OleQueryLinkFromData(IDataObject* inSrcDataObject)
{
    HRESULT retval = E_NOTIMPL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API LPITEMIDLIST _SHBrowseForFolder(LPBROWSEINFO inBI)
{
    LPITEMIDLIST retval = NULL;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL SetMenu(HWND inWnd, HMENU inMenu)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


MOZ_SHUNT_PPC2002_API BOOL GetUserName(LPTSTR inBuffer, LPDWORD inoutSize)
{
    BOOL retval = FALSE;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    *inoutSize = 0;

    return retval;
}


MOZ_SHUNT_PPC2002_API DWORD GetShortPathName(LPCTSTR inLongPath, LPTSTR outShortPath, DWORD inBufferSize)
{
    DWORD retval = 0;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return retval;
}


#if 0
{
#endif
} /* extern "C" */
