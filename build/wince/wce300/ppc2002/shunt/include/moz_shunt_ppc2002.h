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

#if !defined __moz_shunt_ppc2002_h
#define __moz_shunt_ppc2002_h

/*
**  Avoid some annoying macro defs.
*/
#define NOMINMAX

#include <windows.h>
#include "moz_shunt_ppc2002_exdispid.h"

/*
**  Force the annoying macro defs to not exist.
*/
#if defined(min)
#undef min
#endif
#if defined(max)
#undef max
#endif


/*
**  All stub/shunt include files in general should include this one.
**  Everthing should be contained inside this one file.
**  Some files of course will not be overridden (i.e. string.h) but still
**      missing significant directives.
**  This global file mitigates that problem by simply forcing the include
**      of this very file by proper name.
*/


/****************************************************************************
**  C API Section.
****************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
#if 0
}
#endif


/****************************************************************************
**  How to export things from the missing lib.
****************************************************************************/
#ifdef MOZ_SHUNT_PPC2002_EXPORTS
#define MOZ_SHUNT_PPC2002_API __declspec(dllexport)
#else
#define MOZ_SHUNT_PPC2002_API __declspec(dllimport)
#endif


/****************************************************************************
**  Useful conversion functions.
****************************************************************************/
MOZ_SHUNT_PPC2002_API int a2w_buffer(LPCSTR inACPString, int inACPChars, LPWSTR outWideString, int inWideChars)
/*
**  Perform the requested conversion using the buffer provided.
**
**  inACPString     The wide character string to convert.
**  inACPChars      Count of acp multibyte characters in inACPString to be
**                      converted.
**                  If -1, assume a terminated string and the terminating
**                      character will also be appended to outWideString.
**  outWideString   The buffer to store the converted string.
**  inWideChars     Number of characters (not bytes) outWideString can hold.
**                  If this value is zero, then the character count required
**                      for the conversion is returned and outWideString is
**                      untouched.
**  returns int     The number of characters (not bytes) converted/required.
**                  Zero indicates failure.
**                  Generally you could use this value - 1 to avoid a
**                      wcslen() call after the conversion took place
**                      should the string be terminated (i.e. if inACPChars
**                      included a terminating character for the conversion).
*/
;
MOZ_SHUNT_PPC2002_API LPWSTR a2w_malloc(LPCSTR inACPString, int inACPChars, int* outWideChars)
/*
**  Perform the requested conversion using heap memory.
**  The caller/client of this function must use free() to release the
**      resultant string to the heap once finished with said string.
**  This function is best used when the conversion length of inACPString
**      is not known beforehand.
**
**  inACPString     The acp multibyte character string to convert.
**  inACPChars      Count of acp multibyte characters in inACPString to be
**                      converted.
**                  If -1, assume a terminated string and the terminating
**                      character will also be appended to the return value.
**  outWideChars    Optional argument, can be NULL.
**                  Holds number of characters (not bytes) written into
**                      return value.
**                  Generally you would use outWideChars - 1 to avoid a
**                      wcslen() call after the conversion took place
**                      should the string be terminated (i.e. if inACPChars
**                      included a terminating character for the conversion).
**  returns LPWSTR  The malloced converted string which must eventually be
**                      free()d.
**                  NULL on failure.
*/
;
MOZ_SHUNT_PPC2002_API int w2a_buffer(LPCWSTR inWideString, int inWideChars, LPSTR outACPString, int inACPChars)
/*
**  Perform the requested conversion using the buffer provided.
**
**  inWideString    The wide character string to convert.
**  inWideChars     Count of wide characters (not bytes) in
**                      inWideString to be converted.
**                  If -1, assume a terminated string and the terminating
**                      character will also be appended to outACPString.
**  outACPString    The buffer to store the converted string.
**  inACPChars      Number of characters outACPString can hold.
**                  If this value is zero, then the character count required
**                      for the conversion is returned and outACPString is
**                      untouched.
**  returns int     The number of characters converted or required.
**                  Zero indicates failure.
**                  Generally you could use this value - 1 to avoid a
**                      strlen() call after the conversion took place
**                      should the string be terminated (i.e. if inWideChars
**                      included a terminating character for the conversion).
*/
;
MOZ_SHUNT_PPC2002_API LPSTR w2a_malloc(LPCWSTR inWideString, int inWideChars, int* outACPChars)
/*
**  Perform the requested conversion using heap memory.
**  The caller/client of this function must use free() to release the
**      resultant string to the heap once finished with said string.
**  This function is best used when the conversion length of inWideString
**      is not known beforehand.
**
**  inWideString    The wide character string to convert.
**  inWideChars     Count of wide characters (not bytes) in
**                      inWideString to be converted.
**                  If -1, assume a terminated string and the terminating
**                      character will also be appended to the return value.
**  outACPChars     Optional argument, can be NULL.
**                  Holds number of characters written into return value.
**                  Generally you would use outACPChars - 1 to avoid a
**                      strlen() call after the conversion took place
**                      should the string be terminated (i.e. if inWideChars
**                      included a terminating character for the conversion).
**  returns LPSTR   The malloced converted string which must eventually be
**                      free()d.
**                  NULL on failure.
*/
;


