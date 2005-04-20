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
 *  Garrett Arch Blythe 01/25/2002
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

#ifndef nspr_win32_time_h___
#define nspr_win32_time_h___

#include <windows.h>

/*
 * _win23_time.h
 *
 * This file mainly exists because when porting NSPR to WinCE there was
 *  a lack of LIBC support.  Conversion between window's FILETIME and
 *  time_t was going to be a common operation; thus this file.
 *
 * I assume compiler support for int64.
 */

#if !defined(__GNUC__)
#define _PR_I64_CONST(number) number ## i64
#else
#define _PR_I64_COSNT(number) number ## LL
#endif

/*
 * FILETIME has an epoch of 1601.
 * Precomputed the 1970 epoch so we do not have to below.
 */
#define _MD_FILETIME_1970 _PR_I64_CONST(116444736000000000)

/*
 * Marco to support add/sub/mul/div on a FILETIME level.
 */
#define _MD_FILETIME_ARITH(outFileTime, inFileTime, inOperation, inValue) \
     PR_BEGIN_MACRO \
         ULARGE_INTEGER _buffer1; \
         \
         _buffer1.LowPart = inFileTime.dwLowDateTime; \
         _buffer1.HighPart = inFileTime.dwHighDateTime; \
         _buffer1.QuadPart = _buffer1.QuadPart inOperation inValue; \
         outFileTime.dwLowDateTime = _buffer1.LowPart; \
         outFileTime.dwHighDateTime = _buffer1.HighPart; \
     PR_END_MACRO

/*
 * FILETIME is in 100 nanosecond units.
 * Provide macros for conversion to other second units.
 */
#define _MD_FILETIME_2_MICROSECONDS(outTime, inFileTime) \
     PR_BEGIN_MACRO \
         ULARGE_INTEGER _buffer2; \
         \
         _buffer2.LowPart = inFileTime.dwLowDateTime; \
         _buffer2.HighPart = inFileTime.dwHighDateTime; \
         outTime = _buffer2.QuadPart / _PR_I64_CONST(10); \
     PR_END_MACRO
#define _MD_FILETIME_2_MILLISECONDS(outTime, inFileTime) \
     PR_BEGIN_MACRO \
         ULARGE_INTEGER _buffer3; \
         \
         _buffer3.LowPart = inFileTime.dwLowDateTime; \
         _buffer3.HighPart = inFileTime.dwHighDateTime; \
         outTime = _buffer3.QuadPart / _PR_I64_CONST(10000); \
     PR_END_MACRO
#define _MD_FILETIME_2_SECONDS(outTime, inFileTime) \
     PR_BEGIN_MACRO \
         ULARGE_INTEGER _buffer4; \
         \
         _buffer4.LowPart = inFileTime.dwLowDateTime; \
         _buffer4.HighPart = inFileTime.dwHighDateTime; \
         outTime = _buffer4.QuadPart / _PR_I64_CONST(10000000); \
     PR_END_MACRO
#define _MD_SECONDS_2_FILETIME(outFileTime, inTime) \
     PR_BEGIN_MACRO \
         ULARGE_INTEGER _buffer5; \
         \
         _buffer5.QuadPart = (ULONGLONG)inTime * _PR_I64_CONST(10000000); \
         outFileTime.dwLowDateTime = _buffer5.LowPart; \
         outFileTime.dwHighDateTime = _buffer5.HighPart; \
     PR_END_MACRO

/*
 * Conversions from FILETIME 1601 epoch time to LIBC/NSPR 1970 time.epoch.
 */
#define _MD_FILETIME_2_PRTime(outPRTime, inFileTime) \
     PR_BEGIN_MACRO \
         FILETIME _result1; \
         ULONGLONG _conversion1; \
         \
         _MD_FILETIME_ARITH(_result1, inFileTime, -, _MD_FILETIME_1970); \
         _MD_FILETIME_2_MICROSECONDS(_conversion1, _result1); \
         outPRTime = (PRTime)_conversion1; \
     PR_END_MACRO
#define _MD_FILETIME_2_time_t(outTimeT, inFileTime) \
     PR_BEGIN_MACRO \
         FILETIME _result2; \
         ULONGLONG _conversion2; \
         \
         _MD_FILETIME_ARITH(_result2, inFileTime, -, _MD_FILETIME_1970); \
         _MD_FILETIME_2_SECONDS(_conversion2, _result2); \
         outTimeT = (time_t)_conversion2; \
     PR_END_MACRO
#define _MD_time_t_2_FILETIME(outFileTime, inTimeT) \
     PR_BEGIN_MACRO \
         FILETIME _conversion3; \
         \
         _MD_SECONDS_2_FILETIME(_conversion3, inTimeT); \
         _MD_FILETIME_ARITH(outFileTime, _conversion3, +, _MD_FILETIME_1970); \
     PR_END_MACRO


/*
 * Sometimes SYSTEMTIME needs to be handled as well.
 */
#define _MD_SYSTEMTIME_2_PRTime(outPRTime, inSystemTime) \
     PR_BEGIN_MACRO \
         FILETIME _result3; \
         \
         SystemTimeToFileTime(&inSystemTime, &_result3); \
         _MD_FILETIME_2_PRTime(outPRTime, _result3); \
     PR_END_MACRO
#define _MD_SYSTEMTIME_2_time_t(outTimeT, inSystemTime) \
     PR_BEGIN_MACRO \
         FILETIME _result4; \
         \
         SystemTimeToFileTime(&inSystemTime, &_result4); \
         _MD_FILETIME_2_time_t(outTimeT, _result4); \
     PR_END_MACRO
#define _MD_time_t_2_SYSTEMTIME(outSystemTime, inTimeT) \
     PR_BEGIN_MACRO \
         FILETIME _conversion4; \
         \
         _MD_time_t_2_FILETIME(_conversion4, inTimeT); \
         FileTimeToSystemTime(&_conversion4, &outSystemTime); \
     PR_END_MACRO
#define _MD_time_t_2_LOCALSYSTEMTIME(outSystemTime, inTimeT) \
     PR_BEGIN_MACRO \
         FILETIME _conversion5; \
         FILETIME localConversion; \
         \
         _MD_time_t_2_FILETIME(_conversion5, inTimeT); \
         FileTimeToLocalFileTime(&_conversion5, &localConversion); \
         FileTimeToSystemTime(&localConversion, &outSystemTime); \
     PR_END_MACRO

#endif /* nspr_win32_time_h___ */
