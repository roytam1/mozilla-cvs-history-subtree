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
 *    Garrett Arch Blythe, 04/11/2002
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
#if !defined(__mozgdi_h__)
#define __mozgdi_h__

/*
**  mozgdi.h
**
**  This file exists solely to provide needed functionality of missing GDI
**      api functions which our code has come to unfortunately use.
**  This provision was made in the interests of moving the WinCE port
**      along as it is stuck until the appropriate pieces of code can
**      be rewritten by the domain owners (hahahahaha) to be optimized
**      for the platform in question.
*/

/*
**  Windows CE only.
*/
#if defined(WINCE)

#if defined(__cplusplus)
extern "C" {
#endif
#if 0
}
#endif

/*
**  Import or export.
*/
#if defined(A2WW2A_API)
#undef A2WW2A_API
#endif
#if defined(BUILD_a2ww2a)
#define A2WW2A_API __declspec(dllexport)
#else
#define A2WW2A_API __declspec(dllimport)
#endif


A2WW2A_API int GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage)
/*
**  Copy the specified DIB data into a buffer, possibly with conversion.
**
**  NOTE:   This is not and never will be a replacement for the real thing.
**          This only exists to implement the functionality the specific
**              software in mind needed at the time of writing.
*/
;


A2WW2A_API int SetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPCVOID lpvBits, CONST LPBITMAPINFO lpbmi, UINT fuColorUse)
/*
**  Copy the specified DIB data into a bitmap, possibly with conversion.
**
**  NOTE:   This is not and never will be a replacement for the real thing.
**          This only exists to implement the functionality the specific
**              software in mind needed at the time of writing.
*/
;


#if 0
{
#endif
#if defined(__cplusplus)
}   /* extern "C" */
#endif

#endif /* WINCE */
#endif /* __a2ww2a_h__ */