/****************************************************************************
**  assert.h
****************************************************************************/
#define assert(expression) _assert((int)(expression))
MOZ_SHUNT_PPC2002_API void _assert(int inExpression);


/****************************************************************************
**  direct.h
****************************************************************************/
MOZ_SHUNT_PPC2002_API int mkdir(const char* inDirname);
MOZ_SHUNT_PPC2002_API int rmdir(const char* inDirname);


/****************************************************************************
**  errno.h
****************************************************************************/
extern MOZ_SHUNT_PPC2002_API int errno;

#define EINVAL              __LINE__
#define EAGAIN              __LINE__
#define EINTR               __LINE__
#define ENOMEM              __LINE__
#define EBADF               __LINE__
#define EFAULT              __LINE__
#define EACCES              __LINE__
#define EIO                 __LINE__
#define ENOTDIR             __LINE__
#define EDEADLOCK           __LINE__
#define EFBIG               __LINE__
#define ENOSPC              __LINE__
#define EPIPE               __LINE__
#define ESPIPE              __LINE__
#define EISDIR              __LINE__
#define ENOENT              __LINE__
#define EROFS               __LINE__
#define EBUSY               __LINE__
#define EXDEV               __LINE__
#define EEXIST              __LINE__
#define ENFILE              __LINE__
#define EDEADLK             __LINE__
#define ERANGE              __LINE__
#define EPERM               __LINE__


/****************************************************************************
**  io.h
****************************************************************************/
#define isatty(handle) _isatty((int)(handle))

MOZ_SHUNT_PPC2002_API int chmod(const char* inFilename, int inMode);
MOZ_SHUNT_PPC2002_API int _isatty(int inHandle);


/****************************************************************************
**  mbstring.h
****************************************************************************/
MOZ_SHUNT_PPC2002_API unsigned char* _mbsinc(const unsigned char* inCurrent);
MOZ_SHUNT_PPC2002_API unsigned char* _mbspbrk(const unsigned char* inString, const unsigned char* inStrCharSet);
MOZ_SHUNT_PPC2002_API unsigned char* _mbschr(const unsigned char* inString, unsigned int inC);
MOZ_SHUNT_PPC2002_API unsigned char* _mbsrchr(const unsigned char* inString, unsigned int inC);


/****************************************************************************
**  process.h
****************************************************************************/
MOZ_SHUNT_PPC2002_API void abort(void);
MOZ_SHUNT_PPC2002_API char* getenv(const char* inName);
MOZ_SHUNT_PPC2002_API int getpid(void);


/****************************************************************************
**  signal.h
****************************************************************************/
#define SIGABRT         0
#define SIGSEGV         1
#define _SIGCOUNT       2 /* LAST ONE, SIZES BUFFER */

typedef void (*_sigsig)(int inSignal);

MOZ_SHUNT_PPC2002_API int raise(int inSignal);
MOZ_SHUNT_PPC2002_API _sigsig signal(int inSignal, _sigsig inFunc);


