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
#include "time_conversions.h"

extern "C" {
#if 0
}
#endif


static const int sDaysOfYear[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
static struct tm tmStorage;


MOZ_SHUNT_PPC2002_API struct tm* localtime_r(const time_t* inTimeT,struct tm* outRetval)
{
    struct tm* retval = NULL;

    if(NULL != inTimeT && NULL != outRetval)
    {
        SYSTEMTIME winLocalTime;
        
        time_t_2_LOCALSYSTEMTIME(winLocalTime, *inTimeT);
        
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

        retval = outRetval;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API struct tm* localtime(const time_t* inTimeT)
{
    return localtime_r(inTimeT, &tmStorage);
}


MOZ_SHUNT_PPC2002_API struct tm* gmtime_r(const time_t* inTimeT, struct tm* outRetval)
{
    struct tm* retval = NULL;

    if(NULL != inTimeT)
    {
        SYSTEMTIME winGMTime;
        
        time_t_2_SYSTEMTIME(winGMTime, *inTimeT);
        
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

        retval = outRetval;
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API struct tm* gmtime(const time_t* inTimeT)
{
    return gmtime_r(inTimeT, &tmStorage);
}


MOZ_SHUNT_PPC2002_API time_t mktime(struct tm* inTM)
{
    time_t retval = (time_t)-1;

    if(NULL != inTM)
    {
        SYSTEMTIME winTime;
        struct tm* gmTime = NULL;

        memset(&winTime, 0, sizeof(winTime));

        /*
         * Ignore tm_wday and tm_yday.
         * We likely have some problems with dst.
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
        SYSTEMTIME_2_time_t(retval, winTime);

        /*
         * Now overwrite the struct passed in with what we believe it should be.
         */
        gmTime = gmtime_r(&retval, inTM);
    }

    return retval;
}


static void helper_Winstrftime(LPCWSTR inWStr, char** outAStr, size_t* outAStrMax, bool* outHadError, bool* outEnoughSpace, char inTestEnd)
{
    int w2aRes = 0;

    w2aRes = w2a_buffer(inWStr, -1, *outAStr, *outAStrMax);
    if(0 != w2aRes)
    {
        size_t written = w2aRes - 1;

        (*outAStr) += written;
        (*outAStrMax) -= written;

        if(0 == (*outAStrMax) && '\0' != inTestEnd)
        {
            /* next one will fail actually, not this one */
            *outEnoughSpace = false;
        }
    }
    else
    {
        *outHadError = true;
    }
}


MOZ_SHUNT_PPC2002_API size_t strftime(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
{
    size_t retval = 0;

    if(NULL != strDest && 0 != maxsize && NULL != format && NULL != timeptr)
    {
        const char* traverse = format;
        char* outDst = strDest;
        size_t outMax = maxsize - 1;
        bool hadEnoughSpace = true;
        struct tm convertTM;
        time_t convertT;
        SYSTEMTIME sysTime;
        bool errorOut = false;

        /*
         * Convert the struct tm to SYSTEMTIME.
         * The SYSTEMTIME will be used in the API calls.
         */
        memcpy(&convertTM, timeptr, sizeof(convertTM));
        convertT = mktime(&convertTM);
        time_t_2_SYSTEMTIME(sysTime, convertT);

        /*
         * Format may be empty string.
         */
        *outDst = '\0';

        /*
         * Loop over the format and do the right thing.
         */
        while('\0' != *traverse && true == hadEnoughSpace && false == errorOut)
        {
            switch(*traverse)
            {
            case '%':
                {
                    int offset = 0;
                    bool poundOutput = false;
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
                        poundOutput = true;
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
                                errorOut = true;
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
                                errorOut = true;
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
                                errorOut = true;
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
                                errorOut = true;
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
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, (true == poundOutput) ? DATE_LONGDATE : DATE_SHORTDATE, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                                errorOut = true;
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
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("d") : _T("dd"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("H") : _T("HH"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("h") : _T("hh"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
                            }
                        }
                        break;
                    case 'j':
                        {
                            /*
                             * Day of year.
                             */
                            traverse++;
                            wsprintfW(buf, (true == poundOutput) ? _T("d") : _T(".3d"), convertTM.tm_yday);
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
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("M") : _T("MM"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("m") : _T("mm"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                                errorOut = true;
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
                            getRes = GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sysTime, (true == poundOutput) ? _T("s") : _T("ss"), buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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

                            wsprintfW(buf, (true == poundOutput) ? _T("d") : _T(".2d"), weekOfYear);
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

                            wsprintfW(buf, (true == poundOutput) ? _T("d") : _T(".2d"), weekOfYear);
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
                            getRes = GetDateFormatW(LOCALE_USER_DEFAULT, (true == poundOutput) ? DATE_LONGDATE : DATE_SHORTDATE, &sysTime, NULL, buf, sizeof(buf) / sizeof(WCHAR));
                            if(0 != getRes)
                            {
                                helper_Winstrftime(buf, &outDst, &outMax, &errorOut, &hadEnoughSpace, *traverse);
                            }
                            else
                            {
                                errorOut = true;
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
                                errorOut = true;
                            }
                        }
                        break;
                    case 'y':
                        {
                            /*
                             * Year without century in decimal. (00-99)
                             */
                            traverse++;
                            wsprintfW(buf, (true == poundOutput) ? _T("d") : _T(".2d"), convertTM.tm_year % 100);
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
                                errorOut = true;
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
                                hadEnoughSpace = false;
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
                        hadEnoughSpace = false;
                    }
                }
                break;
            }
        }

        if(false == errorOut && true == hadEnoughSpace)
        {
            /*
             * Number of chars written is return value.
             */
            retval = maxsize - outMax;
        }
    }

    return retval;
}


MOZ_SHUNT_PPC2002_API time_t time(time_t* inStorage)
{
    time_t retval = 0;

    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
    SYSTEMTIME_2_time_t(retval, sysTime);

    if(NULL != inStorage)
    {
        *inStorage = retval;
    }
    return retval;
}


MOZ_SHUNT_PPC2002_API char* ctime(const time_t* timer)
{
    char* retval = NULL;

    if(NULL != timer)
    {
        struct tm tmLocal;
        struct tm* localRes = NULL;

        localRes = localtime_r(timer, &tmLocal);
        if(NULL != localRes)
        {
            static char ctimeBuf[32];
            size_t strftimeRes = 0;

            // Sat Dec 20 01:05:05 1969\n\0
            strftimeRes = strftime(ctimeBuf, sizeof(ctimeBuf), "%a %b %d %H %Y\n", &tmLocal);
            if(0 != strftimeRes)
            {
                retval = ctimeBuf;
            }
        }
    }

    return retval;
}


#if 0
{
#endif
} /* extern "C" */

