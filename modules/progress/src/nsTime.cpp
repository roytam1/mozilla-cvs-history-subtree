/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsTime.h"

nsTime::nsTime(void)
    : fValue(LL_ZERO)
{
}


nsTime::nsTime(const PRTime t)
    : fValue(t)
{
}


nsTime::nsTime(const nsTime& t)
    : fValue(t.fValue)
{
}


const nsTime&
nsTime::operator =(const nsTime& t)
{
    fValue = t.fValue;
    return *this;
}


const nsTime&
nsTime::operator =(const PRTime t)
{
    fValue = t;
    return *this;
}


PRUint32
nsTime::ToMSec(void)
{
    PRTime factor;
    LL_UI2L(factor, PR_USEC_PER_MSEC);

    PRTime timeInMSec;
    LL_DIV(timeInMSec, fValue, factor);

    PRUint32 result;
    LL_L2UI(result, timeInMSec);

    return result;
}


const nsTime
operator +(const nsTime& t1, const nsTime& t2)
{
    nsTime result;
    LL_ADD(result.fValue, t1.fValue, t2.fValue);
    return result;
}



const nsTime
operator -(const nsTime& t1, const nsTime& t2)
{
    nsTime result;
    LL_SUB(result.fValue, t1.fValue, t2.fValue);
    return result;
}




const nsTime
nsTime::FromMSec(PRUint32 msec)
{
    PRTime usec;
    LL_UI2L(usec, msec);

    PRTime factor;
    LL_UI2L(factor, PR_USEC_PER_MSEC);

    LL_MUL(usec, usec, factor);

    return nsTime(usec);
}


const nsTime&
nsTime::operator -=(const nsTime& t)
{
    LL_SUB(fValue, fValue, t.fValue);
    return *this;
}


const nsTime&
nsTime::operator -=(const PRTime t)
{
    LL_SUB(fValue, fValue, t);
    return *this;
}



const nsTime&
nsTime::operator +=(const nsTime& t)
{
    LL_ADD(fValue, fValue, t.fValue);
    return *this;
}


const nsTime&
nsTime::operator +=(const PRTime t)
{
    LL_ADD(fValue, fValue, t);
    return *this;
}