/****************************************************************************
**  stdio.h
****************************************************************************/
MOZ_SHUNT_PPC2002_API void rewind(FILE* inStream);
MOZ_SHUNT_PPC2002_API FILE* _fdopen(int inFD, const char* inMode);
MOZ_SHUNT_PPC2002_API void perror(const char* inString);
MOZ_SHUNT_PPC2002_API int remove(const char* inPath);


/****************************************************************************
**  stdlib.h
****************************************************************************/
#define _MAX_DRIVE      MAX_PATH
#define _MAX_DIR        MAX_PATH
#define _MAX_EXT        MAX_PATH
#define _MAX_FNAME      MAX_PATH
#define _MAX_PATH       MAX_PATH

MOZ_SHUNT_PPC2002_API void _splitpath(const char* inPath, char* outDrive, char* outDir, char* outFname, char* outExt);
MOZ_SHUNT_PPC2002_API void _makepath(char* outPath, const char* inDrive, const char* inDir, const char* inFname, const char* inExt);


/****************************************************************************
**  string.h
****************************************************************************/
#define stricmp             _stricmp
#define strcmpi             _stricmp
#define strdup              _strdup

MOZ_SHUNT_PPC2002_API char* strerror(int inErrno);


/****************************************************************************
**  sys/types.h
****************************************************************************/
typedef int ptrdiff_t;
typedef long off_t;
typedef long _off_t;


/****************************************************************************
**  sys/stat.h
****************************************************************************/
#if !defined(_STAT_DEFINED)
#define _STAT_DEFINED
#define _S_IFDIR    0040000 /* stat, is a directory */
#define _S_IFREG    0100000 /* stat, is a normal file */
#define _S_IREAD    0000400 /* stat, can read */
#define _S_IWRITE   0000200 /* stat, can write */
struct stat
{
    unsigned short st_mode;
    _off_t st_size;
    time_t st_ctime;
    time_t st_atime;
    time_t st_mtime;
};

#define _stat(path, stats) stat((path), (stats))
MOZ_SHUNT_PPC2002_API int stat(const char* inPath, struct stat* outStats);
#endif /* _STAT_DEFINED */


/****************************************************************************
**  time.h
****************************************************************************/
#ifndef _TM_DEFINED
#define _TM_DEFINED
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
#endif

MOZ_SHUNT_PPC2002_API time_t time(time_t* inStorage);
MOZ_SHUNT_PPC2002_API char* ctime(const time_t* timer);
MOZ_SHUNT_PPC2002_API struct tm* localtime_r(const time_t* inTimeT,struct tm* outRetval);
MOZ_SHUNT_PPC2002_API struct tm* localtime(const time_t* inTimeT);
MOZ_SHUNT_PPC2002_API struct tm* gmtime_r(const time_t* inTimeT, struct tm* outRetval);
MOZ_SHUNT_PPC2002_API struct tm* gmtime(const time_t* inTimeT);
MOZ_SHUNT_PPC2002_API time_t mktime(struct tm* inTM);
MOZ_SHUNT_PPC2002_API size_t strftime(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr);


