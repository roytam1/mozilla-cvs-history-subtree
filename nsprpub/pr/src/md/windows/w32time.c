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
 *  Winlocaltime
 *
 *  As LIBC localtime
 */
struct tm* Winlocaltime_r(const time_t* inTimeT, struct tm* outRetval)
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
struct tm* Winlocaltime(const time_t* inTimeT)
{
    return Winlocaltime_r(inTimeT, &tmStorage);
}

/*
 *  Wingmtime
 *
 *  As LIBC gmtime
 */
struct tm* Wingmtime_r(const time_t* inTimeT, struct tm* outRetval)
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
struct tm* Wingmtime(const time_t* inTimeT)
{
    return Wingmtime_r(inTimeT, &tmStorage);
}

/*
 *  Winmktime
 *
 *  As LIBCs mktime
 *  We likely have a deficiency with the handling of tm_isdst...
 */
time_t Winmktime(struct tm* inTM)
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
        gmTime = Wingmtime_r(&retval, inTM);
    }

    return retval;
}

static void helper_Winstrftime(LPCWSTR inWStr, char** outAStr, int* outAStrMax, PRBool* outHadError, PRBool* outEnoughSpace, char inTestEnd)
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
 *  Winstrftime
 *
 *  As LIBCs strftime
 *  Use GetTimeFormat and GetDateFormat to implement.
 */
size_t Winstrftime(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
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
        convertT = Winmktime(&convertTM);
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
                        PR_ASSERT(0);
                        break;
                    case 'M':
                        PR_ASSERT(0);
                        break;
                    case 'p':
                        PR_ASSERT(0);
                        break;
                    case 'S':
                        PR_ASSERT(0);
                        break;
                    case 'U':
                        PR_ASSERT(0);
                        break;
                    case 'w':
                        PR_ASSERT(0);
                        break;
                    case 'W':
                        PR_ASSERT(0);
                        break;
                    case 'x':
                        PR_ASSERT(0);
                        break;
                    case 'X':
                        PR_ASSERT(0);
                        break;
                    case 'y':
                        PR_ASSERT(0);
                        break;
                    case 'Y':
                        PR_ASSERT(0);
                        break;
                    case 'z':
                        PR_ASSERT(0);
                        break;
                    case 'Z':
                        PR_ASSERT(0);
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
