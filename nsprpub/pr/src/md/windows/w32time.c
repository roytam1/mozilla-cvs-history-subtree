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
 * Winlocaltime
 *
 * As LIBC localtime
 */
struct tm* Winlocaltime(const time_t* inTimeT)
{
    struct tm* retval = NULL;

    if(NULL != inTimeT)
    {
        static struct tm tmStorage;
        static const int daysOfYear[12] =
        {
            0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
        };
        SYSTEMTIME winLocalTime;
        TIME_ZONE_INFORMATION  winTZInfo;
        DWORD winDST;
        
        _MD_time_t_2_LOCALSYSTEMTIME(winLocalTime, *inTimeT);
        
        tmStorage.tm_sec = (int)winLocalTime.wSecond;
        tmStorage.tm_min = (int)winLocalTime.wMinute;
        tmStorage.tm_hour = (int)winLocalTime.wHour;
        tmStorage.tm_mday = (int)winLocalTime.wDay;
        tmStorage.tm_mon = (int)(winLocalTime.wMonth - 1);
        tmStorage.tm_year = (int)(winLocalTime.wYear - 1900);
        tmStorage.tm_wday = (int)winLocalTime.wDayOfWeek;
        
        tmStorage.tm_yday = (int)winLocalTime.wDay + daysOfYear[tmStorage.tm_mon];
        if(0 == (winLocalTime.wYear & 3))
        {
            if(2 < winLocalTime.wMonth)
            {
                if(0 == winLocalTime.wYear % 100)
                {
                    if(0 == winLocalTime.wYear % 400)
                    {
                        tmStorage.tm_yday++;
                    }
                }
                else
                {
                    tmStorage.tm_yday++;
                }
            }
        }

        winDST = GetTimeZoneInformation(&winTZInfo);

        switch(winDST)
        {
        case TIME_ZONE_ID_STANDARD:
            tmStorage.tm_isdst = 0;
            break;
        case TIME_ZONE_ID_DAYLIGHT:
            tmStorage.tm_isdst = 1;
            break;
        default:
            tmStorage.tm_isdst = -1;
            break;
        }

        retval = &tmStorage;
    }

    return retval;
}

/*
 *  Winmktime
 *
 *  As LIBCs mktime
 */
time_t Winmktime(struct tm* inTM)
{
    time_t retval = (time_t)-1;

    /*
    ** FIXME TODO
    **
    ** More here
    */

    return retval;
}

/*
 *  Winstrftime
 *
 *  As LIBCs strftime
 */
size_t Winstrftime(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
{
    size_t retval = 0;

    /*
    ** FIXME TODO
    **
    ** More here.
    **
    ** Use GetTimeFormat and GetDateFormat
    */

    return retval;
}