/****************************************************************************
**  windows.h
****************************************************************************/
/* API misdirection */
#define wsprintfA                       sprintf
#define wvsprintfA                      vsprintf
#define lstrcpyA                        strcpy
#define lstrlenA                        strlen
#define lstrcmpA                        strcmp
#define lstrcatA                        strcat
#define lstrcmpiA                       stricmp
#define GetTextExtentExPointA           _GetTextExtentExPointA
#define MessageBoxA                     _MessageBoxA
#define CreateDCA                       _CreateDCA
#define ExtTextOutA                     _ExtTextOutA
#define LoadStringA                     _LoadStringA
#define OutputDebugStringA              _OutputDebugStringA
#define DrawTextA                       _DrawTextA
#define SetDlgItemTextA                 _SetDlgItemTextA
#define CoInitialize(null)              CoInitializeEx((null), COINIT_MULTITHREADED)
#define OleInitialize(null)             CoInitializeEx((null), COINIT_MULTITHREADED)
#define OleUninitialize                 CoUninitialize
#define CoLockObjectExternal            _CoLockObjectExternal
#define OleSetClipboard                 _OleSetClipboard
#define OleGetClipboard                 _OleGetClipboard
#define OleFlushClipboard               _OleFlushClipboard
#define OleQueryLinkFromData            _OleQueryLinkFromData
#define SHBrowseForFolder               _SHBrowseForFolder
#define LoadImageA                      _LoadImageA
#define FindWindowA                     _FindWindowA
#define RegisterClipboardFormatA        _RegisterClipboardFormatA
#define W2T(string)                     (string)
#define T2W(string)                     (string)
#if defined(UNICODE)
#define RegSetValue                     RegSetValueW
#define RegOpenKey                      RegOpenKeyW
#else /* UNICODE */
#define RegSetValue                     RegSetValueA
#define RegOpenKey                      RegOpenKeyA
#endif /* UNICODE */
#define RegSetValueA(h, k, t, d, s)     RegSetValueExA(h, k, 0, t, d, s)
#define RegSetValueW(h, k, t, d, s)     RegSetValueExW(h, k, 0, t, d, s)
#define RegOpenKeyA(h, k, r)            RegOpenKeyExA(h, k, 0, 0, r)
#define RegOpenKeyW(h, k, r)            RegOpenKeyExW(h, k, 0, 0, r)

/* misc */
#define HINSTANCE_ERROR         1
#define DETACHED_PROCESS        0
#define NORMAL_PRIORITY_CLASS   0
#define OUT_TT_PRECIS           OUT_DEFAULT_PRECIS
#define SM_CXFRAME              SM_CXFIXEDFRAME
#define SM_CYFRAME              SM_CYFIXEDFRAME
#define SW_SHOWMINIMIZED        SW_MINIMIZE
#define SWP_NOREDRAW            0
#define LR_LOADFROMFILE         0
#define LR_SHARED               0
#define SS_SUNKEN               0
#define RDW_INVALIDATE          0
#define RDW_UPDATENOW           0
#if !defined(FINDMSGSTRING)
#define FINDMSGSTRING            _T("commdlg_FindReplace")
#endif /* FINDMSGSTRING */
#define SPIF_SENDWININICHANGE   0
#define TPM_RIGHTBUTTON         0

/* SetPolyFillMode */
#define ALTERNATE   1
#define WINDING     2

/* SetStretchBltMode */
#define BLACKONWHITE        1
#define COLORONCOLOR        2
#define HALFTONE            3
#define WHITEONBLACK        4
#define STRETCH_ANDSCANS    BLACKONWHITE
#define STRETCH_DELETESCANS COLORONCOLOR
#define STRETCH_HALFTONE    HALFTONE
#define STRETCH_ORSCANS     WHITEONBLACK

/* SetTextAlign */
#define TA_BASELINE     ((UINT)1 << 0)
#define TA_BOTTOM       ((UINT)1 << 1)
#define TA_TOP          ((UINT)1 << 2)
#define TA_CENTER       ((UINT)1 << 3)
#define TA_LEFT         ((UINT)1 << 4)
#define TA_RIGHT        ((UINT)1 << 5)
#define TA_NOUPDATECP   ((UINT)1 << 6)
#define TA_RTLREADING   ((UINT)1 << 7)
#define TA_UPDATECP     ((UINT)1 << 8)
#define VTA_BASELINE    ((UINT)1 << 9)
#define VTA_CENTER      ((UINT)1 << 10)

/* SetArcDirection */
#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE 2

/* GetGlyphOutline */
#define GGO_BITMAP          1
#define GGO_NATIVE          2
#define GGO_METRICS         3
#define GGO_GRAY2_BITMAP    4
#define GGO_GRAY4_BITMAP    5
#define GGO_GRAY8_BITMAP    6
#define GGO_GLYPH_INDEX     7

/* GetMapMode */
#define MM_TEXT         1

/* CreateDIBitmap */
#define CBM_INIT        1

