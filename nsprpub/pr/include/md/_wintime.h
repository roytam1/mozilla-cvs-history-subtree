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

#ifndef nspr_wintime_h___
#define nspr_wintime_h___

#include <windows.h>

/*
 * _wintime.h
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
        ULARGE_INTEGER buffer; \
        \
        buffer.LowPart = inFileTime.dwLowDateTime; \
        buffer.HighPart = inFileTime.dwHighDateTime; \
        buffer.QuadPart = buffer.QuadPart inOperation inValue; \
        outFileTime.dwLowDateTime = buffer.LowPart; \
        outFileTime.dwHighDateTime = buffer.HighPart; \
    PR_END_MACRO

/*
 * FILETIME is in 100 nanosecond units.
 * Provide macros for conversion to other second units.
 */
#define _MD_FILETIME_2_MICROSECONDS(outTime, inFileTime) \
    PR_BEGIN_MACRO \
        ULARGE_INTEGER buffer; \
        \
        buffer.LowPart = inFileTime.dwLowDateTime; \
        buffer.HighPart = inFileTime.dwHighDateTime; \
        outTime = buffer.QuadPart / _PR_I64_CONST(10); \
    PR_END_MACRO
#define _MD_FILETIME_2_MILLISECONDS(outTime, inFileTime) \
    PR_BEGIN_MACRO \
        ULARGE_INTEGER buffer; \
        \
        buffer.LowPart = inFileTime.dwLowDateTime; \
        buffer.HighPart = inFileTime.dwHighDateTime; \
        outTime = buffer.QuadPart / _PR_I64_CONST(10000); \
    PR_END_MACRO
#define _MD_FILETIME_2_SECONDS(outTime, inFileTime) \
    PR_BEGIN_MACRO \
        ULARGE_INTEGER buffer; \
        \
        buffer.LowPart = inFileTime.dwLowDateTime; \
        buffer.HighPart = inFileTime.dwHighDateTime; \
        outTime = buffer.QuadPart / _PR_I64_CONST(10000000); \
    PR_END_MACRO
#define _MD_SECONDS_2_FILETIME(outFileTime, inTime) \
    PR_BEGIN_MACRO \
        ULARGE_INTEGER buffer; \
        \
        buffer.QuadPart = (ULONGLONG)inTime * _PR_I64_CONST(10000000); \
        outFileTime.dwLowDateTime = buffer.LowPart; \
        outFileTime.dwHighDateTime = buffer.HighPart; \
    PR_END_MACRO

/*
 * Conversions from FILETIME 1601 epoch time to LIBC/NSPR 1970 time.epoch.
 */
#define _MD_FILETIME_2_PRTime(outPRTime, inFileTime) \
    PR_BEGIN_MACRO \
        FILETIME result; \
        ULONGLONG conversion; \
        \
        _MD_FILETIME_ARITH(result, inFileTime, -, _MD_FILETIME_1970); \
        _MD_FILETIME_2_MICROSECONDS(conversion, result); \
        outPRTime = (PRTime)conversion; \
    PR_END_MACRO
#define _MD_FILETIME_2_time_t(outTimeT, inFileTime) \
    PR_BEGIN_MACRO \
        FILETIME result; \
        ULONGLONG conversion; \
        \
        _MD_FILETIME_ARITH(result, inFileTime, -, _MD_FILETIME_1970); \
        _MD_FILETIME_2_SECONDS(conversion, result); \
        outTimeT = (time_t)conversion; \
    PR_END_MACRO
#define _MD_time_t_2_FILETIME(outFileTime, inTimeT) \
    PR_BEGIN_MACRO \
        FILETIME conversion; \
        \
        _MD_SECONDS_2_FILETIME(conversion, inTimeT); \
        _MD_FILETIME_ARITH(outFileTime, conversion, +, _MD_FILETIME_1970); \
    PR_END_MACRO


/*
 * Sometimes SYSTEMTIME needs to be handled as well.
 */
#define _MD_SYSTEMTIME_2_PRTime(outPRTime, inSystemTime) \
    PR_BEGIN_MACRO \
        FILETIME result; \
        \
        SystemTimeToFileTime(&inSystemTime, &result); \
        _MD_FILETIME_2_PRTime(outPRTime, result); \
    PR_END_MACRO
#define _MD_time_t_2_LOCALSYSTEMTIME(outSystemTime, inTimeT) \
    PR_BEGIN_MACRO \
        FILETIME conversion; \
        FILETIME localConversion; \
        \
        _MD_time_t_2_FILETIME(conversion, inTimeT); \
        FileTimeToLocalFileTime(&conversion, &localConversion); \
        FileTimeToSystemTime(&localConversion, &outSystemTime); \
    PR_END_MACRO

/*
 * tm is needed.
 * Used by Winlocaltime.
 */
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

#endif /* nspr_wintime_h___ */
