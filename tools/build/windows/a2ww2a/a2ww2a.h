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
#if !defined(__a2ww2a_h__)
#define __a2ww2a_h__
/*
**  a2ww2a.h
**
**  This file exists to provide some simple APIs to convert
**      from unicode wide characters to ansi code page multibyte
**      characters, vice versa, using the appropriate win32 APIs.
*/

/*
**  Windows only.
*/
#if defined(_WIN32)

#if defined(__cplusplus)
extern "C" {
#endif
#if 0
}
#endif

/*
**  Import or export.
*/
#if defined(BUILD_a2ww2a)
#define A2WW2A_API __declspec(dllexport)
#else
#define A2WW2A_API __declspec(dllimport)
#endif


A2WW2A_API int w2a_buffer(LPCWSTR inWideString, int inWideChars, LPSTR outACPString, int inACPChars)
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


A2WW2A_API LPSTR w2a_malloc(LPCWSTR inWideString, int inWideChars, int* outACPChars)
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


A2WW2A_API int a2w_buffer(LPCSTR inACPString, int inACPChars, LPWSTR outWideString, int inWideChars)
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


A2WW2A_API LPWSTR a2w_malloc(LPCSTR inACPString, int inACPChars, int* outWideChars)
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


#if 0
{
#endif
#if defined(__cplusplus)
}   /* extern "C" */
#endif

#endif /* _WIN32 */
#endif /* __a2ww2a_h__ */