/* SetWindowsHookEx */
#define WH_MOUSE            1
#define WH_CALLWNDPROC      2
#define WH_MSGFILTER        3

/* GetGlyphOutline */
typedef struct __struct_GLYPHMETRICS
{
    UINT  gmBlackBoxX;
    UINT  gmBlackBoxY;
    POINT gmptGlyphOrigin;
    short gmCellIncX;
    short gmCellIncY;
}
GLYPHMETRICS, *LPGLYPHMETRICS;

/* GetGlyphOutline */
typedef struct __struct_FIXED
{
    WORD  fract;
    short value;
}
FIXED, *LPFIXED;

/* GetGlyphOutline */
typedef struct __struct_MAT2
{
    FIXED eM11;
    FIXED eM12;
    FIXED eM21;
    FIXED eM22;
}
MAT2, *LPMAT2;

/* GetOutlineTextMetrics */
typedef struct __struct_PANOSE
{
    BYTE bFamilyType;
    BYTE bSerifStyle;
    BYTE bWeight;
    BYTE bProportion;
    BYTE bContrast;
    BYTE bStrokeVariation;
    BYTE bArmStyle;
    BYTE bLetterform;
    BYTE bMidline;
    BYTE bXHeight;
}
PANOSE, *LPPANOSE;

/* GetOutlineTextMetrics */
typedef struct __struct_OUTLINETEXTMETRIC
{
    UINT        otmSize;
    TEXTMETRIC  otmTextMetrics;
    BYTE        otmFiller;
    PANOSE      otmPanoseNumber;
    UINT        otmfsSelection;
    UINT        otmfsType;
    int         otmsCharSlopeRise;
    int         otmsCharSlopeRun;
    int         otmItalicAngle;
    UINT        otmEMSquare;
    int         otmAscent;
    int         otmDescent;
    UINT        otmLineGap;
    UINT        otmsCapEmHeight;
    UINT        otmsXHeight;
    RECT        otmrcFontBox;
    int         otmMacAscent;
    int         otmMacDescent;
    UINT        otmMacLineGap;
    UINT        otmusMinimumPPEM;
    POINT       otmptSubscriptSize;
    POINT       otmptSubscriptOffset;
    POINT       otmptSuperscriptSize;
    POINT       otmptSuperscriptOffset;
    UINT        otmsStrikeoutSize;
    int         otmsStrikeoutPosition;
    int         otmsUnderscoreSize;
    int         otmsUnderscorePosition;
    PSTR        otmpFamilyName;
    PSTR        otmpFaceName;
    PSTR        otmpStyleName;
    PSTR        otmpFullName;
}
OUTLINETEXTMETRIC, *LPOUTLINETEXTMETRIC;

/* SetWindowsHookEx */
typedef struct __struct_CWPSTRUCT
{
    LPARAM  lParam;
    WPARAM  wParam;
    UINT    message;
    HWND    hwnd;
}
CWPSTRUCT;
typedef struct __struct_MOUSEHOOKSTRUCT
{
    POINT   pt;
    HWND    hwnd;
    UINT    wHitTestCode;
    DWORD   dwExtraInfo;
}
MOUSEHOOKSTRUCT;

typedef LRESULT (CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);
typedef VOID (CALLBACK* LINEDDAPROC)(int inX, int inY, LPARAM inData);

/* SHBrowseForFolder */
struct _ITEMIDLIST;
struct _browseinfo;
typedef struct _ITEMIDLIST	ITEMIDLIST, *LPITEMIDLIST;
typedef struct _browseinfo BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

#if defined(UNICODE)
#define GetWindowsDirectory GetWindowsDirectoryW
#else
#define GetWindowsDirectory GetWindowsDirectoryA
#endif
MOZ_SHUNT_PPC2002_API UINT GetWindowsDirectoryA(LPSTR inBuffer, UINT inSize);
MOZ_SHUNT_PPC2002_API UINT GetWindowsDirectoryW(LPWSTR inBuffer, UINT inSize);

