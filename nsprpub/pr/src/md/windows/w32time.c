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
 *  Garrett Arch Blythe, 01/28/2002
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

/*
 * w32time.c
 *
 * This file exists mainly to provide an implementation of the time
 *  functions that are missing from LIBC for certain toolsets; namely
 *  MS eMbedded Visual Tools (Windows CE).
 */

#include "primpl.h"

/*
 * Ugh, LIBC docs should have warned you.
 * A signle static storage for the struct tm's returned by some funcs.
 */
static const int sDaysOfYear[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
static struct tm tmStorage;

/*
 *  localtime
 *
 *  As LIBC localtime
 */
struct tm* localtime_r(const time_t* inTimeT,struct tm* outRetval)
{
    struct tm* retval = NULL;

    if(NULL != inTimeT && NULL != outRetval)
    {
        SYSTEMTIME winLocalTime;
        
        _MD_time_t_2_LOCALSYSTEMTIME(winLocalTime, *inTimeT);
        
        outRetval->tm_sec = (int)winLocalTime.wSecond;
        outRetval->tm_min = (int)winLocalTime.wMinute;
        outRetval->tm_hour = (int)winLocalTime.wHour;
        outRetval->tm_mday = (int)winLocalTime.wDay;
        outRetval->tm_mon = (int)(winLocalTime.wMonth - 1);
        outRetval->tm_year = (int)(winLocalTime.wYear - 1900);
        outRetval->tm_wday = (int)winLocalTime.wDayOfWeek;
        outRetval->tm_isdst = -1;

        outRetval->tm_yday = (int)winLocalTime.wDay + sDaysOfYear[outRetval->tm_mon];
        if(0 == (winLocalTime.wYear & 3))
        {
            if(2 < winLocalTime.wMonth)
            {
                if(0 == winLocalTime.wYear % 100)
                {
                    if(0 == winLocalTime.wYear % 400)
                    {
                        outRetval->tm_yday++;
                    }
                }
                else
                {
                    outRetval->tm_yday++;
                }
            }
        }

        retval = &tmStorage;
    }

    return retval;
}

__declspec(dllexport)
struct tm* localtime(const time_t* inTimeT)
{
    return localtime_r(inTimeT, &tmStorage);
}

/*
 *  gmtime
 *
 *  As LIBC gmtime
 */
struct tm* gmtime_r(const time_t* inTimeT, struct tm* outRetval)
{
    struct tm* retval = NULL;

    if(NULL != inTimeT)
    {
        SYSTEMTIME winGMTime;
        
        _MD_time_t_2_SYSTEMTIME(winGMTime, *inTimeT);
        
        outRetval->tm_sec = (int)winGMTime.wSecond;
        outRetval->tm_min = (int)winGMTime.wMinute;
        outRetval->tm_hour = (int)winGMTime.wHour;
        outRetval->tm_mday = (int)winGMTime.wDay;
        outRetval->tm_mon = (int)(winGMTime.wMonth - 1);
        outRetval->tm_year = (int)(winGMTime.wYear - 1900);
        outRetval->tm_wday = (int)winGMTime.wDayOfWeek;
        outRetval->tm_isdst = -1;

        outRetval->tm_yday = (int)winGMTime.wDay + sDaysOfYear[outRetval->tm_mon];
        if(0 == (winGMTime.wYear & 3))
        {
            if(2 < winGMTime.wMonth)
            {
                if(0 == winGMTime.wYear % 100)
                {
                    if(0 == winGMTime.wYear % 400)
                    {
                        outRetval->tm_yday++;
                    }
                }
                else
                {
                    outRetval->tm_yday++;
                }
            }
        }

        retval = &tmStorage;
    }

    return retval;
}

__declspec(dllexport)
struct tm* gmtime(const time_t* inTimeT)
{
    return gmtime_r(inTimeT, &tmStorage);
}

/*
 *  mktime
 *
 *  As LIBCs mktime
 *  We likely have a deficiency with the handling of tm_isdst...
 */
__declspec(dllexport)
time_t mktime(struct tm* inTM)
{
    time_t retval = (time_t)-1;

    if(NULL != inTM)
    {
        SYSTEMTIME winTime;
        struct tm* gmTime = NULL;

        memset(&winTime, 0, sizeof(winTime));

        /*
         * Ignore tm_wday and tm_yday.
         */
        winTime.wSecond = inTM->tm_sec;
        winTime.wMinute = inTM->tm_min;
        winTime.wHour = inTM->tm_hour;
        winTime.wDay = inTM->tm_mday;
        winTime.wMonth = inTM->tm_mon + 1;
        winTime.wYear = inTM->tm_year + 1900;

        /*
         * First get our time_t.
         */
        _MD_SYSTEMTIME_2_time_t(retval, winTime);

        /*
         * Now overwrite the struct passed in with what we believe it should be.
         */
        gmTime = gmtime_r(&retval, inTM);
    }

    return retval;
}

static void helper_Winstrftime(LPCWSTR inWStr,
                               char** outAStr,
                               int* outAStrMax,
                               PRBool* outHadError,
                               PRBool* outEnoughSpace,
                               char inTestEnd)
{
    char *w2aRes = NULL;

    w2aRes = _PR_MD_W2A(inWStr, *outAStr, *outAStrMax);
    if(NULL != w2aRes)
    {
        size_t written = strlen(*outAStr);

        (*outAStr) += written;
        (*outAStrMax) -= written;

        if(0 == (*outAStrMax) && '\0' != inTestEnd)
        {
            /* next one will fail actually, not this one */
            *outEnoughSpace = PR_FALSE;
        }
    }
    else
    {
        *outHadError = PR_TRUE;
    }
}

/*
 *  strftime
 *
 *  As LIBCs strftime
 *  Use GetTimeFormat and GetDateFormat to implement.
 */
__declspec(dllexport)
size_t strftime(char *strDest,
                size_t maxsize,
                const char *format,
                const struct tm *timeptr)
{
    size_t retval = 0;

    if(NULL != strDest && 0 != maxsize && NULL != format && NULL != timeptr)
    {
        const char* traverse = format;
        char* outDst = strDest;
        size_t outMax = maxsize - 1;
        PRBool hadEnoughSpace = PR_TRUE;
        struct tm convertTM;
        time_t convertT;
        SYSTEMTIME sysTime;
        PRBool errorOut = PR_FALSE;

        /*
         * Convert the struct tm to SYSTEMTIME.
         * The SYSTEMTIME will be used in the API calls.
         */
        memcpy(&convertTM, timeptr, sizeof(convertTM));
        convertT = mktime(&convertTM);
        _MD_time_t_2_SYSTEMTIME(sysTime, convertT);

        /*
         * Format may be empty string.
         */
        *outDst = '\0';

        /*
         * Loop over the format and do the right thing.
         */
        while('\0' != *traverse && PR_TRUE == hadEnoughSpace && PR_FALSE == errorOut)
        {
            switch(*traverse)
            {
            case '%':
                {
                    PRIntn offset = 0;
                    PRBool poundOutput = PR_FALSE;
                    WCHAR buf[128];

                    traverse++;
                    offset++;

                    /*
                     * Skip the '#' formatting option.
                     */
                    if('#' == *traverse)
                    {
                        traverse++;
                        offset++;
                        poundOutput = PR_TRUE;
                    }

                    switch(*traverse)
                    {
                    case 'a':
                        {
                            int getRes = 0;

                            /*
                             * Abbreviated weekday name.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("ddd"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'A':
                        {
                            int getRes = 0;

                            /*
                             * Full weekday name.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("dddd"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'b':
                        {
                            int getRes = 0;

                            /*
                             * Abbreviated month name.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("MMM"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'B':
                        {
                            int getRes = 0;

                            /*
                             * Full month name.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("MMMM"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'c':
                        {
                            int getRes = 0;

                            /*
                             * Date and time representation for locale.
                             */
                            traverse++;

                            /*
                             * First the date.
                             */
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, (PR_TRUE == poundOutput) ? DATE_LONGDATE : DATE_SHORTDATE, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }

                            /*
                             * Uhm, a guess just a space between date and time.
                             */
                            helper_Winstrftime(_T(" "), &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);


                            /*
                             * Last the time.
                             */
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'd':
                        {
                            int getRes = 0;

                            /*
                             * Day of month as decimal.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("d") : _T("dd"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'H':
                        {
                            int getRes = 0;

                            /*
                             * Hour in 24 hour format.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("H") : _T("HH"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'I':
                        {
                            int getRes = 0;

                            /*
                             * Hour in 12 hour format.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("h") : _T("hh"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'j':
                        {
                            /*
                             * Day of year.
                             */
                            traverse++;
                            wsprintfW(buf, (PR_TRUE == poundOutput) ? _T("d") : _T(".3d"), convertTM.tm_yday);
                            helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    case 'm':
                        {
                            int getRes = 0;

                            /*
                             * Month in decimal.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("M") : _T("MM"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'M':
                        {
                            int getRes = 0;

                            /*
                             * Minute in decimal.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("m") : _T("mm"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'p':
                        {
                            int getRes = 0;

                            /*
                             * 12 hour indicator.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("tt"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'S':
                        {
                            int getRes = 0;

                            /*
                             * Second in decimal.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (PR_TRUE == poundOutput) ? _T("s") : _T("ss"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'U':
                        {
                            int weekOfYear = 0;
                            int dayAdjust = convertTM.tm_yday;

                            /*
                             * Week of year in decimal.
                             * Sunday as first day.
                             */
                            traverse++;
                            if(dayAdjust >= convertTM.tm_wday)
                            {
                                dayAdjust -= convertTM.tm_wday;
                            }
                            else
                            {
                                dayAdjust = 0;
                            }

                            weekOfYear = dayAdjust / 7;

                            wsprintfW(buf, (PR_TRUE == poundOutput) ? _T("d") : _T(".2d"), weekOfYear);
                            helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    case 'w':
                        {
                            /*
                             * Weekday as decimal.
                             */
                            traverse++;
                            wsprintfW(buf, _T("d"), convertTM.tm_wday);
                            helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    case 'W':
                        {
                            int weekOfYear = 0;
                            int dayOfWeek = convertTM.tm_wday;
                            int dayAdjust = convertTM.tm_yday;

                            /*
                             * Week of year in decimal.
                             * Monday as first day.
                             */
                            traverse++;
                            if(0 == dayOfWeek)
                            {
                                dayOfWeek = 6;
                            }
                            else
                            {
                                dayOfWeek--;
                            }
                            if(dayAdjust >= dayOfWeek)
                            {
                                dayAdjust -= dayOfWeek;
                            }
                            else
                            {
                                dayAdjust = 0;
                            }

                            weekOfYear = dayAdjust / 7;

                            wsprintfW(buf, (PR_TRUE == poundOutput) ? _T("d") : _T(".2d"), weekOfYear);
                            helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    case 'x':
                        {
                            int getRes = 0;

                            /*
                             * Date representation for locale.
                             */
                            traverse++;
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, (PR_TRUE == poundOutput) ? DATE_LONGDATE : DATE_SHORTDATE, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'X':
                        {
                            int getRes = 0;

                            /*
                             * Time representation for locale.
                             */
                            traverse++;
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'y':
                        {
                            /*
                             * Year without century in decimal. (00-99)
                             */
                            traverse++;
                            wsprintfW(buf, (PR_TRUE == poundOutput) ? _T("d") : _T(".2d"), convertTM.tm_year % 100);
                            helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    case 'Y':
                        {
                            int getRes = 0;

                            /*
                             * Year with century, in decimal.
                             */
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, _T("YYYY"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = PR_TRUE;
                            }
                        }
                        break;
                    case 'z':
                    case 'Z':
                        {
                            /*
                             * Time zone name or abbreviation, or nothing if not known.
                             * We could possibly do something here with GetTimeZoneInformation,
                             *  but is that relevant to the tm passed in?
                             */
                            traverse++;
                        }
                        break;
                    case '%':
                        {
                            /*
                             * A signle percent.
                             */
                            traverse++;
                            helper_Winstrftime(_T("%"), &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                        }
                        break;
                    default:
                        {
                            /*
                             * Unrecognized formatting option.
                             * Back up and just output it.
                             */
                            traverse -= offset;

                            outDst[0] = *traverse;
                            outDst[1] = '\0';

                            outMax--;
                            outDst++;
                            traverse++;

                            /*
                             * Check to see if we used it all up.
                             */
                            if(0 == outMax && '\0' != *traverse)
                            {
                                hadEnoughSpace = PR_FALSE;
                            }
                        }
                        break;
                    }
                }
                break;
            default:
                {
                    /*
                     * Add custom formatting to output.
                     */
                    outDst[0] = *traverse;
                    outDst[1] = '\0';

                    outMax--;
                    outDst++;
                    traverse++;

                    /*
                     * Check to see if we used it all up.
                     */
                    if(0 == outMax && '\0' != *traverse)
                    {
                        hadEnoughSpace = PR_FALSE;
                    }
                }
                break;
            }
        }

        if(PR_FALSE == errorOut && PR_TRUE == hadEnoughSpace)
        {
            /*
             * Number of chars written is return value.
             */
            retval = maxsize - outMax;
        }
    }

    return retval;
}