#if defined(UNICODE)
#define GetSystemDirectory GetSystemDirectoryW
#else
#define GetSystemDirectory GetSystemDirectoryA
#endif
MOZ_SHUNT_PPC2002_API UINT GetSystemDirectoryA(LPSTR inBuffer, UINT inSize);
MOZ_SHUNT_PPC2002_API UINT GetSystemDirectoryW(LPWSTR inBuffer, UINT inSize);

#if defined(UNICODE)
#define OpenSemaphore OpenSemaphoreW
#else
#define OpenSemaphore OpenSemaphoreA
#endif
MOZ_SHUNT_PPC2002_API HANDLE OpenSemaphoreA(DWORD inDesiredAccess, BOOL inInheritHandle, LPCSTR inName);
MOZ_SHUNT_PPC2002_API HANDLE OpenSemaphoreW(DWORD inDesiredAccess, BOOL inInheritHandle, LPCWSTR inName);

#if defined(UNICODE)
#define GetGlyphOutline GetGlyphOutlineW
#else
#define GetGlyphOutline GetGlyphOutlineA
#endif
MOZ_SHUNT_PPC2002_API DWORD GetGlyphOutlineA(HDC inDC, CHAR inChar, UINT inFormat, LPGLYPHMETRICS inGM, DWORD inBufferSize, LPVOID outBuffer, CONST LPMAT2 inMAT2);
MOZ_SHUNT_PPC2002_API DWORD GetGlyphOutlineW(HDC inDC, WCHAR inChar, UINT inFormat, LPGLYPHMETRICS inGM, DWORD inBufferSize, LPVOID outBuffer, CONST LPMAT2 inMAT2);

#if defined(UNICODE)
#define GetCurrentDirectory GetCurrentDirectoryW
#define SetCurrentDirectory SetCurrentDirectoryW
#else
#define GetCurrentDirectory GetCurrentDirectoryA
#define SetCurrentDirectory SetCurrentDirectoryA
#endif
MOZ_SHUNT_PPC2002_API DWORD GetCurrentDirectoryW(DWORD inBufferLength, LPTSTR outBuffer);
MOZ_SHUNT_PPC2002_API BOOL SetCurrentDirectoryW(LPCTSTR inPathName);
MOZ_SHUNT_PPC2002_API DWORD GetCurrentDirectoryA(DWORD inBufferLength, LPSTR outBuffer);
MOZ_SHUNT_PPC2002_API BOOL SetCurrentDirectoryA(LPCSTR inPathName);

MOZ_SHUNT_PPC2002_API BOOL _GetTextExtentExPointA(HDC inDC, LPCSTR inStr, int inLen, int inMaxExtent, LPINT outFit, LPINT outDx, LPSIZE inSize);
MOZ_SHUNT_PPC2002_API int _MessageBoxA(HWND inWnd, LPCSTR inText, LPCSTR inCaption, UINT uType);
MOZ_SHUNT_PPC2002_API HDC _CreateDCA(LPCSTR inDriver, LPCSTR inDevice, LPCSTR inOutput, CONST DEVMODEA* inInitData);
MOZ_SHUNT_PPC2002_API HDC _CreateDCA2(LPCSTR inDriver, LPCSTR inDevice, LPCSTR inOutput, CONST DEVMODE* inInitData);
MOZ_SHUNT_PPC2002_API BOOL _ExtTextOutA(HDC inDC, int inX, int inY, UINT inOptions, const LPRECT inRect, LPCSTR inString, UINT inCount, const LPINT inDx);
MOZ_SHUNT_PPC2002_API int _LoadStringA(HINSTANCE inInstance, UINT inID, LPSTR outBuffer, int inBufferMax);
MOZ_SHUNT_PPC2002_API VOID _OutputDebugStringA(LPCSTR inOutputString);
MOZ_SHUNT_PPC2002_API int _DrawTextA(HDC inDC, LPCSTR inString, int inCount, LPRECT inRect, UINT inFormat);
MOZ_SHUNT_PPC2002_API BOOL _SetDlgItemTextA(HWND inDlg, int inIDDlgItem, LPCSTR inString);
MOZ_SHUNT_PPC2002_API HANDLE _LoadImageA(HINSTANCE inInst, LPCSTR inName, UINT inType, int inCX, int inCY, UINT inLoad);
MOZ_SHUNT_PPC2002_API int MulDiv(int inNumber, int inNumerator, int inDenominator);
MOZ_SHUNT_PPC2002_API int GetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, LPVOID inBits, LPBITMAPINFO inInfo, UINT inUsage);
MOZ_SHUNT_PPC2002_API int SetDIBits(HDC inDC, HBITMAP inBMP, UINT inStartScan, UINT inScanLines, CONST LPVOID inBits, CONST LPBITMAPINFO inInfo, UINT inUsage);
MOZ_SHUNT_PPC2002_API HBITMAP CreateDIBitmap(HDC inDC, CONST BITMAPINFOHEADER *inBMIH, DWORD inInit, CONST VOID *inBInit, CONST BITMAPINFO *inBMI, UINT inUsage);
MOZ_SHUNT_PPC2002_API int SetPolyFillMode(HDC inDC, int inPolyFillMode);
MOZ_SHUNT_PPC2002_API int SetStretchBltMode(HDC inDC, int inStretchMode);
MOZ_SHUNT_PPC2002_API UINT SetTextAlign(HDC inDC, UINT inMode);
MOZ_SHUNT_PPC2002_API int ExtSelectClipRgn(HDC inDC, HRGN inRGN, int inMode);
MOZ_SHUNT_PPC2002_API BOOL LineDDA(int inXStart, int inYStart, int inXEnd, int inYEnd, LINEDDAPROC inLineFunc, LPARAM inData);
MOZ_SHUNT_PPC2002_API BOOL MoveToEx(HDC inDC, int inX, int inY, LPPOINT outPoint);
MOZ_SHUNT_PPC2002_API BOOL LineTo(HDC inDC, int inXEnd, int inYEnd);
MOZ_SHUNT_PPC2002_API int FrameRect(HDC inDC, CONST RECT *inRect, HBRUSH inBrush);
MOZ_SHUNT_PPC2002_API BOOL InvertRect(HDC inDC, CONST RECT *inRect);
MOZ_SHUNT_PPC2002_API int SetArcDirection(HDC inDC, int inArcDirection);
MOZ_SHUNT_PPC2002_API BOOL Arc(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXStartArc, int inYStartArc, int inXEndArc, int inYEndArc);
MOZ_SHUNT_PPC2002_API BOOL Pie(HDC inDC, int inLeftRect, int inTopRect, int inRightRect, int inBottomRect, int inXRadial1, int inYRadial1, int inXRadial2, int inYRadial2);
MOZ_SHUNT_PPC2002_API DWORD GetFontData(HDC inDC, DWORD inTable, DWORD inOffset, LPVOID outBuffer, DWORD inData);
MOZ_SHUNT_PPC2002_API UINT GetTextCharset(HDC inDC);
MOZ_SHUNT_PPC2002_API UINT GetOutlineTextMetrics(HDC inDC, UINT inData, LPOUTLINETEXTMETRIC outOTM);
MOZ_SHUNT_PPC2002_API int EnumFontFamiliesEx(HDC inDC, LPLOGFONT inLogfont, FONTENUMPROC inFunc, LPARAM inParam, DWORD inFlags);
MOZ_SHUNT_PPC2002_API int GetMapMode(HDC inDC);
MOZ_SHUNT_PPC2002_API UINT GetTextCharsetInfo(HDC inDC, LPFONTSIGNATURE outSig, DWORD inFlags);
MOZ_SHUNT_PPC2002_API int StretchDIBits(HDC inDC, int inXDest, int inYDest, int inDestWidth, int inDestHeight, int inXSrc, int inYSrc, int inSrcWidth, int inSrcHeight, CONST VOID *inBits, CONST BITMAPINFO *inBitsInfo, UINT inUsage, DWORD inRop);
MOZ_SHUNT_PPC2002_API BOOL GetIconInfo(HICON inIcon, PICONINFO outIconinfo);
MOZ_SHUNT_PPC2002_API BOOL LPtoDP(HDC inDC, LPPOINT inoutPoints, int inCount);
MOZ_SHUNT_PPC2002_API LONG RegCreateKey(HKEY inKey, LPCTSTR inSubKey, PHKEY outResult);
MOZ_SHUNT_PPC2002_API BOOL WaitMessage(VOID);
MOZ_SHUNT_PPC2002_API BOOL FlashWindow(HWND inWnd, BOOL inInvert);
MOZ_SHUNT_PPC2002_API BOOL EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam);
MOZ_SHUNT_PPC2002_API BOOL EnumThreadWindows(DWORD inThreadID, WNDENUMPROC inFunc, LPARAM inParam);
MOZ_SHUNT_PPC2002_API BOOL IsIconic(HWND inWnd);
MOZ_SHUNT_PPC2002_API BOOL OpenIcon(HWND inWnd);
MOZ_SHUNT_PPC2002_API BOOL InvalidateRgn(HWND inWnd, HRGN inRgn, BOOL inErase);
MOZ_SHUNT_PPC2002_API HHOOK SetWindowsHookEx(int inType, HOOKPROC inFunc, HINSTANCE inMod, DWORD inThreadId);
MOZ_SHUNT_PPC2002_API BOOL UnhookWindowsHookEx(HHOOK inHook);
MOZ_SHUNT_PPC2002_API LRESULT CallNextHookEx(HHOOK inHook, int inCode, WPARAM wParam, LPARAM lParam);
MOZ_SHUNT_PPC2002_API BOOL InvertRgn(HDC inDC, HRGN inRGN);
MOZ_SHUNT_PPC2002_API int GetScrollPos(HWND inWnd, int inBar);
MOZ_SHUNT_PPC2002_API BOOL GetScrollRange(HWND inWnd, int inBar, LPINT outMinPos, LPINT outMaxPos);
MOZ_SHUNT_PPC2002_API HRESULT _CoLockObjectExternal(IUnknown* inUnk, BOOL inLock, BOOL inLastUnlockReleases);
MOZ_SHUNT_PPC2002_API HRESULT _OleSetClipboard(IDataObject* inDataObj);
MOZ_SHUNT_PPC2002_API HRESULT _OleGetClipboard(IDataObject** outDataObj);
MOZ_SHUNT_PPC2002_API HRESULT _OleFlushClipboard(void);
MOZ_SHUNT_PPC2002_API HRESULT _OleQueryLinkFromData(IDataObject* inSrcDataObject);
MOZ_SHUNT_PPC2002_API HWND _FindWindowA(LPCSTR inClass, LPCSTR inWindow);
MOZ_SHUNT_PPC2002_API UINT _RegisterClipboardFormatA(LPCSTR inFormat);
MOZ_SHUNT_PPC2002_API LPITEMIDLIST _SHBrowseForFolder(LPBROWSEINFO inBI);
MOZ_SHUNT_PPC2002_API BOOL SetMenu(HWND inWnd, HMENU inMenu);
MOZ_SHUNT_PPC2002_API BOOL GetUserName(LPTSTR inBuffer, LPDWORD inoutSize);
MOZ_SHUNT_PPC2002_API DWORD GetShortPathName(LPCTSTR inLongPath, LPTSTR outShortPath, DWORD inBufferSize);


/****************************************************************************
**  End C API Section.
****************************************************************************/
#if 0
{
#endif
#if defined(__cplusplus)
} /* extern "C" */
#endif


/****************************************************************************
**  C API Section.
****************************************************************************/
#if defined(__cplusplus)
extern "C++" {
#if 0
}
#endif


/****************************************************************************
**  new.h
****************************************************************************/
#if !defined(_AFX)
inline void* __cdecl operator new(size_t, void* inMem)
{
    return inMem;
}
inline void __cdecl operator delete(void*, void*)
{
    return;
}
#endif /* _AFX */


/****************************************************************************
**  End C++ API Section.
****************************************************************************/
#if 0
{
#endif
} /* extern "C++" */
#endif


#endif /* __moz_shunt_ppc2002_h */
